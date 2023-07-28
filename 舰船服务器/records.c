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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pso_character.h>

#define MAX_RANK_TITLE_LENGTH 100

uint16_t encode_xrgb1555(uint32_t xrgb8888) {
    return ((xrgb8888 >> 9) & 0x7C00) | ((xrgb8888 >> 6) & 0x03E0) | ((xrgb8888 >> 3) & 0x001F);
}

uint32_t encrypt_challenge_time(uint16_t value) {
    uint8_t available_bits[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    uint16_t mask = 0;
    uint8_t num_one_bits = (rand() % 9) + 4; // Range [4, 12]
    for (; num_one_bits; num_one_bits--) {
        uint8_t index = rand() % (sizeof(available_bits) / sizeof(available_bits[0]));
        uint8_t* it = &available_bits[index];
        mask |= (1 << *it);
        memmove(it, it + 1, sizeof(available_bits) - (index + 1));
    }

    return (mask << 16) | (value ^ mask);
}

uint16_t decrypt_challenge_time(uint32_t value) {
    uint16_t mask = (value >> 0x10);
    uint8_t mask_one_bits = 0;
    for (int i = 0; i < 16; i++) {
        if (mask & (1 << i)) {
            mask_one_bits++;
        }
    }

    return ((mask_one_bits < 4) || (mask_one_bits > 12))
        ? 0xFFFF
        : ((mask ^ value) & 0xFFFF);
}

uint16_t* crypt_challenge_rank_text(const uint16_t* src, size_t count, bool is_encrypt) {
    if (count == 0) {
        return NULL;
    }

    uint16_t* ret = (uint16_t*)malloc((count + 1) * sizeof(uint16_t));
    memset(ret, 0, (count + 1) * sizeof(uint16_t));

    const uint16_t* src_ptr = src;
    uint16_t prev = 0;
    for (size_t i = 0; i < count; i++) {
        uint16_t ch = src_ptr[i];
        if (ch == 0) {
            break;
        }
        if (ret[0] == 0) {
            ret[0] = ch ^ 0x7F;
        }
        else {
            ret[i] = (is_encrypt ? ((ch - prev) ^ 0x7F) : ((ch ^ 0x7F) + ret[i - 1])) & 0xFF;
        }
        prev = ch;
    }

    return ret;
}

char* encrypt_challenge_rank_text(const char* src, size_t count) {
    uint16_t* encrypted_text = crypt_challenge_rank_text((const uint16_t*)src, count, true);
    char* ret = (char*)malloc((count + 1) * sizeof(char));
    for (size_t i = 0; i < count; i++) {
        ret[i] = (char)(encrypted_text[i] & 0xFF);
    }
    ret[count] = '\0';
    free(encrypted_text);
    return ret;
}

char* decrypt_challenge_rank_text(const char* src, size_t count) {
    uint16_t* decrypted_text = crypt_challenge_rank_text((const uint16_t*)src, count, false);
    size_t decrypted_count = wcslen(decrypted_text);
    char* ret = (char*)malloc((decrypted_count + 1) * sizeof(char));
    for (size_t i = 0; i < decrypted_count; i++) {
        ret[i] = (char)(decrypted_text[i] & 0xFF);
    }
    ret[decrypted_count] = '\0';
    free(decrypted_text);
    return ret;
}

void convert_dc_to_bb_challenge(const dc_challenge_records_t* rec_dc, bb_challenge_records_t* rec_bb) {
    rec_bb->title_color = rec_dc->title_color;
    rec_bb->unknown_u0 = rec_dc->unknown_u0;
    memcpy(rec_bb->times_ep1_online, rec_dc->times_ep1_online, sizeof(rec_bb->times_ep1_online));
    memset(rec_bb->times_ep2_online, 0, sizeof(rec_bb->times_ep2_online));
    memset(rec_bb->times_ep1_offline, 0, sizeof(rec_bb->times_ep1_offline));
    rec_bb->grave_unk4 = 0;
    //memset(rec_bb->grave_unk4, 0, sizeof(rec_bb->grave_unk4));
    rec_bb->grave_deaths = rec_dc->grave_deaths;
    rec_bb->unknown_u4 = 0;
    //memset(rec_bb->unknown_u4, 0, sizeof(rec_bb->unknown_u4));
    memcpy(rec_bb->grave_coords_time, rec_dc->grave_coords_time, sizeof(rec_bb->grave_coords_time));
    memcpy(rec_bb->grave_team, rec_dc->grave_team, sizeof(rec_bb->grave_team));
    memcpy(rec_bb->grave_message, rec_dc->grave_message, sizeof(rec_bb->grave_message));
    memset(rec_bb->unk3, 0, sizeof(rec_bb->unk3));
    memset(rec_bb->string, 0, sizeof(rec_bb->string));
    char* decrypted_rank_title = decrypt_challenge_rank_text(rec_dc->rank_title, sizeof(rec_dc->rank_title) / sizeof(char));
    memcpy(rec_bb->rank_title, encrypt_challenge_rank_text(decrypted_rank_title, strlen(decrypted_rank_title)), sizeof(rec_bb->rank_title));
    free(decrypted_rank_title);
    memset(rec_bb->battle, 0, sizeof(rec_bb->battle));
}

void convert_bb_to_dc_challenge(const bb_challenge_records_t* rec_bb, dc_challenge_records_t* rec_dc) {
    rec_dc->title_color = rec_bb->title_color;
    rec_dc->unknown_u0 = rec_bb->unknown_u0;
    char* decrypted_rank_title = decrypt_challenge_rank_text((char*)rec_bb->rank_title, sizeof(rec_bb->rank_title) / sizeof(uint16_t));
    memcpy(rec_dc->rank_title, encrypt_challenge_rank_text(decrypted_rank_title, strlen(decrypted_rank_title)), sizeof(rec_dc->rank_title));
    free(decrypted_rank_title);
    memcpy(rec_dc->times_ep1_online, rec_bb->times_ep1_online, sizeof(rec_dc->times_ep1_online));
    rec_dc->grave_unk4 = 0;
    rec_dc->grave_deaths = rec_bb->grave_deaths;
    memcpy(rec_dc->grave_coords_time, rec_bb->grave_coords_time, sizeof(rec_dc->grave_coords_time));
    memcpy(rec_dc->grave_team, rec_bb->grave_team, sizeof(rec_dc->grave_team));
    memcpy(rec_dc->grave_message, rec_bb->grave_message, sizeof(rec_dc->grave_message));
    memcpy(rec_dc->times_ep1_offline, rec_bb->times_ep1_offline, sizeof(rec_dc->times_ep1_offline));
    memset(rec_dc->battle, 0, sizeof(rec_dc->battle));
}
//
//void convert_PlayerRecordsBB_Challenge_from_DC(const dc_challenge_records_t* rec, bb_challenge_records_t* bb_rec) {
//    bb_rec->title_color = rec->title_color;
//    bb_rec->unknown_u0 = rec->unknown_u0;
//    bb_rec->times_ep1_online = rec->times_ep1_online;
//    bb_rec->times_ep2_online = 0;
//    bb_rec->times_ep1_offline = 0;
//    bb_rec->unknown_g3 = rec->unknown_g3;
//    bb_rec->grave_deaths = rec->grave_deaths;
//    bb_rec->unknown_u4 = 0;
//    bb_rec->grave_coords_time = rec->grave_coords_time;
//    bb_rec->grave_team = rec->grave_team;
//    bb_rec->grave_message = rec->grave_message;
//    bb_rec->unknown_m5 = 0;
//    bb_rec->unknown_t6 = 0;
//    encrypt_challenge_rank_text(decode_sjis(decrypt_challenge_rank_text(rec->rank_title)));
//    strncpy(bb_rec->rank_title, rec->rank_title, MAX_RANK_TITLE_LENGTH - 1);
//    bb_rec->rank_title[MAX_RANK_TITLE_LENGTH - 1] = '\0';
//    bb_rec->unknown_l7 = 0;
//}
//
//void convert_PlayerRecordsBB_Challenge_from_PC(const pc_challenge_records_t* rec, bb_challenge_records_t* bb_rec) {
//    bb_rec->title_color = rec->title_color;
//    bb_rec->unknown_u0 = rec->unknown_u0;
//    bb_rec->times_ep1_online = rec->times_ep1_online;
//    bb_rec->times_ep2_online = 0;
//    bb_rec->times_ep1_offline = 0;
//    bb_rec->unknown_g3 = rec->unknown_g3;
//    bb_rec->grave_deaths = rec->grave_deaths;
//    bb_rec->unknown_u4 = 0;
//    bb_rec->grave_coords_time = rec->grave_coords_time;
//    bb_rec->grave_team = rec->grave_team;
//    bb_rec->grave_message = rec->grave_message;
//    bb_rec->unknown_m5 = 0;
//    bb_rec->unknown_t6 = 0;
//    strcpy(bb_rec->rank_title, rec->rank_title);
//    bb_rec->unknown_l7 = 0;
//}
//
//void convert_PlayerRecordsBB_Challenge_from_V3(const PlayerRecordsV3_Challenge* rec, bb_challenge_records_t* bb_rec) {
//    bb_rec->title_color = rec->title_color;
//    bb_rec->unknown_u0 = rec->unknown_u0;
//    bb_rec->times_ep1_online = rec->times_ep1_online;
//    bb_rec->times_ep2_online = rec->times_ep2_online;
//    bb_rec->times_ep1_offline = rec->times_ep1_offline;
//    bb_rec->unknown_g3 = rec->unknown_g3;
//    bb_rec->grave_deaths = rec->grave_deaths;
//    bb_rec->unknown_u4 = rec->unknown_u4;
//    bb_rec->grave_coords_time = rec->grave_coords_time;
//    bb_rec->grave_team = rec->grave_team;
//    bb_rec->grave_message = rec->grave_message;
//    bb_rec->unknown_m5 = rec->unknown_m5;
//    bb_rec->unknown_t6 = rec->unknown_t6;
//    encrypt_challenge_rank_text(decode_sjis(decrypt_challenge_rank_text(rec->rank_title)));
//    strncpy(bb_rec->rank_title, rec->rank_title, MAX_RANK_TITLE_LENGTH - 1);
//    bb_rec->rank_title[MAX_RANK_TITLE_LENGTH - 1] = '\0';
//    bb_rec->unknown_l7 = rec->unknown_l7;
//}
//
//void convert_PlayerRecordsDC_Challenge_from_BB(const bb_challenge_records_t* bb_rec, dc_challenge_records_t* rec) {
//    rec->title_color = bb_rec->title_color;
//    rec->unknown_u0 = bb_rec->unknown_u0;
//    rec->times_ep1_online = bb_rec->times_ep1_online;
//    rec->unknown_g3 = 0;
//    rec->grave_deaths = bb_rec->grave_deaths;
//    rec->grave_coords_time = bb_rec->grave_coords_time;
//    rec->grave_team = bb_rec->grave_team;
//    rec->grave_message = bb_rec->grave_message;
//    rec->times_ep1_offline = bb_rec->times_ep1_offline;
//    memset(rec->unknown_l4, 0, sizeof(rec->unknown_l4));
//    encrypt_challenge_rank_text(encode_sjis(decrypt_challenge_rank_text(bb_rec->rank_title)));
//    strncpy(rec->rank_title, bb_rec->rank_title, MAX_RANK_TITLE_LENGTH - 1);
//    rec->rank_title[MAX_RANK_TITLE_LENGTH - 1] = '\0';
//}
//
//void convert_PlayerRecordsPC_Challenge_from_BB(const bb_challenge_records_t* bb_rec, pc_challenge_records_t* rec) {
//    rec->title_color = bb_rec->title_color;
//    rec->unknown_u0 = bb_rec->unknown_u0;
//    strncpy(rec->rank_title, bb_rec->rank_title, MAX_RANK_TITLE_LENGTH - 1);
//    rec->rank_title[MAX_RANK_TITLE_LENGTH - 1] = '\0';
//    rec->times_ep1_online = bb_rec->times_ep1_online;
//    rec->unknown_g3 = 0;
//    rec->grave_deaths = bb_rec->grave_deaths;
//    rec->grave_coords_time = bb_rec->grave_coords_time;
//    rec->grave_team = bb_rec->grave_team;
//    rec->grave_message = bb_rec->grave_message;
//    rec->times_ep1_offline = bb_rec->times_ep1_offline;
//    memset(rec->unknown_l4, 0, sizeof(rec->unknown_l4));
//}
//
//void convert_PlayerRecordsV3_Challenge_from_BB(const bb_challenge_records_t* bb_rec, PlayerRecordsV3_Challenge* rec) {
//    rec->title_color = bb_rec->title_color;
//    rec->unknown_u0 = bb_rec->unknown_u0;
//    rec->times_ep1_online = bb_rec->times_ep1_online;
//    rec->times_ep2_online = bb_rec->times_ep2_online;
//    rec->times_ep1_offline = bb_rec->times_ep1_offline;
//    rec->unknown_g3 = bb_rec->unknown_g3;
//    rec->grave_deaths = bb_rec->grave_deaths;
//    rec->unknown_u4 = bb_rec->unknown_u4;
//    rec->grave_coords_time = bb_rec->grave_coords_time;
//    rec->grave_team = bb_rec->grave_team;
//    rec->grave_message = bb_rec->grave_message;
//    rec->unknown_m5 = bb_rec->unknown_m5;
//    rec->unknown_t6 = bb_rec->unknown_t6;
//    encrypt_challenge_rank_text(encode_sjis(decrypt_challenge_rank_text(bb_rec->rank_title)));
//    strncpy(rec->rank_title, bb_rec->rank_title, MAX_RANK_TITLE_LENGTH - 1);
//    rec->rank_title[MAX_RANK_TITLE_LENGTH - 1] = '\0';
//}

void convert_dc_to_pc(dc_challenge_records_t* dc, pc_challenge_records_t* pc) {
    memset(pc, 0, sizeof(pc_challenge_records_t));
    pc->title_color = dc->title_color;
    memcpy(pc->rank_title, dc->rank_title, sizeof(dc->rank_title));
    memcpy(pc->times_ep1_online, dc->times_ep1_online, sizeof(dc->times_ep1_online));
    pc->grave_deaths = dc->grave_deaths;
    memcpy(pc->grave_coords_time, dc->grave_coords_time, sizeof(dc->grave_coords_time));
    memcpy(pc->grave_team, dc->grave_team, sizeof(dc->grave_team));
    memcpy(pc->grave_message, dc->grave_message, sizeof(dc->grave_message));
    memcpy(pc->times_ep1_offline, dc->times_ep1_offline, sizeof(dc->times_ep1_offline));
}

void convert_dc_to_v3(dc_challenge_records_t* dc, v3_challenge_records_t* v3) {
    memset(v3, 0, sizeof(v3_challenge_records_t));
    v3->title_color = dc->title_color;
    memcpy(v3->times_ep1_online, dc->times_ep1_online, sizeof(dc->times_ep1_online));
    memcpy(v3->times_ep1_offline, dc->times_ep1_offline, sizeof(dc->times_ep1_offline));
    v3->grave_deaths = dc->grave_deaths;
    memcpy(v3->grave_coords_time, dc->grave_coords_time, sizeof(dc->grave_coords_time));
    memcpy(v3->grave_team, dc->grave_team, sizeof(dc->grave_team));
    memcpy(v3->grave_message, dc->grave_message, sizeof(dc->grave_message));
    memcpy(v3->rank_title, dc->rank_title, sizeof(dc->rank_title));
}

void convert_dc_to_bb(dc_challenge_records_t* dc, bb_challenge_records_t* bb) {
    memset(bb, 0, sizeof(bb_challenge_records_t));
    bb->title_color = dc->title_color;
    memcpy(bb->times_ep1_online, dc->times_ep1_online, sizeof(dc->times_ep1_online));
    memcpy(bb->times_ep1_offline, dc->times_ep1_offline, sizeof(dc->times_ep1_offline));
    bb->grave_deaths = dc->grave_deaths;
    memcpy(bb->grave_coords_time, dc->grave_coords_time, sizeof(dc->grave_coords_time));
    memcpy(bb->grave_team, dc->grave_team, sizeof(dc->grave_team));
    memcpy(bb->grave_message, dc->grave_message, sizeof(dc->grave_message));
    memcpy(bb->rank_title, dc->rank_title, sizeof(dc->rank_title));
}

void convert_pc_to_dc(pc_challenge_records_t* pc, dc_challenge_records_t* dc) {
    memset(dc, 0, sizeof(dc_challenge_records_t));
    dc->title_color = pc->title_color;
    memcpy(dc->rank_title, pc->rank_title, sizeof(pc->rank_title));
    memcpy(dc->times_ep1_online, pc->times_ep1_online, sizeof(pc->times_ep1_online));
    dc->grave_deaths = pc->grave_deaths;
    memcpy(dc->grave_coords_time, pc->grave_coords_time, sizeof(pc->grave_coords_time));
    memcpy(dc->grave_team, pc->grave_team, sizeof(pc->grave_team));
    memcpy(dc->grave_message, pc->grave_message, sizeof(pc->grave_message));
    memcpy(dc->times_ep1_offline, pc->times_ep1_offline, sizeof(pc->times_ep1_offline));
}

void convert_pc_to_v3(pc_challenge_records_t* pc, v3_challenge_records_t* v3) {
    memset(v3, 0, sizeof(v3_challenge_records_t));
    v3->title_color = pc->title_color;
    memcpy(v3->times_ep1_online, pc->times_ep1_online, sizeof(pc->times_ep1_online));
    memcpy(v3->times_ep1_offline, pc->times_ep1_offline, sizeof(pc->times_ep1_offline));
    v3->grave_deaths = pc->grave_deaths;
    memcpy(v3->grave_coords_time, pc->grave_coords_time, sizeof(pc->grave_coords_time));
    memcpy(v3->grave_team, pc->grave_team, sizeof(pc->grave_team));
    memcpy(v3->grave_message, pc->grave_message, sizeof(pc->grave_message));
    memcpy(v3->rank_title, pc->rank_title, sizeof(pc->rank_title));
}

void convert_pc_to_bb(pc_challenge_records_t* pc, bb_challenge_records_t* bb) {
    memset(bb, 0, sizeof(bb_challenge_records_t));
    bb->title_color = pc->title_color;
    memcpy(bb->times_ep1_online, pc->times_ep1_online, sizeof(pc->times_ep1_online));
    memcpy(bb->times_ep1_offline, pc->times_ep1_offline, sizeof(pc->times_ep1_offline));
    bb->grave_deaths = pc->grave_deaths;
    memcpy(bb->grave_coords_time, pc->grave_coords_time, sizeof(pc->grave_coords_time));
    memcpy(bb->grave_team, pc->grave_team, sizeof(pc->grave_team));
    memcpy(bb->grave_message, pc->grave_message, sizeof(pc->grave_message));
    memcpy(bb->rank_title, pc->rank_title, sizeof(pc->rank_title));
}

void convert_v3_to_dc(v3_challenge_records_t* v3, dc_challenge_records_t* dc) {
    memset(dc, 0, sizeof(dc_challenge_records_t));
    dc->title_color = v3->title_color;
    memcpy(dc->rank_title, v3->rank_title, sizeof(v3->rank_title));
    memcpy(dc->times_ep1_online, v3->times_ep1_online, sizeof(v3->times_ep1_online));
    memcpy(dc->times_ep1_offline, v3->times_ep1_offline, sizeof(v3->times_ep1_offline));
    dc->grave_deaths = v3->grave_deaths;
    memcpy(dc->grave_coords_time, v3->grave_coords_time, sizeof(v3->grave_coords_time));
    memcpy(dc->grave_team, v3->grave_team, sizeof(v3->grave_team));
    memcpy(dc->grave_message, v3->grave_message, sizeof(v3->grave_message));
}

void convert_v3_to_pc(v3_challenge_records_t* v3, pc_challenge_records_t* pc) {
    memset(pc, 0, sizeof(pc_challenge_records_t));
    pc->title_color = v3->title_color;
    memcpy(pc->rank_title, v3->rank_title, sizeof(v3->rank_title));
    memcpy(pc->times_ep1_online, v3->times_ep1_online, sizeof(v3->times_ep1_online));
    memcpy(pc->times_ep1_offline, v3->times_ep1_offline, sizeof(v3->times_ep1_offline));
    pc->grave_unk4 = v3->unknown_g3[0];
    pc->grave_deaths = v3->grave_deaths;
    memcpy(pc->grave_coords_time, v3->grave_coords_time, sizeof(v3->grave_coords_time));
    memcpy(pc->grave_team, v3->grave_team, sizeof(v3->grave_team));
    memcpy(pc->grave_message, v3->grave_message, sizeof(v3->grave_message));
}

void convert_v3_to_bb(v3_challenge_records_t* v3, bb_challenge_records_t* bb) {
    memset(bb, 0, sizeof(bb_challenge_records_t));
    bb->title_color = v3->title_color;
    memcpy(bb->times_ep1_online, v3->times_ep1_online, sizeof(v3->times_ep1_online));
    memcpy(bb->times_ep2_online, v3->times_ep2_online, sizeof(v3->times_ep2_online));
    memcpy(bb->times_ep1_offline, v3->times_ep1_offline, sizeof(v3->times_ep1_offline));
    bb->grave_unk4 = v3->unknown_g3[0];
    bb->grave_deaths = v3->grave_deaths;
    memcpy(bb->grave_coords_time, v3->grave_coords_time, sizeof(v3->grave_coords_time));
    memcpy(bb->grave_team, v3->grave_team, sizeof(v3->grave_team));
    memcpy(bb->grave_message, v3->grave_message, sizeof(v3->grave_message));
    memcpy(bb->rank_title, v3->rank_title, sizeof(v3->rank_title));
}

void convert_bb_to_dc(bb_challenge_records_t* bb, dc_challenge_records_t* dc) {
    memset(dc, 0, sizeof(dc_challenge_records_t));
    dc->title_color = bb->title_color;
    memcpy(dc->rank_title, bb->rank_title, sizeof(bb->rank_title));
    memcpy(dc->times_ep1_online, bb->times_ep1_online, sizeof(bb->times_ep1_online));
    memcpy(dc->times_ep1_offline, bb->times_ep1_offline, sizeof(bb->times_ep1_offline));
    dc->grave_deaths = bb->grave_deaths;
    memcpy(dc->grave_coords_time, bb->grave_coords_time, sizeof(bb->grave_coords_time));
    memcpy(dc->grave_team, bb->grave_team, sizeof(bb->grave_team));
    memcpy(dc->grave_message, bb->grave_message, sizeof(bb->grave_message));
}

void convert_bb_to_pc(bb_challenge_records_t* bb, pc_challenge_records_t* pc) {
    memset(pc, 0, sizeof(pc_challenge_records_t));
    pc->title_color = bb->title_color;
    memcpy(pc->rank_title, bb->rank_title, sizeof(bb->rank_title));
    memcpy(pc->times_ep1_online, bb->times_ep1_online, sizeof(bb->times_ep1_online));
    memcpy(pc->times_ep1_offline, bb->times_ep1_offline, sizeof(bb->times_ep1_offline));
    pc->grave_unk4 = bb->grave_unk4;
    pc->grave_deaths = bb->grave_deaths;
    memcpy(pc->grave_coords_time, bb->grave_coords_time, sizeof(bb->grave_coords_time));
    memcpy(pc->grave_team, bb->grave_team, sizeof(bb->grave_team));
    memcpy(pc->grave_message, bb->grave_message, sizeof(bb->grave_message));
}

void convert_bb_to_v3(bb_challenge_records_t* bb, v3_challenge_records_t* v3) {
    memset(v3, 0, sizeof(v3_challenge_records_t));
    v3->title_color = bb->title_color;
    memcpy(v3->times_ep1_online, bb->times_ep1_online, sizeof(bb->times_ep1_online));
    memcpy(v3->times_ep2_online, bb->times_ep2_online, sizeof(bb->times_ep2_online));
    memcpy(v3->times_ep1_offline, bb->times_ep1_offline, sizeof(bb->times_ep1_offline));
    v3->grave_deaths = bb->grave_deaths;
    memcpy(v3->grave_coords_time, bb->grave_coords_time, sizeof(bb->grave_coords_time));
    memcpy(v3->grave_team, bb->grave_team, sizeof(bb->grave_team));
    memcpy(v3->grave_message, bb->grave_message, sizeof(bb->grave_message));
    memcpy(v3->rank_title, bb->rank_title, sizeof(bb->rank_title));
}