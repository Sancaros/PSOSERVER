/*
	�λ�֮���й� �ַ�����ȡ��
	��Ȩ (C) 2022, 2023 Sancaros

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

// StringReader�ṹ�嶨��
typedef struct {
	const char* data;
	size_t length;
	size_t offset;
} StringReader;

/* ��ʼ��һ����ȡ�� */
StringReader* StringReader_init();

// StringReader_destroy����ʵ��
void StringReader_destroy(StringReader* reader);

// StringReader_setData����ʵ��
void StringReader_setData(StringReader* reader, const char* data, size_t length, size_t offset);

/* �ַ�����ȡ�� �����ĸ��ڵ��� */
size_t StringReader_where(const StringReader* reader);

size_t StringReader_size(const StringReader* reader);

size_t StringReader_remaining(const StringReader* reader);

void StringReader_truncate(StringReader* reader, size_t new_size);

void StringReader_go(StringReader* reader, size_t offset);

void StringReader_skip(StringReader* reader, size_t bytes);

int StringReader_skip_if(StringReader* reader, const void* data, size_t size);

int StringReader_eof(const StringReader* reader);

const char* StringReader_peek(const StringReader* reader, size_t size);

// StringReader_read����ʵ��
char* StringReader_read(StringReader* reader, size_t size, int advance);

char* StringReader_all(StringReader* reader);

char* StringReader_sub(const StringReader* reader, size_t offset, size_t size);

char* StringReader_subx(const StringReader* reader, size_t offset);

char* StringReader_get_line(StringReader* reader, int advance);

char* StringReader_get_cstr(StringReader* reader, int advance);

#endif // !PSO_STRINGREADER
