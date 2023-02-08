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

#ifndef BB_MAX_TECH_H
#define BB_MAX_TECH_H

//19 个科技类别 12个 职业 完成结构数据读取
#define BB_MAX_TECH_LEVEL 19
#define BB_MAX_CLASS      12

typedef struct bb_max_tech_level {
    char tech_name[12];
    unsigned char max_lvl[BB_MAX_CLASS];
} bb_max_tech_level_t;

bb_max_tech_level_t max_tech_level[BB_MAX_TECH_LEVEL];
//
//static old_bb_max_tech_level_t old_max_tech_level[BB_MAX_TECH_LEVEL] = {
//    //{"Foie",     15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Foie",     0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Gifoie",   15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Gifoie",   0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Rafoie",   15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Rafoie",   0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Barta",    15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Barta",    0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Gibarta",  15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Gibarta",  0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Rabarta",  15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Rabarta",  0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Zonde",    15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Zonde",    0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Gizonde",  15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Gizonde",  0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Razonde",  15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Razonde",  0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Grants",   0,   0,    0,    0,    0,    0,    30,   30,   30,   0,    30,   0  },
//    {"Grants",   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x00 },
//
//    //{"Deband",   13,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Deband",   0x0D, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Jellen",   15,  20,   0,    0,    0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Jellen",   0x0F, 0x14, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Zalure",   15,  20,   0,    0,    0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Zalure",   0x0F, 0x14, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Shifta",   13,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Shifta",   0x0D, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Ryuker",   1,   1,    0,    1,    0,    0,    1,    1,    1,    0,    1,    1  },
//    {"Ryuker",   0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01 },
//
//    //{"Resta",    15,  20,   0,    15,   0,    0,    30,   30,   30,   0,    30,   20 },
//    {"Resta",    0x0F, 0x14, 0x00, 0x0F, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x14 },
//
//    //{"Anti",     5,   6,    0,    5,    0,    0,    7,    7,    7,    0,    7,    6  },
//    {"Anti",     0x05, 0x06, 0x00, 0x05, 0x00, 0x00, 0x07, 0x07, 0x07, 0x00, 0x07, 0x06 },
//
//    //{"Reverser", 0,   0,    0,    0,    0,    0,    1,    1,    1,    0,    1,    0  },
//    {"Reverser", 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00 },
//
//    //{"Megid",    0,   0,    0,    0,    0,    0,    30,   30,   30,   0,    30,   0  },
//    {"Megid",    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1E, 0x00 },
//
//    /*
//    {"Foie",     15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Gifoie",   15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Rafoie",   15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Barta",    15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Gibarta",  15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Rabarta",  15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Zonde",    15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Gizonde",  15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Razonde",  15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Grants",   0,  0,  0, 0,  0, 0, 30, 30, 30, 0, 30, 0  },
//    {"Deband",   13, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Jellen",   15, 20, 0, 0,  0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Zalure",   15, 20, 0, 0,  0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Shifta",   13, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Ryuker",   1,  1,  0, 1,  0, 0, 1,  1,  1,  0, 1,  1  },
//    {"Resta",    15, 20, 0, 15, 0, 0, 30, 30, 30, 0, 30, 20 },
//    {"Anti",     5,  6,  0, 5,  0, 0, 7,  7,  7,  0, 7,  6  },
//    {"Reverser", 0,  0,  0, 0,  0, 0, 1,  1,  1,  0, 1,  0  },
//    {"Megid",    0,  0,  0, 0,  0, 0, 30, 30, 30, 0, 30, 0  },*/
//};

#endif /* !BB_MAX_TECH_H */