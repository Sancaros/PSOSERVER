/*
    This file is part of Sylverant PSO Server.

    Copyright (C) 2009, 2011, 2019, 2020 Lawrence Sebald

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

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>

#include "debug.h"

static int min_level = DBG_LOGS;
static FILE* dfp = NULL;

void debug_set_threshold(int level) {
    min_level = level;
}

FILE* debug_set_file(FILE* fp) {
    FILE* ofp = dfp;

    if (fp)
        dfp = fp;

    return ofp;
}

void debug(int level, const char* fmt, ...) {
    va_list args;

    /* If the default file we write to hasn't been initialized, set it to
       stdout. */
    if (!dfp)
        dfp = stdout;

    /* Make sure we want to receive messages at this level. */
    if (level < min_level)
        return;

    va_start(args, fmt);
    vfdebug(dfp, fmt, args);
    va_end(args);
}

int fdebug(FILE* fp, int level, const char* fmt, ...) {
    va_list args;
    int rv;

    if (!fp || !fmt)
        return -1;

    if (level < min_level)
        return 0;

    va_start(args, fmt);
    rv = vfdebug(fp, fmt, args);
    va_end(args);

    return rv;
}

int vfdebug(FILE* fp, const char* fmt, va_list args) {
    //struct timeval rawtime;
    //struct tm cooked = {0};
    SYSTEMTIME rawtime;

    if (!fp || !fmt)
        return -1;

    /* Get the timestamp */
    //GetTickCount64(&rawtime, NULL);
    GetLocalTime(&rawtime);

    /* Get UTC */
    //gmtime_r(&rawtime.tv_sec, &cooked);

    /* Print the timestamp */
    fprintf(fp, "[%u:%02u:%02u: %02u:%02u:%02u.%03u]: ", rawtime.wYear,
        rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute,
        rawtime.wSecond, rawtime.wMilliseconds);

    vfprintf(fp, fmt, args);
    fflush(fp);
    return 0;
}
