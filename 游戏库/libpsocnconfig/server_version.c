/*
    梦幻之星中国 服务器版本文件.
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
#include "Software_Defines.h"
#include "f_logs.h"

// 定义全局变量存储版本号
int version_major = 0;
int version_minor = 0;
int version_patch = 0;

// 递增版本号
void increment_version() {
    if (version_patch == 9) {
        if (version_minor == 9) {
            version_major++;
            version_minor = 0;
        }
        else {
            version_minor++;
        }
        version_patch = 0;
    }
    else {
        version_patch++;
    }
}

// 读取版本号
static int read_version_from_file(const char* version_file) {
    FILE* file = fopen(version_file, "r");
    if (file == NULL) {
        ERR_LOG("无法打开文件 %s\n", version_file);
        return -1;
    }

    fscanf(file, "%d.%d.%d", &version_major, &version_minor, &version_patch);

    fclose(file);

    return 0;
}

void read_specific_line(const char* filename, int line_number) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        ERR_LOG("无法打开文件 %s\n", filename);
        return;
    }

    char line[256];
    int current_line = 1;

    while (fgets(line, sizeof(line), file)) {
        if (current_line == line_number) {
            printf("文件 %s 的第 %d 行内容是：%s", filename, line_number, line);
            break;
        }
        current_line++;
    }

    fclose(file);
}

// 将版本号写入文本文件
static int write_version_to_file(const char* version_file) {
    FILE* file = fopen(version_file, "w");
    if (file == NULL) {
        ERR_LOG("无法打开文件 %s\n", version_file);
        return -1;
    }

    fprintf(file, "%d.%d.%d", version_major, version_minor, version_patch);

    fclose(file);

    return 0;
}

int build_server_version(const char* version_file) {
    int rv = 0;

    if (read_version_from_file(version_file)) {
        rv = -1;
    }

    // 递增版本号
    increment_version();

    // 将版本号写入文本文件
    if (write_version_to_file(version_file)) {
        rv = -1;
    }

    return rv;
}