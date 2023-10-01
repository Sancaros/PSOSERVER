#pragma once
#include <stdint.h>
#include <windows.h>

#if PSOBBCN_PLATFORM == PSOBBCN_PLATFORM_WINDOWS
#  ifdef _WIN64
#    define PSOBBCN_PLATFORM_STR "Win64"
#  else
#    define PSOBBCN_PLATFORM_STR "Win32"
#  endif
#elif PSOBBCN_PLATFORM == PSOBBCN_PLATFORM_APPLE
#  define PSOBBCN_PLATFORM_STR "MacOSX"
#elif PSOBBCN_PLATFORM == PSOBBCN_PLATFORM_INTEL
#  define PSOBBCN_PLATFORM_STR "Intel"
#else // PSOBBCN_PLATFORM_UNIX
#  define PSOBBCN_PLATFORM_STR "Unix"
#endif

#ifndef PSOBBCN_API_USE_DYNAMIC_LINKING
#  define PSOBBCN_LINKAGE_TYPE_STR "��̬��"
#else
#  define PSOBBCN_LINKAGE_TYPE_STR "��̬��"
#endif

#if (SIZE_MAX == 0xFF)
#define SIZE_T_BITS 8
#elif (SIZE_MAX == 0xFFFF)
#define SIZE_T_BITS 16
#elif (SIZE_MAX == 0xFFFFFFFF)
#define SIZE_T_BITS 32
#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
#define SIZE_T_BITS 64
#else
#error SIZE_MAX is not a recognized value
#endif

#define STR(s)     #s
#define VERSION(a,b,c)  STR(a)"."STR(b)"."STR(c)" - "__DATE__" - "__TIME__ 

#define PATCH_SERVER 0x00
#define AUTH_SERVER  0x01
#define SHIPS_SERVER 0x02
#define SGATE_SERVER 0x03
#define DNS_SERVER  0x04

typedef struct server_map {
	uint32_t serv_num;
	const char* name;
} server_map_st;

static server_map_st server_name[] = {
	{ PATCH_SERVER, "����������" },
	{ AUTH_SERVER, "��֤������" },
	{ SHIPS_SERVER, "����������" },
	{ SGATE_SERVER, "��բ������" },
	{ DNS_SERVER, "DNS������" },
};

uint32_t server_name_num;


#define MAX_TMP_BUFF 64000
#define MAX_PACKET_BUFF ( MAX_TMP_BUFF * 16 )

char windows[4096];
char dp[MAX_PACKET_BUFF];

void parse_version(uint8_t* maj, uint8_t* min, uint8_t* mic,
	const char* ver);

void set_console_title(const char* fmt, ...);

void load_program_info(const char* servername, const char* ver);