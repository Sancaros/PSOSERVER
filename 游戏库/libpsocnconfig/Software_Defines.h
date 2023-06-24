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
#  define PSOBBCN_LINKAGE_TYPE_STR "静态库"
#else
#  define PSOBBCN_LINKAGE_TYPE_STR "动态库"
#endif

#define STR(s)     #s 

#define PATCH_SERVER 0x00
#define AUTH_SERVER  0x01
#define SHIPS_SERVER 0x02
#define SGATE_SERVER 0x03

typedef struct server_map {
	uint32_t serv_num;
	const char* name;
} server_map_st;

static server_map_st server_name[] = {
	{ PATCH_SERVER, "补丁服务器" },
	{ AUTH_SERVER, "认证服务器" },
	{ SHIPS_SERVER, "舰船服务器" },
	{ SGATE_SERVER, "船闸服务器" },
};

uint32_t server_name_num;

#define TCP_BUFFER_SIZE 64000

char windows[TCP_BUFFER_SIZE * 4];
char dp[TCP_BUFFER_SIZE * 4];

void parse_version(uint8_t* maj, uint8_t* min, uint8_t* mic,
	const char* ver);

void load_program_info(const char* servername, const char* ver);