/*
	梦幻之星中国 文本读取
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
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>

#include "f_logs.h"
#include "pso_StringReader.h"

#define PHOSG_WINDOWS

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

#define KB_SIZE 1024ULL
#define MB_SIZE (KB_SIZE * 1024ULL)
#define GB_SIZE (MB_SIZE * 1024ULL)
#define TB_SIZE (GB_SIZE * 1024ULL)
#define PB_SIZE (TB_SIZE * 1024ULL)
#define EB_SIZE (PB_SIZE * 1024ULL)
#define ZB_SIZE (EB_SIZE * 1024ULL)
#define YB_SIZE (ZB_SIZE * 1024ULL)
#define HB_SIZE (YB_SIZE * 1024ULL)

#if (SIZE_T_BITS == 8)

char* format_size(size_t size, bool include_bytes) {
    return string_printf("%zu bytes", size);
}

#elif (SIZE_T_BITS == 16)

char* format_size(size_t size, bool include_bytes) {
    if (size < KB_SIZE) {
        return string_printf("%zu bytes", size);
    }
    if (include_bytes) {
        return string_printf("%zu bytes (%.02f KB)", size, (float)size / KB_SIZE);
    }
    else {
        return string_printf("%.02f KB", (float)size / KB_SIZE);
    }
}

#elif (SIZE_T_BITS == 32)

char* format_size(size_t size, bool include_bytes) {
    if (size < KB_SIZE) {
        return string_printf("%zu bytes", size);
    }
    if (include_bytes) {
        if (size < MB_SIZE) {
            return string_printf("%zu bytes (%.02f KB)", size, (float)size / KB_SIZE);
        }
        if (size < GB_SIZE) {
            return string_printf("%zu bytes (%.02f MB)", size, (float)size / MB_SIZE);
        }
        return string_printf("%zu bytes (%.02f GB)", size, (float)size / GB_SIZE);
    }
    else {
        if (size < MB_SIZE) {
            return string_printf("%.02f KB", (float)size / KB_SIZE);
        }
        if (size < GB_SIZE) {
            return string_printf("%.02f MB", (float)size / MB_SIZE);
        }
        return string_printf("%.02f GB", (float)size / GB_SIZE);
    }
}

#elif (SIZE_T_BITS == 64)

char* format_size(size_t size, bool include_bytes) {
    if (size < KB_SIZE) {
        return string_printf("%zu bytes", size);
    }
    if (include_bytes) {
        if (size < MB_SIZE) {
            return string_printf("%zu bytes (%.02f KB)", size, (float)size / KB_SIZE);
        }
        if (size < GB_SIZE) {
            return string_printf("%zu bytes (%.02f MB)", size, (float)size / MB_SIZE);
        }
        if (size < TB_SIZE) {
            return string_printf("%zu bytes (%.02f GB)", size, (float)size / GB_SIZE);
        }
        if (size < PB_SIZE) {
            return string_printf("%zu bytes (%.02f TB)", size, (float)size / TB_SIZE);
        }
        if (size < EB_SIZE) {
            return string_printf("%zu bytes (%.02f PB)", size, (float)size / PB_SIZE);
        }
        return string_printf("%zu bytes (%.02f EB)", size, (float)size / EB_SIZE);
    }
    else {
        if (size < MB_SIZE) {
            return string_printf("%.02f KB", (float)size / KB_SIZE);
        }
        if (size < GB_SIZE) {
            return string_printf("%.02f MB", (float)size / MB_SIZE);
        }
        if (size < TB_SIZE) {
            return string_printf("%.02f GB", (float)size / GB_SIZE);
        }
        if (size < PB_SIZE) {
            return string_printf("%.02f TB", (float)size / TB_SIZE);
        }
        if (size < EB_SIZE) {
            return string_printf("%.02f PB", (float)size / PB_SIZE);
        }
        return string_printf("%.02f EB", (float)size / EB_SIZE);
    }
}

#endif

size_t parse_size(const char* str) {
    // input is like [0-9](\.[0-9]+)? *[KkMmGgTtPpEe]?[Bb]?
    // fortunately this can just be parsed left-to-right
    double fractional_part = 0.0;
    size_t integer_part = 0;
    size_t unit_scale = 1;

    for (; isdigit(*str); str++) {
        integer_part = integer_part * 10 + (*str - '0');
    }

    if (*str == '.') {
        str++;
        double factor = 0.1;
        for (; isdigit(*str); str++) {
            fractional_part += factor * (*str - '0');
            factor *= 0.1;
        }
    }

    for (; *str == ' '; str++) {
    }

#if SIZE_T_BITS >= 16
    if (*str == 'K' || *str == 'k') {
        unit_scale = 1024;
#elif SIZE_T_BITS >= 32
    }
 else if (*str == 'M' || *str == 'm') {
     unit_scale = 1024 * 1024;
  }
 else if (*str == 'G' || *str == 'g') {
     unit_scale = 1024 * 1024 * 1024;
#elif SIZE_T_BITS == 64
  }
 else if (*str == 'T' || *str == 't') {
     unit_scale = 1024 * 1024 * 1024 * 1024;
  }
 else if (*str == 'P' || *str == 'p') {
     unit_scale = 1024 * 1024 * 1024 * 1024 * 1024;
  }
 else if (*str == 'E' || *str == 'e') {
     unit_scale = 1024 * 1024 * 1024 * 1024 * 1024 * 1024;
#endif
  }

  return integer_part * unit_scale + (size_t)(fractional_part * unit_scale);
}

//这个案例演示了如何使用StringReader类来从字符串中读取数据。
//首先，我们创建了一个StringReader对象，并将要读取的字符串数据、长度和偏移量设置到对象中。
//然后，通过调用StringReader_read方法，我们可以指定要读取的字符数，并选择是否前进到下一个位置。
//在本例中，我们先读取了前5个字符，然后再读取剩余的所有字符。
//最后，我们销毁StringReader对象并释放之前分配的内存。
StringReader* StringReader_init() {
    StringReader* reader = (StringReader*)malloc(sizeof(StringReader));
    reader->data = NULL;
    reader->length = 0;
    reader->offset = 0;
    return reader;
}

// StringReader_destroy方法实现
void StringReader_destroy(StringReader* reader) {
    free_safe(reader);
}

// StringReader_setData方法实现
void StringReader_setData(StringReader* reader, const char* data, size_t length, size_t offset) {
    reader->data = data;
    reader->length = length;
    reader->offset = offset;
}

size_t StringReader_where(const StringReader* reader) {
    return reader->offset;
}

size_t StringReader_size(const StringReader* reader) {
    return reader->length;
}

size_t StringReader_remaining(const StringReader* reader) {
    return reader->length - reader->offset;
}

void StringReader_truncate(StringReader* reader, size_t new_size) {
    if (new_size < reader->length) {
        reader->length = new_size;
        if (reader->offset > new_size) {
            reader->offset = new_size;
        }
    }
}

void StringReader_go(StringReader* reader, size_t offset) {
    if (offset <= reader->length) {
        reader->offset = offset;
    }
}

void StringReader_skip(StringReader* reader, size_t bytes) {
    if (reader->offset + bytes <= reader->length) {
        reader->offset += bytes;
    }
}

int StringReader_skip_if(StringReader* reader, const void* data, size_t size) {
    if (reader->offset + size <= reader->length &&
        memcmp(reader->data + reader->offset, data, size) == 0) {
        reader->offset += size;
        return 1;
    }
    return 0;
}

int StringReader_eof(const StringReader* reader) {
    return reader->offset >= reader->length;
}

const char* StringReader_peek(const StringReader* reader, size_t size) {
    if (reader->offset + size <= reader->length) {
        return reader->data + reader->offset;
    }
    return NULL;
}

// StringReader_read方法实现
char* StringReader_read(StringReader* reader, size_t size, int advance) {
    if (reader->offset + size <= reader->length) {
        char* result = (char*)malloc(size + 1);
        memcpy(result, reader->data + reader->offset, size);
        result[size] = '\0';
        if (advance) {
            reader->offset += size;
        }
        return result;
    }
    return NULL;
}

char* StringReader_all(const StringReader* reader) {
    return StringReader_read(reader, reader->length - reader->offset, 1);
}

char* StringReader_sub(const StringReader* reader, size_t offset, size_t size) {
    if (reader->offset + offset + size <= reader->length) {
        char* result = (char*)malloc(size + 1);
        memcpy(result, reader->data + reader->offset + offset, size);
        result[size] = '\0';
        return result;
    }
    return NULL;
}

char* StringReader_subx(const StringReader* reader, size_t offset) {
    return StringReader_sub(reader, offset, reader->length - reader->offset - offset);
}

char* StringReader_get_line(StringReader* reader, int advance) {
    const char* start = reader->data + reader->offset;
    const char* end = strchr(start, '\n');
    if (end != NULL) {
        size_t size = end - start;
        char* result = (char*)malloc(size + 1);
        memcpy(result, start, size);
        result[size] = '\0';
        if (advance) {
            reader->offset += size + 1;
        }
        return result;
    }
    return NULL;
}

char* StringReader_get_cstr(StringReader* reader, int advance) {
    const char* start = reader->data + reader->offset;
    const char* end = strchr(start, '\0');
    if (end != NULL) {
        size_t size = end - start;
        char* result = (char*)malloc(size + 1);
        memcpy(result, start, size);
        result[size] = '\0';
        if (advance) {
            reader->offset += size + 1;
        }
        return result;
    }
    return NULL;
}
//
//int main() {
//    const char* data = "Hello, World!";
//    size_t length = strlen(data);
//
//    // 创建StringReader对象
//    StringReader* reader = (StringReader*)malloc(sizeof(StringReader));
//    StringReader_setData(reader, data, length, 0);
//
//    // 读取前5个字符
//    char* str1 = StringReader_read(reader, 5, 1);
//    printf("Read: %s\n", str1);  // 输出: Read: Hello
//
//    // 读取剩余的所有字符
//    char* str2 = StringReader_read(reader, StringReader_remaining(reader), 1);
//    printf("Read: %s\n", str2);  // 输出: Read: , World!
//
//    // 销毁StringReader对象
//    StringReader_destroy(reader);
//    free(str1);
//    free(str2);
//
//    return 0;
//}
//Read: Hello
//Read : , World!