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

#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#include <inttypes.h>
#endif

#include "AFS.h"

struct afs_filename_ent {
    char filename[32];
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint32_t size;
};

struct afs_file {
    uint32_t offset;
    uint32_t size;
    struct afs_filename_ent fn_ent;
};

struct pso_afs_read {
    FILE* fd;
    struct afs_file* files;

    uint32_t file_count;
    uint32_t flags;
};

#ifdef _WIN32
time_t my_timegm(struct tm* tm) {
    time_t ret;
    //char* tz;

    /* Should really restore the TZ after this... At some point. */

    if (0 == _putenv("TZ")) {
        _tzset();
        ret = mktime(tm);
    }
    else
        ret = 0;
    return ret;
}

#define timegm my_timegm
#endif

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

static int digits(uint32_t n) {
    int r = 1;
    while (n /= 10) ++r;
    return r;
}

pso_afs_read_t* pso_afs_read_open_fd(FILE* fd, uint32_t len, uint32_t flags,
    pso_error_t* err) {
    pso_afs_read_t* rv;
    pso_error_t erv = PSOARCHIVE_EFATAL;
    uint32_t i, files;
    uint8_t buf[48];

    /* Read the beginning of the file to make sure it is an AFS archive and to
       get the number of files... */
    if (fread(buf, 1, 8, fd) != 8) {
    //if (read(fd, buf, 8) != 8) {
        erv = PSOARCHIVE_NOARCHIVE;
        goto ret_err;
    }

    /* The first 4 bytes must be 'AFS\0' */
    if (buf[0] != 0x41 || buf[1] != 0x46 || buf[2] != 0x53 || buf[3] != 0x00) {
        erv = PSOARCHIVE_NOARCHIVE;
        goto ret_err;
    }

    files = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);
    if (files > 65535) {
        erv = PSOARCHIVE_EFATAL;
        goto ret_err;
    }

    /* Allocate our archive handle... */
    if (!(rv = (pso_afs_read_t*)malloc(sizeof(pso_afs_read_t)))) {
        erv = PSOARCHIVE_EMEM;
        goto ret_err;
    }

    /* Allocate some file handles... */
    rv->files = (struct afs_file*)malloc(sizeof(struct afs_file) * files + 1);
    if (!rv->files) {
        erv = PSOARCHIVE_EMEM;
        goto ret_handle;
    }

    /* Read each file's metadata in. */
    for (i = 0; i < files; ++i) {
        if (fread(buf, 1, 8, fd) != 8) {
            erv = PSOARCHIVE_EIO;
            goto ret_files;
        }

        rv->files[i].offset = buf[0] | (buf[1] << 8) | (buf[2] << 16) |
            (buf[3] << 24);
        rv->files[i].size = buf[4] | (buf[5] << 8) | (buf[6] << 16) |
            (buf[7] << 24);

        /* Make sure it looks sane... */
        if (rv->files[i].offset + rv->files[i].size > len) {
            erv = PSOARCHIVE_ERANGE;
            goto ret_files;
        }
    }

    /* If the file has a filename list and the user has asked for support for
       it, read it in. */
    if ((flags & PSO_AFS_FN_TABLE)) {
        if (fread(buf, 1, 8, fd) != 8) {
            erv = PSOARCHIVE_EIO;
            goto ret_files;
        }

        rv->files[files].offset = buf[0] | (buf[1] << 8) | (buf[2] << 16) |
            (buf[3] << 24);
        rv->files[files].size = buf[4] | (buf[5] << 8) | (buf[6] << 16) |
            (buf[7] << 24);

        /* See if there's anything there... */
        if (rv->files[files].offset != 0 && rv->files[files].size != 0) {
            /* Make sure it looks sane... */
            if (rv->files[files].offset + rv->files[files].size > len) {
                erv = PSOARCHIVE_ERANGE;
                goto ret_files;
            }

            /* Make sure the size is right. */
            if (rv->files[files].size != files * 48) {
                erv = PSOARCHIVE_EBADMSG;
                goto ret_files;
            }

            /* Figure out how long the file is. */
            if (fseek(fd, rv->files[files].offset, SEEK_SET)) {
                erv = PSOARCHIVE_EIO;
                goto ret_files;
            }

            /* Move the file pointer to the filename table.*/
            if (ftell(fd) == (off_t)-1) {
                erv = PSOARCHIVE_EIO;
                goto ret_files;
            }

            /* Read each one in...  */
            for (i = 0; i < files; ++i) {
                if (fread(buf, 1, 48, fd) != 48) {
                    erv = PSOARCHIVE_EIO;
                    goto ret_files;
                }

                memcpy(rv->files[i].fn_ent.filename, buf, 32);
                rv->files[i].fn_ent.year = buf[32] | (buf[33] << 8);
                rv->files[i].fn_ent.month = buf[34] | (buf[35] << 8);
                rv->files[i].fn_ent.day = buf[36] | (buf[37] << 8);
                rv->files[i].fn_ent.hour = buf[38] | (buf[39] << 8);
                rv->files[i].fn_ent.minute = buf[40] | (buf[41] << 8);
                rv->files[i].fn_ent.second = buf[42] | (buf[43] << 8);
                rv->files[i].fn_ent.size = buf[44] | (buf[45] << 8) |
                    (buf[46] << 16) | (buf[47] << 24);

                /* Make sure it looks sane... */
                if (rv->files[i].fn_ent.size != rv->files[i].size) {
                    erv = PSOARCHIVE_EBADMSG;
                    goto ret_files;
                }
            }
        }
        else {
            flags &= ~(PSO_AFS_FN_TABLE);
        }
    }

    /* Set the file count in the handle */
    rv->fd = fd;
    rv->file_count = files;
    rv->flags = flags;

    /* We're done, return... */
    if (err)
        *err = PSOARCHIVE_OK;

    return rv;

ret_files:
    free(rv->files);
ret_handle:
    free(rv);
ret_err:
    if (err)
        *err = erv;

    return NULL;
}

pso_afs_read_t* pso_afs_read_open(const char* fn, uint32_t flags,
    pso_error_t* err) {
    FILE* fd;
    off_t total;
    pso_error_t erv = PSOARCHIVE_EFATAL;
    pso_afs_read_t* rv;

    /* Open the file... */
    if (!(fd = fopen(fn, "rb"))) {
        erv = PSOARCHIVE_EFILE;
        goto ret_err;
    }

    /* Figure out how long the file is. */
    if (fseek(fd, 0, SEEK_END)) {
        erv = PSOARCHIVE_EIO;
        goto ret_file;
    }

    /* Figure out how long the file is. */
    if ((total = ftell(fd)) == (off_t)-1) {
        erv = PSOARCHIVE_EIO;
        goto ret_file;
    }

    if (fseek(fd, 0, SEEK_SET)) {
        erv = PSOARCHIVE_EIO;
        goto ret_file;
    }

    if ((rv = pso_afs_read_open_fd(fd, (uint32_t)total, flags, err)))
        return rv;

    /* 如果我们到了这里, the pso_afs_read_open_fd() function encountered an error.
       Clean up the file descriptor and return NULL. The error code is already
       set in err, if applicable. */
    fclose(fd);
    return NULL;

ret_file:
    fclose(fd);
ret_err:
    if (err)
        *err = erv;

    return NULL;
}

pso_error_t pso_afs_read_close(pso_afs_read_t* a) {
    if (!a || a->fd < 0 || !a->files)
        return PSOARCHIVE_EFATAL;

    fclose(a->fd);
    free(a->files);
    free(a);

    return PSOARCHIVE_OK;
}

uint32_t pso_afs_file_count(pso_afs_read_t* a) {
    if (!a)
        return 0;

    return a->file_count;
}

uint32_t pso_afs_file_lookup(pso_afs_read_t* a, const char* fn) {
    uint32_t i;

    if (!a || a->fd < 0 || !a->files || !fn)
        return PSOARCHIVE_HND_INVALID;

    if (!(a->flags & PSO_AFS_FN_TABLE))
        return PSOARCHIVE_HND_INVALID;

    /* Look through the list for the one specified. */
    for (i = 0; i < a->file_count; ++i) {
        if (!strcmp(a->files[i].fn_ent.filename, fn))
            return i;
    }

    return PSOARCHIVE_HND_INVALID;
}

pso_error_t pso_afs_file_name(pso_afs_read_t* a, uint32_t hnd, char* fn,
    size_t len) {
    int dg;

    /* Make sure the arguments are sane... */
    if (!a || hnd >= a->file_count)
        return PSOARCHIVE_EFATAL;

    /* Do we have a filename table? */
    if ((a->flags & PSO_AFS_FN_TABLE)) {
        /* Yep. Return the name from the table. */
        if (len > 32) {
            /* I dunno if filenames of length 32 are valid in AFS, or if they
               have to be NUL terminated, so assume (for safety) that they can
               be non-NUL terminated. */
            memset(fn + 32, 0, len - 32);
            memcpy(fn, a->files[hnd].fn_ent.filename, 32);
        }
        else {
            strncpy(fn, a->files[hnd].fn_ent.filename, len);
        }
    }
    else {
        /* Nope. Do it based on the file number. */
        dg = digits(a->file_count);

#ifndef _WIN32
        snprintf(fn, len, "%0*" PRIu32 ".bin", dg, hnd);
#else
        snprintf(fn, len, "%0*I32u.bin", dg, hnd);
#endif
    }

    return PSOARCHIVE_OK;
}

psize_t pso_afs_file_size(pso_afs_read_t* a, uint32_t hnd) {
    /* Make sure the arguments are sane... */
    if (!a || hnd >= a->file_count)
        return PSOARCHIVE_EFATAL;

    return (psize_t)a->files[hnd].size;
}

pso_error_t pso_afs_file_stat(pso_afs_read_t* a, uint32_t hnd,
    struct stat* st) {
    struct tm mtm;

    /* Make sure the arguments are sane... */
    if (!a || hnd >= a->file_count || !st)
        return PSOARCHIVE_EFATAL;

    /* Clear it and fill in what we can. */
    memset(st, 0, sizeof(struct stat));
    st->st_size = (off_t)a->files[hnd].size;

    if ((a->flags & PSO_AFS_FN_TABLE)) {
        memset(&mtm, 0, sizeof(struct tm));
        mtm.tm_year = LE16(a->files[hnd].fn_ent.year) - 1900;
        mtm.tm_mon = LE16(a->files[hnd].fn_ent.month);
        mtm.tm_mday = LE16(a->files[hnd].fn_ent.day);
        mtm.tm_hour = LE16(a->files[hnd].fn_ent.hour);
        mtm.tm_min = LE16(a->files[hnd].fn_ent.minute);
        mtm.tm_sec = LE16(a->files[hnd].fn_ent.second);

        st->st_mtime = timegm(&mtm);
    }

    return PSOARCHIVE_OK;
}

psize_t pso_afs_file_read(pso_afs_read_t* a, uint32_t hnd, void* buf,
    size_t len) {
    /* Make sure the arguments are sane... */
    if (!a || hnd > a->file_count || !buf || !len)
        return PSOARCHIVE_EFATAL;

    /* Figure out how long the file is. */
    if (fseek(a->fd, a->files[hnd].offset, SEEK_SET)) 
        return PSOARCHIVE_EIO;

    /* Seek to the appropriate position in the file. */
    if (ftell(a->fd) == (off_t)-1)
        return PSOARCHIVE_EIO;

    /* Figure out how much we're going to read... */
    if (a->files[hnd].size < len)
        len = a->files[hnd].size;

    if (fread(buf, 1, len, a->fd) != len)
        return PSOARCHIVE_EIO;

    return (psize_t)len;
}
