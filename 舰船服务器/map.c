/*
    梦幻之星中国 舰船服务器 游戏地图解析
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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char* name_token;
    uint32_t* variation1_values;
    uint32_t* variation2_values;
} AreaMapFileIndex;

typedef struct {
    AreaMapFileIndex*** map_files;
} MapFileInfo;

MapFileInfo* create_map_file_info() {
    MapFileInfo* map_file_info = (MapFileInfo*)malloc(sizeof(MapFileInfo));

    // Allocate memory for episode 1
    map_file_info->map_files = (AreaMapFileIndex***)malloc(3 * sizeof(AreaMapFileIndex**));

    // Allocate memory for episode 1, non-solo
    map_file_info->map_files[0] = (AreaMapFileIndex**)malloc(16 * sizeof(AreaMapFileIndex*));
    map_file_info->map_files[0][0] = (AreaMapFileIndex*)malloc(sizeof(AreaMapFileIndex));
    map_file_info->map_files[0][0]->name_token = "city00";
    map_file_info->map_files[0][0]->variation1_values = NULL;
    map_file_info->map_files[0][0]->variation2_values = (uint32_t*)malloc(sizeof(uint32_t));
    map_file_info->map_files[0][0]->variation2_values[0] = 0;

    // Allocate memory for episode 1, solo
    map_file_info->map_files[0][1] = (AreaMapFileIndex*)malloc(sizeof(AreaMapFileIndex));
    map_file_info->map_files[0][1]->name_token = "city00";
    map_file_info->map_files[0][1]->variation1_values = NULL;
    map_file_info->map_files[0][1]->variation2_values = (uint32_t*)malloc(sizeof(uint32_t));
    map_file_info->map_files[0][1]->variation2_values[0] = 0;

    // Allocate memory for episode 2
    map_file_info->map_files[1] = (AreaMapFileIndex**)malloc(16 * sizeof(AreaMapFileIndex*));
    map_file_info->map_files[1][0] = (AreaMapFileIndex*)malloc(sizeof(AreaMapFileIndex));
    map_file_info->map_files[1][0]->name_token = "labo00";
    map_file_info->map_files[1][0]->variation1_values = NULL;
    map_file_info->map_files[1][0]->variation2_values = (uint32_t*)malloc(sizeof(uint32_t));
    map_file_info->map_files[1][0]->variation2_values[0] = 0;

    // Allocate memory for episode 2, solo
    map_file_info->map_files[1][1] = (AreaMapFileIndex*)malloc(sizeof(AreaMapFileIndex));
    map_file_info->map_files[1][1]->name_token = "labo00";
    map_file_info->map_files[1][1]->variation1_values = NULL;
    map_file_info->map_files[1][1]->variation2_values = (uint32_t*)malloc(sizeof(uint32_t));
    map_file_info->map_files[1][1]->variation2_values[0] = 0;

    // Allocate memory for episode 4
    map_file_info->map_files[2] = (AreaMapFileIndex**)malloc(16 * sizeof(AreaMapFileIndex*));
    map_file_info->map_files[2][0] = (AreaMapFileIndex*)malloc(sizeof(AreaMapFileIndex));
    map_file_info->map_files[2][0]->name_token = "city02";
    map_file_info->map_files[2][0]->variation1_values = (uint32_t*)malloc(sizeof(uint32_t));
    map_file_info->map_files[2][0]->variation1_values[0] = 0;
    map_file_info->map_files[2][0]->variation2_values = (uint32_t*)malloc(1 * sizeof(uint32_t));
    map_file_info->map_files[2][0]->variation2_values[0] = 0;

    // Allocate memory for episode 4, solo
    map_file_info->map_files[2][1] = (AreaMapFileIndex*)malloc(sizeof(AreaMapFileIndex));
    map_file_info->map_files[2][1]->name_token = "city02";
    map_file_info->map_files[2][1]->variation1_values = (uint32_t*)malloc(sizeof(uint32_t));
    map_file_info->map_files[2][1]->variation1_values[0] = 0;
    map_file_info->map_files[2][1]->variation2_values = (uint32_t*)malloc(1 * sizeof(uint32_t));
    map_file_info->map_files[2][1]->variation2_values[0] = 0;

    return map_file_info;
}

void delete_map_file_info(MapFileInfo* map_file_info) {
    // Deallocate memory for each episode
    for (int episode = 0; episode < 3; episode++) {
        // Deallocate memory for each non-solo and solo AreaMapFileIndex
        for (int i = 0; i < 2; i++) {
            if (map_file_info->map_files[episode][i]->variation1_values != NULL)
                free(map_file_info->map_files[episode][i]->variation1_values);
            if (map_file_info->map_files[episode][i]->variation2_values != NULL)
                free(map_file_info->map_files[episode][i]->variation2_values);
            free(map_file_info->map_files[episode][i]);
        }
        // Deallocate memory for episode
        free(map_file_info->map_files[episode]);
    }

    // Deallocate memory for map_files
    free(map_file_info->map_files);

    // Deallocate memory for map_file_info
    free(map_file_info);
}

const char* map_filenames_for_variation(
    MapFileInfo* map_file_info, int episode, int is_solo, int area, uint32_t var1, uint32_t var2) {
    AreaMapFileIndex* a = NULL;
    if (is_solo) {
        a = map_file_info->map_files[episode][1];
    }
    if (!a || !a->name_token) {
        a = map_file_info->map_files[episode][0];
    }
    if (!a->name_token) {
        return NULL;
    }

    char* filename = (char*)malloc(256 * sizeof(char));

    if (!filename)
        return NULL;

    sprintf(filename, "map_%s", a->name_token);

    if (a->variation1_values != NULL) {
        char temp[3] = "";
        sprintf(temp, "%02X", a->variation1_values[var1]);
        strcat(filename, "_");
        strcat(filename, temp);
    }

    if (a->variation2_values != NULL) {
        char temp[3] = "";
        sprintf(temp, "%02X", a->variation2_values[var2]);
        strcat(filename, "_");
        strcat(filename, temp);
    }

    if (is_solo) {
        strcat(filename, "e_s.dat");
    }
    else {
        strcat(filename, "e.dat");
    }

    return filename;
}

int main() {
    // 创建地图文件信息
    MapFileInfo* map_file_info = create_map_file_info();

    // 获取地图文件名
    const char* filename = map_filenames_for_variation(map_file_info, 0, 0, 1, 0, 0);
    if (filename != NULL && strcmp(filename, "0") != 0) {
        printf("地图文件名：%s\n", filename);
    }
    else {
        printf("未找到地图文件名。\n");
    }

    // 释放地图文件信息
    delete_map_file_info(map_file_info);

    system("pause");

    return 0;
}