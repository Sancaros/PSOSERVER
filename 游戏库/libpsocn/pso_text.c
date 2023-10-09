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

#include <stdbool.h>
#include <ctype.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <Windows.h>
#endif

#include "f_logs.h"

#include "pso_text.h"

int32_t ext24(uint32_t a) {
    return (a & 0x00800000) ? (a | 0xFF000000) : a;
}

int64_t ext48(uint64_t a) {
    return (a & 0x00008000000000) ? (a | 0xFFFF0000000000) : a;
}

uint8_t bswap8(uint8_t a) {
    return a;
}

uint16_t bswap16(uint16_t a) {
    return ((a >> 8) & 0x00FF) |
        ((a << 8) & 0xFF00);
}

uint32_t bswap24(uint32_t a) {
    return ((a >> 16) & 0x000000FF) |
        (a & 0x0000FF00) |
        ((a << 16) & 0x00FF0000);
}

int32_t bswap24s(int32_t a) {
    int32_t r = bswap24(a);
    if (r & 0x00800000) {
        return r | 0xFF000000;
    }
    return r;
}

uint32_t bswap32(uint32_t a) {
    return ((a >> 24) & 0x000000FF) |
        ((a >> 8) & 0x0000FF00) |
        ((a << 8) & 0x00FF0000) |
        ((a << 24) & 0xFF000000);
}

uint64_t bswap48(uint64_t a) {
    return ((a >> 40) & 0x00000000000000FF) |
        ((a >> 24) & 0x000000000000FF00) |
        ((a >> 8) & 0x0000000000FF0000) |
        ((a << 8) & 0x00000000FF000000) |
        ((a << 24) & 0x000000FF00000000) |
        ((a << 40) & 0x0000FF0000000000);
}

int64_t bswap48s(int64_t a) {
    int64_t r = bswap48(a);
    if (r & 0x0000800000000000) {
        return r | 0xFFFF000000000000;
    }
    return r;
}

uint64_t bswap64(uint64_t a) {
    return ((a >> 56) & 0x00000000000000FF) |
        ((a >> 40) & 0x000000000000FF00) |
        ((a >> 24) & 0x0000000000FF0000) |
        ((a >> 8) & 0x00000000FF000000) |
        ((a << 8) & 0x000000FF00000000) |
        ((a << 24) & 0x0000FF0000000000) |
        ((a << 40) & 0x00FF000000000000) |
        ((a << 56) & 0xFF00000000000000);
}

float bswap32f(uint32_t a) {
    float f;
    *(uint32_t*)(&f) = bswap32(a);
    return f;
}

double bswap64f(uint64_t a) {
    double d;
    *(uint64_t*)(&d) = bswap64(a);
    return d;
}

uint32_t bswap32ff(float a) {
    uint32_t i = *(uint32_t*)(&a);
    return bswap32(i);
}

uint64_t bswap64ff(double a) {
    uint64_t i = *(uint64_t*)(&a);
    return bswap64(i);
}

void u32_to_u8_array(uint32_t value, uint8_t* array, size_t size) {
    for (size_t i = 0; i < size; i++) {
        array[i] = value & 0xFF;
        value >>= 8;
    }
}

void u32_to_u8(uint32_t value, uint8_t parts[4], bool big_endian) {
    if (big_endian) {
        parts[0] = (uint8_t)((value >> 24) & 0xFF);
        parts[1] = (uint8_t)((value >> 16) & 0xFF);
        parts[2] = (uint8_t)((value >> 8) & 0xFF);
        parts[3] = (uint8_t)(value & 0xFF);
    }
    else {
        parts[0] = (uint8_t)(value & 0xFF);
        parts[1] = (uint8_t)((value >> 8) & 0xFF);
        parts[2] = (uint8_t)((value >> 16) & 0xFF);
        parts[3] = (uint8_t)((value >> 24) & 0xFF);
    }
}

bool between(const uint8_t* start, const uint8_t* end, const uint8_t* p) {
    return (p < end) && (p >= start);
}

bool safe_check(const uint8_t* dst, size_t len, const uint8_t* start, const uint8_t* end) {
    if (len < 1) {
        return false;
    }
    if (!dst || !start || !end) {
        return false;
    }
    const uint8_t* last_pos = dst + len - 1;
    if (last_pos < dst) {
        return false;
    }
    return between(start, end, dst) && between(start, end, last_pos);
}

bool safe_memcpy(uint8_t* dst, const uint8_t* src, size_t len, const uint8_t* start, const uint8_t* end) {
    if (!safe_check(dst, len, start, end)) {
        return false;
    }
    if (!src) {
        return false;
    }
    memcpy(dst, src, len);
    return true;
}

void safe_free(const char* func, uint32_t line, void** ptr) {
    if (ptr != NULL && *ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
    else
        ERR_LOG("(%s[%04d])内存释放错误, 本身已为悬空指针", func, line);
}

#ifndef  _WIN32
void SecureErase(void* buffer, size_t size) {
    volatile char* p = (volatile char*)buffer;  // 使用 volatile 修饰符确保内存读写不被优化
    while (size--) {
        *p++ = 0;
    }
}
#else
void SecureErase(void* buffer, size_t size) {
    typedef VOID(WINAPI* RtlSecureZeroMemory_t)(PVOID, SIZE_T);

    // 获取函数地址
    RtlSecureZeroMemory_t pRtlSecureZeroMemory =
        (RtlSecureZeroMemory_t)GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlSecureZeroMemory");

    if (pRtlSecureZeroMemory) {
        // 调用 RtlSecureZeroMemory 函数
        pRtlSecureZeroMemory(buffer, size);
    }
    else {
        // 如果无法获取函数地址，则使用 memset 函数清零
        memset(buffer, 0, size);
    }
}
#endif // ! _WIN32

int isPacketEmpty(const char* dataPacket, int packetLength) {
    for (int i = 0; i < packetLength; i++) {
        if (dataPacket[i] != 0) {
            return 0; // 数据包非空
        }
    }

    return 1; // 数据包为空
}

int isEmptyString(const char* str) {
    return (str == NULL || strlen(str) == 0);
}

int isEmptyInt(int num) {
    return (num == 0);
}

int isEmptyFloat(float val) {
    return (val == 0.0);
}

void write_data(write_data_func func, const void* data, size_t len) {
    func(data, len);
}

Buffer* create_buffer(size_t len) {
    Buffer* buffer = (Buffer*)malloc(sizeof(Buffer));
    if (buffer == NULL) {
        return NULL;
    }
    buffer->data = (uint8_t*)malloc(len);
    if (buffer->data == NULL) {
        free_safe(buffer);
        return NULL;
    }
    buffer->len = len;
    return buffer;
}

void destroy_buffer(Buffer* buffer) {
    if (buffer != NULL) {
        free_safe(buffer->data);
        free_safe(buffer);
    }
}

char* string_vprintf(const char* fmt, va_list va) {
    char* result = NULL;
    int size = _vscprintf(fmt, va);
    result = (char*)malloc((size + 1) * sizeof(char));
    vsprintf_s(result, size + 1, fmt, va);
    return result;
}

wchar_t* wstring_vprintf(const wchar_t* fmt, va_list va) {
    size_t size = _vscwprintf(fmt, va);
    wchar_t* result = (wchar_t*)malloc((size + 1) * sizeof(wchar_t));
    vswprintf_s(result, size + 1, fmt, va);
    return result;
}

char* string_printf(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    char* ret = string_vprintf(fmt, va);
    va_end(va);
    return ret;
}

wchar_t* wstring_printf(const wchar_t* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    wchar_t* ret = wstring_vprintf(fmt, va);
    va_end(va);
    return ret;
}

uint8_t hexToByte(char* hs)
{
    uint32_t b;

    if (hs[0] < 58) b = (hs[0] - 48); else b = (hs[0] - 55);
    b *= 16;
    if (hs[1] < 58) b += (hs[1] - 48); else b += (hs[1] - 55);
    return (uint8_t)b;
}

// Adds a $E or $J (or $whatever) before a string.
bool char_add_language_marker(char* a, char e) {
    if (a[0] != '\t') memmove(&a[2], a, strlen(a) + 1);
    else if (a[1] == 'C') memmove(&a[2], a, strlen(a) + 1);
    a[0] = '\t';
    a[1] = e;
    return true;
}

bool wchar_add_language_marker(wchar_t* a, wchar_t e) {
    if (a[0] != L'\t') memmove(&a[2], a, 2 * (wcslen(a) + 1));
    else if (a[1] == L'C') memmove(&a[2], a, 2 * (wcslen(a) + 1));
    a[0] = L'\t';
    a[1] = e;
    return true;
}

// Removes the $E, $J, etc. from the beginning of a string.
bool char_remove_language_marker(char* a) {
    if ((a[0] == '\t') && (a[1] != 'C')) strcpy_s(a, sizeof(a), &a[2]);
    return true;
}

bool wchar_remove_language_marker(wchar_t* a) {
    if ((a[0] == L'\t') && (a[1] != L'C')) wcscpy_s(a, sizeof(a), &a[2]);
    return true;
}

// Replaces all instances of a character in a string.
int char_replace_char(char* a, char f, char r) {
    while (*a) {
        if (*a == f) *a = r;
        a++;
    }
    return 0;
}

int wchar_replace_char(wchar_t* a, wchar_t f, wchar_t r) {
    while (*a) {
        if (*a == f) *a = r;
        a++;
    }
    return 0;
}

char reps[] = { 'd', 'x', 'p', '+', '1', '2', '3', 'c', 'l', 'y', 'X', 'Y', 'Z', '?', 'C', 'R', 's', '%', 'n', 0 };
char repd[] = { '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '$', '%', '#', 0 };
wchar_t wreps[] = { L'd', L'x', L'p', L'+', L'1', L'2', L'3', L'c', L'l', L'y', L'X', L'Y', L'Z', L'?', L'C', L'R', L's', L'%', L'n', 0 };
wchar_t wrepd[] = { L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'?', L'$', L'%', L'#', 0 };

int detectCharacterType(char* str) {
    int result = 0; // 用于存储结果，0表示单字符，1表示宽字符

    // 检查字符串是否包含非ASCII字符
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if ((unsigned char)str[i] >= 128) {
            result = 1; // 包含非ASCII字符，字符串是宽字符
            break;
        }
    }

    return result;
}

/* 单字节字符串增加颜色识别 */
int char_add_color_tag(char* a) {
    long x;
    while (*a) {
        if (*a == '$') *a = '\t';
        if (*a == '#') *a = '\n';
        if (*a == '%') {
            for (x = 0; reps[x]; x++) if (reps[x] == a[1]) break;
            if (reps[x]) {
                strcpy_s(a, sizeof(a), &a[1]);
                *a = repd[x];
            }
        }
        a++;
    }
    return 0;
}

/* 宽字节字符串增加颜色识别 */
int wchar_add_color_tag(wchar_t* a) {
    unsigned long x;
    while (*a) {
        if (*a == L'$') *a = L'\t';
        if (*a == L'#') *a = L'\n';
        if (*a == '%') {
            for (x = 0; wreps[x]; x++) if (wreps[x] == a[1]) break;
            if (wreps[x]) {
                errno_t result = wcscpy_s(a, sizeof(a), &a[1]);
                if (result == 0) {
                    *a = wrepd[x];
                }
                else {
                    ERR_LOG("发生错误，错误码: %d", result);
                    result = 0;
                }
            }
        }
        a++;
    }
    return 0;
}

int add_color_tag(void* str) {
    if (detectCharacterType(str))
        return wchar_add_color_tag(str);
    else
        return char_add_color_tag(str);
}

// Checksums an amount of data.
long checksum(void* data, int size) {
    long cs = 0xA486F731;
    int shift = 0;
    while (size) {
        if (size == 1) cs ^= *(unsigned char*)data;
        else {
            shift = *(unsigned char*)((unsigned long)data + 1) % 25;
            cs ^= *(unsigned char*)data << shift;
        }
        data = (void*)((long)data + 1);
        size--;
    }
    return cs;
}

// Reads formatted data from a string. For example, the string
// "hi there" 0A 12345678 #30 'pizza'
// would become:
// 68 69 20 74 68 65 72 65 0A 12 34 56 78 1E 00 00 00 70 00 69 00 7A 00 7A 00 61 00
// to better understand how to use it, see the code below.
int tx_read_string_data(char* buffer, unsigned long length, void* data, unsigned long max) {
    if (!length) length = strlen(buffer);
    unsigned long x, size = 0;
    int chr = 0;
    bool read, string = false, unicodestring = false, high = true;
    unsigned char* cmdbuffer = (unsigned char*)data;
    for (x = 0; (x < length) && (size < max); x++) {
        read = false;
        // strings in "" are copied exactly into the data
        if (string) {
            if (buffer[x] == '\"') string = false;
            else {
                cmdbuffer[size] = buffer[x];
                size++;
            }
            // strings in '' are copied into the data as Unicode
        }
        else if (unicodestring) {
            if (buffer[x] == '\'') unicodestring = false;
            else {
                cmdbuffer[size] = buffer[x];
                cmdbuffer[size + 1] = 0;
                size += 2;
            }
            // #<decimal> is inserted as a DWORD
        }
        else if (buffer[x] == '#') {
            sscanf_s(&buffer[x + 1], "%ld", (unsigned long*)&cmdbuffer[size]);
            size += 4;
            // any hex data is parsed and added to the data
        }
        else {
            if ((buffer[x] >= '0') && (buffer[x] <= '9'))
            {
                read = true;
                chr |= (buffer[x] - '0');
            }
            if ((buffer[x] >= 'A') && (buffer[x] <= 'F'))
            {
                read = true;
                chr |= (buffer[x] - 'A' + 0x0A);
            }
            if ((buffer[x] >= 'a') && (buffer[x] <= 'f'))
            {
                read = true;
                chr |= (buffer[x] - 'a' + 0x0A);
            }
            if (buffer[x] == '\"') string = true;
            if (buffer[x] == '\'') unicodestring = true;
        }
        if (read) {
            if (high) chr = chr << 4;
            else {
                cmdbuffer[size] = (char)chr;
                chr = 0;
                size++;
            }
            high = !high;
        }
    }
    return size;
}

/* 实现字符的小写转换 */
char* tolower_c(const char* s) {
    size_t length = strlen(s);
    char* ret = (char*)malloc((length + 1) * sizeof(char)); // 分配足够的空间来存储转换后的字符串（包括结尾的空字符）
    if (ret == NULL) {
        return NULL; // 内存分配失败，返回空指针
    }

    for (size_t i = 0; i < length; i++) {
        ret[i] = (char)tolower(s[i]); // 将每个字符转换为小写形式
    }
    ret[length] = '\0'; // 在字符串末尾添加空字符

    return ret;
}

void padToTenDigits(const char* str, char* padded_str) {
    int str_len = strlen(str);

    if (str_len < 10) {
        int padding_count = 10 - str_len;
        memset(padded_str, '0', padding_count);
        padded_str[padding_count] = '\0';
        strncpy_s(padded_str, sizeof(char) * 11, str, str_len);
    }
    else {
        strncpy_s(padded_str, sizeof(char) * 11, str, 10);
        padded_str[10] = '\0';
    }
}
