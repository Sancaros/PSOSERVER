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

#define MAX_PLAYER_BANK_ITEMS           200
#define MAX_PLAYER_INV_ITEMS            30
#define MAX_PLAYER_TECHNIQUES           19
#define MAX_TRADE_ITEMS                 200
#define MAX_PLAYER_MESETA               999999

/* 20 字节 */
typedef struct item_data {
    // 这是一个关于物品格式的参考注释，用于解释不同类型物品的数据格式。
    // 以下是各种物品类型及其相应的数据格式：
    //                    0 1 2 3  4 5 6 7  8 9 A B  C D E F    
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
    // 美赛塔（Meseta）：04000000 00000000 00000000 MMMMMMMM
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
    // M 表示美赛塔数量，
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
        uint8_t datab[12];//字节
        uint16_t dataw[6];//宽字节
        uint32_t datal[3];//32位数值
    }PACKED;
    uint32_t item_id;
    union {
        uint8_t data2b[4];
        uint16_t data2w[2];
        uint32_t data2l;
    }PACKED;
} PACKED item_t;

// 武器物品参数结构
typedef struct bb_item_wp {
    union data_l {
        uint32_t data_l[3];//32位数值
        uint16_t data_w[6];//宽字节
        struct data_b {
            //物品类型
            uint8_t type;
            //子类型
            uint8_t subtype;
            //用于物品类索引
            uint8_t index;
            //打磨 最大值貌似是 35
            uint8_t polish;

            //special 0x00 - 0x28 / untekked 0x80 / presen 0x40 
            //增加标签 则不同标签值相加
            uint8_t flags;
            //presen color type 0x00 - 0x0C 
            uint8_t subflags;
            // 属性1 属性1百分比
            // none native a beast machine dark hitt 0x00 - 0x05
            // 0x00 0x01   0x02    0x03    0x04 0x05
            uint8_t attrb1;
            uint8_t attrb1_add;

            // 属性2 属性2百分比
            // none native a beast machine dark hitt 0x00 - 0x05
            // 0x00 0x01   0x02    0x03    0x04 0x05
            uint8_t attrb2;
            uint8_t attrb2_add;
            // 属性3 属性3百分比
            // none native a beast machine dark hitt 0x00 - 0x05
            // 0x00 0x01   0x02    0x03    0x04 0x05
            uint8_t attrb3;
            uint8_t attrb3_add;
        };
    };

    uint32_t item_id;              /* Set to 0xFFFFFFFF */

    union data2_l {
        uint32_t amt;
        uint16_t data2_w[2];
        uint8_t data2_b[4];
    };
} bb_item_wp_t;

static int sadd213213213as = sizeof(bb_item_wp_t);

// BB 美赛塔物品参数结构
typedef struct bb_item_mst {
    uint8_t type; //aways 0x04
    uint8_t unused1;
    uint8_t unused2;
    uint8_t unused3;
    uint32_t unused4;
    uint32_t unused5;
    uint32_t amt; //mst amount
} PACKED bb_item_mst_t;

//PSO V2在字符结构中存储了一些额外的数据，格式如下
//当然，世嘉认为向后兼容性非常聪明，但对我们来说
//只是很烦人。具体来说，他们使用了
//InventoryItem结构来存储V1中不存在的一些东西。游戏
//存储跨这些结构条带化的字节数组。在newserv中，我们调用
//那些字段extension_data。它们包含：
//items[0]。extension_data1到items[19]。extension_data1：
//扩展的技术水平。technique_levels_v1数组中的值
//只有14级（技术等级15）；如果玩家有以上技术
//级别15，相应的extension_data1字段保存剩余的
//等级（因此，一个20级的技术将在technique_levels_v1和5中有14个
//在对应项目的extension_data1字段中）。
//items[0]。extension_data2到items[3]。extension_data2：
//PSOGCCharacterFile:：Character结构中的flags字段；看见
//SaveFileFormats.hh获取详细信息。
//items[4]。extension_data2到items[7]。extension_data2：
//上次保存字符时的时间戳，以秒为单位
//2000年1月1日。存储了小endian，所以items[4]包含LSB。
//items[8].exextension_data2到items[12].exextension _data2：
//力量材料、心理材料、躲避材料、def的数量
//材料和玩家（分别）使用的运气材料。
//items[13]。extension_data2到items[15]。extension_data2：
//未知。这些不是一个数组，但看起来确实是相关的。
// PSO V2 stored some extra data in the character structs in a format that I'm
// sure Sega thought was very clever for backward compatibility, but for us is
// just plain annoying. Specifically, they used the third and fourth bytes of
// the InventoryItem struct to store some things not present in V1. The game
// stores arrays of bytes striped across these structures. In newserv, we call
// those fields extension_data. They contain:
//   items[0].extension_data1 through items[19].extension_data1:
//       Extended technique levels. The values in the technique_levels_v1 array
//       only go up to 14 (tech level 15); if the player has a technique above
//       level 15, the corresponding extension_data1 field holds the remaining
//       levels (so a level 20 tech would have 14 in technique_levels_v1 and 5
//       in the corresponding item's extension_data1 field).
//   items[0].extension_data2 through items[3].extension_data2:
//       The flags field from the PSOGCCharacterFile::Character struct; see
//       SaveFileFormats.hh for details.
//   items[4].extension_data2 through items[7].extension_data2:
//       The timestamp when the character was last saved, in seconds since
//       January 1, 2000. Stored little-endian, so items[4] contains the LSB.
//   items[8].extension_data2 through items[12].extension_data2:
//       Number of power materials, mind materials, evade materials, def
//       materials, and luck materials (respectively) used by the player.
//   items[13].extension_data2 through items[15].extension_data2:
//       Unknown. These are not an array, but do appear to be related.
/* 28 字节 */
typedef struct psocn_iitem {
    uint16_t present; // 0x0001 = 物品槽使用中, 0xFF00 = 未使用
  // See note above about these fields
    uint8_t extension_data1;  //是否鉴定
    uint8_t extension_data2;
    uint32_t flags;// 0x00000008 = 已装备
    item_t data;
} PACKED iitem_t;

/* 844 字节 */
typedef struct psocn_inventory {
    uint8_t item_count;
    uint8_t hpmats_used;
    uint8_t tpmats_used;
    uint8_t language;
    iitem_t iitems[MAX_PLAYER_INV_ITEMS];
} PACKED inventory_t;

/* 24 字节*/
typedef struct psocn_bitem {
    item_t data;
    uint16_t amount;
    uint16_t show_flags; //是否显示
} PACKED bitem_t;

/* 4808 字节 */
typedef struct psocn_bank {
    uint32_t item_count;
    uint32_t meseta;
    bitem_t bitems[MAX_PLAYER_BANK_ITEMS];
} PACKED psocn_bank_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED


#endif // !PSOCN_STRUCT_ITEM_H