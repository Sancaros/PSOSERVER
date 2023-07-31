/*
	√Œª√÷Æ–«÷–π˙ »’÷æ
	∞Ê»® (C) 2022, 2023 Sancaros

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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pso_cmd_name.h"
#include "pso_timer.h"
#include "pso_text.h"
#include "Software_Defines.h"

enum Log_files_Num {
	PATCH_LOG, //≤π∂°
	AUTH_LOG, //µ«¬Ω
	SHIPS_LOG, //Ω¢¥¨
	BLOCK_LOG, //Ω¢¥¨
	ERR_LOG, //µ«¬Ω Ω¢¥¨
	LOBBY_LOG, //Ω¢¥¨
	SGATE_LOG, //Ω¢¥¨
	LOGIN_LOG, //µ«¬Ω Ω¢¥¨
	ITEM_LOG, //Ω¢¥¨
	MYSQLERR_LOG, //µ«¬Ω
	QUESTERR_LOG, //Ω¢¥¨
	GM_LOG, //Ω¢¥¨
	DEBUG_LOG, //≤π∂° µ«¬Ω Ω¢¥¨
	FILE_LOG, //≤π∂° µ«¬Ω Ω¢¥¨
	HOST_LOG, //≤π∂° µ«¬Ω Ω¢¥¨
	UNKNOW_PACKET_LOG, //µ«¬Ω Ω¢¥¨
	UNDONE_PACKET_LOG, //µ«¬Ω Ω¢¥¨
	UNUSED, //µ«¬Ω Ω¢¥¨
	DC_LOG, //µ«¬Ω Ω¢¥¨
	DONT_SEND_LOG, //Ω¢¥¨
	TEST_LOG, //≤π∂° µ«¬Ω Ω¢¥¨
	MONSTERID_ERR_LOG, //Ω¢¥¨
	CONFIG_LOG, //Ω¢¥¨
	SCRIPT_LOG,
	DNS_LOG,
	LOG,
	LOG_FILES_MAX,
};

typedef struct log_map {
	uint32_t num;
	const char* name;
} log_map_st;

static log_map_st log_header[] = {
	{ PATCH_LOG, "≤π∂°" },
	{ AUTH_LOG, "»œ÷§" }, //µ«¬Ω
	{ SHIPS_LOG, "Ω¢¥¨" }, //Ω¢¥¨
	{ BLOCK_LOG, "Ω¢≤÷" }, //Ω¢¥¨
	{ ERR_LOG, "¥ÌŒÛ" }, //µ«¬Ω Ω¢¥¨
	{ LOBBY_LOG, "¥ÛÃ¸" }, //Ω¢¥¨
	{ SGATE_LOG, "¥¨’¢" }, //Ω¢¥¨
	{ LOGIN_LOG, "µ«¬Ω" }, //µ«¬Ω Ω¢¥¨
	{ ITEM_LOG, "ŒÔ∆∑" }, //Ω¢¥¨
	{ MYSQLERR_LOG, " ˝æ›" }, //µ«¬Ω
	{ QUESTERR_LOG, "»ŒŒÒ" }, //Ω¢¥¨
	{ GM_LOG, "π‹¿Ì" }, //Ω¢¥¨
	{ DEBUG_LOG, "µ˜ ‘" }, //≤π∂° µ«¬Ω Ω¢¥¨
	{ FILE_LOG, "Œƒº˛" }, //≤π∂° µ«¬Ω Ω¢¥¨
	{ HOST_LOG, "Õ¯¬Á" }, //≤π∂° µ«¬Ω Ω¢¥¨
	{ UNKNOW_PACKET_LOG, "ΩÿªÒ" }, //µ«¬Ω Ω¢¥¨
	{ UNDONE_PACKET_LOG, "»± ß" }, //µ«¬Ω Ω¢¥¨
	{ UNUSED, "Œ¥∆Ù”√" }, //µ«¬Ω Ω¢¥¨
	{ DC_LOG, "∂œ¡¨" }, //µ«¬Ω Ω¢¥¨
	{ DONT_SEND_LOG, "Ωÿ∂œ" }, //Ω¢¥¨
	{ TEST_LOG, "≤‚ ‘" }, //≤π∂° µ«¬Ω Ω¢¥¨
	{ MONSTERID_ERR_LOG, "π÷ŒÔ" }, //Ω¢¥¨
	{ CONFIG_LOG, "…Ë÷√" }, //Ω¢¥¨
	{ SCRIPT_LOG, "Ω≈±æ" },
	{ DNS_LOG, "DNS" },
	{ LOG, "»’÷æ" },
	{ LOG_FILES_MAX, "Œ¥÷™»’÷æ¥ÌŒÛ" },
};

/////////////////////////////////////////
// »’÷æ
extern int32_t console_log_hide_or_show;

extern int32_t patch_log_console_show;
extern int32_t auth_log_console_show;
extern int32_t ships_log_console_show;
extern int32_t blocks_log_console_show;
extern int32_t lobbys_log_console_show;
extern int32_t sgate_log_console_show;
extern int32_t dns_log_console_show;

extern int32_t login_log_console_show;
extern int32_t item_log_console_show;
extern int32_t mysqlerr_log_console_show;
extern int32_t questerr_log_console_show;
extern int32_t gm_log_console_show;
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


/* »˝ƒø‘ÀÀ„ ∂‘±» a b µƒ÷µ ∑µªÿ◊Ó¥Ûµƒ÷µ œ‡µ»‘Ú∑µªÿ b */
#define MAX(a, b) (a > b ? a : b)

/* »˝ƒø‘ÀÀ„ ∂‘±» a b µƒ÷µ ∑µªÿ◊Ó–°µƒ÷µ œ‡µ»‘Ú∑µªÿ b */
#define MIN(a, b) (a < b ? a : b)

/* º∆À„ ˝◊Èµƒ‘™Àÿ ˝¡ø */
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

#define P_DATA(DATA, LENGTH) display_packet(DATA, LE16(LENGTH));
//#define qWiFiDebug(format, ...) qDebug("[WiFi] "format" File:%s, Line:%d, Function:%s", ##__VA_ARGS__, __FILE__, __LINE__ , __func__); 

#define PATCH_LOG(...) flog(__LINE__, patch_log_console_show, PATCH_LOG, __VA_ARGS__)
#define AUTH_LOG(...) flog(__LINE__, auth_log_console_show, AUTH_LOG, __VA_ARGS__)
#define SHIPS_LOG(...) flog(__LINE__, ships_log_console_show, SHIPS_LOG, __VA_ARGS__)
#define BLOCK_LOG(...) flog(__LINE__, blocks_log_console_show, BLOCK_LOG, __VA_ARGS__)
#define DNS_LOG(...) flog(__LINE__, dns_log_console_show, DNS_LOG, __VA_ARGS__)
#define ERR_LOG(...) flog_err(logfilename(__FILE__), __LINE__, error_log_console_show, ERR_LOG, __VA_ARGS__)
#define ERR_EXIT(...) \
    do { \
        ERR_LOG(__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } while(0)

#define LOBBY_LOG(...) flog(__LINE__, lobbys_log_console_show, LOBBY_LOG, __VA_ARGS__)
#define SGATE_LOG(...) flog(__LINE__, sgate_log_console_show, SGATE_LOG, __VA_ARGS__)
#define ITEM_LOG(...) flog(__LINE__, item_log_console_show, ITEM_LOG, __VA_ARGS__)
#define SQLERR_LOG(...) flog_err(logfilename(__FILE__), __LINE__, mysqlerr_log_console_show, MYSQLERR_LOG, __VA_ARGS__)
#define QERR_LOG(...) flog_err(logfilename(__FILE__), __LINE__, questerr_log_console_show, QUESTERR_LOG, __VA_ARGS__)
#define GM_LOG(...) flog(__LINE__, gm_log_console_show, GM_LOG, __VA_ARGS__)
#define DBG_LOG(...) flog_debug(logfilename(__FILE__), __LINE__, debug_log_console_show, DEBUG_LOG, __VA_ARGS__)
#define FILE_LOG(...) flog(__LINE__, file_log_console_show, FILE_LOG, __VA_ARGS__)
#define HOST_LOG(...) flog(__LINE__, host_log_console_show, HOST_LOG, __VA_ARGS__)

#define UNK_CPD(CODE,VERSION,DATA) unk_cpd(c_cmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, logfilename(__FILE__))
#define UDONE_CPD(CODE,VERSION,DATA) udone_cpd(c_cmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, logfilename(__FILE__))
#define UNK_CSPD(CODE,VERSION,DATA) unk_cpd(c_subcmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, logfilename(__FILE__))
#define UDONE_CSPD(CODE,VERSION,DATA) udone_cpd(c_subcmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, logfilename(__FILE__))
#define ERR_CSPD(CODE,VERSION,DATA) err_cpd(c_subcmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, logfilename(__FILE__))

#define UNK_SPD(CODE,DATA) unk_spd(s_cmd_name(CODE, 0), (unsigned char*)DATA, __LINE__, logfilename(__FILE__))
#define UDONE_SPD(CODE,DATA) udone_spd(s_cmd_name(CODE, 0), (unsigned char*)DATA, __LINE__, logfilename(__FILE__))

#define DC_LOG(...) flog(__LINE__, disconnect_log_console_show, DC_LOG, __VA_ARGS__)
#define DSENT_LOG(...) flog(__LINE__, dont_send_log_console_show, DONT_SEND_LOG, __VA_ARGS__)
#define TEST_LOG(...) flog_debug(logfilename(__FILE__), __LINE__, debug_log_console_show, TEST_LOG, __VA_ARGS__)
#define MERR_LOG(...) flog_err(logfilename(__FILE__), __LINE__, monster_error_log_console_show, MONSTERID_ERR_LOG, __VA_ARGS__)
#define CONFIG_LOG(...) flog(__LINE__, config_log_console_show, CONFIG_LOG, __VA_ARGS__)
#define SCRIPT_LOG(...) flog(__LINE__, script_log_console_show, SCRIPT_LOG, __VA_ARGS__)
#define LOG_LOG(...) flog(__LINE__, config_log_console_show, LOG, __VA_ARGS__)

#define CHECK3(...) { printf(__VA_ARGS__); }

#define free_safe(EXP)  if((EXP)!=NULL && \
                        (unsigned int)(EXP)>(unsigned int)0x00000000 && \
                        (unsigned int)(EXP)<(unsigned int)0xFFFFFFFF ){\
                            free((EXP));\
                        }else if((EXP)!=NULL){\
                            flog(__LINE__, \
                            error_log_console_show, \
                            ERR_LOG, "%s:%d––:FREE_ERROR:%s ƒ⁄¥Ê 0x%08x", \
                            logfilename(__FILE__),__LINE__,#EXP,(unsigned int)(EXP) ); \
                            free((EXP)); \
                        }

/* »’÷æ…Ë÷√ */

#ifdef WIN32
int gettimeofday(struct timeval* timevaltmp, void* tzp);
#endif

extern void load_log_config(void);
extern void color(uint32_t x);

extern void packet_to_text(uint8_t* buf, size_t len, bool show);
extern void display_packet(void* buf, size_t len);

extern void flog(int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);
extern void flog_err(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);
extern void flog_debug(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);
extern void flog_undone(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...);
extern void flog_unknow(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...);


extern void unk_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
extern void udone_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
extern void err_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
extern void unk_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
extern void udone_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
