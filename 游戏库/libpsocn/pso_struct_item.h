/*
    梦幻之星中国 结构长度表

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

#ifndef PSOCN_STRUCT_ITEM_H
#define PSOCN_STRUCT_ITEM_H

#include <stdint.h>

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct item_data { // 0x14 bytes
    // 这是一个关于物品格式的参考注释，用于解释不同类型物品的数据格式。
    // 以下是各种物品类型及其相应的数据格式：
    // 武器（Weapon）  ：00ZZZZGG SS00AABB AABBAABB 00000000
    // 
    // 防具（Armor）   ：0101ZZ00 FFTTDDDD EEEE0000 00000000
    // 
    // 盾牌（Shield）  ：0102ZZ00 FFTTDDDD EEEE0000 00000000
    // 
    // 插件（Unit）    ：0103ZZ00 FF0000RR RR000000 00000000
    // 
    // 玛古（Mag）     ：02ZZLLWW HHHHIIII JJJJKKKK YYQQPPVV
    // 
    // 工具（Tool）    ：03ZZZZFF 00CC0000 00000000 00000000
    // 
    // 梅塞塔（Meseta）：04000000 00000000 00000000 MMMMMMMM
    // 
    // 在上述格式中，每个大写字母代表一个特定的属性或数值字段。
    // 例如，
    // A 表示属性类型（对于 S 级物品，则表示自定义名称），
    // B 表示属性数量（对于 S 级物品，则表示自定义名称），
    // C 表示堆叠大小（对于工具类物品），
    // D 表示防御加成，
    // E 表示闪避加成，
    // F 表示标志位（40 表示存在；对于工具类物品而言，如果物品可堆叠，则此字段未使用），
    // G 表示武器打磨等级，
    // H 表示玛古防御力，
    // I 表示玛古攻击力，
    // J 表示玛古敏捷度，
    // K 表示玛古力量，
    // L 表示玛古等级，
    // M 表示梅塞塔数量，
    // P 表示玛古标志位（40 表示存在，04 表示拥有左侧 PB，02 表示拥有右侧 PB，01 表示拥有中心 PB），
    // Q 表示玛古智力，
    // R 表示单位修饰符（小端序），
    // S 表示武器标志位（80 表示未鉴定，40 表示存在），
    // T 表示插槽数量，
    // V 表示玛古颜色，
    // W 表示光子爆发，
    // Y 表示玛古同步，
    // Z 表示物品 ID。
    // 请注意：PSO GC 版本在处理玛古物品时会错误地对 data2 进行字节交换（byteswaps），
    // 即使该物品实际上是小端序。
    // 这导致它与其他版本的 PSO（即所有其他版本）不兼容。
    // 我们需要在接收和发送数据之前手动对 data2 进行字节交换来解决这个问题。

    union {
        uint8_t data_b[12];//字节
        uint16_t data_w[6];//宽字节
        uint32_t data_l[3];//32位数值
    };

    uint32_t item_id;

    union {
        uint8_t data2_b[4];
        uint16_t data2_w[2];
        uint32_t data2_l;
    };
} PACKED item_t;

// PSO V2 stored some extra data in the character structs in a format that I'm
// sure Sega thought was very clever for backward compatibility, but for us is
// just plain annoying. Specifically, they used the third and fourth bytes of
// the InventoryItem struct to store some things not present in V1. The game
// stores arrays of bytes striped across these structures. In newserv, we call
// those fields extension_data. They contain:
//   items[0].extension_data1 through items[19].extension_data1:
//       Extended technique levels. The values in the v1_technique_levels array
//       only go up to 14 (tech level 15); if the player has a technique above
//       level 15, the corresponding extension_data1 field holds the remaining
//       levels (so a level 20 tech would have 14 in v1_technique_levels and 5
//       in the corresponding item's extension_data1 field).
//   items[0].extension_data2 through items[3].extension_data2:
//       The value known as unknown_a1 in the PSOGCCharacterFile::Character
//       struct. See SaveFileFormats.hh.
//   items[4].extension_data2 through items[7].extension_data2:
//       The timestamp when the character was last saved, in seconds since
//       January 1, 2000. Stored little-endian, so items[4] contains the LSB.
//   items[8].extension_data2 through items[12].extension_data2:
//       Number of power materials, mind materials, evade materials, def
//       materials, and luck materials (respectively) used by the player.
//   items[13].extension_data2 through items[15].extension_data2:
//       Unknown. These are not an array, but do appear to be related.

typedef struct psocn_iitem { // 0x1c bytes  
    uint16_t present; // 0x0001 = 物品槽使用中, 0xFF00 = 未使用
    uint16_t tech;  //是否鉴定
    uint32_t flags;// 0x00000008 = 已装备
    item_t data;
} PACKED iitem_t;

typedef struct psocn_inventory {
    uint8_t item_count;
    uint8_t hpmats_used;
    uint8_t tpmats_used;
    uint8_t language;
    iitem_t iitems[30];
} PACKED inventory_t;

typedef struct psocn_bitem { // 0x18 bytes
    item_t data;
    uint16_t amount;
    uint16_t show_flags; //是否显示
} PACKED bitem_t;

typedef struct psocn_bank {
    uint32_t item_count;
    uint32_t meseta;
    bitem_t bitems[200];
} PACKED psocn_bank_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED


#endif // !PSOCN_STRUCT_ITEM_H