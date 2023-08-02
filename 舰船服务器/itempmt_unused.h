#pragma once

#include <stdint.h>

/* PSOBB ITEMPMT STURCT data. */
/* 玛古喂养表结构 */
typedef struct MagFeedItem_Pmt {
    int8_t defence;
    int8_t power;
    int8_t dexterity;
    int8_t mind;
    int8_t iq;   // Reversed iq / sync 8/15/2015
    int8_t sync;
    int8_t reserved[2];
}PMT_MAGFEEDITEM;

/* 玛古喂养表结构 */
typedef struct MagFeed_Pmt {
    PMT_MAGFEEDITEM item[11];
}PMT_MAGFEED;

/* 玛古喂养表结构 */
typedef struct magfeedtable_pmt {
    PMT_MAGFEED table[8];
}PMT_MAGFEEDTABLE;

/* 物品的星星数？*/
typedef struct starvalue_pmt {
    uint8_t starvalue;
}PMT_STARVALUE;

/* 特殊类型结构 */
typedef struct special_pmt {
    int16_t type;
    int16_t amount;
}PMT_SPECIAL;

/* 状态提升结构 */
typedef struct statboost_pmt {
    uint8_t stat1;
    uint8_t stat2;
    int16_t amount1;
    int16_t amount2;
}PMT_STATBOOST;

/* 科技结构 */
typedef struct techuseclass_pmt {
    int8_t _class[12];
}PMT_TECHUSECLASS;

/* 科技结构 */
typedef struct techusetable_pmt {
    PMT_TECHUSECLASS tech[19];
}PMT_TECHUSETABLE;

/* 科技提升结构 */
typedef struct techboost_pmt {
    int32_t tech1;
    float boost1;
    int32_t tech2;
    float boost2;
    int32_t tech3;
    float boost3;
}PMT_TECHBOOST;

/* 合成表结构 */
typedef struct combination_pmt {
    uint8_t useitem[3];
    uint8_t equippeditem[3];
    uint8_t itemresult[3];
    uint8_t maglevel;
    uint8_t grind;
    uint8_t level;
    uint8_t _class;
    uint8_t reserved[3];
}PMT_COMBINATION;

/* 节日物品结构 */
typedef struct eventitem_pmt {
    uint8_t item[3];
    uint8_t chance;
}PMT_EVENTITEM;

/* 不合理物品结构 */
typedef struct unsealable_pmt {
    uint8_t item[3];
    uint8_t reserved;
}PMT_UNSEALABLE;

/* 武器销售除数 */
typedef struct weaponsaledivisor_pmt {
    float saledivisor[165];
}PMT_WEAPONSALEDIVISOR;

/* 盔甲盾牌销售除数 */
typedef struct armor_shield_saledivisor_pmt {
    float armor;
    float shield;
    float unit;
    float mag;
}PMT_ARMOR_SHIELD_SALEDIVISOR;

/* 攻击动画结构 */
typedef struct attack_animation_pmt {
    uint8_t animation;
}PMT_ATTACKANIMATION;

/* 查询表结构 */
typedef struct lookup_table_item_pmt {
    int32_t pointer;
}PMT_LOOKUP_TABLE_ITEM;

/* 物品数量结构 */
typedef struct item_count_pmt {
    uint32_t weapon_count;
    uint32_t armor_count;
    uint32_t shield_count;
    uint32_t unit_count;
    uint32_t item_count;
    uint32_t mag_count;
    uint32_t starvalue_count;
    uint32_t special_count;
    uint32_t statboost_count;
    uint32_t combination_count;
    uint32_t techboost_count;
    uint32_t unwrap_count;
    uint32_t xmasitem_count;
    uint32_t easteritem_count;
    uint32_t halloweenitem_count;
    uint32_t unsealable_count;
}PMT_ITEM_COUNT;

PMT_ITEM_COUNT Item_Pmt_Count;

/* 完整ITEMPMT聚合结构 */
typedef struct Item_Pmt {
    PMT_ITEM_COUNT item_count;
    PMT_LOOKUP_TABLE_ITEM pmt_lookup_table[0x04][0xFF][0xFF];
    PMT_MAGFEEDTABLE* magfeedtable;
    PMT_STARVALUE* starvaluetable;
    uint32_t starvalueOffset;
    PMT_SPECIAL* specialtable;
    PMT_STATBOOST* statboosttable;
    PMT_TECHUSETABLE* techusetable;
    PMT_COMBINATION* combinationtable;
    PMT_TECHBOOST* techboosttable;
    PMT_ATTACKANIMATION* attackanimationtable;
    PMT_EVENTITEM* xmasitemtable;
    PMT_EVENTITEM* easteritemtable;
    PMT_EVENTITEM* halloweenitemtable;
    PMT_UNSEALABLE* unsealabletable;
    PMT_WEAPONSALEDIVISOR* weaponsaledivisortable;
    PMT_ARMOR_SHIELD_SALEDIVISOR* armorshieldsaledivisortable;
}ITEMPMT;

//ITEMPMT Item_Pmt_Tbl;

//https://www.pioneer2.net/community/threads/editing-some-things-p.71/
/*
The file contains many pointers.The pointers are located at the end of the file.To process the file first read the pointer located at the end of the file minus 16 bytes(filesize - 16).The original value at this offset is 0x15064. This pointer is the Weapon pointer table.Once at offset 0x15064 increment by 4 bytes for the other tables.

0x15064 : Weapon Pointer Table(0x1489C)
0x15068 : Armor Pointer Table(0x147A4)
0x1506c : Units Pointer Table(0x147B4)
0x15070 : Item Pointer Table(0x147C4)
0x15074 : Mag Pointer Table(0x147BC)
0x15078 : Attack Animation Table(0xF4CC)
0x1507c : Photon Colour Table(0xDE7C)
0x15080 : Weapon Range Table(0xE194)
0x15084 : Weapon Sales Divisor Table Pointer(0xF5BC)
0x15088 : Sales Divisor Table Pointer(0xF850)
0x1508c : Mag Feed Table Pointer(0x15044)
0x15090 : Star Value Table Pointer(0xFB20)
0x15094 : Special Data Table Pointer(0xFE50)
0x15098 : Weapon SFX Table Pointer(0xFEF4)
0x1509c : Stat Boost Table Pointer(0x12770)
0x150a0 : Shield SFX Table Pointer(0x11C94)
0x150a4 : Tech use table Pointer(0x128A8)
0x150a8 : Combination Table Pointer(0x1500C)
0x150ac : Unknown01 Table Pointer(0x12768)
0x150b0 : Tech Boosts Table Pointer(0x1428C)
0x150b4 : Unwrap Table Pointer(0x1502C)
0x150b8 : Unsealable Table Pointer(0x15034)
0x150bc : Ranged Special Table Pointer(0x1503c)

You can read a lot of the above tables by using the above method.I personally only read the Weapon Sales Divisor Table Pointerand Sales Divisor Table Pointer from there.

I use the other pointer table to read the other tables.If you look at the pointer value located at filesize - 32 bytes.The original value at this offset you should find the value of 0x150c0. Next to this value is 0x13a and next to that is 0x01. This indicates that there is 1 pointer table containing 314 entries(0x13a) and it is located at 0x150c0. This pointer table structure is a table of short values.The original values are 0x49d2, 0x01, 0x02, 0x01, 0x02 and so on.
To read this you multiply the value by 4 and add it to the previous value.This will give you the pointer. for example the first value of 0x49d2 would give you 0x12748 this pointer is Hit SFX table pointer which will be 0x121bc:
使用上述方法，您可以阅读上面的许多表格。我个人只从那里读取武器销售除数表指针和销售除数表指针。

我使用其他指针表读取其他表。如果查看位于filesize-32字节的指针值。此偏移量处的原始值应为0x150c0。该值旁边是0x13a，该值旁边是0x01。这表示有一个指针表包含314个条目（0x13a），它位于0x150c0。此指针表结构是一个短值表。原始值为0x49d2、0x01、0x02、0x01、0x02等。

要读取此值，请将该值乘以4，然后将其与上一个值相加。这将为您提供指针。例如，0x49d2的第一个值将为您提供0x12748此指针为Hit SFX表格指针，其将为0x121bc：
0x150c0 : Short 0x49d2 x 4 = Pointer 0x12748 = 0x121bc - Hit SFX Force
0x150c2 : Short 0x01 x 4 + 0x12748 = Pointer 0x1274c = 0x12364 - Hit SFX Force
0x150c4 : Short 0x02 x 4 + 0x1274c = Pointer 0x12754 = 0x12394 - Hit SFX Hunter
0x150c6 : Short 0x01 x 4 + 0x12754 = Pointer 0x12758 = 0x1253c - Hit SFX Hunter
0x150c8 : Short 0x02 x 4 + 0x12758 = Pointer 0x12760 = 0x1256c - Hit SFX Ranger
0x150ca : Short 0x01 x 4 + 0x12760 = Pointer 0x12764 = 0x12714 - Hit SFX Ranger
0x150cc : Short 0x02 x 4 + 0x12764 = Pointer 0x1276c = 0x12744 - Hit SFX Force Default
0x150ce : Short 0x080f x 4 + 0x1276c = Pointer 0x147A8 = 0x1500 - Armour Table
0x150d0 : Short 0x02 x 4 + 0x147A8 = Pointer 0x147b0 = 0x40 - Shields Table
0x150d2 : Short 0x02 x 4 + 0x147b0 = Pointer 0x147b8 = 0x2020 - Units Table
0x150d4 : Short 0x02 x 4 + 0x147b8 = Pointer 0x147c0 = 0x2804 - Mags Table
0x150d6 : Short 0x02 x 4 + 0x147c0 = Pointer 0x147c8 = 0x3118 - Items Table(Mates)
0x150d8 : Short 0x02 x 4 + 0x147c8 = Pointer 0x147d0 = 0x3160 - Items Table(Fluids)

And so on.

The number of items in each table is specified also with each pointer.For instance the Armour table.Pointer at 0x147A8 to the left of this value is 0x59 (89 dec) and there are 89 armours by default (including the ? ? ? ? / Unknown last entry).

Reading the file is the easy part adding to the file requires that you update all of the pointersand item counts.Also maintaining the ? ? ? ? item as the last entry in the tables and insuring you renumber the item ID numbers when adding items so that the star values will work correctly.

Hope you find the above info useful.
等等

每个表中的项目数也随每个指针一起指定。例如，Armour表。该值左侧0x147A8处的指针为0x59（89 dec），默认情况下有89个Armour（包括？？？/未知的最后一个条目）。

读取文件是添加到文件中最简单的部分，需要更新所有指针和项目计数？项目作为表中的最后一项，并确保在添加项目时对项目ID号重新编号，以便星号值正常工作。

希望以上信息对您有用*/


