#pragma once

#include <WinSock2.h>
#include "winsock2i.h"
#include <WS2tcpip.h>

#include <pso_crash_handle.h>

#pragma comment(lib , "Ws2_32.lib")

#ifdef _WIN323333
typedef struct in_addr in_addr_t;
typedef unsigned int mode_t;
#else
typedef unsigned long in_addr_t;
typedef unsigned int mode_t;
#endif

#ifdef WIN32
//typedef int socklen_t;
//typedef int ssize_t;
#endif

#ifdef __Linux__
typedef int SOCKET;
typedef unsigned char BYTE;
typedef unsigned long DWORD;
#define FALSE 0
#define SOCKET_ERROR (-1)
#endif

//WSADATA winsock_data;