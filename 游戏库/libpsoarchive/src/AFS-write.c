/*
    This file is part of libpsoarchive.

    Copyright (C) 2015, 2016 Lawrence Sebald

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include <fcntl.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "AFS.h"

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#define LE16(x) (((x >> 8) & 0xFF00) | ((x & 0xFF00) << 8))
#define LE32(x) (((x >> 24) & 0x00FF) | \
                 ((x >>  8) & 0xFF00) | \
                 ((x & 0xFF00) <<  8) | \
                 ((x & 0x00FF) << 24))
#else
#define LE16(x) x
#define LE32(x) x
#endif

struct afs_fn {
    char filename[32];
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint32_t size;
};

struct pso_afs_write {
    FILE* fd;

    int ftab_used;

    uint32_t flags;

    off_t ftab_pos;
    off_t data_pos;

    struct afs_fn* fns;
    int fns_allocd;
};

static off_t pad_file(FILE* fd, int boundary) {
    fseek(fd, 0, SEEK_CUR);
    off_t pos = ftell(fd);
    uint8_t tmp = 0;

    /* If we aren't actually padding, don't do anything. */
    if (boundary <= 0)
        return pos;

    pos = (pos & ~(boundary - 1)) + boundary;

    if (fseek(fd, pos - 1, SEEK_SET))
        return (off_t)-1;

    if (ftell(fd) == (off_t)-1)
        return (off_t)-1;

    if (fwrite(&tmp, 1, 1, fd) != 1)
        return (off_t)-1;

    return pos;
}

pso_afs_write_t* pso_afs_new(const char* fn, uint32_t flags, pso_error_t* err) {
    pso_afs_write_t* rv;
    pso_error_t erv = PSOARCHIVE_OK;

    /* Allocate space for our write context. */
    if (!(rv = (pso_afs_write_t*)malloc(sizeof(pso_afs_write_t)))) {
        erv = PSOARCHIVE_EMEM;
        goto ret_err;
    }

    /* Open the file specified. */
    if ((rv->fd = fopen(fn, "rb")) < 0) {
        erv = PSOARCHIVE_EFILE;
        goto ret_mem;
    }

    /* Allocate a filename table array, if the user has asked for it. */
    if ((flags & PSO_AFS_FN_TABLE)) {
        if (!(rv->fns = (struct afs_fn*)malloc(sizeof(struct afs_fn) * 64))) {
            erv = PSOARCHIVE_EMEM;
            fclose(rv->fd);
            goto ret_mem;
        }

        memset(rv->fns, 0, sizeof(struct afs_fn) * 64);
        rv->fns_allocd = 64;
    }

    /* Fill in the base structure with our defaults. */
    rv->ftab_used = 0;
    rv->ftab_pos = 8;
    rv->data_pos = 0x80000;
    rv->flags = flags;

    /* We're done, return success. */
    if (err)
        *err = PSOARCHIVE_OK;

    return rv;

ret_mem:
    free(rv);
ret_err:
    if (err)
        *err = erv;

    return NULL;
}

pso_afs_write_t* pso_afs_new_fd(FILE* fd, uint32_t flags, pso_error_t* err) {
    pso_afs_write_t* rv;
    pso_error_t erv = PSOARCHIVE_OK;

    /* Allocate space for our write context. */
    if (!(rv = (pso_afs_write_t*)malloc(sizeof(pso_afs_write_t)))) {
        erv = PSOARCHIVE_EMEM;
        goto ret_err;
    }

    /* Allocate a filename table array, if the user has asked for it. */
    if ((flags & PSO_AFS_FN_TABLE)) {
        if (!(rv->fns = (struct afs_fn*)malloc(sizeof(struct afs_fn) * 64))) {
            erv = PSOARCHIVE_EMEM;
            free(rv);
            goto ret_err;
        }

        memset(rv->fns, 0, sizeof(struct afs_fn) * 64);
        rv->fns_allocd = 64;
    }

    /* Fill in the base structure with our data. */
    rv->fd = fd;
    rv->ftab_used = 0;
    rv->ftab_pos = 8;
    rv->data_pos = 0x80000;
    rv->flags = flags;

    /* We're done, return success. */
    if (err)
        *err = PSOARCHIVE_OK;

    return rv;

ret_err:
    if (err)
        *err = erv;

    return NULL;
}

pso_error_t pso_afs_write_close(pso_afs_write_t* a) {
    uint8_t buf[48] = { 0 };
    uint32_t len;
    int i;

    if (!a || a->fd < 0)
        return PSOARCHIVE_EFATAL;

    /* Figure out how long the file is. */
    if (fseek(a->fd, 0, SEEK_SET)) {
        return PSOARCHIVE_EIO;
    }

    /* Put the header at the beginning of the file. */
    if (ftell(a->fd) == (off_t)-1)
        return PSOARCHIVE_EIO;

    buf[0] = 0x41;
    buf[1] = 0x46;
    buf[2] = 0x53;
    buf[3] = 0x00;
    buf[4] = (uint8_t)(a->ftab_used);
    buf[5] = (uint8_t)(a->ftab_used >> 8);
    buf[6] = (uint8_t)(a->ftab_used >> 16);
    buf[7] = (uint8_t)(a->ftab_used >> 24);

    if (fwrite(buf, 1, 8, a->fd) != 8)
        return PSOARCHIVE_EIO;

    /* If the user has asked for a filename array, write it out too. */
    if ((a->flags & PSO_AFS_FN_TABLE)) {
        /* First, write the entry in the file table. */
        len = a->ftab_used * 48;

        if (fseek(a->fd, a->ftab_pos, SEEK_SET))
            return PSOARCHIVE_EIO;

        if (ftell(a->fd) == (off_t)-1)
            return PSOARCHIVE_EIO;

        /* Copy the file data into the buffer... */
        buf[0] = (uint8_t)(a->data_pos);
        buf[1] = (uint8_t)(a->data_pos >> 8);
        buf[2] = (uint8_t)(a->data_pos >> 16);
        buf[3] = (uint8_t)(a->data_pos >> 24);
        buf[4] = (uint8_t)(len);
        buf[5] = (uint8_t)(len >> 8);
        buf[6] = (uint8_t)(len >> 16);
        buf[7] = (uint8_t)(len >> 24);

        if (fwrite(buf, 1, 8, a->fd) != 8)
            return PSOARCHIVE_EIO;

        /* Next, write out the data. */
        if (fseek(a->fd, a->data_pos, SEEK_SET))
            return PSOARCHIVE_EIO;

        /* Next, write out the data. */
        if (ftell(a->fd) == (off_t)-1)
            return PSOARCHIVE_EIO;

        for (i = 0; i < a->ftab_used; ++i) {
            /* Fill it in first. */
            memcpy(buf, a->fns[i].filename, 32);
            buf[32] = (uint8_t)(a->fns[i].year);
            buf[33] = (uint8_t)(a->fns[i].year >> 8);
            buf[34] = (uint8_t)(a->fns[i].month);
            buf[35] = (uint8_t)(a->fns[i].month >> 8);
            buf[36] = (uint8_t)(a->fns[i].day);
            buf[37] = (uint8_t)(a->fns[i].day >> 8);
            buf[38] = (uint8_t)(a->fns[i].hour);
            buf[39] = (uint8_t)(a->fns[i].hour >> 8);
            buf[40] = (uint8_t)(a->fns[i].minute);
            buf[41] = (uint8_t)(a->fns[i].minute >> 8);
            buf[42] = (uint8_t)(a->fns[i].second);
            buf[43] = (uint8_t)(a->fns[i].second >> 8);
            buf[44] = (uint8_t)(a->fns[i].size);
            buf[45] = (uint8_t)(a->fns[i].size >> 8);
            buf[46] = (uint8_t)(a->fns[i].size >> 16);
            buf[47] = (uint8_t)(a->fns[i].size >> 24);

            /* Write it out. */
            if (fwrite(buf, 1, 48, a->fd) != 48)
                return PSOARCHIVE_EIO;
        }

        /* Pad the data position out to a nice boundary. */
        a->data_pos = pad_file(a->fd, 2048);
    }

    fclose(a->fd);
    free(a);

    return PSOARCHIVE_OK;
}

pso_error_t pso_afs_write_add(pso_afs_write_t* a, const char* fn,
    const uint8_t* data, uint32_t len) {
    return pso_afs_write_add_ex(a, fn, data, len, time(NULL));
}

pso_error_t pso_afs_write_add_ex(pso_afs_write_t* a, const char* fn,
    const uint8_t* data, uint32_t len,
    time_t ts) {
    uint8_t buf[8];
    void* tmp;
    struct tm* tmv;

    if (!a)
        return PSOARCHIVE_EFATAL;

    /* Go to where we'll be writing into the file table... */
    if (fseek(a->fd, a->ftab_pos, SEEK_SET))
        return PSOARCHIVE_EIO;

    /* Go to where we'll be writing into the file table... */
    if (ftell(a->fd) == (off_t)-1)
        return PSOARCHIVE_EIO;

    /* Copy the file data into the buffer... */
    buf[0] = (uint8_t)(a->data_pos);
    buf[1] = (uint8_t)(a->data_pos >> 8);
    buf[2] = (uint8_t)(a->data_pos >> 16);
    buf[3] = (uint8_t)(a->data_pos >> 24);
    buf[4] = (uint8_t)(len);
    buf[5] = (uint8_t)(len >> 8);
    buf[6] = (uint8_t)(len >> 16);
    buf[7] = (uint8_t)(len >> 24);

    /* Fill in the file information in the filename table, if applicable. */
    if ((a->flags & PSO_AFS_FN_TABLE)) {
        /* Do we need to reallocate the filename table? */
        if (a->ftab_used == a->fns_allocd) {
            tmp = realloc(a->fns, sizeof(struct afs_fn) * (a->fns_allocd * 2));
            if (!tmp)
                return PSOARCHIVE_EMEM;

            a->fns_allocd *= 2;
            a->fns = (struct afs_fn*)tmp;
        }

        /* Fill in the entry. */
        strncpy(a->fns[a->ftab_used].filename, fn, 32);

        /* Parse the date from the timestamp passed in and save it. */
        tmv = gmtime(&ts);

        a->fns[a->ftab_used].year = LE16(tmv->tm_year + 1900);
        a->fns[a->ftab_used].month = LE16(tmv->tm_mon);
        a->fns[a->ftab_used].day = LE16(tmv->tm_mday);
        a->fns[a->ftab_used].hour = LE16(tmv->tm_hour);
        a->fns[a->ftab_used].minute = LE16(tmv->tm_min);
        a->fns[a->ftab_used].second = LE16(tmv->tm_sec);
        a->fns[a->ftab_used].size = LE32(len);
    }

    /* Write out the header... */
    if (fwrite(buf, 1, 8, a->fd) != 8)
        return PSOARCHIVE_EIO;

    a->ftab_pos += 8;
    ++a->ftab_used;

    /* Seek to where the file data goes... */
    if (fseek(a->fd, a->data_pos, SEEK_SET))
        return PSOARCHIVE_EIO;

    /* Seek to where the file data goes... */
    if (ftell(a->fd) == (off_t)-1)
        return PSOARCHIVE_EIO;

    /* Write the file data out. */
    if (fwrite(data, 1, len, a->fd) != (psize_t)len)
        return PSOARCHIVE_EIO;

    /* Pad the data position out to where the next file will start. */
    a->data_pos = pad_file(a->fd, 2048);

    /* Done. */
    return PSOARCHIVE_OK;
}

pso_error_t pso_afs_write_add_fd(pso_afs_write_t* a, const char* fn, FILE* fd,
    uint32_t len) {
    uint8_t buf[512];
    psize_t bytes;
    void* tmp;
    struct stat st;
    struct tm* tmv;

    if (!a)
        return PSOARCHIVE_EFATAL;

    /* Go to where we'll be writing into the file table... */
    if (fseek(a->fd, a->ftab_pos, SEEK_SET))
        return PSOARCHIVE_EIO;

    /* Go to where we'll be writing into the file table... */
    if (ftell(a->fd) == (off_t)-1)
        return PSOARCHIVE_EIO;

    /* Copy the file data into the buffer... */
    buf[0] = (uint8_t)(a->data_pos);
    buf[1] = (uint8_t)(a->data_pos >> 8);
    buf[2] = (uint8_t)(a->data_pos >> 16);
    buf[3] = (uint8_t)(a->data_pos >> 24);
    buf[4] = (uint8_t)(len);
    buf[5] = (uint8_t)(len >> 8);
    buf[6] = (uint8_t)(len >> 16);
    buf[7] = (uint8_t)(len >> 24);

    /* Fill in the file information in the filename table, if applicable. */
    if ((a->flags & PSO_AFS_FN_TABLE)) {
        /* Do we need to reallocate the filename table? */
        if (a->ftab_used == a->fns_allocd) {
            tmp = realloc(a->fns, sizeof(struct afs_fn) * (a->fns_allocd * 2));
            if (!tmp)
                return PSOARCHIVE_EMEM;

            a->fns_allocd *= 2;
            a->fns = (struct afs_fn*)tmp;
        }

        /* Fill in the entry. */
        strncpy(a->fns[a->ftab_used].filename, fn, 32);

        /* Get the modification date of the file, so we can fill in the
           timestamp properly. */
        if ((fstat((int)fd, &st)) < 0)
            return PSOARCHIVE_EFILE;

        tmv = gmtime(&st.st_mtime);

        a->fns[a->ftab_used].year = LE16(tmv->tm_year + 1900);
        a->fns[a->ftab_used].month = LE16(tmv->tm_mon);
        a->fns[a->ftab_used].day = LE16(tmv->tm_mday);
        a->fns[a->ftab_used].hour = LE16(tmv->tm_hour);
        a->fns[a->ftab_used].minute = LE16(tmv->tm_min);
        a->fns[a->ftab_used].second = LE16(tmv->tm_sec);
        a->fns[a->ftab_used].size = LE32(len);
    }

    /* Write out the header... */
    if (fwrite(buf, 1, 8, a->fd) != 8)
        return PSOARCHIVE_EIO;

    a->ftab_pos += 8;
    ++a->ftab_used;

    /* Seek to where the file data goes... */
    if (fseek(a->fd, a->data_pos, SEEK_SET))
        return PSOARCHIVE_EIO;

    /* Seek to where the file data goes... */
    if (ftell(a->fd) == (off_t)-1)
        return PSOARCHIVE_EIO;

    /* While there's still data to be read from the file, read it into our
       buffer and write it out to the file. */
    while (len) {
        bytes = len > 512 ? 512 : len;

        if (fread(buf, 1,bytes, fd) != bytes)
            return PSOARCHIVE_EIO;

        if (fwrite(buf, 1, bytes, a->fd) != bytes)
            return PSOARCHIVE_EIO;

        len -= bytes;
    }

    /* Pad the data position out to where the next file will start. */
    a->data_pos = pad_file(a->fd, 2048);

    /* Done. */
    return PSOARCHIVE_OK;
}

pso_error_t pso_afs_write_add_file(pso_afs_write_t* a, const char* afn,
    const char* fn) {
    FILE* fd;
    pso_error_t err;
    off_t len;

    /* Open the file. */
    if ((fd = fopen(fn, "rb")) < 0)
        return PSOARCHIVE_EFILE;

    /* Figure out how long the file is. */
    if (fseek(fd, 0, SEEK_END)) {
        fclose(fd);
        return PSOARCHIVE_EIO;
    }

    /* Figure out how long the file is. */
    if ((len = ftell(fd)) == (off_t)-1) {
        fclose(fd);
        return PSOARCHIVE_EIO;
    }

    if (fseek(fd, 0, SEEK_SET)) {
        fclose(fd);
        return PSOARCHIVE_EIO;
    }

    if (ftell(fd) == (off_t)-1) {
        fclose(fd);
        return PSOARCHIVE_EIO;
    }

    /* Add it to the archive... */
    err = pso_afs_write_add_fd(a, afn, fd, (uint32_t)len);

    /* Clean up... */
    fclose(fd);
    return err;
}
