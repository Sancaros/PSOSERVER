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

// an entry in PSO's ItemRT*.rel files
typedef struct
{
    BYTE probability;
    BYTE itemcode[3];
} DROPITEM_V3;

// the format of the ItemRT*.rel files
typedef struct
{
    DROPITEM_V3 enemy[2][4][10][100];
    DROPITEM_V3 box[2][4][10][30];
    BYTE area[2][4][10][30];
} DROPDATA_V3;

DROPDATA_V3 dropdata_v3;

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

// loads all ItemRT*.rel files for episodes 1 and 2 from itemrt.gsl
bool LoadV3DropInfoFile(const char* fn)
{
    // HANDLE file;
    // DWORD size, bytesread;
    int w, x, y, z, a = 0;
    BYTE data[0x800];
    char filename[0x20];
    // wchar_t buffer[0x20];

    gsl_header* gsl = GSL_OpenArchive(fn);
    if (!gsl)
        return false;

    char* prefixes[] = { "ItemRT", "ItemRTl", "ItemRTc" , "ItemRTbb" };
    char diffs[] = "nhvu";
    for (w = 0; w < 4; w++) // episode
    {
        for (x = 0; x < 4; x++) // difficulty
        {
            for (y = 0; y < 10; y++) // sec id
            {
                sprintf(filename, "%s%c%d.rel", prefixes[w], diffs[x], y);
                if (GSL_GetFileSize(gsl, filename) > 0x800)
                    return false;
                if (!GSL_ReadFile(gsl, filename, data))
                    return false;

                for (z = 0; z < 99; z++) // monster id
                    memcpy(&dropdata_v3.enemy[w][x][y][z], &data[(z * 4) + 4], 4);
                for (z = 0; z < 30; z++) // box id
                {
                    memcpy(&dropdata_v3.box[w][x][y][z], &data[(z * 4) + 0x01B2], 4);
                    dropdata_v3.area[w][x][y][z] = data[z + 0x0194];
                }
            }
        }
    }
    GSL_CloseArchive(gsl);
    return true;
}
//
//int main(int argc, char** argv)
//{
//    // Load itemrt.gsl and export data as TSV (itemrt.gsl must be in folder)
//    if (LoadV3DropInfoFile(cfg->bb_rtdata_file))
//    {
//        FILE* enemy_data;
//        FILE* box_data;
//        enemy_data = fopen("itemrt-enemy.tsv", "w");
//        box_data = fopen("itemrt-box.tsv", "w");
//
//        int ep, diff, secid, mobid, boxid;
//        const char* episodenames[] = { "I", "II", "CH", "IV" };
//        const char* diffnames[] = { "Normal", "Hard", "Very-Hard", "Ultimate" };
//        const char* diffnames_cn[] = { "普通", "困难", "极难", "极限" };
//        const char* secidnames[] = { "Viridia", "Greenill", "Skyly", "Bluefull", "Purplenum", "Pinkal", "Redria", "Oran", "Yellowboze", "Whitill" };
//        const char* secidnames_cn[] = { "Viridia", "Greenill", "Skyly", "Bluefull", "Purplenum", "Pinkal", "Redria", "Oran", "Yellowboze", "Whitill" };
//
//        // const char *mobnames_ep1[] = {"Hildebear", "Hildeblue", "Mothmant", "Monest", "Rag Rappy", "Al Rappy", "Savage Wolf", "Barbarous Wolf", "Booma",
//        //                               "Gobooma", "Gigobooma", "Grass Assassin", "Poison Lily", "Nar Lily", "Nano Dragon", "Evil Shark", "Pal Shark", 
//        //                               "Guil Shark", "Poifuilly Slime", "Pouilly Slime", "Pan Arms", "Migium", "Hidoom", "Dubchic", "Garanz", "Sinow Beat", 
//        //                               "Sinow Gold", "Canadine", "Canane", "Delsaber", "Chaos Sorcerer", "BEE R", "BEE L", "Dark Gunner", "Death Gunner", 
//        //                               "Chaos Bringer", "Dark Belra", "Claw", "Bulk", "Bulkclaw", "Dimenian", "La Dimenian", "So Dimenian", "Dragon", 
//        //                               "De Rol Le", "Vol Opt", "Dark Falz", "Container", "Dubwitch", "Gilchic"};
//
//        const char* mobnames[] = { "Hildebear", "Hildeblue", "Mothmant", "Monest", "Rag Rappy", "Al Rappy", "Savage Wolf", "Barbarous Wolf", "Booma", "Gobooma",
//                                  "Gigobooma", "Grass Assassin", "Poison Lily", "Nar Lily", "Nano Dragon", "Evil Shark", "Pal Shark", "Guil Shark", "Poifuilly Slime",
//                                  "Pouilly Slime", "Pan Arms", "Migium", "Hidoom", "Dubchic", "Garanz", "Sinow Beat", "Sinow Gold", "Canadine", "Canane", "Delsaber",
//                                  "Chaos Sorcerer", "BEE R", "BEE L", "Dark Gunner", "Death Gunner", "Chaos Bringer", "Dark Belra", "Claw", "Bulk", "Bulkclaw",
//                                  "Dimenian", "La Dimenian", "So Dimenian", "Dragon", "De Rol Le", "Vol Opt", "Dark Falz", "Container", "Dubwitch", "Gilchic",
//                                  "Love Rappy", "Merillia", "Meriltas", "Gee", "Gig Gue", "Mericarol", "Merikle", "Mericus", "Ul Gibbon", "Zol Gibbon", "Gibbles",
//                                  "Sinow Berill", "Sinow Spigell", "Dolmolm", "Dolmdarl", "Morfos", "Recobox", "Recon", "Sinow Zoa", "Sinow Zele", "Deldepth",
//                                  "Delbiter", "Barba Ray", "Pig Ray", "Ul Ray", "Gol Dragon", "Gal Gryphon", "Olga Flow", "St. Rappy", "Halo Rappy", "Egg Rappy",
//                                  "Ill Gill", "Del Lily", "Epsilon", "Gael", "Giel", "Epsigard", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A" };
//
//        const char* mobnames_ult[] = {
//                                  "Hildelt", "Hildetorr", "Mothvert", "Mothvist", "El Rappy", "Pal Rappy", "Gulgus", "Gulgus-gue", "Bartle", "Barble",
//                                  "Tollaw", "Crimson Assassin", "Ob Lily", "Mil Lily", "Nano Dragon", "Vulmer", "Govulmer", "Melqueek", "Poifuilly Slime",
//                                  "Pouilly Slime", "Pan Arms", "Migium", "Hidoom", "Dubchich", "Baranz", "Sinow Blue", "Sinow Red", "Canabin", "Canune", "Delsaber",
//                                  "Grand Sorcerer", "BEE R", "BEE L", "Dark Gunner", "Death Gunner", "Dark Bringer", "Indi Belra", "Claw", "Bulk", "Bulkclaw",
//                                  "Arlan", "Merlan", "Del-D", "Sil Dragon", "Dal Ra Lie", "Vol Opt ver. 2", "Dark Falz", "Container", "Dubwitch", "Gilchich",
//                                  "Love Rappy", "Merillia", "Meriltas", "Gee", "Gig Gue", "Mericarol", "Merikle", "Mericus", "Ul Gibbon", "Zol Gibbon", "Gibbles",
//                                  "Sinow Berill", "Sinow Spigell", "Dolmolm", "Dolmdarl", "Morfos", "Recobox", "Recon", "Sinow Zoa", "Sinow Zele", "Deldepth",
//                                  "Delbiter", "Barba Ray", "Pig Ray", "Ul Ray", "Gol Dragon", "Gal Gryphon", "Olga Flow", "St. Rappy", "Halo Rappy", "Egg Rappy",
//                                  "Ill Gill", "Del Lily", "Epsilon", "Gael", "Giel", "Epsigard", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A" };
//
//        const char* mobnames_ep4[] = {
//                                  "Astark", "Yowie", "Satellite Lizard", "Merissa A", "Merissa AA", "Girtablulu", "Zu", "Pazuzu", "Boota", "Za Boota",
//                                  "Ba Boota", "Dorphon", "Dorphon Eclair", "Goran", "Goran Detonator", "Pyro Goran", "Sand Rappy", "Del Rappy", "Saint Million",
//                                  "Shambertin", "Kondrieu", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
//                                  "N/A", "N/A", "N/A", "N/A" };
//
//        fprintf(enemy_data, "章节\t");
//        fprintf(enemy_data, "章节命名\t");
//        fprintf(enemy_data, "难度\t");
//        fprintf(enemy_data, "难度命名\t");
//        fprintf(enemy_data, "难度命名(中文)\t");
//        fprintf(enemy_data, "颜色ID\t");
//        fprintf(enemy_data, "颜色ID命名\t");
//        fprintf(enemy_data, "颜色ID命名(中文)\t");
//        fprintf(enemy_data, "怪物ID\t");
//        fprintf(enemy_data, "怪物命名\t");
//        fprintf(enemy_data, "物品ID (Hex)\t");
//        fprintf(enemy_data, "掉率\n");
//
//        fprintf(box_data, "章节\t");
//        fprintf(box_data, "章节命名\t");
//        fprintf(box_data, "难度\t");
//        fprintf(box_data, "难度命名\t");
//        fprintf(box_data, "难度命名(中文)\t");
//        fprintf(box_data, "颜色ID\t");
//        fprintf(box_data, "颜色ID命名\t");
//        fprintf(box_data, "颜色ID命名(中文)\t");
//        fprintf(box_data, "箱子索引\t");
//        fprintf(box_data, "区域\t");
//        fprintf(box_data, "物品ID (Hex)\t");
//        fprintf(box_data, "掉率\n");
//
//        for (ep = 0; ep < 4; ep++) // episode
//        {
//            for (diff = 0; diff < 4; diff++) // difficulty
//            {
//                for (secid = 0; secid < 10; secid++) // sec id
//                {
//                    for (mobid = 0; mobid < (ep == 3 ? 22 : ep == 0 ? 50 : 100); mobid++) // monster id
//                    {
//                        BYTE mobitemcode[3];
//                        memcpy(mobitemcode, dropdata_v3.enemy[ep][diff][secid][mobid].itemcode, 3);
//                        BYTE enemyprob = dropdata_v3.enemy[ep][diff][secid][mobid].probability;
//
//                        fprintf(enemy_data, "%d\t", ep);
//                        fprintf(enemy_data, "Episode %s\t", episodenames[ep]);
//                        fprintf(enemy_data, "%d\t", diff);
//                        fprintf(enemy_data, "%s\t", diffnames[diff]);
//                        fprintf(enemy_data, "%s\t", diffnames_cn[diff]);
//                        fprintf(enemy_data, "%d\t", secid);
//                        fprintf(enemy_data, "%s\t", secidnames[secid]);
//                        fprintf(enemy_data, "%s\t", secidnames_cn[secid]);
//                        fprintf(enemy_data, "%d\t", mobid);
//                        fprintf(enemy_data, "%s\t", ep == 3 ? mobnames_ep4[mobid] : ep == 0 ? mobnames[mobid] : mobnames_ult[mobid]);
//                        fprintf(enemy_data, "0x%02X%02X%02X\t", mobitemcode[0], mobitemcode[1], mobitemcode[2]);
//                        fprintf(enemy_data, "%d\n", enemyprob);
//                    }
//
//                    for (boxid = 0; boxid < 30; boxid++) // box id
//                    {
//                        BYTE boxitemcode[3];
//                        memcpy(boxitemcode, dropdata_v3.box[ep][diff][secid][boxid].itemcode, 3);
//                        BYTE boxprob = dropdata_v3.box[ep][diff][secid][boxid].probability;
//                        BYTE area = dropdata_v3.area[ep][diff][secid][boxid];
//
//                        fprintf(box_data, "%d\t", ep);
//                        fprintf(box_data, "Episode %s\t", episodenames[ep]);
//                        fprintf(box_data, "%d\t", diff);
//                        fprintf(box_data, "%s\t", diffnames[diff]);
//                        fprintf(box_data, "%s\t", diffnames_cn[diff]);
//                        fprintf(box_data, "%d\t", secid);
//                        fprintf(box_data, "%s\t", secidnames[secid]);
//                        fprintf(box_data, "%s\t", secidnames_cn[secid]);
//                        fprintf(box_data, "%d\t", boxid);
//                        fprintf(box_data, "%d\t", area);
//                        fprintf(box_data, "0x%02X%02X%02X\t", boxitemcode[0], boxitemcode[1], boxitemcode[2]);
//                        fprintf(box_data, "%d\n", boxprob);
//                    }
//                }
//            }
//        }
//
//        fclose(enemy_data);
//        fclose(box_data);
//    }
//    else
//    {
//        printf("Error while reading ItemRT.gsl");
//        return EXIT_FAILURE;
//    }
//
//    return EXIT_SUCCESS;
//}