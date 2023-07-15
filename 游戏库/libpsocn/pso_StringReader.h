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

#ifndef PSO_STRINGREADER
#define PSO_STRINGREADER

// StringReader结构体定义
typedef struct {
	const char* data;
	size_t length;
	size_t offset;
} StringReader;

/* 初始化一个读取器 */
StringReader* StringReader_init();

// StringReader_destroy方法实现
void StringReader_destroy(StringReader* reader);

// StringReader_setData方法实现
void StringReader_setData(StringReader* reader, const char* data, size_t length, size_t offset);

/* 字符串读取器 读到哪个节点了 */
size_t StringReader_where(const StringReader* reader);

size_t StringReader_size(const StringReader* reader);

size_t StringReader_remaining(const StringReader* reader);

void StringReader_truncate(StringReader* reader, size_t new_size);

void StringReader_go(StringReader* reader, size_t offset);

void StringReader_skip(StringReader* reader, size_t bytes);

int StringReader_skip_if(StringReader* reader, const void* data, size_t size);

int StringReader_eof(const StringReader* reader);

const char* StringReader_peek(const StringReader* reader, size_t size);

// StringReader_read方法实现
char* StringReader_read(StringReader* reader, size_t size, int advance);

char* StringReader_all(StringReader* reader);

char* StringReader_sub(const StringReader* reader, size_t offset, size_t size);

char* StringReader_subx(const StringReader* reader, size_t offset);

char* StringReader_get_line(StringReader* reader, int advance);

char* StringReader_get_cstr(StringReader* reader, int advance);

#endif // !PSO_STRINGREADER
