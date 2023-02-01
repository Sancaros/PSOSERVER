/*
    �λ�֮���й� ����������
    ��Ȩ (C) 2022 Sancaros

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

static int char_dc_hdrsize2 = sizeof(subcmd_pkt_t);

typedef struct bb_subcmd_pkt {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t data[0];
} PACKED bb_subcmd_pkt_t;

static int char_bb_hdrsize2 = sizeof(bb_subcmd_pkt_t);

// subcmdָ�ͨ������ͷ.

// ���ͻ���ID�ĸ�ָ��ṹ
typedef struct client_id_hdr {
    uint8_t type; //subcmd ָ��
    uint8_t size;
    uint16_t client_id; // <= 12
} PACKED client_id_hdr_t;

// ������ID�ĸ�ָ��ṹ
typedef struct enemy_id_hdr {
    uint8_t type; //subcmd ָ��
    uint8_t size;
    uint16_t enemy_id; // ��Χ [0x1000, 0x4000)
} PACKED enemy_id_hdr_t;

// ������ID�ĸ�ָ��ṹ
typedef struct object_id_hdr {
    uint8_t type; //subcmd ָ��
    uint8_t size;
    uint16_t object_id; // >= 0x4000, != 0xFFFF
} PACKED object_id_hdr_t;

// �������ĸ�ָ��ṹ
typedef struct params_hdr {
    uint8_t type; //subcmd ָ��
    uint8_t size;
    uint16_t params;
} PACKED params_hdr_t;

// ��ͨ�޲����ĸ�ָ��ṹ
typedef struct unused_hdr {
    uint8_t type; //subcmd ָ��
    uint8_t size;
    uint16_t unused;
} PACKED unused_hdr_t;

// 0x04: Unknown δ֪
typedef struct subcmd_unknown_04 {
    client_id_hdr_t header;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED subcmd_unknown_04_t;

// ע��: data.shdr.object_id ֵΪ 0xFFFF ʱ ��ʾ����Ĺ����������
// 0x05: Switch state changed
// Some things that don't look like switches are implemented as switches using
// this subcommand. For example, when all enemies in a room are defeated, this
// subcommand is used to unlock the doors.
typedef struct subcmd_bb_switch_changed {
    bb_pkt_hdr_t hdr;
    bb_switch_changed_t data;
} PACKED subcmd_bb_switch_changed_pkt_t;

// 0x06: Send guild card
// Guild card send packet (Dreamcast).
typedef struct subcmd_dc_gcsend {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
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

// 0x06: Send guild card
// Guild card send packet (PC).
typedef struct subcmd_pc_gcsend {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
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

// 0x06: Send guild card
// Guild card send packet (Gamecube).
typedef struct subcmd_gc_gcsend {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
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

// 0x06: Send guild card
// Guild card send packet (Xbox).
typedef struct subcmd_xb_gcsend {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;                       /* unused = 0x0D 0xFB */
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

// 0x06: Send guild card
// Guild card send packet (Blue Burst)
typedef struct subcmd_bb_gc_send {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t guildcard;
    uint16_t name[0x18];
    uint16_t guild_name[0x10];
    uint16_t text[0x58];
    uint8_t one;
    uint8_t language;
    uint8_t section;
    uint8_t char_class;
} PACKED subcmd_bb_gcsend_t;

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

// 0x09: Unknown δ֪
typedef struct subcmd_unknown_09 {
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
    // Note: enemy_id (in header) is in the range [0x1000, 0x4000)
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
struct subcmd_add_or_remove_condition_6x0C_6x0D {
    client_id_hdr_t shdr;
    uint32_t unknown_a1; // Probably condition type
    uint32_t unknown_a2;
} PACKED;

// 0x0E: Unknown δ֪
typedef struct subcmd_unknown_0E {
    client_id_hdr_t shdr;
} PACKED subcmd_unknown_0E_t;

// 0x10: Unknown (��֧�� Episode 3)
// 0x11: Unknown (��֧�� Episode 3)
// Same format as 0x10
// 0x12: Dragon boss actions (��֧�� Episode 3)
// Same format as 0x10
// 0x14: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
// Same format as 0x10
struct subcmd_unknown_6x10_6x11_6x12_6x14 {
    enemy_id_hdr_t shdr;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint32_t unknown_a4;
} PACKED;

// 0x10: Unknown (��֧�� Episode 3)
// 0x11: Unknown (��֧�� Episode 3)
// Same format as 0x10
// 0x12: Dragon boss actions (��֧�� Episode 3)
// Same format as 0x10
// 0x14: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
// Same format as 0x10
// Packet used by the Dragon boss to deal with its actions.
typedef struct subcmd_dragon_act {
    dc_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
} PACKED subcmd_dragon_act_t;

// 0x10: Unknown (��֧�� Episode 3)
// 0x11: Unknown (��֧�� Episode 3)
// Same format as 0x10
// 0x12: Dragon boss actions (��֧�� Episode 3)
// Same format as 0x10
// 0x14: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
// Same format as 0x10
// Packet used by the Dragon boss to deal with its actions.
typedef struct subcmd_bb_dragon_act {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
} PACKED subcmd_bb_dragon_act_t;

// 0x13: De Rol Le boss actions (��֧�� Episode 3)
struct subcmd_DeRolLeBossActions_6x13 {
    enemy_id_hdr_t shdr;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
} PACKED;

// 0x15: Vol Opt boss actions (��֧�� Episode 3)
struct subcmd_VolOptBossActions_6x15 {
    enemy_id_hdr_t shdr;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
    uint16_t unknown_a5;
} PACKED;

// 0x16: Vol Opt boss actions (��֧�� Episode 3)
struct G_VolOptBossActions_6x16 {
    enemy_id_hdr_t shdr;
    uint8_t unknown_a2[6];
    uint16_t unknown_a3;
} PACKED;

// 0x17: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
// Packet used to teleport to a specified position
typedef struct subcmd_teleport {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    float x;
    float y;
    float z;
    float w;
} PACKED subcmd_teleport_t;

// 0x18: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
struct G_Unknown_6x18 {
    client_id_hdr_t shdr;
    uint16_t unknown_a2[4];
} PACKED;

// 0x19: Dark Falz boss actions (��֧�� Episode 3)
struct G_DarkFalzActions_6x19 {
    enemy_id_hdr_t shdr;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint32_t unknown_a4;
    uint32_t unused;
} PACKED;

// 0x1B: Unknown (��֧�� Episode 3)
struct G_Unknown_6x1B {
    client_id_hdr_t shdr;
} PACKED;

// 0x1C: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
struct G_Unknown_6x1C {
    client_id_hdr_t shdr;
} PACKED;

// 0x1F: Unknown (ָ����Ч��Χ; ��������Ϸ)
struct G_Unknown_6x1F {
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED;

// 0x20: Set position (existing clients send when a new client joins a lobby/game)
// Packet used to update other people when a player warps to another area
typedef struct subcmd_set_area {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float y;
    float z;
    uint32_t unknown_a1;
} PACKED subcmd_set_area_t;

// 0x20: Set position (existing clients send when a new client joins a lobby/game)
// Packet used to update other people when a player warps to another area
typedef struct subcmd_bb_set_area {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float y;
    float z;
    uint32_t unknown_a1;
} PACKED subcmd_bb_set_area_t;

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
typedef struct subcmd_set_player_visibility_6x22_6x23 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_set_player_visibility_6x22_6x23_t;

// 0x24: subcmd_bb_set_pos_0x24 (ָ����Ч��Χ; ������Ϸ)
typedef struct subcmd_bb_set_pos_0x24 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
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

// 0x28: Feed MAG
typedef struct subcmd_bb_feed_mag {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t mag_id;
    uint32_t item_id;
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
    //uint32_t data_l[3];
    //uint32_t item_id;
    //uint32_t data2_l;
    uint32_t unk; //DC�汾������
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
typedef struct subcmd_talk_npc {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk;           /* Always 0xFFFF for NPCs? */
    uint16_t zero;          /* Always 0? */
    float x;
    float z;
    float unused2;       /* Always zero? */
} PACKED subcmd_talk_npc_t;

// 0x2C: Talk to NPC
// Packet sent when talking to an NPC on Pioneer 2 (and other purposes).
typedef struct subcmd_bb_talk_npc {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk;           /* Always 0xFFFF for NPCs? */
    uint16_t zero;          /* Always 0? */
    float x;
    float z;
    float unused2;       /* Always zero? */
} PACKED subcmd_bb_talk_npc_t;

// 0x2D: Done talking to NPC
typedef struct subcmd_end_talk_to_npc {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_end_talk_to_npc_t;

// 0x2D: Done talking to NPC
typedef struct subcmd_bb_end_talk_to_npc {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_end_talk_to_npc_t;

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
struct subcmd_UseMedicalCenter_6x31 {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x31: Medical center
// 0x32: Unknown (occurs when using Medical Center)
struct subcmd_bb_UseMedicalCenter_6x31 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

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

// 0x36: Unknown (ָ����Ч��Χ; ������Ϸ)
// This subcommand is completely ignored (at least, by PSO GC).


// 0x37: Photon blast
typedef struct subcmd_photon_blast {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED subcmd_photon_blast_t;

// 0x37: Photon blast
typedef struct subcmd_bb_photon_blast {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED subcmd_bb_photon_blast_t;

// 0x38: Unknown
struct G_Unknown_6x38 {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED;

// 0x39: Photon blast ready
typedef struct subcmd_photon_blast_ready {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_photon_blast_ready_t;

// 0x3A: Unknown (ָ����Ч��Χ; ������Ϸ)
struct G_Unknown_6x3A {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x3A: Unknown (ָ����Ч��Χ; ������Ϸ)
typedef struct subcmd_bb_cmd_3a {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_cmd_3a_t;

// 0x3B: Unknown (ָ����Ч��Χ; ��������Ϸ)
struct G_Unknown_6x3B {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

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


// 0x46: Attack finished (sent after each of 43, 44, and 45)
// Packet sent when an object is hit by a physical attack.
typedef struct subcmd_objhit_phys {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t hit_count;
    struct {
        uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
        uint16_t unused;
    } objects[];
} PACKED subcmd_objhit_phys_t;

// 0x46: Attack finished (sent after each of 43, 44, and 45)
// Packet sent when an object is hit by a physical attack.
typedef struct subcmd_bb_objhit_phys {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t hit_count;
    struct {
        uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
        uint16_t unused;
    } objects[];
} PACKED subcmd_bb_objhit_phys_t;


// 0x47: Cast technique
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
    struct {
        uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
        uint16_t unused;
    } objects[];
} PACKED subcmd_objhit_tech_t;

// 0x47: Cast technique
// Packet sent when an object is hit by a technique.
typedef struct subcmd_bb_objhit_tech {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t tech;
    // Note: The level here isn't the actual tech level that was cast, if the
    // level is > 15. In that case, a 0x8D is sent first, which contains the
    // additional level which is added to this level at cast time. They probably
    // did this for legacy reasons when dealing with v1/v2 compatibility, and
    // never cleaned it up.
    uint8_t level;
    uint8_t hit_count;
    struct {
        uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
        uint16_t unused;
    } objects[];
} PACKED subcmd_bb_objhit_tech_t;

// 0x48: Cast technique complete
// Packet used after a client uses a tech.
typedef struct subcmd_used_tech {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t tech;
    uint16_t level;
} PACKED subcmd_used_tech_t;

// 0x48: Cast technique complete
// Packet used after a client uses a tech.
typedef struct subcmd_bb_used_tech {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t tech;
    uint16_t level;
} PACKED subcmd_bb_used_tech_t;

// 0x49: Subtract PB energy
struct G_SubtractPBEnergy_6x49 {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t unknown_a1;
    uint8_t unknown_a2;
    uint16_t entry_count;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
    struct {
        uint16_t unknown_a1;
        uint16_t unknown_a2;
    } entries[];
} PACKED;

// 0x4A: Fully shield attack
struct G_ShieldAttack_6x4A {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x4B: Hit by enemy
// Packet used when a client takes damage.
// 0x4C: Hit by enemy
// Same format as 0x4B
typedef struct subcmd_take_damage {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk1;
    uint16_t hp_rem;
    uint32_t unk2[2];
} PACKED subcmd_take_damage_t;

// 0x4B: Hit by enemy
// Packet used when a client takes damage.
// 0x4C: Hit by enemy
// Same format as 0x4B
typedef struct subcmd_bb_take_damage {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unk1;
    uint16_t hp_rem;
    uint32_t unk2[2];
} PACKED subcmd_bb_take_damage_t;

// 0x4D: death sync (ָ����Ч��Χ; ��������Ϸ)
typedef struct subcmd_bb_death_sync {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t flag;
    uint8_t unk2[3];
} PACKED subcmd_bb_death_sync_t;

// 0x4E: Unknown (ָ����Ч��Χ; ��������Ϸ)
typedef struct subcmd_bb_cmd_4e {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED subcmd_bb_cmd_4e_t;

// 0x4F: Unknown (ָ����Ч��Χ; ��������Ϸ)
struct G_Unknown_6x4F {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x50: Unknown (ָ����Ч��Χ; ��������Ϸ)
typedef struct subcmd_bb_switch_req {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t unk2;
    float x;
    float y;
    uint8_t unk3[2];
} PACKED subcmd_bb_switch_req_t;

// 0x51: Invalid subcommand

// 0x52: Toggle shop/bank interaction
typedef struct subcmd_bb_menu_req {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t menu_id;
    uint16_t unk; //һֱ���� 00 00 �ѵ���menu_idһ��ģ�
    uint16_t unk2;
    uint16_t unk3;
} PACKED subcmd_bb_menu_req_t;

// 0x53: Unknown (ָ����Ч��Χ; ������Ϸ)
struct G_Unknown_6x53 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x54: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x55: Intra-map warp
typedef struct subcmd_bb_map_warp {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1; //80 Ϊ �ܶ��� 00 Ϊ������2��
    float x1;
    float y1;
    float z1;
    float x2;
    float y2;
    float z2;
} PACKED subcmd_bb_map_warp_t;

// 0x56: Unknown (ָ����Ч��Χ; ��������Ϸ)
struct G_Unknown_6x56 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
    float x;
    float y;
    float z;
} PACKED;

// 0x57: Unknown (ָ����Ч��Χ; ��������Ϸ)
struct G_Unknown_6x57 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x58: CTRL+W ���� SUBCMD_LOBBY_ACTION (ָ����Ч��Χ; ������Ϸ)
typedef struct subcmd_bb_lobby_act {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t act_id;
    uint16_t unused;                 // 0x0000
} PACKED subcmd_bb_lobby_act_t;

// 0x59: ʰȡ�ػ�ɾ����ͼ��Ʒ���ݽṹ
typedef struct subcmd_bb_destroy_map_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t area;
    uint32_t item_id;
} PACKED subcmd_bb_destroy_map_item_t;

// 0x5A: Request to pick up item
struct G_PickUpItemRequest_6x5A {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t item_id;
    uint16_t area;
    uint16_t unused;
} PACKED;

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
struct G_Unknown_6x5C {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
} PACKED;

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
    uint32_t two; //DCû��
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
    sitem_t data;
} PACKED subcmd_buy_t;

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
    uint32_t unk2[2];
} PACKED subcmd_bb_itemreq_t;

// 0x61: Feed MAG
//[2023��02��01�� 00:24 : 12 : 940] ����(subcmd - bb.c 4279) : BB�������� GC 42004064 60ָ�� : 0x61
//(00000000)   14 00 60 00 00 00 00 00  61 03 85 00 02 00 21 00    ..`.....a.....!.
//(00000010)   02 00 00 00                                         ....
//[2023��02��01�� 00:25 : 43 : 303] ����(subcmd - bb.c 4279) : BB�������� GC 42004064 60ָ�� : 0x61
//(00000000)   14 00 60 00 00 00 00 00  61 03 85 00 02 00 21 00    ..`.....a.....!.
//(00000010)   02 00 00 00                                         ....
//(00000000)   14 00 60 00 00 00 00 00  61 03 85 00 02 00 01 00    ..`.....a.......
//(00000010)   02 00 00 00                                         ....
//(00000000)   14 00 60 00 00 00 00 00  61 03 85 00 02 00 01 00    ..`.....a.......
//(00000010)   02 00 00 00                                         ....
//(00000000)   14 00 60 00 00 00 00 00  61 03 85 00 02 00 01 00    ..`.....a.......
//(00000010)   02 00 00 00                                         ....
typedef struct subcmd_bb_levelup_req {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr; /* 0x00 0x85 0x03 0x61*/
    uint16_t unk1; /* 0x0002*/
    uint16_t unk2; /* ���� 0x0021? ��2 0x0001*/
    uint32_t unk3;
} PACKED subcmd_bb_levelup_req_t;

// 0x62: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x63: Destroy item on the ground (used when too many items have been dropped)
struct G_DestroyGroundItem_6x63 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_id;
    uint32_t area;
} PACKED;

// 0x64: Unknown (��֧�� Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x65: Unknown (��֧�� Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x66: Use star atomizer
struct G_UseStarAtomizer_6x66 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t target_client_ids[4];
} PACKED;

// 0x67: Create enemy set
struct G_CreateEnemySet_6x67 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    // unused1 could be area; the client checks this againset a global but the
    // logic is the same in both branches
    uint32_t unused1;
    uint32_t unknown_a1;
    uint32_t unused2;
} PACKED;

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
} PACKED subcmd_pipe_pkt_t;

// 0x68: Telepipe/Ryuker
typedef struct subcmd_bb_pipe_pkt {
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
} PACKED subcmd_bb_pipe_pkt_t;

// 0x69: Unknown (ָ����Ч��Χ; ������Ϸ)
struct G_Unknown_6x69 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t client_id2;
    uint16_t unknown_a1;
    uint16_t what; // 0-3; logic is very different for each value
    uint16_t unknown_a2;
} PACKED;


// 0x6A: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
struct G_Unknown_6x6A {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unused;
} PACKED;

// 0x6B: Sync enemy state (used while loading into game; same header format as 6E)
// 0x6C: Sync object state (used while loading into game; same header format as 6E)
// 0x6D: Sync item state (used while loading into game; same header format as 6E)
// 0x6E: Sync flag state (used while loading into game)
struct G_SyncGameStateHeader_6x6B_6x6C_6x6D_6x6E {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t size;
    uint32_t subcommand_size;
    uint32_t decompressed_size;
    uint32_t compressed_size; // Must be <= subcommand_size - 0x10
    // BC0-compressed data follows here (use bc0_decompress from Compression.hh)
} PACKED;

// 0x6F: Unknown (used while loading into game) 8 + 4 + 512
struct G_Unknown_6x6F {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1[0x200];
} PACKED;

// 0x70: Sync player disp data and inventory (used while loading into game)
// Annoyingly, they didn't use the same format as the 65/67/68 commands here,
// and instead rearranged a bunch of things.
// TODO: Some missing fields should be easy to find in the future (e.g. when the
// sending player doesn't have 0 meseta, for example)
// Packet used to communicate current state of players in-game while a new player is bursting. 
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
    uint32_t tag;
    uint32_t guildcard;
    uint8_t unk3[0x44];
    uint8_t techniques[20];
    psocn_dress_data_t dress_data;
    psocn_pl_stats_t stats;
    uint8_t opt_flag[10];
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    inventory_t inv;
    uint32_t zero;          /* Unused? */
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
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;

    uint32_t size_minus_4;  /* ??? 0x0000040C*/
    uint32_t lobby_num;  /*   0x00000001*/
    uint32_t unused2;  /* ??? 0x01000000*/
    float x;
    float y;
    float z;
    uint8_t unk2[0x90];
    uint32_t unk3;
    uint32_t unk4[4];

    uint32_t tag;
    uint32_t guildcard;
    uint8_t unk5[50];
    uint8_t techniques[20];
    psocn_dress_data_t dress_data;
    psocn_pl_stats_t stats;
    uint8_t opt_flag[10];
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    inventory_t inv;
    uint32_t zero;          /* Unused? */
    /* Xbox has a little bit more at the end than other versions... Probably
       safe to just ignore it. */
} PACKED subcmd_bb_burst_pldata_t;

static int char_bb_size222 = sizeof(subcmd_bb_burst_pldata_t);

typedef struct G_Unknown_6x70 {
    bb_pkt_hdr_t hdr;               /* 0000 0 */
    client_id_hdr_t shdr;           /* 0008 8 */
    // Offsets in this struct are relative to the overall command header
    uint32_t size_minus_4;          /* 000C 12 ? 0x0000040C*/
    // [1] and [3] in this array (and maybe [2] also) appear to be le_floats;
    // they could be the player's current (x, y, z) coords
    uint32_t lobby_num;             /* 0010 16  0x00000001*/
    uint32_t unused2;               /* 0014 20  0x01000000*/
    uint32_t unknown_a2[5];         /* 0018 24 - 43 */

    uint16_t unknown_a3[4];         /* 002C */
    uint32_t unknown_a4[3][5];      /* 0034 */
    uint32_t unknown_a5;            /* 0070 */

    uint32_t player_tag;            /* 0074 */
    uint32_t guild_card_number;     /* 0078 */
    uint32_t unknown_a6[2];         /* 007C */
    struct {                        /* 0084 */
        uint16_t unknown_a1[2];
        uint32_t unknown_a2[6];
    } PACKED unknown_a7;
    uint32_t unknown_a8;            /* 00A0 */
    uint8_t unknown_a9[0x14];       /* 00A4 */
    uint32_t unknown_a10;           /* 00B8 */
    uint32_t unknown_a11;           /* 00BC */
    uint8_t technique_levels[0x14]; /* 00C0 */ // Last byte is uninitialized

    psocn_dress_data_t dress_data;  /* 00D4 */

    psocn_pl_stats_t stats;            /* 0124 */
    uint8_t opt_flag[0x0A];         /* 0132  306 - 315 size 10 */
    uint32_t level;                 /* 013C  316 - 319 size 4 */
    uint32_t exp;                   /* 0140  320 - 323 size 4 */
    uint32_t meseta;                /* 0144  324 - 327 size 4 */

    inventory_t inv;                /* 0148  328 - 1171 size 844 */
    uint32_t unknown_a15;           /* 0494 1172 - 1176 size 4 */
} PACKED G_Unknown_6x70_t;

static int char_bb_size22322 = sizeof(G_Unknown_6x70_t);

// 0x71: Unknown (used while loading into game)
struct G_Unknown_6x71 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
} PACKED;

// 0x72: Unknown (used while loading into game)
struct G_Unknown_6x72 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
} PACKED;

// 0x73: warp packet �����������ݰ�
typedef struct subcmd_bb_warp_ship {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
} PACKED subcmd_bb_warp_ship_t;

// 0x74: Word select
// Packet used for word select
typedef struct subcmd_word_select {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t client_id_gc;
    uint16_t num_words;
    uint16_t ws_type;
    uint16_t words[12];
} PACKED subcmd_word_select_t;

// 0x74: Word select
// Packet used for word select
typedef struct subcmd_bb_word_select {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t num_words;
    uint16_t ws_type;
    uint16_t words[12];
} PACKED subcmd_bb_word_select_t;

// 0x75: Phase setup (ָ����Ч��Χ; ������Ϸ)
// TODO: Not sure about PC format here. Is this first struct DC-only?
typedef struct subcmd_set_flag {
    dc_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t flag; // Must be < 0x400
    uint16_t unknown_a1; // Must be 0 or 1
    uint16_t difficulty;
    uint16_t unused;
} PACKED subcmd_set_flag_t;

// 0x75: Phase setup (ָ����Ч��Χ; ������Ϸ)
typedef struct subcmd_bb_set_flag {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t flag; // Must be < 0x400
    uint16_t unknown_a1; // Must be 0 or 1
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
struct G_Unknown_6x78 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t client_id; // Must be < 12
    uint16_t unused1;
    uint32_t unused2;
} PACKED;

// 0x79: Lobby 14/15 gogo ball (soccer game)
//(00000000)   20 00 60 00 00 00 00 00  79 06 00 00 00 00 00 00  .`.....y.......
//(00000010)   52 3B 00 00 00 00 00 00  00 00 00 00 00 59 66 00 R; ...........Yf.

//( 00000000 )   20 00 60 00 00 00 00 00  79 06 66 68 B6 04 00 00  .`.....y.fh?..
//( 00000010 )   74 EA FF FF 00 00 00 00  00 00 00 00 00 59 66 00 t?
//
//( 00000000 )   20 00 60 00 00 00 00 00  79 06 66 68 4C 05 00 00  .`.....y.fhL...
//( 00000010 )   78 97 FF FF 7A 02 76 C2  C1 7E D2 42 00 59 66 00 x?
//
//( 00000000 )   20 00 60 00 00 00 00 00  79 06 66 68 A6 05 00 00  .`.....y.fh?..
//( 00000010 )   FB BC FF FF 5E CC FF C2  C7 16 41 40 00 59 66 00 ��
typedef struct subcmd_bb_gogo_ball {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr; // unused 0x0000 ���� 0x6866
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    float x;
    float z;
    uint32_t unknown_a5;         //0x00665900
} PACKED subcmd_bb_gogo_ball_t;

// 0x7A: Unknown
struct G_Unknown_6x7A {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x7B: Unknown
struct G_Unknown_6x7B {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x7C: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
// SUBCMD_CMODE_GRAVE
struct G_Unknown_6x7C {
    dc_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint8_t unknown_a1[2];
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
} PACKED;

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

/*��սģʽĹ������ BB) */
//���ݳ��� = 0x0150; 336 = 8 + 4 + 2 + 2 + 320
typedef struct subcmd_bb_grave {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id;
    uint16_t unk0;
    uint8_t data[320];
} PACKED subcmd_bb_grave_t;

static int sdasd = sizeof(subcmd_bb_grave_t);

// 0x7D: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
struct G_Unknown_6x7D {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1; // Must be < 7; used in jump table
    uint8_t unused[3];
    uint32_t unknown_a2[4];
} PACKED;

// 0x7E: Unknown (��֧�� Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x7F: Unknown (��֧�� Episode 3)
struct G_Unknown_6x7F {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1[0x20];
} PACKED;

// 0x80: Trigger trap (��֧�� Episode 3)
struct G_TriggerTrap_6x80 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED;

// 0x81: Unknown
struct G_Unknown_6x81 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x82: Unknown
struct G_Unknown_6x82 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x83: Place trap
struct G_PlaceTrap_6x83 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED;

// 0x84: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
struct G_Unknown_6x84 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1[6];
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unused;
} PACKED;

// 0x85: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
struct G_Unknown_6x85 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t unknown_a1; // Command is ignored unless this is 0
    uint16_t unknown_a2[7]; // Only the first 3 appear to be used
} PACKED;

// 0x86: Hit destructible object (��֧�� Episode 3)
struct G_HitDestructibleObject_6x86 {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
} PACKED;

// 0x87: Unknown
struct G_Unknown_6x87 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    float unknown_a1;
} PACKED;

// 0x88: Unknown (ָ����Ч��Χ; ������Ϸ)
struct G_Unknown_6x88 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0x89: subcmd_bb_player_died (ָ����Ч��Χ; ������Ϸ)
typedef struct subcmd_bb_player_died {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t flag;
    uint16_t unused;
} PACKED subcmd_bb_player_died_t;

// 0x8A: Unknown (��֧�� Episode 3)
struct G_Unknown_6x8A {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1; // Must be < 0x11
} PACKED;

// 0x8B: Unknown (��֧�� Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x8C: Unknown (��֧�� Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x8D: Set technique level override
typedef struct subcmd_bb_set_technique_level_override {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t level_upgrade;
    uint8_t unused1;
    uint16_t unused2;
} PACKED subcmd_bb_set_technique_level_override_t;

// 0x8E: Unknown (��֧�� Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x8F: Unknown (��֧�� Episode 3)
struct G_Unknown_6x8F {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    uint16_t unknown_a1;
} PACKED;

// 0x90: Unknown (��֧�� Episode 3)
struct G_Unknown_6x90 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
} PACKED;

// 0x91: Unknown (ָ����Ч��Χ; ������Ϸ)
struct G_Unknown_6x91 {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    uint16_t unknown_a3;
    uint16_t unknown_a4;
    uint16_t unknown_a5;
    uint8_t unknown_a6[2];
} PACKED;

// 0x92: Unknown (��֧�� Episode 3)
struct G_Unknown_6x92 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t unknown_a1;
    float unknown_a2;
} PACKED;

// 0x93: Timed switch activated (��֧�� Episode 3)
typedef struct subcmd_bb_timed_switch_activated {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t area;
    uint16_t switch_id;
    uint8_t unknown_a1; // �����ֵΪ1�����߼�������ֵ��ͬ 
    uint8_t unused[3];
} PACKED subcmd_bb_timed_switch_activated_t;

// 0x94: Warp (��֧�� Episode 3)
typedef struct subcmd_bb_inter_level_map_warp {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t area;
    uint8_t unused[2];
} PACKED subcmd_bb_inter_level_map_warp_t;

// 0x95: Unknown (��֧�� Episode 3)
struct G_Unknown_6x95 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t client_id;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    uint32_t unknown_a3;
} PACKED;

// 0x96: Unknown (��֧�� Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x97: Unknown (��֧�� Episode 3)
struct G_Unknown_6x97 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t unused1;
    uint32_t unknown_a1; // Must be 0 or 1
    uint32_t unused2;
    uint32_t unused3;
} PACKED;

// 0x98: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x99: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x9A: Update player stat (��֧�� Episode 3)
struct G_UpdatePlayerStat_6x9A {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t client_id2;
    // Values for what:
    // 0 = subtract HP
    // 1 = subtract TP
    // 2 = subtract Meseta
    // 3 = add HP
    // 4 = add TP
    uint8_t what;
    uint8_t amount;
} PACKED;

// 0x9B: Unknown
struct G_Unknown_6x9B {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t unknown_a1;
    uint8_t unused[3];
} PACKED;

// 0x9C: Unknown (ָ����Ч��Χ; ������Ϸ; ��֧�� Episode 3)
struct G_Unknown_6x9C {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unknown_a1;
} PACKED;

// 0x9D: Unknown (��֧�� Episode 3)
struct G_Unknown_6x9D {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t client_id2;
} PACKED;

// 0x9E: Unknown (��֧�� Episode 3)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x9F: Gal Gryphon actions (��֧�� PC or Episode 3)
struct G_GalGryphonActions_6x9F {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unknown_a1;
    float x;
    float z;
} PACKED;

// 0xA0: Gal Gryphon actions (��֧�� PC or Episode 3)
struct G_GalGryphonActions_6xA0 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    float x;
    float y;
    float z;
    uint32_t unknown_a1;
    uint16_t unknown_a2;
    uint16_t unknown_a3;
    uint32_t unknown_a4[4];
} PACKED;

// 0xA1: Unknown (��֧�� PC)
struct G_Unknown_6xA1 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
} PACKED;

// 0xA2: Request for item drop from box (��֧�� PC; handled by server on BB)
struct G_BoxItemDropRequest_6xA2 {
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
} PACKED;

// 0xA2: Request for item drop from box (��֧�� PC; handled by server on BB)
// Request drop from box (GC)
typedef struct subcmd_bitemreq {
    dc_pkt_hdr_t hdr;
    object_id_hdr_t shdr;                      /* �������� object_id 0x80 0x3F*/
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

// 0xA2: Request for item drop from box (��֧�� PC; handled by server on BB)
// Request drop from box (Blue Burst)
typedef struct subcmd_bb_bitemreq {
    bb_pkt_hdr_t hdr;
    object_id_hdr_t shdr;                      /* �������� object_id 0x80 0x3F*/
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

// 0xA3: Episode 2 boss actions (��֧�� PC or Episode 3)
struct G_Episode2BossActions_6xA3 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint8_t unknown_a1;
    uint8_t unknown_a2;
    uint8_t unknown_a3[2];
} PACKED;

// 0xA4: Olga Flow phase 1 actions (��֧�� PC or Episode 3)
struct G_OlgaFlowPhase1Actions_6xA4 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint8_t what;
    uint8_t unknown_a3[3];
} PACKED;

// 0xA5: Olga Flow phase 2 actions (��֧�� PC or Episode 3)
struct G_OlgaFlowPhase2Actions_6xA5 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint8_t what;
    uint8_t unknown_a3[3];
} PACKED;

// 0xA6: Modify trade proposal (��֧�� PC)
struct G_ModifyTradeProposal_6xA6 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint8_t unknown_a1; // Must be < 8
    uint8_t unknown_a2;
    uint8_t unknown_a3[2];
    uint32_t unknown_a4;
    uint32_t unknown_a5;
} PACKED;

typedef struct subcmd_bb_trade_menu {
    uint16_t menu_pos;
    uint16_t menu_id;
} PACKED subcmd_bb_trade_menu_t;

typedef struct subcmd_bb_trade_items {
    item_t titem[0x20][MAX_TRADE_ITEMS];
} PACKED subcmd_bb_trade_items_t;

/* 62���������ݰ�. (Blue Burst) */
typedef struct subcmd_bb_trade {
    bb_pkt_hdr_t hdr;                   /*18 00 62 00 00/01 00 00 00*/
    client_id_hdr_t shdr;
    uint8_t trade_type;                 /*D0��ʼ D1 D2 D3 D4 D5����ȡ��*/
    uint8_t trade_stage;                /*00 02 04*/
    uint8_t unk2;                       /*71 00 AC F6 */
    uint8_t unk3;                       /*00 21*/
    subcmd_bb_trade_menu_t menu;
    //uint32_t menu_id;                   /*�˵����*/
    uint32_t trade_num;                   /*Ŀ�����Ϊ0001*/
    //uint16_t unused;                    /*һֱΪ00 00*/
} PACKED subcmd_bb_trade_t;

// 0xA7: Unknown (��֧�� PC)
// This subcommand is completely ignored (at least, by PSO GC).

// 0xA8: Gol Dragon actions (��֧�� PC or Episode 3)
struct G_GolDragonActions_6xA8 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
    uint32_t unknown_a3;
} PACKED;

// 0xA8: Gol Dragon actions (��֧�� PC or Episode 3)
// Packet used by the Gol Dragon boss to deal with its actions.
typedef struct subcmd_gol_dragon_act {
    dc_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
    uint32_t unk2;
} PACKED subcmd_gol_dragon_act_t;

// 0xA8: Gol Dragon actions (��֧�� PC or Episode 3)
// Packet used by the Gol Dragon boss to deal with its actions.
typedef struct subcmd_bb_gol_dragon_act {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
    uint32_t unk2;
} PACKED subcmd_bb_gol_dragon_act_t;

// 0xA9: Barba Ray actions (��֧�� PC or Episode 3)
struct G_BarbaRayActions_6xA9 {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED;

// 0xAA: Episode 2 boss actions (��֧�� PC or Episode 3)
struct G_Episode2BossActions_6xAA {
    bb_pkt_hdr_t hdr;
    enemy_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
    uint32_t unknown_a3;
} PACKED;

// 0xAB: Create lobby chair (��֧�� PC)
struct G_CreateLobbyChair_6xAB {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED;

// 0xAB: Create lobby chair (��֧�� PC)
typedef struct subcmd_bb_chair_dir {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
} PACKED subcmd_bb_chair_dir_t;

// 0xAC: Unknown (��֧�� PC)
struct G_Unknown_6xAC {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t num_items;
    uint32_t item_ids[0x1E];
} PACKED;

// 0xAD: Unknown (��֧�� PC, Episode 3, or GC Trial Edition)
struct G_Unknown_6xAD {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    // The first byte in this array seems to have a special meaning
    uint8_t unknown_a1[0x40];
} PACKED;

// 0xAE: Set lobby chair state (sent by existing clients at join time)
// This subcommand is ��֧�� DC, PC, or GC Trial Edition.
struct G_SetLobbyChairState_6xAE {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint16_t unknown_a1;
    uint16_t unknown_a2;
    uint32_t unknown_a3;
    uint32_t unknown_a4;
} PACKED;

// 0xAF: Turn lobby chair (��֧�� PC or GC Trial Edition)
struct G_TurnLobbyChair_6xAF {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t angle; // In range [0x0000, 0xFFFF]
} PACKED;

// 0xB0: Move lobby chair (��֧�� PC or GC Trial Edition)

struct G_MoveLobbyChair_6xB0 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t unknown_a1;
} PACKED;

// 0xB1: Unknown (��֧�� PC or GC Trial Edition)
// This subcommand is completely ignored (at least, by PSO GC).

// 0xB2: Unknown (��֧�� PC or GC Trial Edition)
// TODO: It appears this command is sent when the snapshot file is written on
// PSO GC. Verify this.

struct G_Unknown_6xB2 {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint8_t area;
    uint8_t unused;
    uint16_t client_id;
    uint32_t unknown_a3; // PSO GC puts 0x00051720 here
} PACKED;

// 0xB3: Unknown (XBOX)
// 0xB3: CARD battle server data request (Episode 3)
// These commands have multiple subcommands; see the Episode 3 subsubcommand
// table after this table. The common format is:
struct G_CardBattleCommandHeader {
    unused_hdr_t shdr;
    uint8_t subsubcommand; // See 0xBx subcommand table (after this table)
    uint8_t sender_client_id;
    // If mask_key is nonzero, the remainder of the data (after unused2 in this
    // struct) is encrypted using a simple algorithm, which is implemented in
    // set_mask_for_ep3_game_command in SendCommands.cc.
    uint8_t mask_key;
    uint8_t unused2;
} PACKED;

struct G_CardServerDataCommandHeader {
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
} PACKED;

// 0xB4: Unknown (XBOX)
// 0xB4: CARD battle server response (Episode 3) - see 0xB3 (above)
// 0xB5: CARD battle client command (Episode 3) - see 0xB3 (above)

// 0xB5: BB shop request (handled by the server)
// �����̵����ݰ�. (Blue Burst)
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
typedef struct G_MapSubsubcommand_GC_Ep3_6xB6 {
    unused_hdr_t shdr;
    uint32_t size;
    uint8_t subsubcommand; // 0x40 or 0x41
    uint8_t unused[3];
} PACKED G_MapSubsubcommand_GC_Ep3_6xB6_t;

typedef struct G_MapList_GC_Ep3_6xB6x40 {
    G_MapSubsubcommand_GC_Ep3_6xB6_t header;
    uint16_t compressed_data_size;
    uint16_t unused;
    // PRS-compressed map list data follows here. newserv generates this from the
    // map index at startup time; see the MapList struct in Episode3/DataIndex.hh
    // and Episode3::DataIndex::get_compressed_map_list for details on the format.
} PACKED G_MapList_GC_Ep3_6xB6x40_t;

struct G_MapData_GC_Ep3_6xB6x41 {
    G_MapSubsubcommand_GC_Ep3_6xB6_t header;
    uint32_t map_number;
    uint16_t compressed_data_size;
    uint16_t unused;
    // PRS-compressed map data follows here (which decompresses to an
    // Episode3::MapDefinition).
} PACKED;

// 0xB6: BB shop contents (server->client only)
// Packet used for telling a client a shop's inventory. (Blue Burst)
typedef struct subcmd_bb_shop_inv {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;  // params 037F                 /* Set to 0 */
    uint8_t shop_type;
    uint8_t num_items;
    uint16_t unused2;                   /* Set to 0 */
    // Note: costl of these entries should be the price
    sitem_t items[0x14];
} PACKED subcmd_bb_shop_inv_t;

// 0xB7: Unknown (Episode 3 Trial Edition)

// 0xB7: BB buy shop item (handled by the server)
// Packet sent by the client to buy an item from the shop. (Blue Burst)
typedef struct subcmd_bb_shop_buy {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t new_inv_item_id;//0x0C                   /* ��������ɵ���ƷID */
    uint8_t shop_type;//0x10                  /* 0 ���� 1 ���� 2 װ��*/
    uint8_t shop_item_index;//0x11                 /* ��Ʒ�����̵��嵥λ�� */
    uint8_t num_bought;//0x12                 /* �������� ��� 10�� Ҳ���� 09*/
    uint8_t unknown_a1; // TODO: Probably actually unused; verify this
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
struct G_IdentifyResult_BB_6xB9 {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t item;
} PACKED;

// 0xBA: Unknown (Episode 3)
struct G_Unknown_GC_Ep3_6xBA {
    client_id_hdr_t shdr;
    uint16_t unknown_a1; // Low byte must be < 9
    uint16_t unknown_a2;
    uint32_t unknown_a3;
    uint32_t unknown_a4;
} PACKED;

// 0xBA: BB accept tekker result (handled by the server)
struct G_AcceptItemIdentification_BB_6xBA {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_id;
} PACKED;

// 0xBB: Sync card trade state (Episode 3)
// TODO: Certain invalid values for slot/args in this command can crash the
// client (what is properly bounds-checked). Find out the actual limits for
// slot/args and make newserv enforce them.
struct G_SyncCardTradeState_GC_Ep3_6xBB {
    client_id_hdr_t shdr;
    uint16_t what; // Must be < 5; this indexes into a jump table
    uint16_t slot;
    uint32_t args[4];
} PACKED;

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
struct G_CardCounts_GC_Ep3_6xBC {
    unused_hdr_t shdr;
    uint32_t size;
    uint8_t unknown_a1[0x2F1];
    // The client sends uninitialized data in this field
    uint8_t unused[3];
} PACKED;

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
struct G_WordSelectDuringBattle_GC_Ep3_6xBD {
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
} PACKED;

// 0xBD: BB bank action (take/deposit meseta/item) (handled by the server)
// Packet sent by clients to do something at the bank. (Blue Burst)
typedef struct subcmd_bb_bank_act {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_id;
    uint32_t meseta_amount;
    uint8_t action;
    uint8_t item_amount;
    uint16_t unused2;                   /* 0xFFFF */
} PACKED subcmd_bb_bank_act_t;

// 0xBE: Sound chat (Episode 3; not Trial Edition)
// This appears to be the only subcommand ever sent with the CB command.
struct G_SoundChat_GC_Ep3_6xBE {
    unused_hdr_t shdr;
    uint32_t sound_id; // Must be < 0x27
    //be_uint32_t unused;
    uint32_t unused;
} PACKED;

// 0xBE: BB create inventory item (server->client only)
// Packet sent to clients to let them know that an item got picked up off of
// the ground. (Blue Burst)
typedef struct subcmd_bb_create_item {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    item_t item;
    //uint32_t item[3];
    //uint32_t item_id;
    //uint32_t item2;
    uint32_t unused2;
} PACKED subcmd_bb_create_item_t;

// 0xBF: Change lobby music (Episode 3; not Trial Edition)
struct G_ChangeLobbyMusic_GC_Ep3_6xBF {
    unused_hdr_t shdr;
    uint32_t song_number; // Must be < 0x34
} PACKED;

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
    uint32_t sell_num;
} PACKED subcmd_bb_sell_item_t;

// 0xC1: Unknown
// 0xC2: Unknown

// 0xC3: Split stacked item (handled by the server on BB)
// Note: This is not sent if an entire stack is dropped; in that case, a normal
// item drop subcommand is generated instead.
// Packet sent by the client to notify that they're dropping part of a stack of
// items and tell where they're dropping them. (Blue Burst)
typedef struct subcmd_bb_drop_pos {
    bb_pkt_hdr_t hdr;
    client_id_hdr_t shdr;
    uint32_t area;
    float x;
    float z;
    uint32_t item_id;
    uint32_t amount;
} PACKED subcmd_bb_drop_pos_t;

// 0xC4: Sort inventory (handled by the server on BB)
// Packet sent by clients to sort their inventory. (Blue Burst)
typedef struct subcmd_bb_sort_inv {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint32_t item_ids[30];
} PACKED subcmd_bb_sort_inv_t;

// 0xC5: Medical center used

// 0xC6: Invalid subcommand
// 0xC7: Invalid subcommand

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
} PACKED subcmd_bb_req_exp_pkt_t;


// 0xC9: Invalid subcommand
// 0xCA: Invalid subcommand
// 0xCB: Unknown
// 0xCC: Unknown
// 0xCD: Unknown
// 0xCE: Unknown
// 0xCF: Unknown (ָ����Ч��Χ; ������Ϸ; handled by the server on BB)
// 0xD0: Invalid subcommand
// 0xD1: Invalid subcommand

// 0xD2: subcmd_bb_gallon_area
typedef struct subcmd_bb_gallon_area {
    bb_pkt_hdr_t hdr;
    unused_hdr_t shdr;
    uint16_t area;                 /*05 00 */
    uint8_t pos[4];
} PACKED subcmd_bb_gallon_area_pkt_t;

// 0xD3: Invalid subcommand
// 0xD4: Unknown
// 0xD5: Invalid subcommand
// 0xD6: Invalid subcommand
// 0xD7: Invalid subcommand
// 0xD8: Invalid subcommand
// 0xD9: Invalid subcommand
// 0xDA: Invalid subcommand
// 0xDB: Unknown
// 0xDC: Unknown

// 0xDD: Unknown
typedef struct subcmd_bb_set_exp_rate {
    bb_pkt_hdr_t hdr;
    params_hdr_t shdr;
} PACKED subcmd_bb_set_exp_rate_t;

// 0xDE: Invalid subcommand
// 0xDF: Invalid subcommand
// 0xE0: Invalid subcommand
// 0xE1: Invalid subcommand
// 0xE2: Invalid subcommand
// 0xE3: Unknown
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

/* Forward declarations */
static int subcmd_send_drop_stack(ship_client_t* c, uint32_t area, float x,
    float z, iitem_t* item);
static int subcmd_send_create_item(ship_client_t* c, item_t item, int send_to_client);
static int subcmd_send_destroy_map_item(ship_client_t* c, uint16_t area,
    uint32_t item_id);
static int subcmd_send_destroy_item(ship_client_t* c, uint32_t item_id,
    uint8_t amt);

/* Handle a 0x62/0x6D packet. */
int subcmd_handle_one(ship_client_t *c, subcmd_pkt_t *pkt);
int subcmd_bb_handle_one(ship_client_t *c, bb_subcmd_pkt_t *pkt);
int subcmd_bb_handle_one_orignal(ship_client_t* c, bb_subcmd_pkt_t* pkt);

/* Handle a 0x60 packet. */
int subcmd_handle_bcast(ship_client_t *c, subcmd_pkt_t *pkt);
int subcmd_bb_handle_bcast(ship_client_t *c, bb_subcmd_pkt_t *pkt);
int subcmd_bb_handle_bcast_orignal(ship_client_t* c, bb_subcmd_pkt_t* pkt);
int subcmd_dcnte_handle_bcast(ship_client_t *c, subcmd_pkt_t *pkt);

/* Handle an 0xC9/0xCB packet from Episode 3. */
int subcmd_handle_ep3_bcast(ship_client_t *c, subcmd_pkt_t *pkt);

int subcmd_send_lobby_item(lobby_t *l, subcmd_itemreq_t *req,
                           const uint32_t item[4]);
int subcmd_send_bb_lobby_item(lobby_t *l, subcmd_bb_itemreq_t *req,
                              const iitem_t *item);

int subcmd_send_bb_exp(ship_client_t *c, uint32_t exp_amount);
int subcmd_send_bb_set_exp_rate(ship_client_t* c, uint32_t exp_rate);
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
