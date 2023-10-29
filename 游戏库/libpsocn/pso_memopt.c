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

#include "pso_memopt.h"

/*这段代码实现了一个简单的内存分配器。通过链表管理已分配和空闲的内存块，
并使用互斥锁以保证对内存块链表的并发访问的安全性。
其中，主要的函数包括：

    mmalloc：分配指定大小的内存块。
    mcalloc：分配指定数量和大小的内存块，并将其初始化为零。
    mrealloc：重新分配已分配内存块的大小。
    mfree：释放已分配的内存块。
*/

block_t* get_free_block(size_t size)
{
    block_t* curr = head;
    while (curr)
    {
        if (curr->is_free && curr->size >= size)
        {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void mfree(void* block)
{
    if (!block)
        return;

    pthread_mutex_lock(&mem_lock);
    block_t* header = (block_t*)block - 1;
    if (tail == header)
    {
        size_t free_size = header->size - sizeof(block_t);
        VirtualFree(header, free_size, MEM_RELEASE); // 释放内存
        pthread_mutex_unlock(&mem_lock);
        return;
    }
    header->is_free = true; // 设置内存块为空闲状态
    pthread_mutex_unlock(&mem_lock);
    return;
}

void* mmalloc(size_t size)
{
    if (!size)
        return NULL;

    pthread_mutex_lock(&mem_lock);

    block_t* header = get_free_block(size);

    // 如果找到合适的空闲块
    if (header)
    {
        header->is_free = false; // 将内存块标记为已分配状态
        pthread_mutex_unlock(&mem_lock);
        return (void*)(++header); // 返回已分配内存的指针（隐藏头部）
    }

    size_t total_size = sizeof(block_t) + size; // 头部和数据块的总大小，需要为两者都分配内存

    block_t* new_block;
    if ((new_block = (block_t*)VirtualAlloc(NULL, total_size, MEM_COMMIT, PAGE_READWRITE)) == NULL)
    {
        fprintf(stderr, "VirtualAlloc 失败");
        pthread_mutex_unlock(&mem_lock);
        return NULL;
    }

    new_block->size = size;
    new_block->is_free = false;
    new_block->next = NULL;

    if (!head)
    {
        head = new_block;
    }
    if (tail)
    {
        tail->next = new_block;
    }
    tail = new_block;
    pthread_mutex_unlock(&mem_lock);
    // 返回数据块的指针，而不是头部的指针
    return (void*)(++new_block);
}

void* mcalloc(size_t nitems, size_t size)
{
    if (!nitems || !size)
        return NULL;
    size_t total_size = nitems * size;
    void* block = mmalloc(total_size);
    if (!block)
        return NULL;
    memset(block, 0, total_size);
    return block;
}

void* mrealloc(void* block, size_t size)
{
    if (!block || !size)
        return NULL;

    block_t* header = (block_t*)block - 1;

    if (header->size >= size)
    {
        return block;
    }

    void* new_block = mmalloc(size);

    if (new_block)
    {
        memcpy(new_block, block, size);
        mfree(block);
        return new_block;
    }
    return NULL; // 如果 mmalloc 失败
}
//
//int main()
//{
//    // 分配内存块
//    int* ptr1 = (int*)mmalloc(sizeof(int));
//    if (ptr1)
//    {
//        *ptr1 = 10;
//        printf("ptr1: %d\n", *ptr1);
//    }
//
//    // 分配并初始化一段内存
//    int* ptr2 = (int*)mcalloc(5, sizeof(int));
//    if (ptr2)
//    {
//        for (int i = 0; i < 5; i++)
//        {
//            printf("ptr2[%d]: %d\n", i, ptr2[i]);
//        }
//    }
//
//    // 重新分配内存大小
//    int* ptr3 = (int*)mrealloc(ptr2, 10 * sizeof(int));
//    if (ptr3)
//    {
//        for (int i = 0; i < 10; i++)
//        {
//            printf("ptr3[%d]: %d\n", i, ptr3[i]);
//        }
//    }
//
//    // 释放内存块
//    mfree(ptr1);
//    mfree(ptr3);
//
//    return 0;
//}