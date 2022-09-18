/*
    �λ�֮���й� ����������
    ��Ȩ (C) 2022 Sancaros

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

#include <mbstring.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <tchar.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <direct.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <pthread.h>
#include <signal.h>
#include <Software_Defines.h>
#include <WinSock_Defines.h>
#include <windows.h>
#include <urlmon.h>
#include <mtwist.h>

#include <gnutls/gnutls.h>

#include <psoconfig.h>
#include <f_logs.h>
#include <debug.h>
#include <f_iconv.h>
#include <libxml/parser.h>

#include "ship.h"
#include "clients.h"
#include "shipgate.h"
#include "utils.h"
#include "scripts.h"
#include "mapdata.h"
#include "ptdata.h"
#include "pmtdata.h"
#include "rtdata.h"
#include "shopdata.h"
#include "admin.h"
#include "smutdata.h"

#ifndef RUNAS_DEFAULT
#define RUNAS_DEFAULT "psocn"
#endif

#define MYWM_NOTIFYICON (WM_USER+2)
int32_t program_hidden = 1;
HWND consoleHwnd;
uint32_t window_hide_or_show = 1;
// 1024 lines which can carry 64 parameters each 1024�� ÿ�п��Դ��64������
uint32_t csv_lines = 0;
// Release RAM from loaded CSV  �Ӽ��ص�CSV���ͷ�RAM 
char* csv_params[1024][64]; 

/* The actual ship structures. */
ship_t *ship;
int enable_ipv6 = 1;
int restart_on_shutdown = 0;
uint32_t ship_ip4;
uint8_t ship_ip6[16];

/* TLS stuff */
gnutls_anon_client_credentials_t anoncred;
//gnutls_certificate_credentials_t tls_cred;
gnutls_priority_t tls_prio;
//static gnutls_dh_params_t dh_params;

static const char *config_file = NULL;
static const char *custom_dir = NULL;
static int dont_daemonize = 0;
static int check_only = 0;
static const char *pidfile_name = NULL;
//static struct pidfh *pf = NULL;
static const char *runas_user = RUNAS_DEFAULT;

/* Print help to the user to stdout. */
static void print_help(const char *bin) {
    printf("����˵��: %s [arguments]\n"
           "-----------------------------------------------------------------\n"
           "--version       Print version info and exit\n"
           "--verbose       Log many messages that might help debug a problem\n"
           "--quiet         Only log warning and error messages\n"
           "--reallyquiet   Only log error messages\n"
           "-C configfile   Use the specified configuration instead of the\n"
           "                default one.\n"
           "-D directory    Use the specified directory as the root\n"
           "--nodaemon      Don't daemonize\n"
#ifdef PSOCN_ENABLE_IPV6
           "--no-ipv6       Disable IPv6 support for incoming connections\n"
#endif
           "--check-config  Load and parse the configuration, but do not\n"
           "                actually start the ship server. This implies the\n"
           "                --nodaemon option as well.\n"
           "-P filename     Use the specified name for the pid file to write\n"
           "                instead of the default.\n"
           "-U username     Run as the specified user instead of '%s'\n"
           "--help          Print this help and exit\n\n"
           "Note that if more than one verbosity level is specified, the last\n"
           "one specified will be used. The default is --verbose.\n", bin,
           RUNAS_DEFAULT);
}

/* Parse any command-line arguments passed in. */
static void parse_command_line(int argc, char *argv[]) {
    int i;

    for(i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "--version")) {
            load_program_info(SHIPS_SERVER_NAME, SHIPS_SERVER_VERSION);
            exit(0);
        }
        else if(!strcmp(argv[i], "--verbose")) {
            debug_set_threshold(DBG_LOGS);
        }
        else if(!strcmp(argv[i], "--quiet")) {
            debug_set_threshold(DBG_WARN);
        }
        else if(!strcmp(argv[i], "--reallyquiet")) {
            debug_set_threshold(DBG_ERROR);
        }
        else if(!strcmp(argv[i], "-C")) {
            /* Save the config file's name. */
            if(i == argc - 1) {
                print_help(argv[0]);
                ERR_EXIT("-C ȱ�ٲ���!");
            }

            config_file = argv[++i];
        }
        else if(!strcmp(argv[i], "-D")) {
            /* Save the custom dir */
            if(i == argc - 1) {
                print_help(argv[0]);
                ERR_EXIT("-D ȱ�ٲ���!");
            }

            custom_dir = argv[++i];
        }
        else if(!strcmp(argv[i], "--nodaemon")) {
            dont_daemonize = 1;
        }
        else if(!strcmp(argv[i], "--no-ipv6")) {
            enable_ipv6 = 0;
        }
        else if(!strcmp(argv[i], "--check-config")) {
            check_only = 1;
            dont_daemonize = 1;
        }
        else if(!strcmp(argv[i], "-P")) {
            if(i == argc - 1) {
                print_help(argv[0]);
                ERR_EXIT("-P ȱ�ٲ���!");
            }

            pidfile_name = argv[++i];
        }
        else if(!strcmp(argv[i], "-U")) {
            if(i == argc - 1) {
                print_help(argv[0]);
                ERR_EXIT("-U ȱ�ٲ���!");
            }

            runas_user = argv[++i];
        }
        else if(!strcmp(argv[i], "--help")) {
            print_help(argv[0]);
            ERR_EXIT("Illegal command line argument: %s\n", argv[i]);
        }
        else {
            print_help(argv[0]);
            ERR_EXIT("Illegal command line argument: %s\n", argv[i]);
        }
    }
}

/* Load the configuration file and print out parameters with DBG_LOG. */
static psocn_ship_t *load_config(void) {
    psocn_ship_t *cfg;

    if(psocn_read_ship_config(psocn_ship_cfg, &cfg))
        ERR_EXIT("�޷���ȡ�����ļ� %s", psocn_ship_cfg);

    return cfg;
}

//�ͷ�CSV����ռ�õ��ڴ�
void FreeCSV()
{
    uint32_t ch, ch2;

    for (ch = 0; ch < csv_lines; ch++)
    {
        for (ch2 = 0; ch2 < 64; ch2++)
            if (csv_params[ch][ch2] != NULL) free(csv_params[ch][ch2]);
    }
    csv_lines = 0;
    memset(&csv_params, 0, sizeof(csv_params));
}

//����CSV�������ڴ�
void LoadCSV(const char* filename)
{
    FILE* fp;
    char csv_data[1024] = { 0 };
    uint32_t ch, ch2, ch3 = 0;
    //uint32_t ch4;
    int32_t open_quote = 0;
    char* csv_param;

    csv_lines = 0;
    memset(&csv_params, 0, sizeof(csv_params));
    //printf ("Loading CSV file %s ...\n", filename );

    if ((fp = fopen(filename, "r")) == NULL)
    {
        ERR_EXIT("�����ļ� %s �ƺ�ȱʧ��.", filename);
    }

    while (fgets(&csv_data[0], 1023, fp) != NULL)
    {
        // ch2 = current parameter we're on
        // ch3 = current index into the parameter string
        ch2 = ch3 = 0;
        open_quote = 0;
        csv_param = csv_params[csv_lines][0] = malloc(256); // allocate memory for parameter

        if (csv_param) {
            for (ch = 0; ch < strlen(&csv_data[0]); ch++)
            {
                if ((csv_data[ch] == 44) && (!open_quote)) // comma not surrounded by quotations
                {
                    csv_param[ch3] = 0; // null terminate current parameter
                    ch3 = 0;
                    ch2++; // new parameter
                    csv_param = csv_params[csv_lines][ch2] = malloc(256); // allocate memory for parameter
                }
                else
                {
                    if (csv_data[ch] == 34) // quotation mark
                        open_quote = !open_quote;
                    else
                        if (csv_data[ch] > 31) // no loading low ascii
                            csv_param[ch3++] = csv_data[ch];
                }
            }
        }
        if (ch3)
        {
            ch2++;
            csv_param[ch3] = 0;
        }
        /*
        for (ch4=0;ch4<ch2;ch4++)
        printf ("%s,", csv_params[csv_lines][ch4]);
        printf ("\n");
        */
        csv_lines++;
        if (csv_lines > 1023)
            ERR_EXIT("CSV �ļ�����Ŀ̫��.");
    }

    fclose(fp);
}

uint8_t hexToByte(char* hs)
{
    uint32_t b;

    if (hs[0] < 58) b = (hs[0] - 48); else b = (hs[0] - 55);
    b *= 16;
    if (hs[1] < 58) b += (hs[1] - 48); else b += (hs[1] - 55);
    return (uint8_t)b;
}

//���ؿ��ײ���
void Load_Armor_Param(uint32_t file)
{
    uint32_t ch, wi1;

    LoadCSV(ItemPMT_Files[file]);
    /*���Ǹ��ļ��Ĳο���ʽ
typedef struct NO_ALIGN armor_pmt {
             ģ��                                0x010100 Frame����
    1 uint32_t index;                             640
    2 int16_t model;                              -1
    3 int16_t texture;                            -1
    4 int32_t team_points;                        0
    5 int16_t dfp;                                5
    6 int16_t evp;                                5
    7 uint8_t block_particle;                     0
    8 uint8_t block_effect;                       0
    9 uint8_t quipclass; //�ɴ���ְҵ����         255
    10 uint8_t reserved1;
    11 uint8_t requiredlevel;                     0
    12 uint8_t efr;                               5
    13 uint8_t eth;                               0
    14 uint8_t eic;                               0
    15 uint8_t edk;                               5
    16 uint8_t elt;                               0
    17 uint8_t dfp_range;                         2
    18 uint8_t evp_range;                         2
    19 uint8_t stat_boost;                        0
    20 uint8_t tech_boost;                        0
    21 uint8_t unknown1;                          0
    22 uint8_t unknown1;                          0
    23 uint8_t star;                              0
}PMT_ARMOR;
    * 0         1     2   3    4        5         6  7  8  9  10  11 12 13 14 15 16 17 18 19 20 21 22 23
    *         name  index model texture team_ps dfp evp
    0x010100, Frame, 640, -1, -1,       0,        5, 5, 0, 0, 255, 0, 5, 0, 0, 5, 0, 2, 2, 0, 0, 0, 0, 0
    0x010101, Armor, 641, -1, -1,       0,        7, 7, 0, 0, 251, 3, 0, 0, 5, 0, 0, 2, 2, 0, 0, 0
    �ȼ���Ҫ+1�ŶԵģ���Ϊ��0��ʼ������1��
    */
    Item_Pmt_Count.armor_count = csv_lines;

    for (ch = 0; ch < csv_lines; ch++)
    {
        wi1 = hexToByte(&csv_params[ch][0][6]); //index
        pmt_armor_bb[wi1]->index = (uint32_t)atoi(csv_params[ch][2]);
        pmt_armor_bb[wi1]->model = (uint16_t)atoi(csv_params[ch][3]);
        pmt_armor_bb[wi1]->skin = (uint16_t)atoi(csv_params[ch][4]);
        pmt_armor_bb[wi1]->team_points = (uint32_t)atoi(csv_params[ch][5]);
        pmt_armor_bb[wi1]->base_dfp = (uint16_t)atoi(csv_params[ch][6]);
        pmt_armor_bb[wi1]->base_evp = (uint16_t)atoi(csv_params[ch][7]);
        pmt_armor_bb[wi1]->block_particle = (uint8_t)atoi(csv_params[ch][8]);
        pmt_armor_bb[wi1]->block_effect = (uint8_t)atoi(csv_params[ch][9]);
        pmt_armor_bb[wi1]->equip_flag = (uint8_t)atoi(csv_params[ch][10]);
        pmt_armor_bb[wi1]->level_req = (uint8_t)atoi(csv_params[ch][11]);
        pmt_armor_bb[wi1]->efr = (uint8_t)atoi(csv_params[ch][12]);
        pmt_armor_bb[wi1]->eth = (uint8_t)atoi(csv_params[ch][13]);
        pmt_armor_bb[wi1]->eic = (uint8_t)atoi(csv_params[ch][14]);
        pmt_armor_bb[wi1]->edk = (uint8_t)atoi(csv_params[ch][15]);
        pmt_armor_bb[wi1]->elt = (uint8_t)atoi(csv_params[ch][16]);
        pmt_armor_bb[wi1]->dfp_range = (uint8_t)atoi(csv_params[ch][17]);
        pmt_armor_bb[wi1]->evp_range = (uint8_t)atoi(csv_params[ch][18]);
        pmt_armor_bb[wi1]->stat_boost = (uint8_t)atoi(csv_params[ch][19]);
        pmt_armor_bb[wi1]->tech_boost = (uint8_t)atoi(csv_params[ch][20]);
        pmt_armor_bb[wi1]->unknown1 = (uint8_t)atoi(csv_params[ch][21]);
        //pmt_armor_bb[wi1]->unknown2 = (uint8_t)atoi(csv_params[ch][22]); todo 1126
        //pmt_armor_bb[wi1]->star = (uint8_t)atoi(csv_params[ch][23]); todo 1126
        //printf("�������� %s index %d texture %u model %u team_points %u\n", csv_params[ch][1],
            //pmt_armor_bb[wi1]->index, pmt_armor_bb[wi1]->model, pmt_armor_bb[wi1]->texture,
            //pmt_armor_bb[wi1]->team_points
        //);
    }
    FreeCSV();
    //printf("���ؿ��ײ��� %d ��\n", Item_Pmt_Count.armor_count);
}

//���ػ��ܲ���
void Load_Shield_Param(uint32_t file)
{
    uint32_t ch, wi1;

    LoadCSV(ItemPMT_Files[file]);

    Item_Pmt_Count.shield_count = csv_lines;

    for (ch = 0; ch < csv_lines; ch++)
    {
        wi1 = hexToByte(&csv_params[ch][0][6]);
        pmt_shield_bb[wi1]->index = (uint32_t)atoi(csv_params[ch][2]);
        pmt_shield_bb[wi1]->model = (uint16_t)atoi(csv_params[ch][3]);
        pmt_shield_bb[wi1]->skin = (uint16_t)atoi(csv_params[ch][4]);
        pmt_shield_bb[wi1]->team_points = (uint32_t)atoi(csv_params[ch][5]);
        pmt_shield_bb[wi1]->base_dfp = (uint16_t)atoi(csv_params[ch][6]);
        pmt_shield_bb[wi1]->base_evp = (uint16_t)atoi(csv_params[ch][7]);
        pmt_shield_bb[wi1]->block_particle = (uint8_t)atoi(csv_params[ch][8]);
        pmt_shield_bb[wi1]->block_effect = (uint8_t)atoi(csv_params[ch][9]);
        pmt_shield_bb[wi1]->equip_flag = (uint8_t)atoi(csv_params[ch][10]);
        pmt_shield_bb[wi1]->level_req = (uint8_t)atoi(csv_params[ch][11]);
        pmt_shield_bb[wi1]->efr = (uint8_t)atoi(csv_params[ch][12]);
        pmt_shield_bb[wi1]->eth = (uint8_t)atoi(csv_params[ch][13]);
        pmt_shield_bb[wi1]->eic = (uint8_t)atoi(csv_params[ch][14]);
        pmt_shield_bb[wi1]->edk = (uint8_t)atoi(csv_params[ch][15]);
        pmt_shield_bb[wi1]->elt = (uint8_t)atoi(csv_params[ch][16]);
        pmt_shield_bb[wi1]->dfp_range = (uint8_t)atoi(csv_params[ch][17]);
        pmt_shield_bb[wi1]->evp_range = (uint8_t)atoi(csv_params[ch][18]);
        pmt_shield_bb[wi1]->stat_boost = (uint8_t)atoi(csv_params[ch][19]);
        pmt_shield_bb[wi1]->tech_boost = (uint8_t)atoi(csv_params[ch][20]);
        pmt_shield_bb[wi1]->unknown1 = (uint8_t)atoi(csv_params[ch][21]);
        //pmt_shield_bb[wi1]->unknown2 = (uint8_t)atoi(csv_params[ch][22]); todo 1126
        //pmt_shield_bb[wi1]->star = (uint8_t)atoi(csv_params[ch][23]); todo 1126
        //printf("�������� %s index %d texture %u model %u team_points %u\n", csv_params[ch][1],
            //pmt_shield_bb[wi1]->index, pmt_shield_bb[wi1]->model, pmt_shield_bb[wi1]->texture,
            //pmt_shield_bb[wi1]->team_points
        //);
    }
    FreeCSV();
    //printf("���ػ��ܲ��� %d ��\n", Item_Pmt_Count.shield_count);

}

//������������
void Load_Weapon_Param(uint32_t file)
{
    uint32_t ch, wi1, wi2;

    LoadCSV(ItemPMT_Files[file]);
    Item_Pmt_Count.weapon_count = csv_lines;
    /*�����ļ��ο�����
    *
    typedef struct NO_ALIGN weapon_pmt {
        1 uint32_t index;
        2 int16_t model;
        3 int16_t texture;
        4 int32_t team_points;
        5 uint8_t equip_class; //�ɴ���ְҵ����
        6 uint8_t reserved1;
        7 int16_t atpmin;
        8 int16_t atpmax;
        9 int16_t atpreq;
        10 int16_t mstreq;
        11 int16_t atareq;
        12 int16_t mstadd;
        13 uint8_t max_grind;
        14 int8_t photon_color;
        15 uint8_t special_type;
        16 uint8_t ataadd;
        17 uint8_t stat_boost;
        18 uint8_t projectile;
        19 int8_t photon_trail_1_x;
        20 int8_t photon_trail_1_y;
        21 int8_t photon_trail_2_x;
        22 int8_t photon_trail_2_y;
        23 int8_t photon_type;
        24 int8_t unknown1;
        25 int8_t unknown2;
        26 int8_t unknown3;
        27 int8_t unknown4;
        28 int8_t unknown5;
        29 uint8_t tech_boost;
        30 uint8_t combo_type;
        31 uint8_t star;
    }PMT_WEAPON;
    * 0         1     2      3      4      5       6  7      8      9 10 11  12 13 14 15 16 17 18 19 20 21 22 23
    *         name  index model texture team_ps clss minatp maxatp
    0x000000, Saber, 177,   -1,    -1,     0,    255, 5,     7,     0, 0, 60, 0, 0, 0, -1, 0, 10, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0
    0x000100, Saber, 177, 0, 0, 0, 255, 40, 55, 30, 0, 0, 0, 0, 35, 0, 0, 30, 0, 0, 1, 2, -1, -1, 1, 1, -1, -1, 0, 0, 0, 0

    */
    for (ch = 0; ch < csv_lines; ch++)
    {
        wi1 = hexToByte(&csv_params[ch][0][4]);
        wi2 = hexToByte(&csv_params[ch][0][6]);
        pmt_weapon_bb[wi1][wi2]->index = (uint32_t)atoi(csv_params[ch][2]);
        pmt_weapon_bb[wi1][wi2]->model = (uint16_t)atoi(csv_params[ch][3]);
        pmt_weapon_bb[wi1][wi2]->skin = (uint16_t)atoi(csv_params[ch][4]);
        pmt_weapon_bb[wi1][wi2]->team_points = (uint32_t)atoi(csv_params[ch][5]);
        pmt_weapon_bb[wi1][wi2]->equip_flag = (uint8_t)atoi(csv_params[ch][6]);
        pmt_weapon_bb[wi1][wi2]->atp_min = (uint16_t)atoi(csv_params[ch][7]);
        pmt_weapon_bb[wi1][wi2]->atp_max = (uint16_t)atoi(csv_params[ch][8]);
        pmt_weapon_bb[wi1][wi2]->atp_req = (uint16_t)atoi(csv_params[ch][9]);
        pmt_weapon_bb[wi1][wi2]->mst_req = (uint16_t)atoi(csv_params[ch][10]);
        pmt_weapon_bb[wi1][wi2]->ata_req = (uint16_t)atoi(csv_params[ch][11]);
        pmt_weapon_bb[wi1][wi2]->mst_add = (uint16_t)atoi(csv_params[ch][12]);
        pmt_weapon_bb[wi1][wi2]->min_grind = (uint8_t)atoi(csv_params[ch][13]);
        pmt_weapon_bb[wi1][wi2]->max_grind = (uint8_t)atoi(csv_params[ch][14]);
        pmt_weapon_bb[wi1][wi2]->photon_color = (int8_t)atoi(csv_params[ch][15]);
        if ((((wi1 >= 0x70) && (wi1 <= 0x88)) ||
            ((wi1 >= 0xA5) && (wi1 <= 0xA9))) &&
            (wi2 == 0x10))
            pmt_weapon_bb[wi1][wi2]->special_type = 0x0B; // Fix-up S-Rank King's special
        else
            pmt_weapon_bb[wi1][wi2]->special_type = (uint8_t)atoi(csv_params[ch][16]);
        pmt_weapon_bb[wi1][wi2]->ata_add = (uint8_t)atoi(csv_params[ch][17]);
        pmt_weapon_bb[wi1][wi2]->stat_boost = (uint8_t)atoi(csv_params[ch][18]);
        pmt_weapon_bb[wi1][wi2]->projectile = (uint8_t)atoi(csv_params[ch][19]);
        pmt_weapon_bb[wi1][wi2]->ptrail_1_x = (uint8_t)atoi(csv_params[ch][20]);
        pmt_weapon_bb[wi1][wi2]->ptrail_1_y = (uint8_t)atoi(csv_params[ch][21]);
        pmt_weapon_bb[wi1][wi2]->ptrail_2_x = (uint8_t)atoi(csv_params[ch][22]);
        pmt_weapon_bb[wi1][wi2]->ptrail_2_y = (uint8_t)atoi(csv_params[ch][23]);
        pmt_weapon_bb[wi1][wi2]->ptype = (uint8_t)atoi(csv_params[ch][24]);
        pmt_weapon_bb[wi1][wi2]->unknown1 = (uint8_t)atoi(csv_params[ch][25]);
        pmt_weapon_bb[wi1][wi2]->unknown2 = (uint8_t)atoi(csv_params[ch][26]);
        pmt_weapon_bb[wi1][wi2]->unknown3 = (uint8_t)atoi(csv_params[ch][27]);
        pmt_weapon_bb[wi1][wi2]->unknown4 = (uint8_t)atoi(csv_params[ch][28]);
        pmt_weapon_bb[wi1][wi2]->unknown5 = (uint8_t)atoi(csv_params[ch][29]);
        pmt_weapon_bb[wi1][wi2]->tech_boost = (uint8_t)atoi(csv_params[ch][30]);
        pmt_weapon_bb[wi1][wi2]->combo_type = (uint8_t)atoi(csv_params[ch][31]);
        //pmt_weapon_bb[wi1][wi2]->star = (uint8_t)atoi(csv_params[ch][32]);
        //*(uint16_t*)&weapon_atpmax_table[wi1][wi2] = (uint32_t)atoi(csv_params[ch][8]);
        //grind_table[wi1][wi2] = (uint8_t)atoi(csv_params[ch][14]);
        //printf ("�������� %02x%02x, eq: %u, grind: %u, grind2: %u, mstadd %u, atpmax: %u, special: %u \n", wi1, wi2, pmt_weapon_bb[wi1][wi2]->index, grind_table[wi1][wi2], pmt_weapon_bb[wi1][wi2]->max_grind, pmt_weapon_bb[wi1][wi2]->mstadd, pmt_weapon_bb[wi1][wi2]->atpmax, special_table[wi1][wi2]);
    }
    FreeCSV();
    //printf("������������ %d ��\n", Item_Pmt_Count.weapon_count);
}

//����ħ���Ƽ�����
void Load_Tech_Param(uint32_t file)
{
    uint32_t ch, ch2;

    LoadCSV(ItemPMT_Files[file]);
    /*���ڲο�
    * �������ڲο����������ȡ
    Foie,15,20,0,15,0,0,30,30,30,0,30,20
    */
    if (csv_lines != BB_MAX_TECH_LEVEL)
        ERR_EXIT("�Ƽ� tech.ini �ļ�CSV��������.");

    for (ch = 0; ch < BB_MAX_TECH_LEVEL; ch++) // ch = ���ֿƼ�
    {
        for (ch2 = 0; ch2 < BB_MAX_CLASS; ch2++) // ch2 = ����ְҵ
        {
            if (csv_params[ch][ch2 + 1]) {
                max_tech_level[ch][ch2] = ((char)atoi(csv_params[ch][ch2 + 1])) - 1;
                //printf("�Ƽ� %d ְҵ %d �Ƽ����� %u.\n", ch, ch2, max_tech_level[ch][ch2]);
            }
            else
                ERR_EXIT("�Ƽ� tech.ini �ļ�CSV��������.");
        }
    }
    FreeCSV();
    //printf("���ؿƼ����� %d ��ְҵ %d ��\n", ch2, ch);
}

static int init_gnutls() {
    int rv;

    if (gnutls_check_version("3.1.4") == NULL) {
        fprintf(stderr, "GnuTLS 3.1.4 or later is required for this example\n");
        return -1;
    }

    /* Do the global init */
    gnutls_global_init();

    /* Set up our credentials */
    if ((rv = gnutls_anon_allocate_client_credentials(&anoncred))) {
        ERR_LOG(
            "�޷�Ϊ���� GnuTLS �����ڴ�: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    /* Set our priorities */
    if ((rv = gnutls_priority_init(&tls_prio, "PERFORMANCE:+ANON-ECDH:+ANON-DH", NULL))) {
        ERR_LOG(
            "�޷���ʼ�� GnuTLS ����Ȩ: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    return 0;
}

static void cleanup_gnutls() {
    //gnutls_dh_params_deinit(dh_params);
    gnutls_anon_free_client_credentials(anoncred);
    gnutls_priority_deinit(tls_prio);
    gnutls_global_deinit();
}

static int setup_addresses(psocn_ship_t* cfg) {
    struct addrinfo hints;
    struct addrinfo* server, * j;
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in* addr4;
    struct sockaddr_in6* addr6;

    /* Clear the addresses */
    ship_ip4 = 0;
    cfg->ship_ip4 = 0;
    memset(ship_ip6, 0, 16);
    memset(cfg->ship_ip6, 0, 16);

    SHIPS_LOG("��ȡ������ַ...");

    //CONFIG_LOG("���������ȡ: %s", host4);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(cfg->ship_host4, "9000", &hints, &server)) {
        ERR_LOG("��Ч������ַ: %s", cfg->ship_host4);
        return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET) {
            addr4 = (struct sockaddr_in*)j->ai_addr;
            inet_ntop(j->ai_family, &addr4->sin_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv4 ��ַ����: %s", ipstr);
            //    return -1;
            //}
            //else
                SHIPS_LOG("    ��ȡ�� IPv4 ��ַ: %s", ipstr);
            cfg->ship_ip4 = ship_ip4 = addr4->sin_addr.s_addr;
        }
        else if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv6 ��ַ����: %s", ipstr);
            //    //return -1;
            //}
            //else
                SHIPS_LOG("    ��ȡ�� IPv6 ��ַ: %s", ipstr);
            memcpy(ship_ip6, &addr6->sin6_addr, 16);
            memcpy(cfg->ship_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    /* Make sure we found at least an IPv4 address */
    if (!ship_ip4 || !cfg->ship_ip4) {
        ERR_LOG("�޷���ȡ��IPv4��ַ!");
        return -1;
    }

    /* If we don't have a separate IPv6 host set, we're done. */
    if (!cfg->ship_host6) {
        return 0;
    }

    /* Now try with IPv6 only */
    memset(ship_ip6, 0, 16);
    memset(cfg->ship_ip6, 0, 16);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(cfg->ship_host6, "9000", &hints, &server)) {
        ERR_LOG("��Ч������ַ (v6): %s", cfg->ship_host6);
        //return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv6 ��ַ����: %s", ipstr);
            //    //return -1;
            //}
            //else
                SHIPS_LOG("    ��ȡ�� IPv6 ��ַ: %s", ipstr);
            memcpy(ship_ip6, &addr6->sin6_addr, 16);
            memcpy(cfg->ship_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    if (!ship_ip6[0]) {
        ERR_LOG("�޷���ȡ��IPv6��ַ (��������IPv6����)!");
        return -1;
    }

    return 0;
}

static int shipkey_init(psocn_ship_t* cfg) {
    FILE* fp;
    //char Ship_Keys_Dir[255] = "System\\ShipKey\\ship_key.bin";

    //SHIPS_LOG("���ؽ�����Կ %s", &Ship_Keys_Dir[0]);

    errno_t err = fopen_s(&fp, cfg->ship_key, "rb");
    if (err)
    {
        ERR_LOG("δ�ҵ� %s �ļ�!", cfg->ship_key);
        ERR_LOG("���� [�س���] �˳�...");
        gets_s(&dp[0], sizeof(dp));
        return -1;
    }

    fread(&cfg->ship_key_idx, 1, 4, fp);
    fread(&cfg->ship_rc4key[0], 1, 128, fp);
    fclose(fp);

    SHIPS_LOG("���ؽ�����Կ %d �ֽ�", sizeof(cfg->ship_rc4key));

    return 0;
}

static void print_config(psocn_ship_t *cfg) {
    int i;

    /* Print out the configuration. */
    CONFIG_LOG("���ò���:");

    CONFIG_LOG("��������: %s", cfg->shipgate_host);
    CONFIG_LOG("���Ŷ˿�: %d", (int)cfg->shipgate_port);

    /* Print out the ship's information. */
    CONFIG_LOG("��������: %s", cfg->name);

    CONFIG_LOG("���� IPv4 ����: %s", cfg->ship_host4);

    if(cfg->ship_host6)
        CONFIG_LOG("���� IPv6 ����: %s", cfg->ship_host6);
    else
        CONFIG_LOG("���� IPv6 ����: �Զ����û򲻴���");

    CONFIG_LOG("�����˿�: %d", (int)cfg->base_port);
    CONFIG_LOG("��������: %d", cfg->blocks);
    CONFIG_LOG("Ĭ�ϴ�������: %d", cfg->events[0].lobby_event);
    CONFIG_LOG("Ĭ����Ϸ����: %d", cfg->events[0].game_event);

    if(cfg->event_count != 1) {
        for(i = 1; i < cfg->event_count; ++i) {
            CONFIG_LOG("���� (%d-%d �� %d-%d):",
                  cfg->events[i].start_month, cfg->events[i].start_day,
                  cfg->events[i].end_month, cfg->events[i].end_day);
            CONFIG_LOG("\t����: %d, ��Ϸ: %d",
                  cfg->events[i].lobby_event, cfg->events[i].game_event);
        }
    }

    if(cfg->menu_code)
        CONFIG_LOG("�˵�: %c%c", (char)cfg->menu_code,
              (char)(cfg->menu_code >> 8));
    else
        CONFIG_LOG("�˵�: ���˵�");

    if(cfg->v2_map_dir)
        CONFIG_LOG("v2 ��ͼ·��: %s", cfg->v2_map_dir);

    if(cfg->gc_map_dir)
        CONFIG_LOG("GC ��ͼ·��: %s", cfg->bb_map_dir);

    if(cfg->bb_param_dir)
        CONFIG_LOG("BB ����·��: %s", cfg->bb_param_dir);

    if(cfg->v2_param_dir)
        CONFIG_LOG("v2 ����·��: %s", cfg->v2_param_dir);

    if(cfg->bb_map_dir)
        CONFIG_LOG("BB ��ͼ·��: %s", cfg->bb_map_dir);

    if(cfg->v2_ptdata_file)
        CONFIG_LOG("v2 ItemPT �ļ�: %s", cfg->v2_ptdata_file);

    if(cfg->gc_ptdata_file)
        CONFIG_LOG("GC ItemPT �ļ�: %s", cfg->gc_ptdata_file);

    if(cfg->bb_ptdata_file)
        CONFIG_LOG("BB ItemPT �ļ�: %s", cfg->bb_ptdata_file);

    if(cfg->v2_pmtdata_file)
        CONFIG_LOG("v2 ItemPMT �ļ�: %s", cfg->v2_pmtdata_file);

    if(cfg->gc_pmtdata_file)
        CONFIG_LOG("GC ItemPMT �ļ�: %s", cfg->gc_pmtdata_file);

    if(cfg->bb_pmtdata_file)
        CONFIG_LOG("BB ItemPMT �ļ�: %s", cfg->bb_pmtdata_file);

    CONFIG_LOG("װ�� +/- ����: v2: %s, GC: %s, BB: %s",
          (cfg->local_flags & PSOCN_SHIP_PMT_LIMITV2) ? "true" : "false",
          (cfg->local_flags & PSOCN_SHIP_PMT_LIMITGC) ? "true" : "false",
          (cfg->local_flags & PSOCN_SHIP_PMT_LIMITBB) ? "true" : "false");

    if(cfg->v2_rtdata_file)
        CONFIG_LOG("v2 ItemRT �ļ�: %s", cfg->v2_rtdata_file);

    if(cfg->gc_rtdata_file)
        CONFIG_LOG("GC ItemRT �ļ�: %s", cfg->gc_rtdata_file);

    if(cfg->bb_rtdata_file)
        CONFIG_LOG("BB ItemRT �ļ�: %s", cfg->bb_rtdata_file);

    if(cfg->v2_rtdata_file || cfg->gc_rtdata_file || cfg->bb_rtdata_file) {
        CONFIG_LOG("����ϡ�е���: %s",
              (cfg->local_flags & PSOCN_SHIP_QUEST_RARES) ? "true" :
              "false");
        CONFIG_LOG("�����ϡ�е���: %s",
              (cfg->local_flags & PSOCN_SHIP_QUEST_SRARES) ? "true" :
              "false");
    }

    if(cfg->smutdata_file)
        CONFIG_LOG("Smutdata �ļ�: %s", cfg->smutdata_file);

    if(cfg->limits_count) {
        CONFIG_LOG("������ %d /legit ���ļ�:", cfg->limits_count);

        for(i = 0; i < cfg->limits_count; ++i) {
            CONFIG_LOG("%d: \"%s\": %s", i, cfg->limits[i].name,
                  cfg->limits[i].filename);
        }

        CONFIG_LOG("Ĭ�� /legit �ļ�����: %d", cfg->limits_default);
    }

    CONFIG_LOG("���ű�ʶ: 0x%08X", cfg->shipgate_flags);
    CONFIG_LOG("֧�ְ汾:");

    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NODCNTE))
        CONFIG_LOG("Dreamcast Network Trial Edition DC������԰�");
    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NOV1))
        CONFIG_LOG("Dreamcast Version 1 DC�汾 v1");
    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NOV2))
        CONFIG_LOG("Dreamcast Version 2 DC�汾 v2");
    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NOPCNTE))
        CONFIG_LOG("PSO for PC Network Trial Edition PC������԰�");
    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NOPC))
        CONFIG_LOG("PSO for PC PC�����");
    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NOEP12))
        CONFIG_LOG("Gamecube Episode I & II GC�½� 1/2");
    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NOEP3))
        CONFIG_LOG("Gamecube Episode III GC�½� 3");
    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NOPSOX))
        CONFIG_LOG("Xbox Episode I & II XBOX�½� 1/2");
    if(!(cfg->shipgate_flags & SHIPGATE_FLAG_NOBB))
        CONFIG_LOG("Blue Burst ��ɫ����");
}

static void open_log(psocn_ship_t *cfg) {
    char fn[64+ 32];
    FILE *dbgfp;

    sprintf(fn, "Debug\\%s_debug.log", cfg->name);
    errno_t err = fopen_s(&dbgfp, fn, "a");

    if(err) {
        perror("fopen");
        ERR_EXIT("�޷�����־�ļ�");
    }

    debug_set_file(dbgfp);
}

static void reopen_log(void) {
    char fn[64 + 32];
    FILE *dbgfp, *ofp;

    sprintf(fn, "Debug\\%s_debug.log", ship->cfg->name);
    errno_t err = fopen_s(&dbgfp, fn, "a");

    if (err) {
        /* Uhh... Welp, guess we'll try to continue writing to the old one,
           then... */
        perror("fopen");
        ERR_EXIT("�޷����´���־�ļ�");
    }
    else {
        ofp = debug_set_file(dbgfp);
        fclose(ofp);
    }
}

/* Install any handlers for signals we care about */
void handle_signal(int signal) {
    switch (signal) {
#ifdef _WIN32 
    case SIGTERM:
        /* Now, shutdown with slightly more grace! */
        schedule_shutdown(NULL, 0, 0, NULL);
        break;
    case SIGABRT:
    case SIGBREAK:
        break;
#else 
    case SIGHUP:
        reopen_log();
        break;
#endif 
    case SIGINT:
        schedule_shutdown(NULL, 0, 1, NULL);
        break;
    }
}

bool already_hooked_up;

void HookupHandler() {
    if (already_hooked_up) {
        /*SHIPS_LOG(
            "Tried to hookup signal handlers more than once.");
        already_hooked_up = false;*/
    }
    else {
        SHIPS_LOG("%s�������.", SGATE_SERVER_NAME);
        already_hooked_up = true;
    }
#ifdef _WIN32 
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGABRT, handle_signal);
#else 
    struct sigaction sa;
    // Setup the handler 
    sa.sa_handler = &handle_signal;
    // Restart the system call, if at all possible 
    sa.sa_flags = SA_RESTART;
    // Block every signal during the handler 
    sigfillset(&sa.sa_mask);
    // Intercept SIGHUP and SIGINT 
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        SHIPS_LOG(
            "Cannot install SIGHUP handler.");
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        SHIPS_LOG(
            "Cannot install SIGINT handler.");
    }
#endif 
}

void UnhookHandler() {
    if (already_hooked_up) {
#ifdef _WIN32 
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
#else 
        struct sigaction sa;
        // Setup the sighub handler 
        sa.sa_handler = SIG_DFL;
        // Restart the system call, if at all possible 
        sa.sa_flags = SA_RESTART;
        // Block every signal during the handler 
        sigfillset(&sa.sa_mask);
        // Intercept SIGHUP and SIGINT 
        if (sigaction(SIGHUP, &sa, NULL) == -1) {
            SHIPS_LOG(
                "Cannot uninstall SIGHUP handler.");
        }
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            SHIPS_LOG(
                "Cannot uninstall SIGINT handler.");
        }
#endif 

        already_hooked_up = false;
    }
}

//�����ص�ר��
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == MYWM_NOTIFYICON)
    {
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
            switch (wParam)
            {
            case 100:
                if (program_hidden)
                {
                    program_hidden = 0;
                    ShowWindow(consoleHwnd, SW_NORMAL);
                    SetForegroundWindow(consoleHwnd);
                    SetFocus(consoleHwnd);
                }
                else
                {
                    program_hidden = 1;
                    ShowWindow(consoleHwnd, SW_HIDE);
                }
                return TRUE;
                break;
            }
            break;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

int read_param_file(psocn_ship_t* cfg) {
    int rv;

    CONFIG_LOG("��ȡ v2 �����ļ�/////////////////////////");

    /* Try to read the v2 ItemPT data... */
    if (cfg->v2_ptdata_file) {
        CONFIG_LOG("��ȡ v2 ItemPT �ļ�: %s"
            , cfg->v2_ptdata_file);
        if (pt_read_v2(cfg->v2_ptdata_file)) {
            ERR_LOG("�޷���ȡ v2 ItemPT �ļ�: %s"
                ", ȡ��֧�� v2 �汾!", cfg->v2_ptdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOV2;
        }
    }
    else {
        ERR_LOG("δָ�� v2 ItemPT �ļ�"
            ", ȡ��֧�� v2 �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOV2;
    }

    /* Read the v2 ItemPMT file... */
    if (cfg->v2_pmtdata_file) {
        CONFIG_LOG("��ȡ v2 ItemPMT �ļ�: %s"
            , cfg->v2_pmtdata_file);
        if (pmt_read_v2(cfg->v2_pmtdata_file,
            !(cfg->local_flags & PSOCN_SHIP_PMT_LIMITV2))) {
            ERR_LOG("�޷���ȡ v2 ItemPMT �ļ�: %s"
                ", ȡ��֧�� v2 �汾!", cfg->v2_pmtdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOV2;
        }
    }
    else {
        ERR_LOG("δָ�� v2 ItemPMT �ļ�"
            ", ȡ��֧�� v2 �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOV2;
    }

    /* If we have a v2 map dir set, try to read the maps. */
    if (cfg->v2_map_dir) {
        CONFIG_LOG("��ȡ v2 map ����·��: %s"
            , cfg->v2_map_dir);

        if (v2_read_params(cfg) < 0) {
            ERR_LOG("�޷���ȡ v2 map ·��: %s"
                , cfg->v2_map_dir);
            printf("�� %d \n", __LINE__);//ERR_EXIT();
        }
    }

    /* Read the v2 ItemRT file... */
    if (cfg->v2_rtdata_file) {
        CONFIG_LOG("��ȡ v2 ItemRT �ļ�: %s"
            , cfg->v2_rtdata_file);
        if (rt_read_v2(cfg->v2_rtdata_file)) {
            ERR_LOG("�޷���ȡ v2 ItemRT �ļ�: %s"
                , cfg->v2_rtdata_file);
        }
    }

    CONFIG_LOG("��ȡ GameCube �����ļ�/////////////////////////");

    /* Read the GC ItemPT file... */
    if (cfg->gc_ptdata_file) {
        CONFIG_LOG("��ȡ GameCube ItemPT �ļ�: %s"
            , cfg->gc_ptdata_file);

        if (pt_read_v3(cfg->gc_ptdata_file)) {
            ERR_LOG("�޷���ȡ GameCube ItemPT �ļ�: %s"
                , cfg->gc_ptdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOEP12;
        }
    }
    else {
        ERR_LOG("δָ�� GC ItemPT �ļ�"
            ", ȡ��֧�� GameCube �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOEP12;
    }

    /* Read the GC ItemPMT file... */
    if (cfg->gc_pmtdata_file) {
        CONFIG_LOG("��ȡ GameCube ItemPMT �ļ�: %s"
            , cfg->gc_pmtdata_file);
        if (pmt_read_gc(cfg->gc_pmtdata_file,
            !(cfg->local_flags & PSOCN_SHIP_PMT_LIMITGC))) {
            ERR_LOG("�޷���ȡ GameCube ItemPMT �ļ�: %s"
                , cfg->gc_pmtdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOEP12;
        }
    }
    else {
        ERR_LOG("δָ�� GC ItemPMT �ļ�"
            ", ȡ��֧�� GameCube �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOEP12;
    }

    /* If we have a GC map dir set, try to read the maps. */
    if (cfg->gc_map_dir) {
        CONFIG_LOG("��ȡ GameCube map ����·��: %s"
            , cfg->gc_map_dir);

        if (gc_read_params(cfg) < 0)
            ERR_EXIT("�޷���ȡ GameCube map ·��: %s", cfg->v2_map_dir);
    }

    /* Read the GC ItemRT file... */
    if (cfg->gc_rtdata_file) {
        CONFIG_LOG("��ȡ GameCube ItemRT �ļ�: %s"
            , cfg->gc_rtdata_file);
        if (rt_read_gc(cfg->gc_rtdata_file)) {
            ERR_LOG("�޷���ȡ GameCube ItemRT �ļ�: %s"
                , cfg->gc_rtdata_file);
        }
    }

    CONFIG_LOG("��ȡ Blue Burst �����ļ�/////////////////////////");

    /* Read the BB ItemPT data, which is needed for Blue Burst... */
    if (cfg->bb_ptdata_file) {
        CONFIG_LOG("��ȡ Blue Burst ItemPT �ļ�: %s"
            , cfg->bb_ptdata_file);

        if (pt_read_bb(cfg->bb_ptdata_file)) {
            ERR_LOG("�޷���ȡ Blue Burst ItemPT �ļ�: %s"
                ", ȡ��֧�� Blue Burst �汾!", cfg->bb_ptdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
        }
    }
    else {
        ERR_LOG("δָ�� BB ItemPT �ļ�"
            ", ȡ��֧�� Blue Burst �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
    }

    /* Read the BB ItemPMT file... */
    if (cfg->bb_pmtdata_file) {
        CONFIG_LOG("��ȡ Blue Burst ItemPMT �ļ�: %s"
            , cfg->bb_pmtdata_file);
        if (pmt_read_bb(cfg->bb_pmtdata_file,
            !(cfg->local_flags & PSOCN_SHIP_PMT_LIMITBB))) {
            ERR_LOG("�޷���ȡ Blue Burst ItemPMT �ļ�: %s"
                , cfg->bb_pmtdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
        }
    }
    else {
        ERR_LOG("δָ�� BB ItemPMT �ļ�"
            ", ȡ��֧�� Blue Burst �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
    }

    /* If Blue Burst isn't disabled already, read the parameter data and map
       data... */
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOBB)) {
        rv = bb_read_params(cfg);

        /* Read the BB ItemRT file... */
        if (cfg->bb_rtdata_file) {
            CONFIG_LOG("��ȡ Blue Burst ItemRT �ļ�: %s", cfg->bb_rtdata_file);
            if (rv = rt_read_bb(cfg->bb_rtdata_file)) {
                ERR_LOG("�޷���ȡ Blue Burst ItemRT �ļ�: %s", cfg->bb_rtdata_file);
            }

            //char* dir = "System\\ItemRT\\bb\\ItemRTep2.gsl";

            //CONFIG_LOG("��ȡ Blue Burst ItemRT_EP2 �ļ�: %s", dir);
            //if (rv = rt_read_bb_ep2(dir)) {
            //    ERR_LOG("�޷���ȡ Blue Burst ItemRT_EP2 �ļ�: %s", dir);
            //}

            //dir = "System\\ItemRT\\bb\\ItemRTep2.gsl";

            //CONFIG_LOG("��ȡ Blue Burst ItemRT_EP2 �ļ�: %s", dir);
            //if (rv = rt_read_bb_ep2(dir)) {
            //    ERR_LOG("�޷���ȡ Blue Burst ItemRT_EP2 �ļ�: %s", dir);
            //}
        }

        //rv = load_shop_data();

        //printf("###########################\n");
        //if (file_log_console_show) {
        //    printf("���ڴ�Param\\ItemPMT�ļ����м������������ļ���...\n");
        //}
        Load_Weapon_Param(WEAPON);

        //if (file_log_console_show) {
        //    printf("���ڴ�Param\\ItemPMT�ļ����м���װ�׺Ͷ��Ʋ����ļ���...\n");
        //}
        Load_Armor_Param(ARMOR);

        //if (file_log_console_show) {
        //    printf("���ڴ�Param\\ItemPMT�ļ����м���װ�׺Ͷ��Ʋ����ļ���...\n");
        //}
        Load_Shield_Param(SHIELD);

        //if (file_log_console_show) {
        //    printf("���ڴ�Param\\ItemPMT�ļ����м��ؿƼ������ļ���...\n");
        //}
        Load_Tech_Param(TECH);

        /* Less than 0 = fatal error. Greater than 0 = Blue Burst problem. */
        if (rv > 0) {
            ERR_LOG("��ȡ Blue Burst �����ļ�����"
                ", ȡ��֧�� Blue Burst �汾!");
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
        }
        else if (rv < 0) {
            ERR_LOG("�޷���ȡ Blue Burst �����ļ�"
                ", ȡ��֧�� Blue Burst �汾!");
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
        }

        return rv;
    }

    return 0;
}

int __cdecl main(int argc, char** argv) {
    void *tmp;
    psocn_ship_t *cfg;
    WSADATA winsock_data;

    load_log_config();

    errno_t err = WSAStartup(MAKEWORD(2, 2), &winsock_data);

    if (err)
        ERR_EXIT("WSAStartup ����...");

    HWND consoleHwnd;
    WNDCLASS wc = { 0 };
    HWND hwndWindow;
    HINSTANCE hinst = GetModuleHandle(NULL);
    consoleHwnd = GetConsoleWindow();

    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hIcon = LoadIcon(hinst, IDI_APPLICATION);
    wc.hCursor = LoadCursor(hinst, IDC_ARROW);
    wc.hInstance = hinst;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = "Sancaros";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClass(&wc))
        ERR_EXIT("ע����ʧ��.");

    hwndWindow = CreateWindow("Sancaros", "hidden window", WS_MINIMIZE, 1, 1, 1, 1,
        NULL,
        NULL,
        hinst,
        NULL);

    if (!hwndWindow)
        ERR_EXIT("�޷���������.");

    ShowWindow(hwndWindow, SW_HIDE);
    UpdateWindow(hwndWindow);
    MoveWindow(consoleHwnd, 900, 510, 980, 510, SWP_SHOWWINDOW);	//�ѿ���̨�ϵ�(100,100)
    ShowWindow(consoleHwnd, window_hide_or_show);
    UpdateWindow(consoleHwnd);

    log_server_name_num = 2;

    load_program_info(SHIPS_SERVER_NAME, SHIPS_SERVER_VERSION);

    /* Parse the command line... */
    parse_command_line(argc, argv);

    cfg = load_config();

    open_log(cfg);

restart:
    print_config(cfg);

    /* Parse the addresses */
    if(setup_addresses(cfg))
        ERR_EXIT("��ȡ IP ��ַʧ��");

    /* Initialize GnuTLS stuff... */
    if(!check_only) {
        if (init_gnutls())
            ERR_EXIT("�޷����� GnuTLS ֤��");

        //if (shipkey_init(cfg))
        //    ERR_EXIT("�޷����� ������Կ");

        /* Set up things for clients to connect. */
        if(client_init(cfg))
            ERR_EXIT("�޷����� �ͻ��� ����");
    }

    if(0 != read_param_file(cfg))
        ERR_EXIT("�޷���ȡ��������");

    /* Set a few other shipgate flags, if appropriate. */
#ifdef ENABLE_LUA
    cfg->shipgate_flags |= LOGIN_FLAG_LUA;
#endif

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    cfg->shipgate_flags |= LOGIN_FLAG_BE;
#endif

#if (SIZEOF_VOID_P == 4)
    cfg->shipgate_flags |= LOGIN_FLAG_32BIT;
#endif

    /* Initialize all the iconv contexts we'll need */
    if(init_iconv())
        ERR_EXIT("�޷���ȡ iconv ��������");

    /* Init mini18n if we have it */
    init_i18n();

    /* Init the word censor. */
    if(cfg->smutdata_file) {
        SHIPS_LOG("��ȡ smutdata �ļ�: %s", cfg->smutdata_file);
        if(smutdata_read(cfg->smutdata_file)) {
            ERR_LOG("�޷���ȡ smutdata �ļ�!");
        }
    }

    if(!check_only) {
        /* Install signal handlers */
        HookupHandler();

        /* Set up the ship and start it. */
        ship = ship_server_start(cfg);
        if(ship)
            pthread_join(ship->thd, NULL);

        /* Clean up... */
        if((tmp = pthread_getspecific(sendbuf_key))) {
            free_safe(tmp);
            pthread_setspecific(sendbuf_key, NULL);
        }

        if((tmp = pthread_getspecific(recvbuf_key))) {
            free_safe(tmp);
            pthread_setspecific(recvbuf_key, NULL);
        }
    }
    else {
        ship_check_cfg(cfg);
    }

    WSACleanup();
    smutdata_cleanup();
    cleanup_i18n();
    cleanup_iconv();

    if(!check_only) {
        client_shutdown();
        cleanup_gnutls();
    }

    psocn_free_ship_config(cfg);
    bb_free_params();
    v2_free_params();
    gc_free_params();
    pmt_cleanup();

    if(restart_on_shutdown) {
        cfg = load_config();
        goto restart;
    }

    xmlCleanupParser();

    return 0;
}
