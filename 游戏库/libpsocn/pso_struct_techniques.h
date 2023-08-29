/*
    梦幻之星中国 法术结构

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

#ifndef PSO_STRUCT_TECHNIQUES_H
#define PSO_STRUCT_TECHNIQUES_H

#include <stdint.h>

/* 科技函数ID */
/* 火球术 */
#define TECHNIQUE_FOIE              0
/* 火墙术 */
#define TECHNIQUE_GIFOIE            1
/* 炎狱术 */
#define TECHNIQUE_RAFOIE            2
/* 冻气术 */
#define TECHNIQUE_BARTA             3
/* 冰箭术 */
#define TECHNIQUE_GIBARTA           4
/* 极冰术 */
#define TECHNIQUE_RABARTA           5
/* 闪电术 */
#define TECHNIQUE_ZONDE             6
/* 群雷术 */
#define TECHNIQUE_GIZONDE           7
/* 雷爆术 */
#define TECHNIQUE_RAZONDE           8
/* 圣光术 */
#define TECHNIQUE_GRANTS            9
/* 强体术 */
#define TECHNIQUE_DEBAND            10
/* 降攻术 */
#define TECHNIQUE_JELLEN            11
/* 降防术 */
#define TECHNIQUE_ZALURE            12
/* 强攻术 */
#define TECHNIQUE_SHIFTA            13
/* 瞬移术 */
#define TECHNIQUE_RYUKER            14
/* 圣泉术 */
#define TECHNIQUE_RESTA             15
/* 状态术 */
#define TECHNIQUE_ANTI              16
/* 回魂术 */
#define TECHNIQUE_REVERSER          17
/* 死咒术 */
#define TECHNIQUE_MEGID             18

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct player_techniques {
    union {
        struct {
            /* 火球术 */ uint8_t foie;
            /* 火墙术 */ uint8_t gifoie;
            /* 炎狱术 */ uint8_t rafoie;
            /* 冻气术 */ uint8_t barta;
            /* 冰箭术 */ uint8_t gibarta;
            /* 极冰术 */ uint8_t rabarta;
            /* 闪电术 */ uint8_t zonde;
            /* 群雷术 */ uint8_t gizonde;
            /* 雷爆术 */ uint8_t razonde;
            /* 圣光术 */ uint8_t grants;
            /* 强体术 */ uint8_t deband;
            /* 降攻术 */ uint8_t jellen;
            /* 降防术 */ uint8_t zalure;
            /* 强攻术 */ uint8_t shifta;
            /* 瞬移术 */ uint8_t ryuker;
            /* 圣泉术 */ uint8_t resta;
            /* 状态术 */ uint8_t anti;
            /* 回魂术 */ uint8_t reverser;
            /* 死咒术 */ uint8_t megid;
            /* ?????? */ uint8_t unused;
        }PACKED;
        uint8_t all[0x14];
    }PACKED;
} PACKED techniques_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

#endif // !PSO_STRUCT_TECHNIQUES_H
