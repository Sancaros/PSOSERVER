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

#ifndef RECORDS_H
#define RECORDS_H

#include <pso_character.h>

#include <pso_player.h>
#include <pso_items.h>
#include "clients.h"


void convert_dc_to_bb_challenge(const dc_challenge_records_t* rec_dc, bb_challenge_records_t* rec_bb);

void convert_bb_to_dc_challenge(const bb_challenge_records_t* rec_bb, dc_challenge_records_t* rec_dc);

#endif /* !RECORDS_H */
