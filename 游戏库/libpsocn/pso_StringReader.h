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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>
#include <pthread.h>

#include "f_logs.h"

typedef struct {
	const char* data;
	size_t length;
	size_t offset;
	pthread_mutex_t mutex;  // ��ӻ���������
} StringReader;

StringReader* StringReader_init();   // ��ʼ��StringReader���󲢷���ָ��
void StringReader_destroy(StringReader* reader);   // ����StringReader����
void StringReader_setData(StringReader* reader, const char* data, size_t length, size_t offset);   // ����StringReader���������
size_t StringReader_where(const StringReader* reader);   // ��ȡ��ǰ��ȡλ�õ�ƫ����
size_t StringReader_size(const StringReader* reader);    // ��ȡ�ַ������ݵ��ܳ���
size_t StringReader_remaining(const StringReader* reader);   // ��ȡʣ��δ��ȡ�����ݳ���
void StringReader_truncate(StringReader* reader, size_t new_size);   // �ض��ַ������ݣ�����������Ϊnew_size
void StringReader_go(StringReader* reader, size_t offset);    // ��ת��ָ����ƫ������
void StringReader_skip(StringReader* reader, size_t bytes);   // ����ָ���ֽ���
int StringReader_skip_if(StringReader* reader, const void* data, size_t size);   // �����һ���ֽ������������������ͬ�����������ֽ�����
int StringReader_eof(const StringReader* reader);   // �ж��Ƿ��Ѿ������β
const char* StringReader_peek(const StringReader* reader, size_t size);   // Ԥ����һ��ָ�����ȵ��ֽ�����
char* StringReader_read(StringReader* reader, size_t size, int advance);   // ��ȡָ�����ȵ��ֽ����У���ѡ���Ƿ��ƶ���ȡλ��
char* StringReader_all(StringReader* reader);    // ��ȡʣ�����е��ֽ�����
char* StringReader_sub(const StringReader* reader, size_t offset, size_t size);   // ��ָ��ƫ��������ʼ��ȡָ�����ȵ��ֽ�����
char* StringReader_subx(const StringReader* reader, size_t offset);   // ��ָ��ƫ��������ʼ��ȡʣ�����е��ֽ�����
char* StringReader_get_line(StringReader* reader, int advance);   // ��ȡһ�����ݣ��Ի��з�\nΪ�綨��ѡ���Ƿ��ƶ���ȡλ��
char* StringReader_get_cstr(StringReader* reader, int advance);   // ��ȡ�Կ��ַ�\0��β���ַ�����ѡ���Ƿ��ƶ���ȡλ��

#endif // !PSO_STRINGREADER
