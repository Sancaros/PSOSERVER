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

#include "pso_cmd.h"
#include "pso_timer.h"
#include "Software_Defines.h"

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
#define filename(x) strrchr(x,'/')?strrchr(x,'/')+1:x
#else
#define filename(x) strrchr(x,'\\')?strrchr(x,'\\')+1:x
#endif

#define EMPTY_STRING 0xFFFFFFFF

#define m_fun(args) fun(filename(__FILE__),__LINE__, args)

#define PRINT_LINE(FP,MSG)    fprintf(FP,"%s:%d %s\n",filename(__FILE__),__LINE__,MSG)

//#define ERR_EXIT(m) (perror(m),exit(EXIT_FAILURE))
#define CODE_LINE(LINE) fprintf(LINE,"%d",__LINE__)

#define P_DATA(DATA, LENGTH) print_payload((unsigned char*)DATA, LE16(LENGTH));
//#define qWiFiDebug(format, ...) qDebug("[WiFi] "format" File:%s, Line:%d, Function:%s", ##__VA_ARGS__, __FILE__, __LINE__ , __func__); 

#define PATCH_LOG(...) Logs(__LINE__, patch_log_console_show, PATCH_LOG, __VA_ARGS__)
#define AUTH_LOG(...) Logs(__LINE__, auth_log_console_show, AUTH_LOG, __VA_ARGS__)
#define SHIPS_LOG(...) Logs(__LINE__, ships_log_console_show, SHIPS_LOG, __VA_ARGS__)
#define BLOCK_LOG(...) Logs(__LINE__, blocks_log_console_show, BLOCK_LOG, __VA_ARGS__)
#define ERR_LOG(...) err_Logs(filename(__FILE__), __LINE__, error_log_console_show, ERR_LOG, __VA_ARGS__)
#define ERR_EXIT(...) \
    do { \
        ERR_LOG(__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } while(0)

#define LOBBY_LOG(...) Logs(__LINE__, lobbys_log_console_show, LOBBY_LOG, __VA_ARGS__)
#define SGATE_LOG(...) Logs(__LINE__, sgate_log_console_show, SGATE_LOG, __VA_ARGS__)
#define ITEM_LOG(...) Logs(__LINE__, item_log_console_show, ITEM_LOG, __VA_ARGS__)
#define SQLERR_LOG(...) err_Logs(filename(__FILE__), __LINE__, mysqlerr_log_console_show, MYSQLERR_LOG, __VA_ARGS__)
#define QERR_LOG(...) err_Logs(filename(__FILE__), __LINE__, questerr_log_console_show, QUESTERR_LOG, __VA_ARGS__)
#define GM_LOG(...) Logs(__LINE__, gm_log_console_show, GM_LOG, __VA_ARGS__)
#define DBG_LOG(...) dbg_Logs(filename(__FILE__), __LINE__, debug_log_console_show, DEBUG_LOG, __VA_ARGS__)
#define FILE_LOG(...) Logs(__LINE__, file_log_console_show, FILE_LOG, __VA_ARGS__)
#define HOST_LOG(...) Logs(__LINE__, host_log_console_show, HOST_LOG, __VA_ARGS__)
#define UNK_CPD(CODE,VERSION,DATA) unk_cpd(c_cmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, filename(__FILE__))
#define UDONE_CPD(CODE,VERSION,DATA) udone_cpd(c_cmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, filename(__FILE__))
#define UNK_CSPD(CODE,VERSION,DATA) unk_cpd(c_subcmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, filename(__FILE__))
#define UDONE_CSPD(CODE,VERSION,DATA) udone_cpd(c_subcmd_name(CODE, VERSION), (unsigned char*)DATA, __LINE__, filename(__FILE__))
#define UNK_SPD(CODE,DATA) unk_spd(s_cmd_name(CODE, 0), (unsigned char*)DATA, __LINE__, filename(__FILE__))
#define UDONE_SPD(CODE,DATA) udone_spd(s_cmd_name(CODE, 0), (unsigned char*)DATA, __LINE__, filename(__FILE__))
#define DC_LOG(...) Logs(__LINE__, disconnect_log_console_show, DC_LOG, __VA_ARGS__)
#define DSENT_LOG(...) Logs(__LINE__, dont_send_log_console_show, DONT_SEND_LOG, __VA_ARGS__)
#define TEST_LOG(...) Logs(__LINE__, test_log_console_show, TEST_LOG, __VA_ARGS__)
#define MERR_LOG(...) err_Logs(filename(__FILE__), __LINE__, monster_error_log_console_show, MONSTERID_ERR_LOG, __VA_ARGS__)
#define CONFIG_LOG(...) Logs(__LINE__, config_log_console_show, CONFIG_LOG, __VA_ARGS__)
#define SCRIPT_LOG(...) Logs(__LINE__, script_log_console_show, SCRIPT_LOG, __VA_ARGS__)
#define LOG_LOG(...) Logs(__LINE__, config_log_console_show, LOG, __VA_ARGS__)

#define CHECK3(...) { printf(__VA_ARGS__); }

#define free_safe(EXP)  if((EXP)!=NULL && \
                        (unsigned int)(EXP)>(unsigned int)0x00000000 && \
                        (unsigned int)(EXP)<(unsigned int)0xFFFFFFFF ){\
                            free((EXP));\
                        }else if((EXP)!=0){\
                            Logs(__LINE__, \
                            error_log_console_show, \
                            ERR_LOG, "%s:%d––:FREE_ERROR:%s ƒ⁄¥Ê 0x%08x", \
                            filename(__FILE__),__LINE__,#EXP,(unsigned int)(EXP) ); \
                            free((EXP)); \
                        }

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

/* »’÷æ…Ë÷√ */

#ifdef WIN32
int gettimeofday(struct timeval* timevaltmp, void* tzp);
#endif

extern void print_payload(uint8_t* payload, int len);

extern void load_log_config(void);

extern void color(uint32_t x);

extern void packet_to_text(uint8_t* buf, int32_t len);

extern void display_packet(uint8_t* buf, int32_t len);

extern void Logs(int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);

extern void err_Logs(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);

extern void dbg_Logs(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...);

extern void Logs_undone(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...);

extern void Logs_unknow(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...);

extern void unk_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);

extern void udone_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);

extern void unk_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);

extern void udone_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename);
