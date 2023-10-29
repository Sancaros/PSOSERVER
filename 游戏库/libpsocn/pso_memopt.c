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

#include "pso_memopt.h"

/*��δ���ʵ����һ���򵥵��ڴ��������ͨ����������ѷ���Ϳ��е��ڴ�飬
��ʹ�û������Ա�֤���ڴ������Ĳ������ʵİ�ȫ�ԡ�
���У���Ҫ�ĺ���������

    mmalloc������ָ����С���ڴ�顣
    mcalloc������ָ�������ʹ�С���ڴ�飬�������ʼ��Ϊ�㡣
    mrealloc�����·����ѷ����ڴ��Ĵ�С��
    mfree���ͷ��ѷ�����ڴ�顣
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
        VirtualFree(header, free_size, MEM_RELEASE); // �ͷ��ڴ�
        pthread_mutex_unlock(&mem_lock);
        return;
    }
    header->is_free = true; // �����ڴ��Ϊ����״̬
    pthread_mutex_unlock(&mem_lock);
    return;
}

void* mmalloc(size_t size)
{
    if (!size)
        return NULL;

    pthread_mutex_lock(&mem_lock);

    block_t* header = get_free_block(size);

    // ����ҵ����ʵĿ��п�
    if (header)
    {
        header->is_free = false; // ���ڴ����Ϊ�ѷ���״̬
        pthread_mutex_unlock(&mem_lock);
        return (void*)(++header); // �����ѷ����ڴ��ָ�루����ͷ����
    }

    size_t total_size = sizeof(block_t) + size; // ͷ�������ݿ���ܴ�С����ҪΪ���߶������ڴ�

    block_t* new_block;
    if ((new_block = (block_t*)VirtualAlloc(NULL, total_size, MEM_COMMIT, PAGE_READWRITE)) == NULL)
    {
        fprintf(stderr, "VirtualAlloc ʧ��");
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
//
//int main()
//{
//    // �����ڴ��
//    int* ptr1 = (int*)mmalloc(sizeof(int));
//    if (ptr1)
//    {
//        *ptr1 = 10;
//        printf("ptr1: %d\n", *ptr1);
//    }
//
//    // ���䲢��ʼ��һ���ڴ�
//    int* ptr2 = (int*)mcalloc(5, sizeof(int));
//    if (ptr2)
//    {
//        for (int i = 0; i < 5; i++)
//        {
//            printf("ptr2[%d]: %d\n", i, ptr2[i]);
//        }
//    }
//
//    // ���·����ڴ��С
//    int* ptr3 = (int*)mrealloc(ptr2, 10 * sizeof(int));
//    if (ptr3)
//    {
//        for (int i = 0; i < 10; i++)
//        {
//            printf("ptr3[%d]: %d\n", i, ptr3[i]);
//        }
//    }
//
//    // �ͷ��ڴ��
//    mfree(ptr1);
//    mfree(ptr3);
//
//    return 0;
//}