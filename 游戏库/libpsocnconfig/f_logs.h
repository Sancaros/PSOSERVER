/*
	�λ�֮���й� ��־
	��Ȩ (C) 2022, 2023 Sancaros

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

#pragma once
#pragma warning (disable:4819)

#ifndef PSOCN_LOG
#define PSOCN_LOG

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "Software_Defines.h"

/* Get ssize_t. */
# ifdef _MSC_VER
#  include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#  include <sys/types.h>
# else
#  include <sys/types.h>
# endif

#include <pthread.h>

#define MAX_TMP_BUFF 65536
#define MAX_PACKET_BUFF ( MAX_TMP_BUFF * 2 )

char dp[MAX_PACKET_BUFF];
char dbgdp[MAX_PACKET_BUFF];

pthread_mutex_t log_mutex;
pthread_mutex_t log_item_mutex;
pthread_mutex_t pkt_mutex;

enum Log_files_Num {
	PATCH_LOG, //����
	AUTH_LOG, //��½
	SHIPS_LOG, //����
	BLOCK_LOG, //����
	ERR_LOG, //��½ ����
	LOBBY_LOG, //����
	SGATE_LOG, //����
	LOGIN_LOG, //��½ ����
    ITEMS_LOG, //����
	MYSQLERR_LOG, //��½
	QUESTERR_LOG, //����
	GMC_LOG, //����
	DEBUG_LOG, //���� ��½ ����
	FILE_LOG, //���� ��½ ����
	HOST_LOG, //���� ��½ ����
	UNKNOW_PACKET_LOG, //��½ ����
	UNDONE_PACKET_LOG, //��½ ����
	UNUSED, //��½ ����
	DC_LOG, //��½ ����
	DONT_SEND_LOG, //����
	TEST_LOG, //���� ��½ ����
	MONSTERID_ERR_LOG, //����
	CONFIG_LOG, //����
	SCRIPT_LOG,
	DNS_LOG,
	CRASH_LOG,
	LOG,
	LOG_FILES_MAX,
    TRADE_LOG = 8, //����
    PICK_LOG = 8, //����
    DROP_LOG = 8, //����
    MDROP_LOG = 8, //����
    BDROP_LOG = 8, //����
    TEKK_LOG = 8, //����
    BANKD_LOG = 8, //����
    BANKT_LOG = 8, //����
    DATA_LOG = 14, //���� ��½ ����
};

typedef struct log_map {
	uint32_t num;
	const char* name;
} log_map_st;

static log_map_st log_header[] = {
	{ PATCH_LOG,			"����" },
	{ AUTH_LOG,				"��֤" }, //��½
	{ SHIPS_LOG,			"����" }, //����
	{ BLOCK_LOG,			"����" }, //����
	{ ERR_LOG,				"����" }, //��½ ����
	{ LOBBY_LOG,			"����" }, //����
	{ SGATE_LOG,			"��բ" }, //����
	{ LOGIN_LOG,			"��½" }, //��½ ����
	{ ITEMS_LOG,			"��Ʒ" }, //����
	{ MYSQLERR_LOG,			"����" }, //��½
	{ QUESTERR_LOG,			"����" }, //����
	{ GMC_LOG,				"����" }, //����
	{ DEBUG_LOG,			"����" }, //���� ��½ ����
	{ FILE_LOG,				"�ļ�" }, //���� ��½ ����
	{ HOST_LOG,				"����" }, //���� ��½ ����
	{ UNKNOW_PACKET_LOG,	"�ػ�" }, //��½ ����
	{ UNDONE_PACKET_LOG,	"ȱʧ" }, //��½ ����
	{ UNUSED,				"δ����" }, //��½ ����
	{ DC_LOG,				"����" }, //��½ ����
	{ DONT_SEND_LOG,		"�ض�" }, //����
	{ TEST_LOG,				"����" }, //���� ��½ ����
	{ MONSTERID_ERR_LOG,	"����" }, //����
	{ CONFIG_LOG,			"����" }, //����
	{ SCRIPT_LOG,			"�ű�" },
	{ DNS_LOG,				"DNS" },
	{ CRASH_LOG,			"����" },
	{ LOG,					"��־" },
	{ LOG_FILES_MAX, "δ֪��־����" },
    { TRADE_LOG,			"����" }, //����
    { PICK_LOG,			    "ʰȡ" }, //����
    { DROP_LOG,			    "����" }, //����
    { MDROP_LOG,			"�ֵ�" }, //����
    { BDROP_LOG,			"���" }, //����
    { TEKK_LOG,			    "����" }, //����
    { BANKD_LOG,			"����" }, //����
    { BANKT_LOG,			"��ȡ" }, //����
    { DATA_LOG,			    "����" }, //����
};

/////////////////////////////////////////
// ��־
extern int32_t console_log_hide_or_show;

extern int32_t patch_log_console_show;
extern int32_t auth_log_console_show;
extern int32_t ships_log_console_show;
extern int32_t blocks_log_console_show;
extern int32_t lobbys_log_console_show;
extern int32_t sgate_log_console_show;
extern int32_t dns_log_console_show;
extern int32_t crash_log_console_show;

extern int32_t login_log_console_show;
extern int32_t item_log_console_show;
extern int32_t mysqlerr_log_console_show;
extern int32_t questerr_log_console_show;
extern int32_t gmc_log_console_show;
extern int32_t debug_log_console_show;
extern int32_t error_log_console_show;
extern int32_t file_log_console_show;
extern int32_t host_log_console_show;
extern int32_t unknow_packet_log_console_show;
extern int32_t undone_packet_log_console_show;
extern int32_t unused_log_show;
extern int32_t disconnect_log_console_show;
extern int32_t dont_send_log_console_show;
extern int32_t test_log_console_show;
extern int32_t monster_error_log_console_show;
extern int32_t config_log_console_show;
extern int32_t script_log_console_show;

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
#define LE16(x) (((x >> 8) & 0xFF) | ((x & 0xFF) << 8))
#define LE32(x) (((x >> 24) & 0x00FF) | \
                 ((x >>  8) & 0xFF00) | \
                 ((x & 0xFF00) <<  8) | \
                 ((x & 0x00FF) << 24))
#define LE64(x) (((x >> 56) & 0x000000FF) | \
                 ((x >> 40) & 0x0000FF00) | \
                 ((x >> 24) & 0x00FF0000) | \
                 ((x >>  8) & 0xFF000000) | \
                 ((x & 0xFF000000) <<  8) | \
                 ((x & 0x00FF0000) << 24) | \
                 ((x & 0x0000FF00) << 40) | \
                 ((x & 0x000000FF) << 56))
#define BE64(x) x
#else
#define LE16(x) x
#define SWAP16(x) (((x >> 8) & 0xFF) | ((x & 0xFF) << 8))
#define LE32(x) x
#define SWAP32(x) (((x >> 24) & 0x00FF) | \
                   ((x >>  8) & 0xFF00) | \
                   ((x & 0xFF00) <<  8) | \
                   ((x & 0x00FF) << 24))
#define LE64(x) x
#define SWAP64(x) (((x >> 56) & 0x000000FF) | \
                 ((x >> 40) & 0x0000FF00) | \
                 ((x >> 24) & 0x00FF0000) | \
                 ((x >>  8) & 0xFF000000) | \
                 ((x & 0xFF000000) <<  8) | \
                 ((x & 0x00FF0000) << 24) | \
                 ((x & 0x0000FF00) << 40) | \
                 ((x & 0x000000FF) << 56))
#endif


///* ��Ŀ���� �Ա� a b ��ֵ ��������ֵ ����򷵻� b */
//#define MAX(a, b) (a > b ? a : b)
//
///* ��Ŀ���� �Ա� a b ��ֵ ������С��ֵ ����򷵻� b */
//#define MIN(a, b) (a < b ? a : b)

/* ���������Ԫ������ */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define PRINT_IP_FORMAT "%u.%u.%u.%u"
#define PRINT_HIP(x)\
       ((x >> 0) & 0xFF),\
       ((x >> 8) & 0xFF),\
       ((x >> 16) & 0xFF),\
       ((x >> 24) & 0xFF)

#ifndef _WIN32
#define I_RESET(x)\
       ((x >> 0) & 0xFF),\
       ((x >> 8) & 0xFF),\
       ((x >> 16) & 0xFF),\
       ((x >> 24) & 0xFF)

#else
#define I_RESET(x) (x << 24) | (x << 16) | (x << 8) | (x);return x;
#endif

#ifdef GNU
#define logfilename(x) strrchr(x,'/')?strrchr(x,'/')+1:x
#else
#define logfilename(x) strrchr(x,'\\')?strrchr(x,'\\')+1:x
#endif

#define EMPTY_STRING 0xFFFFFFFF

#define m_fun(args) fun(logfilename(__FILE__),__LINE__, args)

#define PRINT_LINE(FP,MSG)    fprintf(FP,"%s:%d %s\n",logfilename(__FILE__),__LINE__,MSG)

//#define ERR_EXIT(m) (perror(m),exit(EXIT_FAILURE))
#define CODE_LINE(LINE) fprintf(LINE,"%d",__LINE__)

#define P_DATA(DATA, LENGTH) print_ascii_hex(DATA, LE16(LENGTH));
//#define qWiFiDebug(format, ...) qDebug("[WiFi] "format" File:%s, Line:%d, Function:%s", ##__VA_ARGS__, __FILE__, __LINE__ , __func__); 


#define PATCH_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, patch_log_console_show, PATCH_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define AUTH_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, auth_log_console_show, AUTH_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define SHIPS_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, ships_log_console_show, SHIPS_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define BLOCK_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, blocks_log_console_show, BLOCK_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define DNS_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, dns_log_console_show, DNS_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define ERR_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog_err(logfilename(__FILE__), __LINE__, error_log_console_show, ERR_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define LOBBY_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, lobbys_log_console_show, LOBBY_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define SGATE_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, sgate_log_console_show, SGATE_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ITEM_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, ITEMS_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

#define BANK_DEPOSIT_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, BANKD_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

#define BANK_TAKE_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, BANKT_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

#define TEKKS_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, TEKK_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

#define TRADES_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, TRADE_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

#define PICKS_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, PICK_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

#define DROPS_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, DROP_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

#define MDROPS_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, MDROP_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

#define BDROPS_LOG(...) \
do { \
    pthread_mutex_lock(&log_item_mutex); \
    flog_item(logfilename(__FILE__), __LINE__, item_log_console_show, BDROP_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_item_mutex); \
} while (0)

///////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SQLERR_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog_err(logfilename(__FILE__), __LINE__, mysqlerr_log_console_show, MYSQLERR_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define QERR_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog_err(logfilename(__FILE__), __LINE__, questerr_log_console_show, QUESTERR_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define DBG_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog_debug(logfilename(__FILE__), __LINE__, debug_log_console_show, DEBUG_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define GM_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, gmc_log_console_show, GMC_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define FILE_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, file_log_console_show, FILE_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define HOST_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, host_log_console_show, HOST_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define DATA_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, host_log_console_show, DATA_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define DC_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, disconnect_log_console_show, DC_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define DSENT_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, dont_send_log_console_show, DONT_SEND_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define TEST_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog_debug(logfilename(__FILE__), __LINE__, debug_log_console_show, TEST_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define MERR_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog_err(logfilename(__FILE__), __LINE__, monster_error_log_console_show, MONSTERID_ERR_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define CONFIG_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, config_log_console_show, CONFIG_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define SCRIPT_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, script_log_console_show, SCRIPT_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define CRASH_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog_crash(logfilename(__FILE__), __LINE__, crash_log_console_show, CRASH_LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define LOG_LOG(...) \
do { \
    pthread_mutex_lock(&log_mutex); \
    flog(__LINE__, config_log_console_show, LOG, __VA_ARGS__); \
    pthread_mutex_unlock(&log_mutex); \
} while (0)

#define ERR_EXIT(...) \
    do { \
        ERR_LOG(__VA_ARGS__); \
        pthread_mutex_destroy(&log_mutex); \
		(void*)getchar(); \
        exit(EXIT_FAILURE); \
    } while(0)

#define CHECK3(...) { printf(__VA_ARGS__); }

#define free_safe(EXP)\
do {\
    pthread_mutex_lock(&log_mutex); \
    if(EXP)safe_free(logfilename(__FILE__), __LINE__, (void **)&EXP);\
    pthread_mutex_unlock(&log_mutex); \
} while (0)

//#define free_safe2(EXP)  if((EXP)!=NULL && \
//                        (unsigned int)(EXP)>(unsigned int)0x00000000 && \
//                        (unsigned int)(EXP)<(unsigned int)0xFFFFFFFF ){\
//                            free((EXP));\
//                        }else if((EXP)!=NULL){\
//                            flog(__LINE__, \
//                            error_log_console_show, \
//                            ERR_LOG, "%s:%d��:FREE_ERROR:%s �ڴ� 0x%08x", \
//                            logfilename(__FILE__),__LINE__,#EXP,(unsigned int)(EXP) ); \
//                            free((EXP)); \
//                        }

/* ��־���� */

#ifdef WIN32
int gettimeofday(struct timeval* timevaltmp, void* tzp);
#endif

/* This function based on information from a couple of different sources, namely
   Fuzziqer's newserv and information from Lee (through Aleron Ives). */

typedef void (*LogFunc)(const char*, ...);

#define PRINT_HEX_LOG(method, data, length) do {\
	pthread_mutex_lock(&pkt_mutex);\
	size_t i;\
	memset(&dp[0], 0, sizeof(dp));\
	if (data == NULL || length <= 0 || length > MAX_PACKET_BUFF) {\
		sprintf(dp, "��ָ�����ݰ�����Ч���� % d ���ݰ�.", length);\
		goto done;\
	}\
	uint8_t* buff = (uint8_t*)data;\
	if (is_all_zero(buff, length)) {\
		sprintf(dp, "�����ݰ� ���� %d.", length);\
		goto done;\
	}\
	strcpy(dp, "���ݰ�����:\n\r");\
	for (i = 0; i < length; i++) {\
		if (i % 16 == 0) {\
			if (i != 0) {\
				strcat(dp, "\n");\
			}\
			sprintf(dp + strlen(dp), "(%08X)", (unsigned int)i);\
		}\
		sprintf(dp + strlen(dp), " %02X", (unsigned char)buff[i]);\
		if (i % 8 == 7 && i % 16 != 15) {\
			strcat(dp, " ");\
		}\
		if (i % 16 == 15 || i == length - 1) {\
			size_t j;\
			strcat(dp, "    ");\
			for (j = i - (i % 16); j <= i; j++) {\
				if (j >= length) {\
					strcat(dp, " ");\
				}\
				else if (buff[j] >= ' ' && buff[j] <= '~') {\
					char tmp_str[2] = { buff[j], '\0' };\
					strcat(dp, tmp_str);\
				}\
				else {\
					strcat(dp, ".");\
				}\
			}\
		}\
	}\
	if (strlen(dp) + 2 + 1 <= MAX_PACKET_BUFF) {\
		strcat(dp, "\n\r");\
	}\
	else {\
		method("���������ɻ��з�");\
	}\
done:\
	method(dp);\
	pthread_mutex_unlock(&pkt_mutex);\
} while (0)

extern void print_ascii_hex(void (*print_method)(const char*), const void* data, size_t length);

extern double expand_rate(uint8_t rate);

extern uint32_t byteswap(uint32_t e);

extern uint32_t be_convert_to_le_uint32(uint8_t item_code[3]);

extern uint32_t little_endian_value(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

extern void load_log_config(void);
extern void color(uint32_t x);

extern void packet_to_text(uint8_t* buf, size_t len, bool show);
extern void display_packet_old(const void* buf, size_t len);

extern void flog(int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);
extern void flog_item(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);
extern void flog_file(int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* file, const char* fmt, ...);
extern void flog_err(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);
extern void flog_crash(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);
extern void flog_debug(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);
extern void flog_undone(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...);
extern void flog_unknow(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...);

extern void unk_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
extern void udone_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
extern void err_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
extern void unk_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
extern void udone_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);

extern int remove_directory(const char* path);

extern ssize_t clamp(ssize_t value, ssize_t min, ssize_t max);

static inline void errl(const char* message) {
    ERR_LOG("%s", message);
}

static inline void dbgl(const char* message) {
    DBG_LOG("%s", message);
}

static inline void gml(const char* message) {
    GM_LOG("%s", message);
}

static inline void iteml(const char* message) {
    ITEM_LOG("%s", message);
}

static inline void pickl(const char* message) {
    PICKS_LOG("%s", message);
}

static inline void dropl(const char* message) {
    DROPS_LOG("%s", message);
}

static inline void mdropl(const char* message) {
    MDROPS_LOG("%s", message);
}

static inline void bdropl(const char* message) {
    BDROPS_LOG("%s", message);
}

static inline void tradel(const char* message) {
    TRADES_LOG("%s", message);
}

static inline void testl(const char* message) {
    TEST_LOG("%s", message);
}

static inline void log_mutex_init() {
    int result = 0;
    errno = 0;
    result |= pthread_mutex_init(&log_mutex, NULL);
    result |= pthread_mutex_init(&log_item_mutex, NULL);
    result |= pthread_mutex_init(&pkt_mutex, NULL);
    if (result != 0 || errno) {
        if (result) {
            ERR_LOG("pthread_mutex_init ��ʼ��ʧ��1��������: %d", result);
            ERR_LOG("������Ϣ1: %s", strerror(result));
        }

        // ����ֱ��ʹ�� errno ��ȡ������
        if (errno) {
            ERR_LOG("pthread_mutex_init ��ʼ��ʧ��2��������: %d", errno);
            ERR_LOG("������Ϣ2: %s", strerror(errno));
        }
    }
    else {
        // ��ʼ���ɹ�
        //DBG_LOG("log_mutex_init ��ʼ���ɹ���");
    }
}

static inline void log_mutex_destory() {
    int result = 0;
    errno = 0;
    result |= pthread_mutex_destroy(&pkt_mutex);
    result |= pthread_mutex_destroy(&log_item_mutex);
    result |= pthread_mutex_destroy(&log_mutex);
    if (result != 0 || errno) {
        if (result) {
            ERR_LOG("pthread_mutex_destroy ����ʧ��1��������: %d", result);
            ERR_LOG("������Ϣ1: %s", strerror(result));
        }

        // ����ֱ��ʹ�� errno ��ȡ������
        if (errno) {
            ERR_LOG("pthread_mutex_destroy ����ʧ��2��������: %d", errno);
            ERR_LOG("������Ϣ2: %s", strerror(errno));
        }
    }
    else {
        // ���ٳɹ�
        //DBG_LOG("log_mutex_destory ���ٳɹ���");
    }
}

#include "pso_cmd_name.h"
#include "pso_text.h"
#include "pso_timer.h"

#endif // !PSOCN_LOG