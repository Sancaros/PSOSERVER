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

#include <windows.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "pso_memopt.h"

/*��δ���ʵ����һ���򵥵��ڴ��������ͨ����������ѷ���Ϳ��е��ڴ�飬
��ʹ�û������Ա�֤���ڴ������Ĳ������ʵİ�ȫ�ԡ�
���У���Ҫ�ĺ���������

    mmalloc������ָ����С���ڴ�顣
    mcalloc������ָ�������ʹ�С���ڴ�飬�������ʼ��Ϊ�㡣
    mrealloc�����·����ѷ����ڴ��Ĵ�С��
    mfree���ͷ��ѷ�����ڴ�顣
*/

struct block
{
    struct block* next; // ��һ���ڴ���ָ��
    size_t size;        // ��ǰ�ڴ��Ĵ�С��������ͷ����
    int is_free;        // ��־��ǰ�ڴ���Ƿ����
};

block_t* head = NULL, * tail = NULL;

pthread_mutex_t global_lock; // ȫ���������ڱ������ڴ������Ĳ�������

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

    pthread_mutex_lock(&global_lock);
    block_t* header = (block_t*)block - 1;
    if (tail == header)
    {
        size_t free_size = header->size - sizeof(block_t);
        VirtualFree(header, free_size, MEM_RELEASE); // �ͷ��ڴ�
        pthread_mutex_unlock(&global_lock);
        return;
    }
    header->is_free = 1; // �����ڴ��Ϊ����״̬
    pthread_mutex_unlock(&global_lock);
    return;
}

void* mmalloc(size_t size)
{
    if (!size)
        return NULL;

    pthread_mutex_lock(&global_lock);

    block_t* header = get_free_block(size);

    // ����ҵ����ʵĿ��п�
    if (header)
    {
        header->is_free = 0; // ���ڴ����Ϊ�ѷ���״̬
        pthread_mutex_unlock(&global_lock);
        return (void*)(++header); // �����ѷ����ڴ��ָ�루����ͷ����
    }

    size_t total_size = sizeof(block_t) + size; // ͷ�������ݿ���ܴ�С����ҪΪ���߶������ڴ�

    block_t* new_block;
    if ((new_block = (block_t*)VirtualAlloc(NULL, total_size, MEM_COMMIT, PAGE_READWRITE)) == NULL)
    {
        fprintf(stderr, "VirtualAlloc ʧ��");
        pthread_mutex_unlock(&global_lock);
        return NULL;
    }

    new_block->size = size;
    new_block->is_free = 0;
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
    pthread_mutex_unlock(&global_lock);
    // �������ݿ��ָ�룬������ͷ����ָ��
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
    return NULL; // ��� mmalloc ʧ��
}