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

typedef struct bb_max_tech_level {
    char tech_name[12];
    uint8_t max_lvl[MAX_PLAYER_CLASS_BB];
} bb_max_tech_level_t;

/* 已改为数据库载入 */
bb_max_tech_level_t max_tech_level[MAX_PLAYER_TECHNIQUES];
#endif /* !BB_MAX_TECH_H */