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

typedef struct block block_t;

block_t* get_free_block(size_t size);

void mfree(void* block);

void* mmalloc(size_t size);

void* mcalloc(size_t nitems, size_t size);

void* mrealloc(void* block, size_t size);

#endif // !PSO_MEMOPT_H