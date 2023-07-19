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
	pthread_mutex_t mutex;  // 添加互斥锁变量
} StringReader;

StringReader* StringReader_init();   // 初始化StringReader对象并返回指针
void StringReader_destroy(StringReader* reader);   // 销毁StringReader对象
void StringReader_setData(StringReader* reader, const char* data, size_t length, size_t offset);   // 设置StringReader对象的数据
size_t StringReader_where(const StringReader* reader);   // 获取当前读取位置的偏移量
size_t StringReader_size(const StringReader* reader);    // 获取字符串数据的总长度
size_t StringReader_remaining(const StringReader* reader);   // 获取剩余未读取的数据长度
void StringReader_truncate(StringReader* reader, size_t new_size);   // 截断字符串数据，将长度缩减为new_size
void StringReader_go(StringReader* reader, size_t offset);    // 跳转到指定的偏移量处
void StringReader_skip(StringReader* reader, size_t bytes);   // 跳过指定字节数
int StringReader_skip_if(StringReader* reader, const void* data, size_t size);   // 如果下一个字节序列与给定的数据相同，则跳过该字节序列
int StringReader_eof(const StringReader* reader);   // 判断是否已经到达结尾
const char* StringReader_peek(const StringReader* reader, size_t size);   // 预览下一个指定长度的字节序列
char* StringReader_read(StringReader* reader, size_t size, int advance);   // 读取指定长度的字节序列，并选择是否移动读取位置
char* StringReader_all(StringReader* reader);    // 读取剩余所有的字节序列
char* StringReader_sub(const StringReader* reader, size_t offset, size_t size);   // 从指定偏移量处开始读取指定长度的字节序列
char* StringReader_subx(const StringReader* reader, size_t offset);   // 从指定偏移量处开始读取剩余所有的字节序列
char* StringReader_get_line(StringReader* reader, int advance);   // 读取一行数据，以换行符\n为界定，选择是否移动读取位置
char* StringReader_get_cstr(StringReader* reader, int advance);   // 读取以空字符\0结尾的字符串，选择是否移动读取位置

#endif // !PSO_STRINGREADER
