/*
    This file is part of libpsoarchive.

    ∞Ê»® (C) 2015, 2016 Sancaros

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 2.1 or
    version 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PSOARCHIVE__AFS_H
#define PSOARCHIVE__AFS_H

#include "psoarchive-error.h"

#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Opaque AFS structures. */
struct pso_afs_read;
typedef struct pso_afs_read pso_afs_read_t;

struct pso_afs_write;
typedef struct pso_afs_write pso_afs_write_t;

/* Values for the flags parameter of the open and new functions.*/
/* Use an existing filename table or generate one for the archive creation
   functions. If this flag is specified for _open() or _open_fd() and the
   archive does not have a filename table, it will be ignored, however any
   file_lookup() operations on that archive will fail. */
#define PSO_AFS_FN_TABLE        (1 << 0)

   /* Archive reading functionality... */
pso_afs_read_t* pso_afs_read_open_fd(FILE* fd, uint32_t len, uint32_t flags,
    pso_error_t* err);
pso_afs_read_t* pso_afs_read_open(const char* fn, uint32_t flags,
    pso_error_t* err);
pso_error_t pso_afs_read_close(pso_afs_read_t* a);

uint32_t pso_afs_file_count(pso_afs_read_t* a);

uint32_t pso_afs_file_lookup(pso_afs_read_t* a, const char* fn);
pso_error_t pso_afs_file_name(pso_afs_read_t* a, uint32_t hnd, char* fn,
    size_t len);
psize_t pso_afs_file_size(pso_afs_read_t* a, uint32_t hnd);
pso_error_t pso_afs_file_stat(pso_afs_read_t* a, uint32_t hnd,
    struct stat* st);
psize_t pso_afs_file_read(pso_afs_read_t* a, uint32_t hnd, uint8_t* buf,
    size_t len);


/* Archive creation/writing functionality... */
pso_afs_write_t* pso_afs_new(const char* fn, uint32_t flags, pso_error_t* err);
pso_afs_write_t* pso_afs_new_fd(FILE* fd, uint32_t flags, pso_error_t* err);

pso_error_t pso_afs_write_close(pso_afs_write_t* a);

pso_error_t pso_afs_write_add(pso_afs_write_t* a, const char* fn,
    const uint8_t* data, uint32_t len);
pso_error_t pso_afs_write_add_ex(pso_afs_write_t* a, const char* fn,
    const uint8_t* data, uint32_t len,
    time_t ts);
pso_error_t pso_afs_write_add_fd(pso_afs_write_t* a, const char* fn, FILE* fd,
    uint32_t len);
pso_error_t pso_afs_write_add_file(pso_afs_write_t* a, const char* afn,
    const char* fn);

#endif /* !PSOARCHIVE__AFS_H */
