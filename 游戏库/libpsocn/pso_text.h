/*
    梦幻之星中国 文本字符串
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

#ifndef PSO_HAVE_TEXT
#define PSO_HAVE_TEXT

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "f_logs.h"

typedef void (*write_data_func)(const void*, size_t);

struct iovec {
    void* iov_base;
    size_t iov_len;
};

enum PrintDataFlags {
    OFFSET_8_BITS = 1 << 0,
    OFFSET_16_BITS = 1 << 1,
    OFFSET_32_BITS = 1 << 2,
    OFFSET_64_BITS = 1 << 3,
    USE_COLOR = 1 << 4,
    PRINT_ASCII = 1 << 5,
    PRINT_FLOAT = 1 << 6,
    PRINT_DOUBLE = 1 << 7,
    REVERSE_ENDIAN_FLOATS = 1 << 8,
    BIG_ENDIAN_FLOATS = 1 << 9,
    LITTLE_ENDIAN_FLOATS = 1 << 10,
    COLLAPSE_ZERO_LINES = 1 << 11,
    SKIP_SEPARATOR = 1 << 12
};

typedef struct {
    uint8_t* data;
    size_t len;
} Buffer;

// 去除字符串中的空格、制表符和换行符，并判断是否包含这些字符
void removeWhitespace(char* str);

// 去除宽字符字符串中的空格、制表符和换行符，并判断是否包含这些字符
void removeWhitespace_w(wchar_t* str);

int32_t ext24(uint32_t a);

int64_t ext48(uint64_t a);

uint8_t bswap8(uint8_t a);

uint16_t bswap16(uint16_t a);

uint32_t bswap24(uint32_t a);

int32_t bswap24s(int32_t a);

uint32_t bswap32(uint32_t a);

uint64_t bswap48(uint64_t a);

int64_t bswap48s(int64_t a);

uint64_t bswap64(uint64_t a);

float bswap32f(uint32_t a);

double bswap64f(uint64_t a);

uint32_t bswap32ff(float a);

uint64_t bswap64ff(double a);

/* 32位数字转8位数组 */
void u32_to_u8_array(uint32_t value, uint8_t* array, size_t size);
void u32_to_u8(uint32_t value, uint8_t parts[4], bool big_endian);

bool safe_memcpy(uint8_t* dst, const uint8_t* src, size_t len, const uint8_t* start, const uint8_t* end);
void safe_free(const char* func, uint32_t line, void** ptr);
#define SAFE_STRCAT(dest, src)                                      \
    do {                                                            \
        char* dest_ptr = dest + strlen(dest);                      \
        size_t dest_len = sizeof(dest) - strlen(dest) - 1;         \
        size_t src_len = strlen(src);                               \
                                                                      \
        if (dest_len >= src_len) {                                  \
            strncpy(dest_ptr, src, dest_len);                       \
            dest_ptr[dest_len] = '\0';                              \
        }                                                           \
        else {                                                      \
            ERR_LOG("错误发生在文件：%s，行号：%d\n", __FILE__, __LINE__);   \
            ERR_LOG("内存拼接错误：长度不足或有空指针\n");   \
            ERR_LOG("数据1：%s\n", dest);                            \
            ERR_LOG("数据1长度：%zu\n", strlen(dest));               \
            ERR_LOG("数据2：%s\n", src);                             \
            ERR_LOG("数据2长度：%zu\n", strlen(src));                \
        }                                                           \
    } while(0)

void safe_strcat(const char* func, uint32_t line, char* dest, const char* src, size_t dest_size);

/* 安全擦除函数 */
void SecureErase(void* buffer, size_t size);

/* 判断数据为空 */
int isPacketEmpty(const char* dataPacket, int packetLength);
/* 判断文本为空 */
int isEmptyString(const char* str);
int isEmptyInt(int num);
int isEmptyFloat(float val);

void write_data(write_data_func func, const void* data, size_t len);

Buffer* create_buffer(size_t len);
void destroy_buffer(Buffer* buffer);

char* string_vprintf(const char* fmt, va_list va);
wchar_t* wstring_vprintf(const wchar_t* fmt, va_list va);
char* string_printf(const char* fmt, ...);
wchar_t* wstring_printf(const wchar_t* fmt, ...);

uint8_t hexToByte(char* hs);

/* 单字节字符串增加颜色识别 */
int char_add_color_tag(char* a);
/* 宽字节字符串增加颜色识别 */
int wchar_add_color_tag(wchar_t* a);
int add_color_tag(void* str);

/* 实现字符的小写转换 */
char* tolower_c(const char* s);

void padToTenDigits(const char* str, char* padded_str);
#endif /* !PSO_HAVE_TEXT */
