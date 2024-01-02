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
#include <SFMT.h>

#include <pso_character.h>

#define MAX_RANK_TITLE_LENGTH 100

uint16_t encode_xrgb1555(uint32_t xrgb8888) {
    return ((xrgb8888 >> 9) & 0x7C00) | ((xrgb8888 >> 6) & 0x03E0) | ((xrgb8888 >> 3) & 0x001F);
}

uint32_t encrypt_challenge_time(sfmt_t* rng, uint16_t value) {
    uint8_t available_bits[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    uint16_t mask = 0;
    uint8_t num_one_bits = (sfmt_genrand_uint32(rng) % 9) + 4; // Range [4, 12]
    for (; num_one_bits; num_one_bits--) {
        uint8_t index = sfmt_genrand_uint32(rng) % (sizeof(available_bits) / sizeof(available_bits[0]));
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