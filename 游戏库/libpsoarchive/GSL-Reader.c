/*
    梦幻之星中国 舰船服务器 GSL Reader
    版权 (C) 2022, 2023 Sancaros

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
#include <string.h>
#include <stdlib.h>
#include <WinSock_Defines.h>

#include <f_logs.h>
#include "GSL.h"

// finds a file entry in a loaded GSL archive
gsl_entry* GSL_FindEntry(gsl_header* gsl, char* filename) {
    int x;
    if (!gsl)
        return NULL;
    for (x = 0; x < 0x100; x++)
        if (!strcmp(gsl->e[x].name, filename))
            break;
    if (x >= 0x100)
        return NULL;
    return &gsl->e[x];
}

// opens (loads) a GSL archive
gsl_header* GSL_OpenArchive(const char* filename) {
    HANDLE file;
    DWORD x;
    file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return NULL;
    gsl_header* gsl = (gsl_header*)malloc(sizeof(gsl_header));
    if (!gsl) {
        free(gsl);
        return NULL;
    }
    ReadFile(file, gsl, 0x3000, &x, NULL);
    gsl->file = file;
    for (x = 0; x < 0x100; x++) {
        gsl->e[x].offset = byteswap(gsl->e[x].offset);
        gsl->e[x].size = byteswap(gsl->e[x].size);
    }
    return gsl;
}

// reads an entire file from a GSL archive
bool GSL_ReadFile(gsl_header* gsl, char* filename, void* buffer) {
    DWORD bytesread;
    if (!gsl)
        return false;
    gsl_entry* e = GSL_FindEntry(gsl, filename);
    if (!e)
        return false;
    SetFilePointer(gsl->file, e->offset * 0x800, NULL, FILE_BEGIN);
    ReadFile(gsl->file, buffer, e->size, &bytesread, NULL);

    // DEBUG
    // printf("bytesread %i / offset %i\n", bytesread, e->offset);
    return true;
}

// gets a file's size inside a GSL archive
DWORD GSL_GetFileSize(gsl_header* gsl, char* filename) {
    gsl_entry* ge = GSL_FindEntry(gsl, filename);
    if (!ge)
        return 0;
    return ge->size;
}

// closes (unloads) a GSL archive
void GSL_CloseArchive(gsl_header* gsl) {
    if (gsl) {
        CloseHandle(gsl->file);
        free_safe(gsl);
    }
}
