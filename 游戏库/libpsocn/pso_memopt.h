/*
    �λ�֮���й� �߳��ڴ����뺯��
    ��Ȩ (C) 2023 Sancaros

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

#ifndef PSO_MEMOPT_H
#define PSO_MEMOPT_H

#include <windows.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef struct block {
    struct block* next; // ��һ���ڴ���ָ��
    size_t size;        // ��ǰ�ڴ��Ĵ�С��������ͷ����
    bool is_free;        // ��־��ǰ�ڴ���Ƿ����
} block_t;

block_t* head = NULL, * tail = NULL;

pthread_mutex_t mem_lock; // ȫ���������ڱ������ڴ������Ĳ�������

block_t* get_free_block(size_t size);

void mfree(void* block);

void* mmalloc(size_t size);

void* mcalloc(size_t nitems, size_t size);

void* mrealloc(void* block, size_t size);

static inline void mem_mutex_init() {
    int result = 0;
    errno = 0;
    result |= pthread_mutex_init(&mem_lock, NULL);
    if (result != 0 || errno) {
        if (result) {
            fprintf(stderr, "pthread_mutex_init ��ʼ��ʧ��1��������: %d", result);
            fprintf(stderr, "������Ϣ1: %s", strerror(result));
        }

        // ����ֱ��ʹ�� errno ��ȡ������
        if (errno) {
            fprintf(stderr, "pthread_mutex_init ��ʼ��ʧ��2��������: %d", errno);
            fprintf(stderr, "������Ϣ2: %s", strerror(errno));
        }
    }
    else {
        // ��ʼ���ɹ�
        //DBG_LOG("log_mutex_init ��ʼ���ɹ���");
    }
}

static inline void mem_mutex_destory() {
    int result = 0;
    errno = 0;
    result |= pthread_mutex_destroy(&mem_lock);
    if (result != 0 || errno) {
        if (result) {
            fprintf(stderr, "pthread_mutex_destroy ����ʧ��1��������: %d", result);
            fprintf(stderr, "������Ϣ1: %s", strerror(result));
        }

        // ����ֱ��ʹ�� errno ��ȡ������
        if (errno) {
            fprintf(stderr, "pthread_mutex_destroy ����ʧ��2��������: %d", errno);
            fprintf(stderr, "������Ϣ2: %s", strerror(errno));
        }
    }
    else {
        // ���ٳɹ�
        //DBG_LOG("log_mutex_destory ���ٳɹ���");
    }
}
#endif // !PSO_MEMOPT_H