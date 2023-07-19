/*
	梦幻之星中国 字符串读取器
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

#include "pso_StringReader.h"

#define PHOSG_WINDOWS

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
    if (reader != NULL) {
        reader->data = NULL;
        reader->length = 0;
        reader->offset = 0;
        pthread_mutex_init(&reader->mutex, NULL);  // 初始化互斥锁
    }
    return reader;
}

void StringReader_destroy(StringReader* reader) {
    pthread_mutex_destroy(&reader->mutex);  // 销毁互斥锁
    free(reader);
}

void StringReader_setData(StringReader* reader, const char* data, size_t length, size_t offset) {
    pthread_mutex_lock(&reader->mutex);  // 上锁
    reader->data = data;
    reader->length = length;
    reader->offset = offset;
    pthread_mutex_unlock(&reader->mutex);  // 解锁
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

char* StringReader_read(StringReader* reader, size_t size, int advance) {
    if (reader->offset + size <= reader->length) {
        char* result = (char*)malloc(size + 1);
        if (result != NULL) {
            memcpy(result, reader->data + reader->offset, size);
            result[size] = '\0';
            if (advance) {
                reader->offset += size;
            }
            return result;
        }
    }
    return NULL;
}

char* StringReader_all(StringReader* reader) {
    return StringReader_read(reader, reader->length - reader->offset, 1);
}

char* StringReader_sub(const StringReader* reader, size_t offset, size_t size) {
    if (reader->offset + offset + size <= reader->length) {
        char* result = (char*)malloc(size + 1);
        if (result != NULL) {
            memcpy(result, reader->data + reader->offset + offset, size);
            result[size] = '\0';
            return result;
        }
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
        if (result != NULL) {
            memcpy(result, start, size);
            result[size] = '\0';
            if (advance) {
                reader->offset += size + 1;
            }
            return result;
        }
    }
    return NULL;
}

char* StringReader_get_cstr(StringReader* reader, int advance) {
    const char* start = reader->data + reader->offset;
    const char* end = strchr(start, '\0');
    if (end != NULL) {
        size_t size = end - start;
        char* result = (char*)malloc(size + 1);
        if (result != NULL) {
            memcpy(result, start, size);
            result[size] = '\0';
            if (advance) {
                reader->offset += size + 1;
            }
            return result;
        }
    }
    return NULL;
}