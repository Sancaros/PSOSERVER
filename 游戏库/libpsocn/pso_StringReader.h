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

#ifndef PSO_STRINGREADER
#define PSO_STRINGREADER

// StringReader结构体定义
typedef struct {
	const char* data;
	size_t length;
	size_t offset;
} StringReader;

StringReader* StringReader_init();

void StringReader_destroy(StringReader* reader);

void StringReader_setData(StringReader* reader, const char* data, size_t length, size_t offset);

char* StringReader_read(StringReader* reader, size_t size, int advance);

#endif // !PSO_STRINGREADER
