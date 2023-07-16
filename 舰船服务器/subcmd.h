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

#ifndef SUBCMD60_H
#define SUBCMD60_H

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

// 含客户端ID的副指令结构 4字节
typedef struct client_id_hdr {
    uint8_t type; //subcmd 指令
    uint8_t size;
    uint16_t client_id; // <= 12
} PACKED client_id_hdr_t;

// 含怪物ID的副指令结构 4字节
typedef struct enemy_id_hdr {
    uint8_t type; //subcmd 指令
    uint8_t size;
    uint16_t enemy_id; // 范围 [0x1000, 0x4000)
} PACKED enemy_id_hdr_t;

// 含对象ID的副指令结构 4字节
typedef struct object_id_hdr {
    uint8_t type; //subcmd 指令
    uint8_t size;
    uint16_t object_id; // >= 0x4000, != 0xFFFF
} PACKED object_id_hdr_t;

// 含参数的副指令结构 4字节
typedef struct params_hdr {
    uint8_t type; //subcmd 指令
    uint8_t size;
    uint16_t params;
} PACKED params_hdr_t;

// 普通无参数的副指令结构 4字节
typedef struct unused_hdr {
    uint8_t type; //subcmd 指令
    uint8_t size;
    uint16_t unused;
} PACKED unused_hdr_t;

/* General format of a subcommand packet. */
typedef struct subcmd_pkt {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unk;
    uint8_t data[0];
} PACKED subcmd_pkt_t;

static int char_dc_hdrsize2 = sizeof(subcmd_pkt_t);

typedef struct subcmd_bb_pkt {
    bb_pkt_hdr_t hdr;             /* 0x00 - 0x07 8 */
    uint8_t type;                 /* 0x08 - 0x08 1 */
    uint8_t size;                 /* 0x09 - 0x09 1 */
    uint16_t param;               /* 0x0A - 0x0B 2 */
    /* 数据头 + 数据信息 占用了 14 个字节 */
    uint8_t data[0];              /* 0x0A -  */
} PACKED subcmd_bb_pkt_t;

static int char_bb_hdrsize2 = sizeof(subcmd_bb_pkt_t);

// subcmd指令集通用数据头.

// 0x04: Unknown 未知
typedef struct subcmd_unknown_04 {
    client_id_hdr_t header;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED subcmd_unknown_04_t;

// 注意: data.shdr.object_id 值为 0xFFFF 时 表示房间的怪物均被击败
// 0x05: Switch state changed
// Some things that don't look like switches are implemented as switches using
// this subcommand. For example, when all enemies in a room are defeated, this
// subcommand is used to unlock the doors.
typedef struct subcmd_bb_switch_changed {
    bb_pkt_hdr_t hdr;
    bb_switch_changed_t data;
} PACKED subcmd_bb_switch_changed_pkt_t;

// 0x06: Send guild card
typedef struct G_SendGuildCard_DC {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t player_tag;
    uint32_t guild_card_number;
    char name[0x18];
    char description[0x48];
    uint8_t unused2[0x11];
    uint8_t present;
    uint8_t present2;
    uint8_t section_id;
    uint8_t char_class;
    uint8_t unused3[3];
} PACKED G_SendGuildCard_DC_6x06;

// 0x06: Send guild card
// Guild card send packet (Dreamcast).
typedef struct subcmd_dc_gcsend {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t player_tag;
    uint32_t guildcard;
    char name[24];
    char guildcard_desc[88];
    uint8_t unused2;
    uint8_t disable_udp;
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
    uint8_t padding[3];
} PACKED subcmd_dc_gcsend_t;

typedef struct G_SendGuildCard_PC {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t player_tag;
    uint32_t guild_card_number;
    uint16_t name[0x18];
    uint16_t description[0x48];
    uint8_t unused2[0x24];
    uint8_t present;
    uint8_t present2;
    uint8_t section_id;
    uint8_t char_class;
} PACKED G_SendGuildCard_PC_6x06;

// 0x06: Send guild card
// Guild card send packet (PC).
typedef struct subcmd_pc_gcsend {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t player_tag;
    uint32_t guildcard;
    uint16_t name[24];
    uint16_t guildcard_desc[0x58];
    uint32_t padding;
    uint8_t disable_udp;
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
} PACKED subcmd_pc_gcsend_t;

typedef struct G_SendGuildCard_V3 {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t player_tag;
    uint32_t guild_card_number;
    char name[0x18];
    char description[0x48];
    uint8_t unused2[0x24];
    uint8_t present;
    uint8_t present2;
    uint8_t section_id;
    uint8_t char_class;
} PACKED G_SendGuildCard_V3_6x06;

// 0x06: Send guild card
// Guild card send packet (Gamecube).
typedef struct subcmd_gc_gcsend {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t player_tag;
    uint32_t guildcard;
    char name[24];
    char guildcard_desc[104];
    uint32_t padding;
    uint8_t disable_udp;
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
} PACKED subcmd_gc_gcsend_t;

// 0x06: Send guild card
// Guild card send packet (Xbox).
typedef struct subcmd_xb_gcsend {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;                       /* unused = 0x0D 0xFB */
    uint32_t player_tag;
    uint32_t guildcard;
    uint64_t xbl_userid;
    char name[24];
    char guildcard_desc[512];                     /* Why so long? */
    uint8_t disable_udp;
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
} PACKED subcmd_xb_gcsend_t;

// 0x06: Send guild card
// Guild card send packet (Blue Burst)
typedef struct subcmd_bb_gc_send {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t guildcard;
    uint16_t name[0x18];
    uint16_t guild_name[0x10];
    uint16_t guildcard_desc[0x58];
    uint8_t disable_udp;
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
} PACKED subcmd_bb_gcsend_t;

typedef struct G_SendGuildCard_BB {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t guild_card_number;
    uint16_t name[0x18];
    uint16_t team_name[0x10];
    uint16_t description[0x58];
    uint8_t present;
    uint8_t present2;
    uint8_t section_id;
    uint8_t char_class;
} PACKED G_SendGuildCard_BB_6x06;

// 0x07: Symbol chat
struct SymbolChat {
    // TODO: How does this format differ across PSO versions? The GC version
    // treats some fields as unexpectedly large values (for example, face_spec
    // through unused2 are byteswapped as an le_uint32_t), so we should verify
    // that the order of these fields is the same on other versions.
    // Bits: ----------------------DMSSSCCCFF
    //   S = sound, C = face color, F = face shape, D = capture, M = mute sound
    /* 00 */ uint32_t spec;
    // Corner objects are specified in reading order ([0] is the top-left one).
    // Bits (each entry): ---VHCCCZZZZZZZZ
    //   V = reverse vertical, H = reverse horizontal, C = color, Z = object
    // If Z is all 1 bits (0xFF), no corner object is rendered.
    /* 04 */ uint16_t corner_objects[4];
    struct FacePart {
        uint8_t type; // FF = no part in this slot
        uint8_t x;
        uint8_t y;
        // Bits: ------VH (V = reverse vertical, H = reverse horizontal)
        uint8_t flags;
    /* 0C */ } PACKED face_parts[12];
    /* 3C */
} PACKED;

struct G_SymbolChat_6x07 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t client_id;
    struct SymbolChat data;
} PACKED;

// 0x07: Symbol chat
typedef struct subcmd_symbol_chat_corner_object {
    uint8_t type; // FF = no object in this slot
    // Bits: 000VHCCC (V = reverse vertical, H = reverse horizontal, C = color)
    uint8_t flags_color;
} PACKED subcmd_symbol_chat_corner_object_t;

// 0x07: Symbol chat
typedef struct subcmd_symbol_chat_face_part {
    uint8_t type; // FF = no part in this slot
    uint8_t x;
    uint8_t y;
    // Bits: 000000VH (V = reverse vertical, H = reverse horizontal)
    uint8_t flags;
} PACKED subcmd_symbol_chat_face_part_t;

// 0x07: Symbol chat
typedef struct subcmd_symbol_chat {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    // TODO: How does this format differ across PSO versions? The GC version
    // treats some fields as unexpectedly large values (for example, face_spec
    // through unused2 are byteswapped as an le_uint32_t), so we should verify
    // that the order of these fields is the same on other versions.
    uint32_t client_id;
    // Bits: SSSCCCFF (S = sound, C = face color, F = face shape)
    uint8_t face_spec;
    uint8_t disable_sound; // If low bit is set, no sound is played
    uint16_t unused2;
    subcmd_symbol_chat_corner_object_t corner_objects[4]; // In reading order (top-left is first)
    subcmd_symbol_chat_face_part_t face_parts[12];
} PACKED subcmd_symbol_chat_t;

// 0x07: Symbol chat
typedef struct subcmd_bb_symbol_chat {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    // TODO: How does this format differ across PSO versions? The GC version
    // treats some fields as unexpectedly large values (for example, face_spec
    // through unused2 are byteswapped as an le_uint32_t), so we should verify
    // that the order of these fields is the same on other versions.
    uint32_t client_id;
    // Bits: SSSCCCFF (S = sound, C = face color, F = face shape)
    uint8_t face_spec;
    uint8_t disable_sound; // If low bit is set, no sound is played
    uint16_t unused2;
    subcmd_symbol_chat_corner_object_t corner_objects[4]; // In reading order (top-left is first)
    subcmd_symbol_chat_face_part_t face_parts[12];
} PACKED subcmd_bb_symbol_chat_t;

// 0x09: Unknown 未知
typedef struct subcmd_unknown_09 {
    dc_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED subcmd_unknown_09_t;

// 0x0A: Enemy hit
// Packet sent by clients to say that a monster has been hit.
typedef struct subcmd_mhit {
    dc_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    // Note: enemy_id (in header) is in the range [0x1000, 0x4000)
    uint16_t enemy_id2;
    uint16_t damage;
    uint32_t flags;
} PACKED subcmd_mhit_pkt_t;

// 0x0A: Enemy hit
// Packet sent by clients to say that a monster has been hit.
typedef struct subcmd_bb_mhit {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    // Note: enemy_id (in sub header) is in the range [0x1000, 0x4000)
    uint16_t enemy_id2;
    uint16_t damage;
    uint32_t flags;
} PACKED subcmd_bb_mhit_pkt_t;

// 0x0B: Box destroyed
// Packet sent by clients to say that a box has been hit.
typedef struct subcmd_bhit {
    dc_pkt_hdr_t hdr;
    object_id_hdr_t shdr;
    uint32_t unk;
    uint16_t box_id;
    uint16_t unk2;
} PACKED subcmd_bhit_pkt_t;

// 0x0B: Box destroyed
// Packet sent by clients to say that a box has been hit.
typedef struct subcmd_bb_bhit {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr;
    uint32_t unk;
    uint16_t box_id;
    uint16_t unk2;
} PACKED subcmd_bb_bhit_pkt_t;

// 0x0C: Add condition (poison/slow/etc.)
// 0x0D: Remove condition (poison/slow/etc.)
typedef struct subcmd_bb_add_or_remove_condition {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t condition_type; // Probably condition type 0x01000000 0x00000000
    uint32_t unknown_a2;// 0x00000000
} PACKED subcmd_bb_add_or_remove_condition_t;

// 0x0E: Unknown 未知
typedef struct subcmd_unknown_0E {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_unknown_0E_t;

/* PC 进入大厅触发 */
//[2023年05月06日 11:24:06:180] 截获(3002): subcmd.c 3002 行 未找到相关指令名称 指令 0x0010 未处理. (数据如下)
//[2023年05月06日 11:24:06:192] 截获(3002):
//( 00000000 )   60 00 10 00 0D 03 00 00  01 00 00 00 00 00 00 00 `...............
//( 00000010 )   01 01 00 00 00 00 00 00  00 00 00 00 01 00 21 00 ..............!.
//( 00000020 )   00 00 00 00 02 00 00 00  4C 00 00 00 02 00 05 00 ........L.......
//( 00000030 )   F4 01 00 00 00 00 00 00  02 00 21 00 00 00 00 00 ?........!.....
//( 00000040 )   01 00 00 00 10 00 00 00  03 00 00 00 00 04 00 00 ................
//( 00000050 )   00 00 00 00 03 00 21 00  00 00 00 00 01 00 00 00 ......!.........

// 0x10: Unknown (不支持 Episode 3)
// 0x11: Unknown (不支持 Episode 3)
// Same format as 0x10
// 0x12: Dragon boss actions (不支持 Episode 3)
// Same format as 0x10
// 0x14: Unknown (指令生效范围; 仅限游戏; 不支持 Episode 3)
// Same format as 0x10
struct subcmd_bb_unknown_6x10_6x11_6x12_6x14 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint32_t unknown_a4;
} PACKED;

// 0x10: Unknown (不支持 Episode 3)
// 0x11: Unknown (不支持 Episode 3)
// Same format as 0x10
// 0x12: Dragon boss actions (不支持 Episode 3)
// Same format as 0x10
// 0x14: Unknown (指令生效范围; 仅限游戏; 不支持 Episode 3)
// Same format as 0x10
// Packet used by the Dragon boss to deal with its actions.
typedef struct subcmd_dragon_act {
    dc_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
} PACKED subcmd_dragon_act_t;

// 0x10: Unknown (不支持 Episode 3)
// 0x11: Unknown (不支持 Episode 3)
// Same format as 0x10
// 0x12: Dragon boss actions (不支持 Episode 3)
// Same format as 0x10
// Packet used by the Dragon boss to deal with its actions.
typedef struct subcmd_bb_dragon_act {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
} PACKED subcmd_bb_dragon_act_t;

// 0x13: De Rol Le boss actions (不支持 Episode 3)
typedef struct subcmd_bb_de_rolLe_boss_act {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t action;              /* 有点像攻击类型 */
    uint16_t stage;               /* 按顺序 从 1 一直数下去 */
} PACKED subcmd_bb_de_rolLe_boss_act_t;

// 0x14: De Rol Le boss special actions (指令生效范围; 仅限游戏; 不支持 Episode 3)
typedef struct subcmd_bb_de_rolLe_boss_sact {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t action;              /* 有点像攻击类型 */
    uint16_t stage;               /* 按顺序 从 1 一直数下去 与 0x13 重叠*/
    uint32_t unused;
} PACKED subcmd_bb_de_rolLe_boss_sact_t;

// 0x15: Vol Opt boss actions (不支持 Episode 3)
typedef struct subcmd_bb_VolOptBossActions_6x15 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
    uint16_t unknown_a5;
} PACKED subcmd_bb_VolOptBossActions_6x15_t;

// 0x16: Vol Opt boss actions (不支持 Episode 3)
typedef struct subcmd_bb_VolOptBossActions_6x16 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
    uint16_t unknown_a5;
} PACKED subcmd_bb_VolOptBossActions_6x16_t;

// 0x17: Unknown (指令生效范围; 仅限游戏; 不支持 Episode 3)
// Packet used to teleport to a specified position
typedef struct subcmd_teleport {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    float x;
    float y;
    float z;
    float w;
} PACKED subcmd_teleport_t;

// 0x17: teleport to a specified position (指令生效范围; 仅限游戏; 不支持 Episode 3)
// Packet used to teleport to a specified position
typedef struct subcmd_bb_teleport {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    float x;
    float y;
    float z;
    float w;
} PACKED subcmd_bb_teleport_t;

// 0x18: Dragon special actions (指令生效范围; 仅限游戏; 不支持 Episode 3)
typedef struct subcmd_bb_dragon_special_act {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    float unknown_a1;
    float unknown_a2;
} PACKED subcmd_bb_dragon_special_act_t;

// 0x19: Dark Falz boss actions (不支持 Episode 3)
typedef struct subcmd_bb_DarkFalzActions_6x19 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint32_t unknown_a4;
    uint32_t unused;
} PACKED subcmd_bb_DarkFalzActions_6x19_t;

// 0x1B: Unknown (不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x1B {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x1B_t;

// 0x1C: 删除NPC (指令生效范围; 仅限游戏; 不支持 Episode 3)
typedef struct subcmd_bb_destory_npc {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_destory_npc_t;

// 0x1D: Set position (existing clients send when a new client joins a lobby/game)
// Packet used to update other people when a player warps to another area
typedef struct subcmd_set_area_1D {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float y;
    float z;
    uint32_t unknown_a1;
} PACKED subcmd_set_area_1D_t;

// 0x1F: subcmd_set_area_1F (指令生效范围; 大厅和游戏)
typedef struct subcmd_set_area_1F {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED subcmd_set_area_1F_t;

// 0x1F: subcmd_bb_set_area_1F (指令生效范围; 大厅和游戏)
typedef struct subcmd_bb_set_area_1F {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED subcmd_bb_set_area_1F_t;

// 0x20: Set position (existing clients send when a new client joins a lobby/game)
// Packet used to update other people when a player warps to another area
typedef struct subcmd_set_area_20 {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float y;
    float z;
    uint32_t unknown_a1;
} PACKED subcmd_set_area_20_t;

// 0x20: Set position (existing clients send when a new client joins a lobby/game)
// Packet used to update other people when a player warps to another area
typedef struct subcmd_bb_set_area_20 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float y;
    float z;
    uint32_t unknown_a1;
} PACKED subcmd_bb_set_area_20_t;

// 0x21: Inter-level warp
typedef struct subcmd_inter_level_warp {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED subcmd_inter_level_warp_t;

// 0x21: Inter-level warp
typedef struct subcmd_bb_inter_level_warp {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED subcmd_bb_inter_level_warp_t;

// 0x22: Set player invisible
// 0x23: Set player visible
typedef struct subcmd_bb_set_player_visibility_6x22_6x23 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_set_player_visibility_6x22_6x23_t;

// 0x24: subcmd_bb_set_pos_0x24 (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_set_pos_0x24 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk1;
    uint16_t unused;
    float x;
    float y;
    float z;
} PACKED subcmd_bb_set_pos_0x24_t;

// 0x25: Equip item
// Packet sent by clients to equip/unequip an item.
// 0x26: Unequip item
// Same format as 0x25
typedef struct subcmd_bb_equip {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
    uint32_t equip_slot; // Unused for 0x26 (unequip item)
} PACKED subcmd_bb_equip_t;

// 0x27: Use item
// Packet used when an item has been used
typedef struct subcmd_use_item {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
} PACKED subcmd_use_item_t;

// 0x27: Use item
// Packet used when an item has been used
typedef struct subcmd_bb_use_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
} PACKED subcmd_bb_use_item_t;

//[2023年06月13日 20:56:21:470] 截获(4905): subcmd-bb.c 4905 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月13日 20:56:21:499] 截获(4905):
//( 00000000 )   14 00 60 00 00 00 00 00  28 03 00 00 00 00 01 00 ..`.....(.......
//( 00000010 )   02 00 81 00                                     ..?
//
//[2023年06月13日 20:56:30:522] 截获(4905): subcmd-bb.c 4905 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月13日 20:56:31:130] 截获(4905):
//( 00000000 )   14 00 60 00 00 00 00 00  28 03 00 00 00 00 01 00 ..`.....(.......
//( 00000010 )   03 00 01 00                                     ....
//
//[2023年06月13日 20:56:35:643] 截获(4905): subcmd-bb.c 4905 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月13日 20:56:35:661] 截获(4905):
//( 00000000 )   14 00 60 00 00 00 00 00  28 03 00 00 00 00 01 00 ..`.....(.......
//( 00000010 )   03 00 01 00                                     ....

// 0x28: Feed MAG
typedef struct subcmd_bb_feed_mag {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t mag_item_id;
    uint32_t fed_item_id;
} PACKED subcmd_bb_feed_mag_t;

// 0x29: Delete inventory item (via bank deposit / sale / feeding MAG)
// This subcommand is also used for reducing the size of stacks - if amount is
// less than the stack count, the item is not deleted and its ID remains valid.
// Packet used to destroy an item on the map or to remove an item from a
// client's inventory
typedef struct subcmd_destroy_item {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
    uint32_t amount;
} PACKED subcmd_destroy_item_t;

// 0x29: Delete inventory item (via bank deposit / sale / feeding MAG)
// This subcommand is also used for reducing the size of stacks - if amount is
// less than the stack count, the item is not deleted and its ID remains valid.
// Packet used to destroy an item on the map or to remove an item from a
// client's inventory
typedef struct subcmd_bb_destroy_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
    uint32_t amount;
} PACKED subcmd_bb_destroy_item_t;

// 0x2A: Drop item
// Packet used when a client drops an item from their inventory
typedef struct subcmd_drop_item {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk; // Should be 1... maybe amount?
    uint16_t area;
    uint32_t item_id;
    float x;
    float y;
    float z;
} PACKED subcmd_drop_item_t;

// 0x2A: Drop item
// Packet used when a client drops an item from their inventory
typedef struct subcmd_bb_drop_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk; // Should be 1... maybe amount?
    uint16_t area;
    uint32_t item_id;
    float x;
    float y;
    float z;
} PACKED subcmd_bb_drop_item_t;


// 0x2B: Create item in inventory (e.g. via tekker or bank withdraw)
// Packet used to take an item from the bank.
typedef struct subcmd_take_item {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t data;
    uint32_t unk; //DC版本不存在
} PACKED subcmd_take_item_t;

// 0x2B: Create item in inventory (e.g. via tekker or bank withdraw)
// Packet used to take an item from the bank.
// BB should never send this command - inventory items should only be
// created by the server in response to shop buy / bank withdraw / etc. reqs
typedef struct subcmd_bb_take_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t data;
    uint8_t unused1;
    uint8_t unknown_a2;
    uint16_t unused2;
} PACKED subcmd_bb_take_item_t;

// 0x2C: Talk to NPC
// Packet sent when talking to an NPC on Pioneer 2 (and other purposes).
typedef struct subcmd_select_menu {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk;           /* Always 0xFFFF for NPCs? */
    uint16_t zero;          /* Always 0? */
    float x;
    float z;
    float unused2;       /* Always zero? */
} PACKED subcmd_select_menu_t;

// 0x2C: Talk to NPC
// Packet sent when talking to an NPC on Pioneer 2 (and other purposes).
typedef struct subcmd_bb_select_menu {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk;           /* Always 0xFFFF for NPCs? */
    uint16_t zero;          /* Always 0? 这是菜单吧? 0xFFFF0000*/
    float x;
    float z;
    float unused2;          /* Always zero? */
} PACKED subcmd_bb_select_menu_t;

// 0x2D: Done Select
typedef struct subcmd_select_done {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_select_done_t;

// 0x2D: Done Select
typedef struct subcmd_bb_select_done {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_select_done_t;

// 0x2E: Set and/or clear player flags
typedef struct subcmd_set_or_clear_player_flags {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t and_mask;
    uint32_t or_mask;
} PACKED subcmd_set_or_clear_player_flags_t;

// 0x2E: Set and/or clear player flags
typedef struct subcmd_bb_set_or_clear_player_flags {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t and_mask;
    uint32_t or_mask;
} PACKED subcmd_bb_set_or_clear_player_flags_t;

// 0x2F: Hit by enemy
typedef struct subcmd_hit_by_enemy {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t hit_type; // 0 = set HP, 1 = add/subtract HP, 2 = add/sub fixed HP
    uint16_t damage;
    uint16_t client_id;
} PACKED subcmd_hit_by_enemy_t;

// 0x2F: Hit by enemy
typedef struct subcmd_bb_hit_by_enemy {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t hit_type; // 0 = set HP, 1 = add/subtract HP, 2 = add/sub fixed HP
    uint16_t damage;
    uint16_t client_id;
} PACKED subcmd_bb_hit_by_enemy_t;

// 0x30: Level up
// Packet sent to clients regarding a level up.
typedef struct subcmd_level_up {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t atp;
    uint16_t mst;
    uint16_t evp;
    uint16_t hp;
    uint16_t dfp;
    uint16_t ata;
    uint32_t level;
} PACKED subcmd_level_up_t;

// 0x30: Level up
// Packet sent to clients regarding a level up. (Blue Burst)
typedef struct subcmd_bb_level_up {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t atp;
    uint16_t mst;
    uint16_t evp;
    uint16_t hp;
    uint16_t dfp;
    uint16_t ata;
    uint32_t level;

    //le_uint16_t level;
    //le_uint16_t unknown_a1; // Must be 0 or 1
} PACKED subcmd_bb_level_up_t;

// 0x31: Medical center
// 0x32: Unknown (occurs when using Medical Center)
typedef struct subcmd_use_medical_center {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_use_medical_center_t;

// 0x31: Medical center
// 0x32: Unknown (occurs when using Medical Center)
typedef struct subcmd_bb_use_medical_center {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_use_medical_center_t;

// 0x33: Revive player (e.g. with moon atomizer)
struct subcmd_RevivePlayer_6x33 {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t unused;
} PACKED;

// 0x34: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x35: Invalid subcommand

// 0x36: Unknown (指令生效范围; 仅限游戏)
// This subcommand is completely ignored (at least, by PSO GC).


// 0x37: Photon blast 光子爆发
typedef struct subcmd_photon_blast {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED subcmd_photon_blast_t;

// 0x37: Photon blast 光子爆发
typedef struct subcmd_bb_photon_blast {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1; //可能是flags
    uint16_t unused;
} PACKED subcmd_bb_photon_blast_t;

// 0x38: Unknown
typedef struct subcmd_bb_Unknown_6x38 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED subcmd_bb_Unknown_6x38_t;

// 0x39: Photon blast ready
typedef struct subcmd_bb_photon_blast_ready {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_photon_blast_ready_t;

// 0x3A: Unknown (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_Unknown_6x3A {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x3A_t;

// 0x3A: 通知其他玩家该玩家离开了游戏 (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_game_client_leave {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_game_client_leave_t;

// 0x3B: Unknown (指令生效范围; 大厅和游戏)
typedef struct subcmd_bb_Unknown_6x3B {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x3B_t;

// 0x3E: Stop moving
// 0x3F: Set position
// Packets used to set a user's position
typedef struct subcmd_set_pos {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float w;
    float x;
    float y;
    float z;
} PACKED subcmd_set_pos_t;

// 0x3E: Stop moving
// 0x3F: Set position
// Packets used to set a user's position
typedef struct subcmd_bb_set_pos {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float w;
    float x;
    float y;
    float z;
} PACKED subcmd_bb_set_pos_t;

// 0x40: Walk
// 0x42: Run
// Packet used for moving around
typedef struct subcmd_move {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    float x;
    float z;
    uint32_t unused2;   /* Not present in 0x42 */
} PACKED subcmd_move_t;

// 0x40: Walk
// 0x42: Run
// Packet used for moving around
typedef struct subcmd_bb_move {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    float x;
    float z;
    uint32_t unused2;   /* Not present in 0x42 */
} PACKED subcmd_bb_move_t;

// 0x41: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x43: First attack
// 0x44: Second attack
// Same format as 0x43
// 0x45: Third attack
// Same format as 0x43
typedef struct subcmd_bb_natk {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED subcmd_bb_natk_t;

typedef struct enemy_entry {
    uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
    uint16_t unused;
} target_entry_t;

// 0x46: Attack finished (sent after each of 43, 44, and 45)
// Packet sent when an object is hit by a physical attack.
typedef struct subcmd_objhit_phys {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t hit_count;
    target_entry_t objects[];
} PACKED subcmd_objhit_phys_t;

// 0x46: Attack finished (sent after each of 43, 44, and 45)
// Packet sent when an object is hit by a physical attack.
typedef struct subcmd_bb_objhit_phys {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t hit_count;
    target_entry_t objects[];
} PACKED subcmd_bb_objhit_phys_t;


// 0x47: 释放科技法术
// Packet sent when an object is hit by a technique.
typedef struct subcmd_objhit_tech {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t tech;
    // Note: The level here isn't the actual tech level that was cast, if the
    // level is > 15. In that case, a 0x8D is sent first, which contains the
    // additional level which is added to this level at cast time. They probably
    // did this for legacy reasons when dealing with v1/v2 compatibility, and
    // never cleaned it up.
    uint8_t level;
    uint8_t hit_count;
    target_entry_t objects[];
} PACKED subcmd_objhit_tech_t;

// 0x47: 释放科技法术
// Packet sent when an object is hit by a technique.
typedef struct subcmd_bb_objhit_tech {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t technique_number;
    /*注：如果等级>15，这里的等级不是实际的技术等级。
    在这种情况下，首先发送一个6x8D，其中包含在铸造时添加到此级别的附加级别。
    在处理v1/v2兼容性时，
    他们这样做可能是出于遗留的原因，从未清理过。*/
    uint8_t level;
    uint8_t target_count;
    target_entry_t objects[];
} PACKED subcmd_bb_objhit_tech_t;

// 0x48: 释放科技法术 complete
// Packet used after a client uses a tech.
typedef struct subcmd_used_tech {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t technique_number;
    uint16_t level;
} PACKED subcmd_used_tech_t;

// 0x48: 释放科技法术 complete
// Packet used after a client uses a tech.
typedef struct subcmd_bb_used_tech {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t technique_number;
    uint16_t level;
} PACKED subcmd_bb_used_tech_t;

// 0x49: Subtract PB energy
typedef struct subcmd_bb_subtract_PB_energy_6x49 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t unknown_a1;
    uint8_t unknown_a2;
    uint16_t entry_count;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
    struct {
        uint16_t unknown_a1;
        uint16_t unknown_a2;
    } entries[14];
} PACKED subcmd_bb_subtract_PB_energy_6x49_t;

// 0x4A: Fully shield attack
typedef struct subcmd_bb_defense_damage {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_defense_damage_t;

// 0x4B: Hit by enemy
// Packet used when a client takes damage.
// 0x4C: Hit by enemy
// Same format as 0x4B
typedef struct subcmd_take_damage {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t angle;
    uint16_t damage;
    float x_velocity;
    float z_velocity;
} PACKED subcmd_take_damage_t;

// 0x4B: Hit by enemy
// Packet used when a client takes damage.
// 0x4C: Hit by enemy
// Same format as 0x4B
typedef struct subcmd_bb_take_damage {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t angle;
    uint16_t damage;
    float x_velocity;
    float z_velocity;
} PACKED subcmd_bb_take_damage_t;

// 0x4D: death sync (指令生效范围; 大厅和游戏)
typedef struct subcmd_bb_death_sync {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t flag;
    uint8_t unk2[3];
} PACKED subcmd_bb_death_sync_t;

// 0x4E: Unknown (指令生效范围; 大厅和游戏)
typedef struct subcmd_bb_cmd_4e {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_cmd_4e_t;

// 0x4F: SUBCMD60_PLAYER_SAVED (指令生效范围; 大厅和游戏)
typedef struct subcmd_bb_player_saved {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_player_saved_t;

// 0x50: Unknown (指令生效范围; 大厅和游戏)
typedef struct subcmd_bb_switch_req {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unk2;
} PACKED subcmd_bb_switch_req_t;

// 0x51: Invalid subcommand

// 0x52: Toggle shop/bank interaction
typedef struct subcmd_bb_menu_req {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED subcmd_bb_menu_req_t;

// 0x53: Unknown (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_Unknown_6x53 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x53_t;

// 0x54: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x55: Intra-map warp
typedef struct subcmd_bb_map_warp {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t area; //0x8000 为 总督府 0x0000 为先驱者2号
    uint16_t unused;
    float x1;
    float y1;
    float z1;
    float x2;
    float y2;
    float z2;
} PACKED subcmd_bb_map_warp_t;

// 0x56: 生成座椅时触发 (指令生效范围; 大厅和游戏)
typedef struct subcmd_bb_Unknown_6x56 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
    float x;
    float y;
    float z;
} PACKED subcmd_bb_Unknown_6x56_t;

// 0x57: Unknown (指令生效范围; 大厅和游戏)
typedef struct subcmd_bb_Unknown_6x57 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x57_t;

// 0x58: CTRL+W 触发 SUBCMD60_LOBBY_ACTION (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_lobby_act {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t act_id;
    uint16_t unused;                 // 0x0000
} PACKED subcmd_bb_lobby_act_t;

// 0x59: 拾取必会删除地图物品数据结构
typedef struct subcmd_bb_destroy_map_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t area;
    uint32_t item_id;
} PACKED subcmd_bb_destroy_map_item_t;

// 0x5A: Request to pick up item
typedef struct subcmd_bb_PickUpItemRequest_6x5A {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
    uint16_t area;
    uint16_t unused;
} PACKED subcmd_bb_PickUpItemRequest_6x5A_t;

// 0x5A: Request to pick up item
typedef struct subcmd_pick_up {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
    uint16_t area;
    uint16_t unused;
} PACKED subcmd_pick_up_t;

// 0x5A: Request to pick up item
typedef struct subcmd_bb_pick_up {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
    uint16_t area;
    uint16_t unused;
} PACKED subcmd_bb_pick_up_t;

// 0x5C: Unknown
typedef struct subcmd_bb_Unknown_6x5C {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
} PACKED subcmd_bb_Unknown_6x5C_t;

// 0x5D: Drop meseta or stacked item
// Packet used when dropping part of a stack of items
typedef struct subcmd_drop_stack {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t area;
    uint16_t unk;
    float x;
    float z;
    item_t data;
    uint32_t two; //DC没有
} PACKED subcmd_drop_stack_t;

// 0x5D: Drop meseta or stacked item
// Packet used when dropping part of a stack of items
typedef struct subcmd_bb_drop_stack {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float z;
    item_t data;
    uint32_t two;
} PACKED subcmd_bb_drop_stack_t;

// 0x5E: Buy item at shop
// Packet used when buying an item from the shop
typedef struct subcmd_buy {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t data;
} PACKED subcmd_buy_t;

// 0x5E: change_chair
// 旋转椅子或改变椅子方向触发
typedef struct subcmd_bb_change_chair {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t data[0];
} PACKED subcmd_bb_change_chair_t;

// 0x5F: Drop item from box/enemy
typedef struct subcmd_item_drop {
    uint8_t area;
    uint8_t from_enemy;
    uint16_t request_id; // < 0x0B50 if from_enemy != 0; otherwise < 0x0BA0
    float x;
    float z;
    uint32_t unk1;
    item_t item;
    uint32_t item2;
} PACKED subcmd_item_drop_t;

// 0x5F: Drop item from box/enemy
typedef struct subcmd_itemgen {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    subcmd_item_drop_t data;
} PACKED subcmd_itemgen_t;

// 0x5F: Drop item from box/enemy
typedef struct subcmd_bb_itemgen {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    subcmd_item_drop_t data;
} PACKED subcmd_bb_itemgen_t;

// 0x60: Request for item drop (handled by the server on BB)
// Request drop from enemy (DC/PC/GC) or box (DC/PC)
typedef struct subcmd_itemreq {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t area;
    uint8_t pt_index;
    uint16_t request_id;
    float x;
    float z;
    uint32_t unk2[2];
} PACKED subcmd_itemreq_t;

// 0x60: Request for item drop (handled by the server on BB)
// Request drop from enemy (Blue Burst)
typedef struct subcmd_bb_itemreq {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t area;
    uint8_t pt_index;
    uint16_t request_id;
    float x;
    float z;
    uint16_t unk1;
    uint16_t unk2;
    uint32_t unk3;
} PACKED subcmd_bb_itemreq_t;

// 0x61: levelup_req
typedef struct subcmd_bb_levelup_req {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr; /* 0x00 0x85 0x03 0x61*/
    uint16_t unk1; /* 0x0002*/
    uint16_t unk2; /* 房主 0x0021? 房2 0x0001*/
    uint32_t unk3;
} PACKED subcmd_bb_levelup_req_t;

// 0x62: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

//[2023年06月10日 14:10:45:951] 截获(4877): subcmd-bb.c 4877 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月10日 14:10:45:967] 截获(4877):
//( 00000000 )   14 00 60 00 00 00 00 00  63 03 D3 1D 01 00 81 00 ..`.....c.?..?
//( 00000010 )   01 00 00 00                                     ....
//
//[2023年06月10日 14:10:46:010] 截获(4877): subcmd-bb.c 4877 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月10日 14:10:46:031] 截获(4877):
//( 00000000 )   14 00 60 00 00 00 00 00  63 03 D2 1D 03 00 81 00 ..`.....c.?..?
//( 00000010 )   01 00 00 00                                     ....
//
//[2023年06月10日 14:10:46:071] 截获(4877): subcmd-bb.c 4877 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月10日 14:10:46:100] 截获(4877):
//( 00000000 )   14 00 60 00 00 00 00 00  63 03 D2 1D 04 00 81 00 ..`.....c.?..?
//( 00000010 )   01 00 00 00                                     ....
//
//[2023年06月10日 14:10:49:818] 截获(4877): subcmd-bb.c 4877 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月10日 14:10:49:841] 截获(4877):
//( 00000000 )   14 00 60 00 00 00 00 00  63 03 D2 1D 05 00 81 00 ..`.....c.?..?
//( 00000010 )   01 00 00 00                                     ....
//
//[2023年06月10日 14:10:54:117] 截获(4877): subcmd-bb.c 4877 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月10日 14:10:54:134] 截获(4877):
//( 00000000 )   14 00 60 00 00 00 00 00  63 03 D2 1D 06 00 81 00 ..`.....c.?..?
//( 00000010 )   01 00 00 00                                     ....
// 0x63: 当地面物品过多时,摧毁6个最早的物品 (用于过多掉落物时)
typedef struct subcmd_bb_destory_ground_item {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_id;
    uint32_t area;
} PACKED subcmd_bb_destory_ground_item_t;

// 0x64: Unknown (不支持 Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x65: Unknown (不支持 Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

//[2023年07月06日 14:40 : 19 : 691] 错误(subcmd_handle.c 0111) : subcmd_get_handler 未完成对 0x60 0x66 版本 5 的处理
//[2023年07月06日 14:40 : 19 : 707] 调试(subcmd_handle_60.c 3061) : 未知 0x60 指令 : 0x66
//(00000000)   14 00 60 00 00 00 00 00   66 03 7B 00 00 00 FF FF  ..`.....f.{...
//(00000010)   7E 15 FF FF                                     ~.
// 0x66: 使用星之粉
typedef struct subcmd_bb_use_star_atomizer {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t target_client_ids[4];
} PACKED subcmd_bb_use_star_atomizer_t;

// 0x67: 生成怪物设置 还需要完成一个数据包发送给每个玩家
typedef struct subcmd_bb_create_enemy_set {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    // unused1 could be area; the client checks this againset a global but the
    // logic is the same in both branches
    uint32_t unused1;
    uint32_t unknown_a1;
    uint32_t unused2;
} PACKED subcmd_bb_create_enemy_set_t;

// 0x68: Telepipe/Ryuker
// Packet sent to create a pipe. Most of this, I haven't bothered looking too much at...
typedef struct subcmd_pipe {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t area;
    uint16_t unused1;
    uint8_t unused2[2];
    float x;
    float y;
    float z;
    uint32_t unused3;                  
} PACKED subcmd_pipe_t;

// 0x68: Telepipe/Ryuker
typedef struct subcmd_bb_pipe {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t area;
    uint16_t unused1;         /* Location is in here. */
    uint8_t unused2[2];
    float x;
    float y;
    float z;
    uint32_t unused3;
} PACKED subcmd_bb_pipe_t;

// 0x69: Unknown (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_Unknown_6x69 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t client_id2;
    uint16_t unknown_a1;
    uint16_t what; // 0-3; logic is very different for each value
    uint16_t unknown_a2;
} PACKED subcmd_bb_Unknown_6x69_t;


// 0x6A: Unknown (指令生效范围; 仅限游戏; 不支持 Episode 3)
//(00000000)   10 00 60 00 00 00 00 00  6A 02 38 40 02 00 66 00
//(00000000)   10 00 60 00 00 00 00 00  6A 02 1F 40 02 00 66 00
//(00000000)   10 00 60 00 00 00 00 00  6A 02 1F 40 02 00 66 00    ..`.....j..@..f.
typedef struct subcmd_bb_Unknown_6x6A {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED subcmd_bb_Unknown_6x6A_t;

// 0x6B: Sync enemy state (used while loading into game; same header format as 6E)
// Decompressed format is a list of these
struct G_SyncGameStateHeader_6x6B_6x6C_6x6D_6x6E {
    unused_hdr_t shdr;
    uint32_t decompressed_size;
    uint32_t compressed_size; // Must be <= subcommand_size - 0x10
    uint8_t data[0]; // BC0-compressed data follows here (see bc0_decompress)
} PACKED;

typedef struct subcmd_bb_G_SyncEnemyState_6x6B_Entry_Decompressed {
    // TODO: Verify this format on DC and PC. It appears correct for GC and BB.
    uint32_t unknown_a1; // Possibly some kind of flags
    // enemy_index is not the same as enemy_id, unfortunately - the enemy_id sent
    // in the 6x76 command when an enemy is killed does not match enemy_index
    uint16_t enemy_index; // FFFF = enemy is dead
    uint16_t damage_taken;
    uint8_t unknown_a4;
    uint8_t unknown_a5;
    uint8_t unknown_a6;
    uint8_t unknown_a7;
} PACKED subcmd_bb_G_SyncEnemyState_6x6B_Entry_Decompressed;

// 0x6C: Sync object state (used while loading into game; same header format as 6E)
// Decompressed format is a list of these
struct G_SyncObjectState_6x6C_Entry_Decompressed {
    // TODO: Verify this format on DC and PC. It appears correct for GC and BB.
    uint16_t flags;
    uint16_t object_index;
} PACKED;

typedef struct FloorItem {
    uint16_t area;
    uint16_t unknown_a1;
    float x;
    float z;
    uint16_t unknown_a2;
    // The drop number is scoped to the area and increments by 1 each time an
    // item is dropped. The last item dropped in each area has drop_number equal
    // to total_items_dropped_per_area[area - 1] - 1.
    uint16_t drop_number;
    item_t data;
} PACKED floor_item_t;

// 0x6D: Sync item state (used while loading into game; same header format as 6E)
struct G_SyncItemState_6x6D_Decompressed {
    // TODO: Verify this format on DC and PC. It appears correct for GC and BB.
    // Note: 16 vs. 15 is not a bug here - there really is an extra field in the
    // total drop count vs. the floor item count. Despite this, Pioneer 2 or Lab
    // (area 0) isn't included in total_items_dropped_per_area (so Forest 1 is [0]
    // in that array) but it is included in floor_item_count_per_area (so Forest 1
    // is [1] there).
    uint16_t total_items_dropped_per_area[16];
    // Only [0]-[3] in this array are ever actually used in normal gameplay, but
    // the client fills in all 12 of these with reasonable values.
    uint32_t next_item_id_per_player[12];
    uint32_t floor_item_count_per_area[15];
    // Variable-length field follows:
    // FloorItem items[sum(floor_item_count_per_area)];
    floor_item_t items[0];
} PACKED;

// 0x6E: Sync flag state (used while loading into game)
typedef struct subcmd_bb_SyncGameStateHeader_6x6B_6x6C_6x6D_6x6E {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t size;
    uint32_t subcommand_size;
    uint32_t decompressed_size;
    uint32_t compressed_size; // Must be <= subcommand_size - 0x10
    // BC0-compressed data follows here (use bc0_decompress from Compression.hh)
} PACKED subcmd_bb_SyncGameStateHeader_6x6B_6x6C_6x6D_6x6E_t;

// 0x6F: subcmd_bb_send_quest_data1 (used while loading into game) 8 + 4 + 512 + 4
typedef struct subcmd_bb_send_quest_data1 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t quest_data1[0x208];
    uint32_t padding;
} PACKED subcmd_bb_send_quest_data1_t;

// 0x70: Sync player disp data and inventory (used while loading into game)
// Annoyingly, they didn't use the same format as the 65/67/68 commands here,
// and instead rearranged a bunch of things.
// TODO: Some missing fields should be easy to find in the future (e.g. when the
// sending player doesn't have 0 meseta, for example)
// Packet used to communicate current state of players in-game while a new player is bursting. 

typedef struct subcmd_burst_pldata_old {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
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
    psocn_dress_data_t dress_data;  /* 00D4 */
    psocn_pl_stats_t stats;         /* 0124 */
    uint8_t opt_flag1;              /* 0132  306 - 315 size 10 */
    uint8_t opt_flag2;
    uint8_t opt_flag3;
    uint8_t opt_flag4;
    uint8_t opt_flag5;
    uint8_t opt_flag6;
    uint8_t opt_flag7;
    uint8_t opt_flag8;
    uint8_t opt_flag9;
    uint8_t opt_flag10;
    //uint8_t opt_flag[0x0A];         /* 0132  306 - 315 size 10 */
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    inventory_t inv;
    uint32_t zero;          /* Unused? */
    /* Xbox has a little bit more at the end than other versions... Probably
       safe to just ignore it. */
} PACKED subcmd_burst_pldata_old_t;

static int char_bb_size2221233 = sizeof(subcmd_burst_pldata_old_t);

typedef struct subcmd_burst_pldata {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t size_minus_4;  /* ??? 0xC0040000*/
    uint32_t lobby_num;
    uint32_t unused2;  /* ??? 0x00000010*/
    float x;
    float y;
    float z;

    uint8_t unk2[0x54];

    uint32_t player_tag;            /* 0074 */
    uint32_t guildcard;             /* 0078 */
    
    uint32_t unknown_a6[2];         /* 007C */
    struct {                        /* 0084 */
        uint16_t unknown_a1[2];
        uint32_t unknown_a2[6];
    } unknown_a7;
    uint32_t unknown_a8;            /* 00A0 */
    uint8_t unknown_a9[0x14];       /* 00A4 */
    uint32_t unknown_a10;           /* 00B8 */
    uint32_t unknown_a11;           /* 00BC */

    uint8_t technique_levels[0x14]; /* 00C0 */ // Last byte is uninitialized

    psocn_dress_data_t dress_data;  /* 00D4 */
    psocn_pl_stats_t stats;         /* 0124 */
    uint8_t opt_flag1;              /* 0132  306 - 315 size 10 */
    uint8_t opt_flag2;
    uint8_t opt_flag3;
    uint8_t opt_flag4;
    uint8_t opt_flag5;
    uint8_t opt_flag6;
    uint8_t opt_flag7;
    uint8_t opt_flag8;
    uint8_t opt_flag9;
    uint8_t opt_flag10;
    //uint8_t opt_flag[0x0A];         /* 0132  306 - 315 size 10 */
    uint32_t level;                 /* 013C  316 - 319 size 4 */
    uint32_t exp;                   /* 0140  320 - 323 size 4 */
    uint32_t meseta;                /* 0144  324 - 327 size 4 */
    inventory_t inv;                /* 0148  328 - 1171 size 844 */
    uint32_t unknown_a15;           /* 0494 1172 - 1176 size 4 */
    /* Xbox has a little bit more at the end than other versions... Probably
       safe to just ignore it. */
} PACKED subcmd_burst_pldata_t;

static int char_bb_size2223 = sizeof(subcmd_burst_pldata_t);

// 0x70: Sync player disp data and inventory (used while loading into game)
// Annoyingly, they didn't use the same format as the 65/67/68 commands here,
// and instead rearranged a bunch of things.
// TODO: Some missing fields should be easy to find in the future (e.g. when the
// sending player doesn't have 0 meseta, for example)
// Packet used to communicate current state of players in-game while a new player is bursting. 
// size = 0x04C8 DC 1176 1224 - 1176 = 48 1242 - 1224 = 18
typedef struct subcmd_bb_burst_pldata {
    bb_pkt_hdr_t hdr;               /* 0000 8 */
    client_id_hdr_t shdr;           /* 0008 4 */
    // Offsets in this struct are relative to the overall command header
    uint32_t size_minus_4;          /* 000C 4  0x000004C0*/
    // [1] and [3] in this array (and maybe [2] also) appear to be le_floats;
    // they could be the player's current (x, y, z) coords
    uint32_t lobby_num;             /* 0010 4  0x00000001*/
    uint32_t unused2;               /* 0014 4  0x01000000*/
    float x;                        /* 0018 24 - 43 */
    float y;                        /* 001C 44 - 43 */
    float z;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
    uint32_t unk4;
    uint32_t unk5;
    uint8_t unk6[64];
    uint32_t player_tag;
    uint32_t guildcard;
    uint8_t unk7[64];
    uint32_t unk8;
    uint8_t techniques[20];
    psocn_dress_data_t dress_data;
    uint16_t name[0x0C];
    uint8_t hw_info[0x08];
    psocn_pl_stats_t stats;
    uint8_t opt_flag1;
    uint8_t opt_flag2;
    uint8_t opt_flag3;
    uint8_t opt_flag4;
    uint8_t opt_flag5;
    uint8_t opt_flag6;
    uint8_t opt_flag7;
    uint8_t opt_flag8;
    uint8_t opt_flag9;
    uint8_t opt_flag10;
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    inventory_t inv;                /* 0170 44 - 43 */
    uint32_t unused[4];               /* Unused? */
    /* Xbox has a little bit more at the end than other versions... Probably
       safe to just ignore it. */
} PACKED subcmd_bb_burst_pldata_t;

static int char_bb_size222 = sizeof(subcmd_bb_burst_pldata_t);

typedef struct G_Unknown_6x70 {
    dc_pkt_hdr_t hdr;               /* 0000 0 */
    client_id_hdr_t shdr;           /* 0008 8 */
    // Offsets in this struct are relative to the overall command header
    uint32_t size_minus_4;          /* 000C 12 ? 0x0000040C*/
    // [1] and [3] in this array (and maybe [2] also) appear to be le_floats;
    // they could be the player's current (x, y, z) coords
    uint32_t lobby_num;             /* 0010 16  0x00000001*/
    uint32_t unused2;               /* 0014 20  0x01000000*/

    float x;                        /* 0018 24 - 43 */
    float y;
    float z;

    uint32_t unknown_a2[2];         /* 0018 24 - 43 */

    uint16_t unknown_a3[4];         /* 002C */
    uint32_t unknown_a4[3][5];      /* 0034 */
    uint32_t unknown_a5;            /* 0070 */

    uint32_t player_tag;            /* 0074 */
    uint32_t guild_card_number;     /* 0078 */

    uint32_t unknown_a6[2];         /* 007C */
    struct {                        /* 0084 */
        uint16_t unknown_a1[2];
        uint32_t unknown_a2[6];
    } unknown_a7;
    uint32_t unknown_a8;            /* 00A0 */
    uint8_t unknown_a9[0x14];       /* 00A4 */
    uint32_t unknown_a10;           /* 00B8 */
    uint32_t unknown_a11;           /* 00BC */


    uint8_t technique_levels[0x14]; /* 00C0 */ // Last byte is uninitialized

    psocn_dress_data_t dress_data;  /* 00D4 */
    psocn_pl_stats_t stats;         /* 0124 */
    uint8_t opt_flag1;              /* 0132  306 - 315 size 10 */
    uint8_t opt_flag2;
    uint8_t opt_flag3;
    uint8_t opt_flag4;
    uint8_t opt_flag5;
    uint8_t opt_flag6;
    uint8_t opt_flag7;
    uint8_t opt_flag8;
    uint8_t opt_flag9;
    uint8_t opt_flag10;
    //uint8_t opt_flag[0x0A];         /* 0132  306 - 315 size 10 */
    uint32_t level;                 /* 013C  316 - 319 size 4 */
    uint32_t exp;                   /* 0140  320 - 323 size 4 */
    uint32_t meseta;                /* 0144  324 - 327 size 4 */
    inventory_t inv;                /* 0148  328 - 1171 size 844 */
    uint32_t unknown_a15;           /* 0494 1172 - 1176 size 4 */
} G_Unknown_6x70_t;

static int char_bb_size22322 = sizeof(G_Unknown_6x70_t);


// 0x71: loading_burst (used while loading into game) SUBCMD62_BURST6
typedef struct subcmd_bb_loading_burst {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
} PACKED subcmd_bb_loading_burst_t;

// 0x72: end_burst (used while loading into game) SUBCMD60_BURST_DONE
typedef struct subcmd_bb_end_burst {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
} PACKED subcmd_bb_end_burst_t;

// 0x73: warp packet 舰船传送数据包
typedef struct subcmd_dc_warp_ship {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED subcmd_dc_warp_ship_t;

// 0x73: warp packet 舰船传送数据包
typedef struct subcmd_pc_warp_ship {
    pc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED subcmd_pc_warp_ship_t;

// 0x73: warp packet 舰船传送数据包
typedef struct subcmd_bb_warp_ship {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED subcmd_bb_warp_ship_t;

typedef struct word_select {
    uint16_t num_words;
    uint16_t ws_type;
    uint16_t words[8];
    uint32_t unused1;
    uint32_t unused2;
} PACKED word_select_t;

// 0x74: Word select
// Packet used for word select
typedef struct subcmd_word_select {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t client_id_gc;
    word_select_t data;
} PACKED subcmd_word_select_t;

// 0x74: Word select
// Packet used for word select
typedef struct subcmd_bb_word_select {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    word_select_t data;
} PACKED subcmd_bb_word_select_t;

// 0x75: Phase setup (指令生效范围; 仅限游戏)
// TODO: Not sure about PC format here. Is this first struct DC-only?
typedef struct subcmd_set_flag {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t flag; // Must be < 0x400
    uint16_t action; // Must be 0 or 1
} PACKED subcmd_set_flag_t;

// 0x75: Phase setup (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_set_flag {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t flag; // Must be < 0x400
    uint16_t action; // Must be 0 or 1
    uint16_t difficulty;
    uint16_t unused;
} PACKED subcmd_bb_set_flag_t;

// 0x76: Enemy killed
typedef struct subcmd_bb_killed_monster {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t flag;
    uint16_t unk1; // Flags of some sort
} PACKED subcmd_bb_killed_monster_t;

// 0x77: Sync quest data
// Packet sent in response to a quest register sync (sync_register, sync_let,
// or sync_leti in qedit terminology).
typedef struct subcmd_sync_reg {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t register_number; // Must be < 0x100
    uint16_t unused;          /* Probably unused junk again. 0x006B*/
    uint32_t value;
} PACKED subcmd_sync_reg_t;

// 0x77: Sync quest data
// Packet sent in response to a quest register sync (sync_register, sync_let,
// or sync_leti in qedit terminology).
typedef struct subcmd_bb_sync_reg {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t register_number; // Must be < 0x100
    uint16_t unused;          /* Probably unused junk again. */
    uint32_t value;
} PACKED subcmd_bb_sync_reg_t;

// 0x78: Unknown
typedef struct subcmd_bb_Unknown_6x78 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t client_id; // Must be < 12
    uint16_t unused1;
    uint32_t unused2;
} PACKED subcmd_bb_Unknown_6x78_t;

// 0x79: Lobby 14/15 gogo ball (soccer game)
typedef struct subcmd_bb_gogo_ball {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr; // unused 0x0000 踢了 0x6866
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    float x;
    float z;
    uint32_t unknown_a5;         //0x00665900
} PACKED subcmd_bb_gogo_ball_t;

// 0x7A: Unknown
typedef struct subcmd_bb_Unknown_6x7A {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x7A_t;

// 0x7B: Unknown
typedef struct subcmd_bb_Unknown_6x7B {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x7B_t;

//[2023年06月23日 08:21:56:261] 截获(5064): subcmd_bb.c 5064 行 未找到相关指令名称 指令 0x0060 未处理. (数据如下)
//[2023年06月23日 08:21:56:294] 截获(5064):
//( 00000000 )   50 01 60 00 00 00 00 00  7C 52 E6 43 00 00 70 42 P.`.....|R鍯..pB
//( 00000010 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000020 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000030 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000040 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000050 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000060 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000070 )   00 00 01 00 01 00 00 00  15 08 17 F6 05 00 00 00 ...........?...
//( 00000080 )   88 82 02 C4 D2 A4 81 41  11 27 0A C3 09 00 42 00 垈.囊A.'.?.B.
//( 00000090 )   31 00 32 00 33 00 00 00  00 00 00 00 00 00 00 00 1.2.3...........
//( 000000A0 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 000000B0 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 000000C0 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 000000D0 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 000000E0 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 000000F0 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000100 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000110 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000120 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000130 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
//( 00000140 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ................
// 0x7C: Set challenge mode data (指令生效范围; 仅限游戏; 不支持 Episode 3)
// SUBCMD60_GAME_MODE 336字节 8 + 4 + 2 + 2 + 320（challenge data）
typedef struct subcmd_bb_set_challenge_mode_data {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t unknown_a0;
    /* challenge_data */
    uint16_t unknown_a2;
    uint8_t unknown_a3[2];
    uint32_t unknown_a4[0x17];
    uint8_t unknown_a5[4];
    uint16_t unknown_a6;
    uint8_t unknown_a7[2];
    uint32_t unknown_a8;
    uint32_t unknown_a9;
    uint32_t unknown_a10;
    uint32_t unknown_a11;
    uint32_t unknown_a12;
    uint8_t unknown_a13[0x34];
    struct Entry {
        uint32_t unknown_a1;
        uint32_t unknown_a2;
    } PACKED entries[3];
} PACKED subcmd_bb_set_challenge_mode_data_t;

/* Packet used for grave data in C-Mode (Dreamcast) */
typedef struct subcmd_dc_grave {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t unk0;
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
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t unk0;
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
//数据长度 = 0x0150; 336 = 8 + 4 + 2 + 2 + 320
typedef struct subcmd_bb_grave {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id;
    uint16_t unk0;
    uint8_t challenge_data[320];
} PACKED subcmd_bb_grave_t;

static int sdasd = sizeof(subcmd_bb_grave_t);

// 0x7D: 设置对战模数据 (指令生效范围; 仅限游戏; 不支持 Episode 3)
typedef struct subcmd_bb_set_battle_mode_data {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1;        // Must be < 7; used in jump table
    uint8_t unused[3];
    uint32_t target_client_id;  //数据包发送对象
    uint32_t unknown_a2;        //0x00000000
    uint32_t unknown_a3[2];
} PACKED subcmd_bb_set_battle_mode_data_t;

// 0x7E: Unknown (不支持 Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x7F: Unknown (不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x7F {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1[0x20];
} PACKED subcmd_bb_Unknown_6x7F_t;

// 0x80: 触发陷阱 (不支持 Episode 3)
typedef struct subcmd_bb_trigger_trap {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED subcmd_bb_trigger_trap_t;

// 0x81: Unknown
typedef struct subcmd_bb_Unknown_6x81 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x81_t;

// 0x82: Unknown
typedef struct subcmd_bb_Unknown_6x82 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_Unknown_6x82_t;

// 0x83: 放置陷阱
typedef struct subcmd_bb_place_trap {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED subcmd_bb_place_trap_t;

// 0x84: Unknown (指令生效范围; 仅限游戏; 不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x84 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1[6];
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unused;
} PACKED subcmd_bb_Unknown_6x84_t;

// 0x85: Unknown (指令生效范围; 仅限游戏; 不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x85 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t unknown_a1; // Command is ignored unless this is 0
    uint16_t unknown_a2[7]; // Only the first 3 appear to be used
} PACKED subcmd_bb_Unknown_6x85_t;

// 0x86: Hit destructible object (不支持 Episode 3)
typedef struct subcmd_bb_HitDestructibleObject_6x86 {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
} PACKED subcmd_bb_HitDestructibleObject_6x86_t;

// 0x87: Unknown
typedef struct subcmd_bb_Unknown_6x87 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    float unknown_a1;
} PACKED subcmd_bb_Unknown_6x87_t;

// 0x88: Unknown (指令生效范围; 仅限游戏) 在任务中 回到先驱者2号触发
//(00000000)   0C 00 60 00 00 00 00 00  88 01 00 00
typedef struct subcmd_bb_arrow_change {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_arrow_change_t;

// 0x89: subcmd_bb_player_died (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_player_died {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t flag;
    uint16_t unused;
} PACKED subcmd_bb_player_died_t;

// 0x8A: 挑战模式进入后触发 里面有客户端的ID (不支持 Episode 3)
typedef struct subcmd_bb_ch_game_select {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
    uint32_t mode; // Must be < 0x11
} PACKED subcmd_bb_ch_game_select_t;

// 0x8B: Unknown (不支持 Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).
    
// 0x8C: Unknown (不支持 Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x8D: Set technique level override
typedef struct subcmd_bb_set_technique_level_override {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t level_upgrade;
    uint8_t unused1;
    uint16_t unused2;
} PACKED subcmd_bb_set_technique_level_override_t;

// 0x8E: Unknown (不支持 Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x8F: Unknown (不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x8F {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t unknown_a1;
} PACKED subcmd_bb_Unknown_6x8F_t;

// 0x90: Unknown (不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x90 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
} PACKED subcmd_bb_Unknown_6x90_t;

// 0x91: Unknown (指令生效范围; 仅限游戏)
typedef struct subcmd_bb_Unknown_6x91 {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
    uint16_t unknown_a5;
    uint8_t unknown_a6[2];
} PACKED subcmd_bb_Unknown_6x91_t;

// 0x92: Unknown (不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x92 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t unknown_a1;
    float unknown_a2;
} PACKED subcmd_bb_Unknown_6x92_t;

// 0x93: Timed switch activated (不支持 Episode 3)
typedef struct subcmd_bb_timed_switch_activated {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t area;
    uint16_t switch_id;
    uint8_t unknown_a1; // 如果此值为1，则逻辑与其他值不同 
    uint8_t unused[3];
} PACKED subcmd_bb_timed_switch_activated_t;

// 0x94: Warp (不支持 Episode 3)
typedef struct subcmd_bb_inter_level_map_warp {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t area;
    uint8_t unused[2];
} PACKED subcmd_bb_inter_level_map_warp_t;

// 0x95: Unknown (不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x95 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t client_id;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    uint32_t unknown_a3;
} PACKED subcmd_bb_Unknown_6x95_t;

// 0x96: Unknown (不支持 Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x97: Unknown (不支持 Episode 3)
typedef struct subcmd_bb_ch_game_cancel {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t unused1;
    uint32_t mode; // Must be 0 or 1
    uint32_t unused2;
    uint32_t unused3;
} PACKED subcmd_bb_ch_game_cancel_t;

// 0x98: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x99: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x9A: Update player stat (不支持 Episode 3)
typedef struct subcmd_update_player_stat {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    // Values for stat:
    // 0 = subtract HP
    // 1 = subtract TP
    // 2 = subtract Meseta
    // 3 = add HP
    // 4 = add TP
    uint8_t stat_type;
    uint8_t amount;
} PACKED subcmd_update_player_stat_t;

// 0x9A: Update player stat (不支持 Episode 3)
typedef struct subcmd_bb_update_player_stat {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    // Values for stat:
    // 0 = subtract HP
    // 1 = subtract TP
    // 2 = subtract Meseta
    // 3 = add HP
    // 4 = add TP
    uint8_t stat_type;
    uint8_t amount;
} PACKED subcmd_bb_update_player_stat_t;

// 0x9B: Unknown
typedef struct subcmd_bb_Unknown_6x9B {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1;
    uint8_t unused[3];
} PACKED subcmd_bb_Unknown_6x9B_t;

// 0x9C: Unknown (指令生效范围; 仅限游戏; 不支持 Episode 3)
typedef struct subcmd_bb_Unknown_6x9C {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unknown_a1;
} PACKED subcmd_bb_Unknown_6x9C_t;

// 0x9D: 挑战模式结束 移除玩家模式flag (不支持 Episode 3)
typedef struct subcmd_bb_ch_game_failure {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t client_id;
    uint16_t unk;
} PACKED subcmd_bb_ch_game_failure_t;

// 0x9E: Unknown (不支持 Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x9F: Gal Gryphon actions (不支持 PC or Episode 3)
typedef struct subcmd_bb_GalGryphonActions_6x9F {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unknown_a1;
    float x;
    float z;
} PACKED subcmd_bb_GalGryphonActions_6x9F_t;

// 0xA0: Gal Gryphon actions (不支持 PC or Episode 3)
typedef struct subcmd_bb_GalGryphonActions_6xA0 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    float x;
    float y;
    float z;
    uint32_t unknown_a1;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint32_t unknown_a4[4];
} PACKED subcmd_bb_GalGryphonActions_6xA0_t;

// 0xA1: SUBCMD60_SAVE_PLAYER_ACT (不支持 PC)
typedef struct subcmd_bb_save_player_act {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_save_player_act_t;

// 0xA2: Request for item drop from box (不支持 PC; handled by server on BB)
typedef struct subcmd_bb_BoxItemDropRequest_6xA2 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t area;
    uint8_t pt_index;                   /* Always 0x30 */
    uint16_t request_id;
    float x;
    float z;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint8_t unknown_a4[4];
    uint32_t unknown_a5;
    uint32_t unknown_a6;
    uint32_t unknown_a7;
    uint32_t unknown_a8;
} PACKED subcmd_bb_BoxItemDropRequest_6xA2_t;

// 0xA2: Request for item drop from box (不支持 PC; handled by server on BB)
// Request drop from box (GC)
typedef struct subcmd_bitemreq {
    dc_pkt_hdr_t hdr;
    object_id_hdr_t shdr;                      /* 箱子物体 object_id 0x80 0x3F*/
    uint8_t area;
    uint8_t pt_index;                   /* Always 0x30 */
    uint16_t request_id;
    float x;
    float z;
    uint32_t unk2[2];
    uint16_t unk3;
    uint16_t object_id;                      /* 0x80 0x3F? */
    uint32_t unused[3];                 /* All zeroes? */
} PACKED subcmd_bitemreq_t;

// 0xA2: Request for item drop from box (不支持 PC; handled by server on BB)
// Request drop from box (Blue Burst)
typedef struct subcmd_bb_bitemreq {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr;                      /* 箱子物体 object_id 0x80 0x3F*/
    uint8_t area;
    uint8_t pt_index;                   /* Always 0x30 */
    uint16_t request_id;
    float x;
    float z;
    uint32_t unk2[2];
    uint16_t unk3;
    uint16_t object_id;                      /* 0x80 0x3F? */
    uint32_t unused[3];                 /* All zeroes? */
} PACKED subcmd_bb_bitemreq_t;

// 0xA3: Episode 2 boss actions (不支持 PC or Episode 3)
typedef struct subcmd_bb_Episode2BossActions_6xA3 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint8_t unknown_a1;
    uint8_t unknown_a2;
    uint8_t unknown_a3[2];
} PACKED subcmd_bb_Episode2BossActions_6xA3_t;

// 0xA4: Olga Flow phase 1 actions (不支持 PC or Episode 3)
typedef struct subcmd_bb_OlgaFlowPhase1Actions_6xA4 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint8_t what;
    uint8_t unknown_a3[3];
} PACKED subcmd_bb_OlgaFlowPhase1Actions_6xA4_t;

// 0xA5: Olga Flow phase 2 actions (不支持 PC or Episode 3)
typedef struct subcmd_bb_OlgaFlowPhase2Actions_6xA5 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint8_t what;
    uint8_t unknown_a3[3];
} PACKED subcmd_bb_OlgaFlowPhase2Actions_6xA5_t;

// 0xA6: Modify trade proposal (不支持 PC)
typedef struct subcmd_bb_ModifyTradeProposal_6xA6 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t unknown_a1; // Must be < 8
    uint8_t unknown_a2;
    uint8_t unknown_a3[2];
    uint32_t unknown_a4;
    uint32_t unknown_a5;
} PACKED subcmd_bb_ModifyTradeProposal_6xA6_t;

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
    client_id_hdr_t shdr;
    uint8_t trade_type;                 /*D0开始 D1 D2 D3 D4 D5交易取消*/
    uint8_t trade_stage;                /*00 02 04*/
    uint8_t unk2;                       /*71 00 AC F6 */
    uint8_t unk3;                       /*00 21*/
    subcmd_bb_trade_menu_t menu;
    //uint32_t menu_id;                   /*菜单序号*/
    uint32_t trade_num;                   /*目标序号为0001*/
    //uint16_t unused;                    /*一直为00 00*/
} PACKED subcmd_bb_trade_t;

// 0xA7: Unknown (不支持 PC)
// This subcommand is completely ignored (at least, by PSO GC).

// 0xA8: Gol Dragon actions (不支持 PC or Episode 3)
typedef struct subcmd_bb_GolDragonActions_6xA8 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
    uint32_t unknown_a3;
} PACKED subcmd_bb_GolDragonActions_6xA8_t;

// 0xA8: Gol Dragon actions (不支持 PC or Episode 3)
// Packet used by the Gol Dragon boss to deal with its actions.
typedef struct subcmd_gol_dragon_act {
    dc_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
    uint32_t unk2;
} PACKED subcmd_gol_dragon_act_t;

// 0xA8: Gol Dragon actions (不支持 PC or Episode 3)
// Packet used by the Gol Dragon boss to deal with its actions.
typedef struct subcmd_bb_gol_dragon_act {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
    uint32_t unk2;
} PACKED subcmd_bb_gol_dragon_act_t;

// 0xA9: Barba Ray actions (不支持 PC or Episode 3)
typedef struct subcmd_bb_BarbaRayActions_6xA9 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED subcmd_bb_BarbaRayActions_6xA9_t;

// 0xAA: Episode 2 boss actions (不支持 PC or Episode 3)
typedef struct subcmd_bb_Episode2BossActions_6xAA {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
    uint32_t unknown_a3;
} PACKED subcmd_bb_Episode2BossActions_6xAA_t;

// 0xAB: 座椅状态 生成椅子 (不支持 PC)
typedef struct subcmd_bb_create_lobby_chair {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED subcmd_bb_create_lobby_chair_t;

// 0xAC: Unknown (不支持 PC)
typedef struct subcmd_bb_Unknown_6xAC {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t num_items;
    uint32_t item_ids[0x1E];
} PACKED subcmd_bb_Unknown_6xAC_t;

// 0xAD: Unknown (不支持 PC, Episode 3, or GC Trial Edition)
typedef struct subcmd_bb_Unknown_6xAD {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    // The first byte in this array seems to have a special meaning
    uint8_t unknown_a1[0x40];
} PACKED subcmd_bb_Unknown_6xAD_t;

// 0xAE: 发送大厅椅子状态 (sent by existing clients at join time)
// 该副指令不支持 DC, PC, GC 测试版.
typedef struct subcmd_bb_send_lobby_chair_state {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;    //client_id 是数据包来源的客户端
    uint16_t act_id;         //0x0036 0x0037 是椅子状态flags
    uint16_t unused;         //0x0000
    uint32_t angle;          //所在的横向坐标？？
    uint32_t face;           //面对的方向 4字节的位置
} PACKED subcmd_bb_send_lobby_chair_state_t;

// 0xAF: 座椅状态 转动椅子后得到的数据包 (不支持 PC or GC Trial Edition)
typedef struct subcmd_bb_turn_lobby_chair {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t angle; // In range [0x0000, 0xFFFF]
} PACKED subcmd_bb_turn_lobby_chair_t;

// 0xB0: 座椅状态 移动椅子 (不支持 PC or GC Trial Edition)
typedef struct subcmd_bb_move_lobby_chair {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t angle; // In range [0x0000, 0xFFFF]
} PACKED subcmd_bb_move_lobby_chair_t;

// 0xB1: Unknown (不支持 PC or GC Trial Edition)
// This subcommand is completely ignored (at least, by PSO GC).

// 0xB2: Unknown (不支持 PC or GC Trial Edition)
// TODO: It appears this command is sent when the snapshot file is written on
// PSO GC. Verify this.

typedef struct subcmd_bb_Unknown_6xB2 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t area;
    uint8_t unused;
    uint16_t client_id;
    uint32_t unknown_a3; // PSO GC puts 0x00051720 here
} PACKED subcmd_bb_Unknown_6xB2_t;

// 0xB3: Unknown (XBOX)
// 0xB3: CARD battle server data request (Episode 3)
// These commands have multiple subcommands; see the Episode 3 subsubcommand
// table after this table. The common format is:
typedef struct subcmd_EP3_CardBattleCommandHeader {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t subsubcommand; // See 0xBx subcommand table (after this table)
    uint8_t sender_client_id;
    // If mask_key is nonzero, the remainder of the data (after unused2 in this
    // struct) is encrypted using a simple algorithm, which is implemented in
    // set_mask_for_ep3_game_command in SendCommands.cc.
    uint8_t mask_key;
    uint8_t unused2;
} PACKED subcmd_EP3_CardBattleCommandHeader_t;

typedef struct subcmd_EP3_CardServerDataCommandHeader {
    unused_hdr_t shdr;
    uint8_t subsubcommand; // See 0xBx subcommand table (after this table)
    uint8_t sender_client_id;
    // If mask_key is nonzero, the remainder of the data (after unused2 in this
    // struct) is encrypted using a simple algorithm, which is implemented in
    // set_mask_for_ep3_game_command in SendCommands.cc.
    uint8_t mask_key;
    uint8_t unused2;
    //be_uint32_t sequence_num;
    //be_uint32_t context_token;
    uint32_t sequence_num;
    uint32_t context_token;
} PACKED subcmd_EP3_CardServerDataCommandHeader_t;

// 0xB4: Unknown (XBOX)
// 0xB4: CARD battle server response (Episode 3) - see 0xB3 (above)
// 0xB5: CARD battle client command (Episode 3) - see 0xB3 (above)

// 0xB5: BB shop request (handled by the server)
// 请求商店数据包. (Blue Burst)
typedef struct subcmd_bb_shop_req {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t shop_type;
} PACKED subcmd_bb_shop_req_t;

// 0xB6: Episode 3 map list and map contents (server->client only)
// Unlike 0xB3-0xB5, these commands cannot be masked. Also unlike 0xB3-0xB5,
// there are only two subsubcommands, so we list them inline here.
// These subcommands can be rather large, so they should be sent with the 6C
// command instead of the 60 command.
typedef struct subcmd_gc_MapSubsubcommand_GC_Ep3_6xB6 {
    unused_hdr_t shdr;
    uint32_t size;
    uint8_t subsubcommand; // 0x40 or 0x41
    uint8_t unused[3];
} PACKED G_MapSubsubcommand_GC_Ep3_6xB6_t;

typedef struct subcmd_gc_MapList_GC_Ep3_6xB6x40 {
    G_MapSubsubcommand_GC_Ep3_6xB6_t header;
    uint16_t compressed_data_size;
    uint16_t unused;
    // PRS-compressed map list data follows here. newserv generates this from the
    // map index at startup time; see the MapList struct in Episode3/DataIndex.hh
    // and Episode3::DataIndex::get_compressed_map_list for details on the format.
} PACKED G_MapList_GC_Ep3_6xB6x40_t;

typedef struct subcmd_gc_MapData_GC_Ep3_6xB6x41 {
    G_MapSubsubcommand_GC_Ep3_6xB6_t header;
    uint32_t map_number;
    uint16_t compressed_data_size;
    uint16_t unused;
    // PRS-compressed map data follows here (which decompresses to an
    // Episode3::MapDefinition).
} PACKED subcmd_gc_MapData_GC_Ep3_6xB6x41_t;

// 0xB6: BB shop contents (server->client only)
// Packet used for telling a client a shop's inventory. (Blue Burst)
typedef struct subcmd_bb_shop_inv {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;  // params 037F                 /* Set to 0 */
    uint8_t shop_type;
    uint8_t num_items;
    uint16_t unused2;                   /* Set to 0 */
    // Note: costl of these entries should be the price
    item_t items[0x14];
} PACKED subcmd_bb_shop_inv_t;

// 0xB7: Unknown (Episode 3 Trial Edition)

// 0xB7: BB buy shop item (handled by the server)
// Packet sent by the client to buy an item from the shop. (Blue Burst)
typedef struct subcmd_bb_shop_buy {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t new_inv_item_id;//0x0C                   /* 购买后生成的物品ID */
    uint8_t shop_type;       //0x10                   /* 0 工具 1 武器 2 装甲*/
    uint8_t shop_item_index; //0x11                   /* 物品所处商店清单位置 */
    uint8_t num_bought;      //0x12                   /* 购买数量 最高 10个 也就是 09*/
    uint8_t unknown_a1; //                            /*TODO: Probably actually unused; verify this*/
} PACKED subcmd_bb_shop_buy_t;

// 0xB8: Unknown (Episode 3 Trial Edition)
// 0xB8: BB accept tekker result (handled by the server)
typedef struct subcmd_bb_tekk_item {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_id;
} PACKED subcmd_bb_tekk_item_t;

// 0xB9: Unknown (Episode 3 Trial Edition)

// 0xB9: BB provisional tekker result
typedef struct subcmd_bb_tekk_identify_result {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t item;
} PACKED subcmd_bb_tekk_identify_result_t;

// 0xBA: Unknown (Episode 3)
typedef struct subcmd_Unknown_GC_Ep3_6xBA {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1; // Low byte must be < 9
    uint16_t unknown_a2;
    uint32_t unknown_a3;
    uint32_t unknown_a4;
} PACKED subcmd_Unknown_GC_Ep3_6xBA_t;

// 0xBA: BB accept tekker result (handled by the server)
typedef struct subcmd_bb_accept_item_identification {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_id;
} PACKED subcmd_bb_accept_item_identification_t;

// 0xBB: Sync card trade state (Episode 3)
// TODO: Certain invalid values for slot/args in this command can crash the
// client (what is properly bounds-checked). Find out the actual limits for
// slot/args and make newserv enforce them.
typedef struct subcmd_SyncCardTradeState_GC_Ep3_6xBB {
    client_id_hdr_t shdr;
    uint16_t what; // Must be < 5; this indexes into a jump table
    uint16_t slot;
    uint32_t args[4];
} PACKED subcmd_SyncCardTradeState_GC_Ep3_6xBB_t;

// 0xBB: BB bank request (handled by the server)
// Packet sent by clients to open the bank menu. (Blue Burst)
typedef struct subcmd_bb_bank_open {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t checksum;                       /* Maybe a checksum or somesuch? */
} PACKED subcmd_bb_bank_open_t;

// 0xBC: Card counts (Episode 3)
// It's possible that this was an early, now-unused implementation of the CAx49
// command. When the client receives this command, it copies the data into a
// globally-allocated array, but nothing ever reads from this array.
// Curiously, this command is smaller than 0x400 bytes, but uses the extended
// subcommand format anyway (and uses the 6D command rather than 62).
typedef struct subcmd_CardCounts_GC_Ep3_6xBC {
    unused_hdr_t shdr;
    uint32_t size;
    uint8_t unknown_a1[0x2F1];
    // The client sends uninitialized data in this field
    uint8_t unused[3];
} PACKED subcmd_CardCounts_GC_Ep3_6xBC_t;

// 0xBC: BB bank contents (server->client only)
// Packet sent to clients to tell them what's in their bank. (Blue Burst)
typedef struct subcmd_bb_bank_inv {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t size;
    uint32_t checksum;
    uint32_t item_count;
    uint32_t meseta;
    bitem_t items[];
} PACKED subcmd_bb_bank_inv_t;


// 0xBD: Word select during battle (Episode 3; not Trial Edition)
// Note: This structure does not have a normal header - the client ID field is
// big-endian!
typedef struct subcmd_WordSelectDuringBattle_GC_Ep3_6xBD {
    client_id_hdr_t header;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
    uint16_t entries[8];
    uint32_t unknown_a4;
    uint32_t unknown_a5;
    // This field has the same meaning as the first byte in an 06 command's
    // message when sent during an Episode 3 battle.
    uint8_t private_flags;
    uint8_t unused[3];
} PACKED subcmd_WordSelectDuringBattle_GC_Ep3_6xBD_t;

// 0xBD: BB bank action (take/deposit meseta/item) (handled by the server)
// Packet sent by clients to do something at the bank. (Blue Burst)
typedef struct subcmd_bb_bank_act {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_id; // 0xFFFFFFFF = meseta; anything else = item
    uint32_t meseta_amount;
    uint8_t action;  // 0 = deposit, 1 = take
    uint8_t item_amount;
    uint16_t unused2;                   /* 0xFFFF */
} PACKED subcmd_bb_bank_act_t;

// 0xBE: Sound chat (Episode 3; not Trial Edition)
// This appears to be the only subcommand ever sent with the CB command.
typedef struct subcmd_SoundChat_GC_Ep3_6xBE {
    unused_hdr_t shdr;
    uint32_t sound_id; // Must be < 0x27
    //be_uint32_t unused;
    uint32_t unused;
} PACKED subcmd_SoundChat_GC_Ep3_6xBE_t;

// 0xBE: BB create inventory item (server->client only)
// Packet sent to clients to let them know that an item got picked up off of
// the ground. (Blue Burst)
typedef struct subcmd_bb_create_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t item;
    uint32_t unused2;
} PACKED subcmd_bb_create_item_t;

// 0xBF: Change lobby music (Episode 3; not Trial Edition)
typedef struct subcmd_change_lobby_music_GC_Ep3 {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    unused_hdr_t shdr;
    uint32_t song_number; // Must be < 0x34
} PACKED subcmd_change_lobby_music_GC_Ep3_t;

// 0xBF: Give EXP (BB) (server->client only)
// Packet sent to clients to give them experience. (Blue Burst)
typedef struct subcmd_bb_exp {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t exp_amount;
} PACKED subcmd_bb_exp_t;


// 0xC0: BB sell item at shop
typedef struct subcmd_bb_sell_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
    uint32_t sell_amount;
} PACKED subcmd_bb_sell_item_t;

// 0xC1: subcmd_bb_guild_invite1
// 0xC2: subcmd_bb_guild_invite2
typedef struct subcmd_bb_guild_invite {
    bb_pkt_hdr_t hdr;//8 + 4 + 4 + 4 + 48 + 32 = 100
    params_hdr_t shdr;
    uint32_t traget_guildcard;
    uint32_t trans_cmd;
    uint16_t inviter_name[0x0018];
    uint16_t guild_name[0x000E];
    uint32_t guild_rank; /*????????? 未知 00 00 D9 01*/
} PACKED subcmd_bb_guild_invite_t;

// 0xC3: Split stacked item (handled by the server on BB)
// Note: This is not sent if an entire stack is dropped; in that case, a normal
// item drop subcommand is generated instead.
// Packet sent by the client to notify that they're dropping part of a stack of
// items and tell where they're dropping them. (Blue Burst)
typedef struct subcmd_bb_drop_split_stacked_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float z;
    uint32_t item_id;
    uint32_t amount;
} PACKED subcmd_bb_drop_split_stacked_item_t;

// 0xC4: Sort inventory (handled by the server on BB)
// Packet sent by clients to sort their inventory. (Blue Burst)
typedef struct subcmd_bb_sort_inv {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_ids[30];
} PACKED subcmd_bb_sort_inv_t;

// 0xC5: Medical center used
typedef struct subcmd_bb_medical_center_used {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_medical_center_used_t;

// 0xC6: BB steal exp subcommand
typedef struct subcmd_bb_steal_exp {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t enemy_id;
    uint16_t unknown_a1;
} PACKED subcmd_bb_steal_exp_t;

// 0xC7: SUBCMD60_CHARGE_ACT (handled by the server on BB)
typedef struct subcmd_bb_charge_act {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t meseta_amount;
} PACKED subcmd_bb_charge_act_t;


// 0xC8: Enemy killed (handled by the server on BB)
// Packet sent by clients to say that they killed a monster. Unfortunately, this
// doesn't always get sent (for instance for the Darvants during a Falz fight),
// thus its not actually used in Sylverant for anything.
typedef struct subcmd_mkill {
    dc_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    //uint8_t client_id;
    //uint8_t unused;
    //uint16_t unk;
    uint16_t enemy_id2;
    uint16_t killer_client_id;
    uint32_t unused;
} PACKED subcmd_mkill_t;

// 0xC8: Enemy killed (handled by the server on BB)
// Packet sent by clients to say that they killed a monster. Unfortunately, this
// doesn't always get sent (for instance for the Darvants during a Falz fight),
// thus its not actually used in Sylverant for anything.
typedef struct subcmd_bb_mkill {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t enemy_id2;
    uint16_t killer_client_id;
    uint32_t unused;
} PACKED subcmd_bb_mkill_t;

// 0xC8: Enemy killed (handled by the server on BB)
// Packet sent by Blue Burst clients to request experience after killing an
// enemy.
typedef struct subcmd_bb_req_exp {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t enemy_id2;
    uint16_t client_id;
    uint8_t last_hitter;
    uint8_t unused2[3];
} PACKED subcmd_bb_req_exp_t;

//[2023年06月10日 13:22:52:762] 截获(1498): subcmd-bb.c 1498 行 GAME_COMMAND_C9_TYPE - 客户端至服务器指令 指令 0x0062 未处理. (数据如下)
//[2023年06月10日 13:22:52:787] 截获(1498):
//( 00000000 )   10 00 62 00 00 00 00 00  C9 02 FF FF DC 05 00 00 ..b.....?[2023年06月10日 13:23:21:695] 断连(0470): 客户 端 123(42004064) 断开连接

//[2023年06月10日 13:25:25:957] 截获(1498): subcmd-bb.c 1498 行 GAME_COMMAND_C9_TYPE - 客户端至服务器指令 指令 0x0062 未处理. (数据如下)
//[2023年06月10日 13:25:25:982] 截获(1498):
//( 00000000 )   10 00 62 00 00 00 00 00  C9 02 FF FF DC 05 00 00 ..b.....?
// 0xC9: 获取任务奖励 类型未知
typedef struct subcmd_bb_quest_reward_meseta {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr; /* object_id FF FF meseta */
    uint32_t amount;
} PACKED subcmd_bb_quest_reward_meseta_t;

// 0xCA: 获取任务奖励物品 类型未知
typedef struct subcmd_bb_quest_reward_item {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr; /* object_id FF FF meseta */
    item_t item_data;
} PACKED subcmd_bb_quest_reward_item_t;

// 0xCB: Request to transfer item (BB)
typedef struct subcmd_bb_UNKNOW_0xCB {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
} PACKED subcmd_bb_UNKNOW_0xCB_t;

// 0xCC: subcmd_bb_guild_ex_item
typedef struct subcmd_bb_guild_ex_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t ex_item_id;
    uint32_t ex_amount;
} PACKED subcmd_bb_guild_ex_item_t;

// 0xCD: subcmd_bb_guild_master_trans1
typedef struct subcmd_bb_guild_master_trans1 {
    bb_pkt_hdr_t hdr;//8 + 4 + 4 + 4 + 48 + 32 = 100 hdr.flags = client_id_target
    params_hdr_t shdr;
    uint32_t client_id_sender;
    uint32_t trans_cmd;
    uint16_t master_name[0x000C];
    uint8_t unknow1[24];
    uint16_t guild_name[0x000E];
    uint8_t unknow2[0]; /* ???????? 未知 00 00 E3 23*/
} PACKED subcmd_bb_guild_master_trans1_t;

// 0xCE: subcmd_bb_guild_master_trans2
typedef struct subcmd_bb_guild_master_trans2 {
    bb_pkt_hdr_t hdr;//8 + 4 + 4 + 4 + 48 + 32 = 100
    params_hdr_t shdr;
    uint32_t traget_guildcard;
    uint32_t trans_cmd;
    uint16_t master_name[0x000C];
    uint8_t unknow1[24];
    uint16_t guild_name[0x000E];
    uint8_t unknow2[0]; /* ???????? 未知 00 00 E3 23*/
} PACKED subcmd_bb_guild_master_trans2_t;

//[2023年05月02日 13:30:08:506] 调试(subcmd-bb.c 5292): 暂未完成 0x60: 0xCF
//
//( 00000000 )   3C 00 60 00 00 00 00 00  CF 0D 76 00 00 00 00 00    <.`.......v.....
//( 00000010 )   01 00 00 00 00 01 01 00  03 05 05 05 05 01 05 01    ................
//( 00000020 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000030 )   0A 00 00 00 01 00 00 00  0A 00 00 00                ............
//[2023年07月12日 20:08:10:017] 调试(subcmd_handle_60.c 3493): 未知 0x60 指令: 0xCF
//( 00000000 )   3C 00 60 00 00 00 00 00   CF 0D 76 00 00 00 00 00  <.`.....?v.....
//( 00000010 )   01 00 00 00 00 01 01 00   03 05 05 05 05 01 05 01  ................
//( 00000020 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 00000030 )   0A 00 00 00 01 00 00 00   0A 00 00 00             ............
// 0xCF: 初始化对战模式 (指令生效范围; 仅限游戏; handled by the server on BB)
typedef struct subcmd_bb_start_battle_mode {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
    uint32_t unknown_a1[11];
    uint32_t unknown_a2;
} PACKED subcmd_bb_start_battle_mode_t;

// 0xD0: Battle mode level up (BB; handled by server)
// Requests the client to be leveled up by num_levels levels. The server should
// respond with a 6x30 command.
typedef struct subcmd_bb_battle_mode_level_up {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t num_levels;
} PACKED subcmd_bb_battle_mode_level_up_t;

// 0xD1: Challenge mode grave (BB; handled by server)
struct G_ChallengeModeGrave_BB_6xD1 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
    uint32_t unknown_a3;
    uint32_t unknown_a4;
    uint32_t unknown_a5;
} PACKED;


// 0xD2: Set quest data 2 (BB)
// Writes 4 bytes to the 32-bit field specified by index.
typedef struct subcmd_bb_gallon_area {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t quest_offset;
    uint32_t value;
} PACKED subcmd_bb_gallon_area_pkt_t;

// 0xD3: Invalid subcommand

// 0xD4: Unknown
typedef struct subcmd_bb_UNKNOW_0xD4 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t action;    // Must be in [0, 4]
    uint8_t unknown_a1; // Must be in [0, 15]
    uint8_t unused;
} PACKED subcmd_bb_UNKNOW_0xD4_t;

// 0xD5: Exchange item in quest (BB; handled by server)
// The client sends this when it executes an F953 quest opcode.
struct G_ExchangeItemInQuest_BB_6xD5 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t unknown_a1; // Only data1[0]-[2] are used
    item_t unknown_a2; // Only data1[0]-[2] are used
    uint16_t unknown_a3;
    uint16_t unknown_a4;
} PACKED;

// 0xD6: Wrap item (BB; handled by server)
typedef struct subcmd_bb_warp_item {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
    item_t item_data;
    uint8_t unknown_a1;
    uint8_t unused[3];
} PACKED subcmd_bb_warp_item_t;

// 0xD7: Paganini Photon Drop exchange (BB; handled by server)
// The client sends this when it executes an F955 quest opcode.
struct G_PaganiniPhotonDropExchange_BB_6xD7 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t unknown_a1; // Only data1[0]-[2] are used
    uint16_t request_id;
    uint16_t unknown_a3;
} PACKED;

// 0xD8: Add S-rank weapon special (BB; handled by server)
// The client sends this when it executes an F956 quest opcode.
struct G_AddSRankWeaponSpecial_BB_6xD8 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t unknown_a1; // Only data1[0]-[2] are used
    uint32_t unknown_a2;
    uint32_t unknown_a3;
    uint16_t unknown_a4;
    uint16_t unknown_a5;
} PACKED;

// 0xD9: Momoka Item Exchange  Momoka物品交换
typedef struct subcmd_bb_MomokaItemExchange_BB_6xD9 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t unknown_a1;
    item_t unknown_a2;
    uint32_t unknown_a3;
    uint32_t unknown_a4;
    uint16_t unknown_a5;
    uint16_t unknown_a6;
} PACKED subcmd_bb_MomokaItemExchange_BB_6xD9_t;

// 0xDA: Upgrade weapon attribute (BB; handled by server)
// The client sends this when it executes an F957 or F958 quest opcode.
struct G_UpgradeWeaponAttribute_BB_6xDA {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t unknown_a1; // Only data1[0]-[2] are used
    uint32_t item_id;
    uint32_t attribute;
    uint32_t unknown_a4; // 0 or 1
    uint32_t unknown_a5;
    uint16_t request_id;
    uint16_t unknown_a7;
} PACKED;

// 0xDB: Exchange item in quest (BB)
typedef struct subcmd_bb_UNKNOW_0xDB {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
    uint32_t item_id;
    uint32_t amount;
    uint32_t unknown_a3;
} PACKED subcmd_bb_UNKNOW_0xDB_t;

// 0xDC: Saint-Million boss actions (BB)
typedef struct subcmd_bb_UNKNOW_0xDC {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED subcmd_bb_UNKNOW_0xDC_t;

// 0xDD:  Set EXP multiplier (BB)
// header.param specifies the EXP multiplier. It is 1-based, so the value 2
// means all EXP is doubled, for example.
typedef struct subcmd_bb_set_exp_rate {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
} PACKED subcmd_bb_set_exp_rate_t;

// 0xDE: Good Luck quest (BB; handled by server)
// The client sends this when it executes an F95C quest opcode.
struct G_GoodLuckQuestActions_BB_6xDE {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t unknown_a1;
    uint8_t unknown_a2;
    uint16_t unknown_a3;
} PACKED;

// 0xDF: Invalid subcommand
typedef struct subcmd_bb_black_paper_deal_photon_drop_exchange {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t data[0];
} PACKED subcmd_bb_black_paper_deal_photon_drop_exchange_t;

// 0xE0: Black Paper's Deal rewards (BB; handled by server)
// The client sends this when it executes an F95E quest opcode.
// 00 - 07 -> 08 09 0A 0B -> 0C 0D 0E 0F 10 11 12 13 14 15 16 17
// hdr        shdr           area        x           z
typedef struct subcmd_bb_black_paper_deal_reward {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float z;
} PACKED subcmd_bb_black_paper_deal_reward_t;

// 0xE1: Gallon's Plan quest (BB; handled by server)
// The client sends this when it executes an F95F quest opcode.
struct G_GallonsPlanQuestActions_BB_6xE1 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t unknown_a1;
    uint8_t unknown_a2;
    uint8_t unknown_a3;
    uint8_t unused;
    uint16_t unknown_a4;
    uint16_t unknown_a5;
} PACKED;

// 0xE2: Coren actions (BB)
// The client sends this when it executes an F960 quest opcode.
struct G_CorenActions_BB_6xE2 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t unknown_a1[12]; // TODO: There might be uint16_ts and uint32_ts in here.
} PACKED;

// 0xE3: Coren actions result (BB)
struct G_CorenActionsResult_BB_6xE3 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t item_data;
} PACKED;

// 0xE4: Invalid subcommand
// 0xE5: Invalid subcommand
// 0xE6: Invalid subcommand
// 0xE7: Invalid subcommand
// 0xE8: Invalid subcommand
// 0xE9: Invalid subcommand
// 0xEA: Invalid subcommand
// 0xEB: Invalid subcommand
// 0xEC: Invalid subcommand
// 0xED: Invalid subcommand
// 0xEE: Invalid subcommand
// 0xEF: Invalid subcommand
// 0xF0: Invalid subcommand
// 0xF1: Invalid subcommand
// 0xF2: Invalid subcommand
// 0xF3: Invalid subcommand
// 0xF4: Invalid subcommand
// 0xF5: Invalid subcommand
// 0xF6: Invalid subcommand
// 0xF7: Invalid subcommand
// 0xF8: Invalid subcommand
// 0xF9: Invalid subcommand
// 0xFA: Invalid subcommand
// 0xFB: Invalid subcommand
// 0xFC: Invalid subcommand
// 0xFD: Invalid subcommand
// 0xFE: Invalid subcommand
// 0xFF: Invalid subcommand

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

int subcmd_send_pos(ship_client_t* dst, ship_client_t* src);

/////////////////////////////////////////////////////////////////////////////
////DC NTE

/* 处理 DC NTE 0x60 来自客户端的数据包. */
int subcmd_dcnte_handle_bcast(ship_client_t* c, subcmd_pkt_t* pkt);

int subcmd_send_lobby_dcnte(lobby_t* l, ship_client_t* c, subcmd_pkt_t* pkt,
    int ignore_check);

/* Stuff dealing with the Dreamcast Network Trial edition */
int subcmd_translate_dc_to_nte(ship_client_t* c, subcmd_pkt_t* pkt);
int subcmd_translate_nte_to_dc(ship_client_t* c, subcmd_pkt_t* pkt);
int subcmd_translate_bb_to_nte(ship_client_t* c, subcmd_bb_pkt_t* pkt);

/////////////////////////////////////////////////////////////////////////////
////DC GC PC V1 V2

/* 处理 DC GC PC V1 V2 0x62 来自客户端的数据包. */
int subcmd_handle_62(ship_client_t* c, subcmd_pkt_t* pkt);

/* 处理 DC GC PC V1 V2 0x6D 来自客户端的数据包. */
int subcmd_handle_6D(ship_client_t* c, subcmd_pkt_t* pkt);

/* 处理 DC GC PC V1 V2 0x60 来自客户端的数据包. */
int subcmd_handle_60(ship_client_t* c, subcmd_pkt_t* pkt);

/* Send a broadcast subcommand to the whole lobby. */
int subcmd_send_lobby_dc(lobby_t* l, ship_client_t* c, subcmd_pkt_t* pkt,
    int ignore_check);

int subcmd_send_lobby_item(lobby_t* l, subcmd_itemreq_t* req, const uint32_t item[4]);

/////////////////////////////////////////////////////////////////////////////
////Blue Burst

/* 处理 BB 0x62 来自客户端的数据包. */
int subcmd_bb_handle_62(ship_client_t* src, subcmd_bb_pkt_t *pkt);

/* 处理 BB 0x6D 来自客户端的数据包. */
int subcmd_bb_handle_6D(ship_client_t* src, subcmd_bb_pkt_t* pkt);

/* 处理 BB 0x60 来自客户端的数据包. */
int subcmd_bb_handle_60(ship_client_t* src, subcmd_bb_pkt_t* pkt);
int subcmd_bb_handle_60_mode(ship_client_t* src, subcmd_bb_pkt_t* pkt);

/////////////////////////////////////////////////////////////////////////////
////Episode 3

/* 处理 EP3 0xC9/0xCB 来自客户端的数据包. */
int subcmd_handle_ep3_bcast(ship_client_t *c, subcmd_pkt_t *pkt);

#endif /* !SUBCMD60_H */
