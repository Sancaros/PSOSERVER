/*
    梦幻之星中国

    版权 (C) 2014, 2021 Sancaros

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

#ifndef PSOCN_MEMORY_H
#define PSOCN_MEMORY_H

#include <stddef.h>
#include "f_logs.h"

extern void* ref_alloc(size_t sz, void (*dtor)(void*));
extern void* ref_retain(void* r);
extern void* ref_release(void* r);
extern void* ref_free(void* r, int skip_dtor);

#define retain(x) ref_retain(x)
#define release(x) ref_release(x)

#endif /* !PSOCN_MEMORY_H */
