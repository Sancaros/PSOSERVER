/*
    梦幻之星中国 设置文件.
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

#ifndef PSOCN_CONFIG_H
#define PSOCN_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "queue.h"
#include "pso_opcodes_block.h"
#include "pso_opcodes_subcmd.h"
#include "pso_opcodes_shipgate.h"

//#define SIZEOF_VOID_P 4

#define SOCKET_ERR(err, s) if(err==-1){perror(s);closesocket(err);return(-1);}
#define LOOP_CHECK(rval, cmd) do{rval = cmd;} while(rval == GNUTLS_E_AGAIN || rval == GNUTLS_E_INTERRUPTED);/*assert(rval >= 0)*/

#ifndef DATAROOTDIR
#define DATAROOTDIR "config\\"
#endif

#define PSOCN_DIRECTORY DATAROOTDIR ""

static const char psocn_directory[] = PSOCN_DIRECTORY;
static const char psocn_global_cfg[] = PSOCN_DIRECTORY "psocn_config_global.xml";
static const char psocn_database_cfg[] = PSOCN_DIRECTORY "psocn_config_database.xml";
static const char psocn_server_cfg[] = PSOCN_DIRECTORY "psocn_config_server.xml";
static const char psocn_patch_cfg[] = PSOCN_DIRECTORY "psocn_config_patch.xml";
static const char psocn_ship_cfg[] = PSOCN_DIRECTORY "psocn_config_ship.xml";

static const char* Welcome_Files[] = {
    "Motd\\Patch_Welcome.txt",
    "Motd\\Login_Welcome.txt",
};

/* Values for the shipgate flags portion of ships/proxies. */
#define SHIPGATE_FLAG_GMONLY    0x00000001
#define SHIPGATE_FLAG_NOV1      0x00000010
#define SHIPGATE_FLAG_NOV2      0x00000020
#define SHIPGATE_FLAG_NOPC      0x00000040
#define SHIPGATE_FLAG_NOEP12    0x00000080
#define SHIPGATE_FLAG_NOEP3     0x00000100
#define SHIPGATE_FLAG_NOBB      0x00000200
#define SHIPGATE_FLAG_NODCNTE   0x00000400
#define SHIPGATE_FLAG_NOPSOX    0x00000800
#define SHIPGATE_FLAG_NOPCNTE   0x00001000

/* The first few (V1, V2, PC) are only valid on the ship server, whereas the
   last couple (Ep3, BB) are only valid on the login server. GC works either
   place, but only some GC versions can see info files on the ship. */
#define PSOCN_INFO_V1       0x00000001
#define PSOCN_INFO_V2       0x00000002
#define PSOCN_INFO_PC       0x00000004
#define PSOCN_INFO_GC       0x00000008
#define PSOCN_INFO_EP3      0x00000010
#define PSOCN_INFO_BB       0x00000020
#define PSOCN_INFO_XBOX     0x00000040

   /* Languages that can be set for the info entries. */
#define PSOCN_INFO_JAPANESE 0x00000001
#define PSOCN_INFO_ENGLISH  0x00000002
#define PSOCN_INFO_GERMAN   0x00000004
#define PSOCN_INFO_FRENCH   0x00000008
#define PSOCN_INFO_SPANISH  0x00000010
#define PSOCN_INFO_CH_SIMP  0x00000020
#define PSOCN_INFO_CH_TRAD  0x00000040
#define PSOCN_INFO_KOREAN   0x00000080

/* Flags for the local_flags of a ship. */
#define PSOCN_SHIP_PMT_LIMITV2  0x00000001
#define PSOCN_SHIP_PMT_LIMITGC  0x00000002
#define PSOCN_SHIP_QUEST_RARES  0x00000004
#define PSOCN_SHIP_QUEST_SRARES 0x00000008
#define PSOCN_SHIP_PMT_LIMITBB  0x00000010

#define PSOCN_REG_DC            0x00000001
#define PSOCN_REG_DCNTE         0x00000002
#define PSOCN_REG_PC            0x00000004
#define PSOCN_REG_GC            0x00000008
#define PSOCN_REG_XBOX          0x00000010
#define PSOCN_REG_BB            0x00000020

/* 用于区分DNS服务器中的客户端端口类型 */
#define CLIENT_TYPE_DNS                0

/* 用于区分补丁服务器中的客户端端口类型 */
#define CLIENT_TYPE_PC_PATCH           0
#define CLIENT_TYPE_PC_DATA            1
#define CLIENT_TYPE_WEB                2
#define CLIENT_TYPE_BB_PATCH           3
#define CLIENT_TYPE_BB_PATCH_SCHTHACK  4
#define CLIENT_TYPE_BB_DATA            5
#define CLIENT_TYPE_BB_DATA_SCHTHACK   6

#ifdef PSOCN_ENABLE_IPV6
#define PATCH_CLIENT_SOCKETS_TYPE_MAX 14
#else
#define PATCH_CLIENT_SOCKETS_TYPE_MAX 7
#endif

#ifndef PSOCN_ENABLE_IPV6
#define NUM_AUTH_DC_SOCKS  3
#define NUM_AUTH_PC_SOCKS  1
#define NUM_AUTH_GC_SOCKS  6
#define NUM_AUTH_EP3_SOCKS 3
#define NUM_AUTH_WEB_SOCKS 1
#define NUM_AUTH_BB_SOCKS  6
#define NUM_AUTH_XB_SOCKS  1
#else
#define NUM_AUTH_DC_SOCKS  6
#define NUM_AUTH_PC_SOCKS  2
#define NUM_AUTH_GC_SOCKS  12
#define NUM_AUTH_EP3_SOCKS 6
#define NUM_AUTH_WEB_SOCKS 2
#define NUM_AUTH_BB_SOCKS  12
#define NUM_AUTH_XB_SOCKS  2
#endif

typedef struct psocn_srvsockets {
    int sock_type;
    int port;
    int port_type;
    char* sockets_name;
} psocn_srvsockets_t;

typedef struct psocn_dbconfig {
    char* type;
    char* host;
    char* user;
    char* pass;
    char* db;
    uint16_t port;
    char* unix_socket;
    char* auto_reconnect;
    char* char_set;
    char* show_setting;
} psocn_dbconfig_t;

typedef struct psocn_patch_port {
    int ptports[PATCH_CLIENT_SOCKETS_TYPE_MAX];
} psocn_patch_port_t;

typedef struct psocn_auth_port {
    int dcports[NUM_AUTH_DC_SOCKS];
    int pcports[NUM_AUTH_PC_SOCKS];
    int gcports[NUM_AUTH_GC_SOCKS];
    int ep3ports[NUM_AUTH_EP3_SOCKS];
    int webports[NUM_AUTH_WEB_SOCKS];
    int bbports[NUM_AUTH_BB_SOCKS];
    int xbports[NUM_AUTH_XB_SOCKS];
} psocn_auth_port_t;

typedef struct psocn_srvconfig {
    char* host4;
    char* host6;
    uint32_t server_ip;
    uint8_t server_ip6[16];
    psocn_patch_port_t patch_port;
    psocn_auth_port_t auth_port;
} psocn_srvconfig_t;

typedef struct psocn_sgconfig {
    uint16_t shipgate_port;
    char* shipgate_cert;
    char* shipgate_key;
    char* shipgate_ca;
} psocn_sgconfig_t;

typedef struct psocn_info_file {
    char* desc;
    char* filename;
    uint32_t versions;
    uint32_t languages;
} psocn_info_file_t;

typedef struct psocn_limit_config {
    uint32_t id;
    char* name;
    char* filename;
    int enforce;
} psocn_limit_config_t;

typedef struct psocn_welcom_motd {
    char* web_host;
    uint16_t web_port;
    char* patch_welcom_file;
    char patch_welcom[4096];
    char* login_welcom_file;
    char login_welcom[4096];
} psocn_welcom_motd_t;

typedef struct psocn_config {
    psocn_srvconfig_t srvcfg;
    psocn_dbconfig_t dbcfg;
    psocn_sgconfig_t sgcfg;
    uint8_t registration_required;
    char* quests_dir;
    psocn_limit_config_t* limits;
    int limits_count;
    int limits_enforced;
    psocn_info_file_t* info_files;
    int info_file_count;
    psocn_welcom_motd_t w_motd;
    char* log_dir;
    char* log_prefix;
    char* patch_dir;
    char* sg_scripts_file;
    char* auth_scripts_file;
    char* socket_dir;
} psocn_config_t;

typedef struct psocn_event {
    uint8_t start_month;
    uint8_t start_day;
    uint8_t end_month;
    uint8_t end_day;
    uint8_t lobby_event;
    uint8_t game_event;
} psocn_event_t;

typedef struct psocn_shipcfg {
    char* shipgate_host;
    uint16_t shipgate_port;

    char* name;
    char* ship_cert;
    char* ship_key;
    char* shipgate_ca;
    char* gm_file;
    psocn_limit_config_t* limits;
    uint32_t globla_exp_mult;
    psocn_info_file_t* info_files;
    char* quests_file;
    char* quests_dir;
    char* bans_file;
    char* scripts_file;
    char* bb_param_dir;
    char* v2_param_dir;
    char* bb_map_dir;
    char* v2_map_dir;
    char* gc_map_dir;
    char* v2_ptdata_file;
    char* gc_ptdata_file;
    char* bb_ptdata_file;
    char* v2_pmtdata_file;
    char* gc_pmtdata_file;
    char* bb_pmtdata_file;
    char* v2_rtdata_file;
    char* gc_rtdata_file;
    char* bb_rtdata_file;
    psocn_event_t* events;
    char* smutdata_file;
    char* mageditdata_file;

    char* ship_host4;
    char* ship_host6;
    uint32_t ship_ip4;
    uint8_t ship_ip6[16];
    uint32_t shipgate_flags;
    uint32_t shipgate_proto_ver;
    uint32_t local_flags;

    uint16_t base_port;
    uint16_t menu_code;

    uint32_t blocks;
    int info_file_count;
    int event_count;
    int limits_count;
    int limits_default;
    uint32_t privileges;
} psocn_ship_t;

/* Patch server configuration structure. */
typedef struct patch_file_entry {
    char* filename;
    uint32_t size;
    uint32_t checksum;
    uint32_t client_checksum;

    struct patch_file_entry* next;
} patch_file_entry_t;

/* Patch file structure. */
typedef struct patch_file {
    TAILQ_ENTRY(patch_file) qentry;

    uint32_t flags;
    char* filename;

    patch_file_entry_t* entries;
} patch_file_t;

#define PATCH_FLAG_HAS_IF       0x00000001
#define PATCH_FLAG_HAS_ELSE     0x00000002
#define PATCH_FLAG_NO_IF        0x00000004

TAILQ_HEAD(file_queue, patch_file);

/* Client-side file structure. */
typedef struct patch_cfile {
    TAILQ_ENTRY(patch_cfile) qentry;

    patch_file_t* file;
    patch_file_entry_t* ent;
} patch_cfile_t;

TAILQ_HEAD(cfile_queue, patch_cfile);

typedef struct patch_config {
    psocn_srvconfig_t srvcfg;

    uint32_t disallow_pc;
    uint32_t disallow_bb;

    psocn_welcom_motd_t w_motd;
    uint16_t* pc_welcome;
    uint16_t* bb_welcome;
    uint16_t pc_welcome_size;
    uint16_t bb_welcome_size;

    char* pc_dir;
    char* bb_dir;

    struct file_queue pc_files;
    struct file_queue bb_files;
} patch_config_t;

char* int2ipstr(const int ip);
int32_t server_is_ip(char* server);
int check_ipaddr(char* IP);


/* 通过代码对比获取指令名称 */
extern const char* c_cmd_name(uint16_t cmd, int32_t version);
extern const char* s_cmd_name(uint16_t cmd, int32_t version);

/* In psoconfig_patch.c */
extern int patch_read_config(const char* fn, patch_config_t** cfg);
extern void patch_free_config(patch_config_t* cfg);

/* Read the configuration for the login server, shipgate, and patch server. */
extern int psocn_read_config(const char* f, psocn_config_t** cfg);

/* Clean up a configuration structure. */
extern void psocn_free_config(psocn_config_t* cfg);

/* 读取服务器地址设置. */
extern int psocn_read_srv_config(const char* f, psocn_srvconfig_t** cfg);

/* Clean up a configuration structure. */
extern void psocn_free_srv_config(psocn_srvconfig_t* cfg);

extern int psocn_read_db_config(const char* f, psocn_dbconfig_t** cfg);

/* Clean up a configuration structure. */
extern void psocn_free_db_config(psocn_dbconfig_t* cfg);

/* Read the ship configuration data. You are responsible for calling the
   function to clean up the configuration. */
extern int psocn_read_ship_config(const char* f, psocn_ship_t** cfg);

/* Clean up a ship configuration structure. */
extern void psocn_free_ship_config(psocn_ship_t* cfg);

extern int psocn_web_server_loadfile(const char* onlinefile, char* dest);

extern int psocn_web_server_loadfile2(const char* onlinefile, uint16_t* dest);

extern int psocn_web_server_getfile(void* HostName, int32_t port, char* file, const char* onlinefile);

#endif /* !PSOCN_CONFIG_H */
