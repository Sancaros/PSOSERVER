/*
    梦幻之星中国 线程内存申请函数
    版权 (C) 2023 Sancaros

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
    struct block* next; // 下一个内存块的指针
    size_t size;        // 当前内存块的大小（不包括头部）
    bool is_free;        // 标志当前内存块是否空闲
} block_t;

block_t* head = NULL, * tail = NULL;

pthread_mutex_t mem_lock; // 全局锁，用于保护对内存块链表的并发访问

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
            fprintf(stderr, "pthread_mutex_init 初始化失败1，错误码: %d", result);
            fprintf(stderr, "错误消息1: %s", strerror(result));
        }

        // 或者直接使用 errno 获取错误码
        if (errno) {
            fprintf(stderr, "pthread_mutex_init 初始化失败2，错误码: %d", errno);
            fprintf(stderr, "错误消息2: %s", strerror(errno));
        }
    }
    else {
        // 初始化成功
        //DBG_LOG("log_mutex_init 初始化成功！");
    }
}

static inline void mem_mutex_destory() {
    int result = 0;
    errno = 0;
    result |= pthread_mutex_destroy(&mem_lock);
    if (result != 0 || errno) {
        if (result) {
            fprintf(stderr, "pthread_mutex_destroy 销毁失败1，错误码: %d", result);
            fprintf(stderr, "错误消息1: %s", strerror(result));
        }

        // 或者直接使用 errno 获取错误码
        if (errno) {
            fprintf(stderr, "pthread_mutex_destroy 销毁失败2，错误码: %d", errno);
            fprintf(stderr, "错误消息2: %s", strerror(errno));
        }
    }
    else {
        // 销毁成功
        //DBG_LOG("log_mutex_destory 销毁成功！");
    }
}
#endif // !PSO_MEMOPT_H