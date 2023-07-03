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

#ifndef WORD_SELECT_H
#define WORD_SELECT_H

int word_select_send_dc(ship_client_t *c, subcmd_word_select_t *pkt);
int word_select_send_pc(ship_client_t *c, subcmd_word_select_t *pkt);
int word_select_send_gc(ship_client_t *c, subcmd_word_select_t *pkt);
int word_select_send_bb(ship_client_t* c, subcmd_bb_word_select_t* pkt);

#endif /* !WORD_SELECT_H */
