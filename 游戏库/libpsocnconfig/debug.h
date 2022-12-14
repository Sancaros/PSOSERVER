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

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>

/* Values for the level parameter of the debug function. */
#define DBG_LOGS        1
#define DBG_NORMAL      2
#define DBG_WARN        10
#define DBG_ERROR       20

void debug_set_threshold(int level);
FILE* debug_set_file(FILE* fp);
void debug(int level, const char* fmt, ...);
int fdebug(FILE* fp, int level, const char* fmt, ...);
int vfdebug(FILE* fp, const char* fmt, va_list args);

#endif /* !DEBUG_H */
