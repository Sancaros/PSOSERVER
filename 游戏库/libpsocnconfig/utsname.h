/*
Copyright 2011 Martin T. Sandsmark <sandsmark@samfundet.no>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#ifndef UTSNAME_H
#define UTSNAME_H

#include "regex.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "WinSock_Defines.h"
#include <windows.h>
#ifndef platform
#define platform "Windows"
#endif

struct win_utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char win_version[65];
	char machine[65];
	char processor[130];
	char full_version[65];
	bool vt_supported;
};
// Get Operating System Version
void win_getfullversion(struct win_utsname* __sys__);
/* # Examples of VER command output:
   #   Windows 2000:  Microsoft Windows 2000 [Version 5.00.2195]
   #   Windows XP:    Microsoft Windows XP [Version 5.1.2600]
   #   Windows Vista: Microsoft Windows [Version 6.0.6002]
   #
   # Note that the "Version" string gets localized on different
   # Windows versions.
*/

void win_getversion(struct win_utsname* __sys__);

/* Gets windows release version
   Examples:
   Version 21H1
   Version 20H1
   Version 2004
   Version 1909
*/
void win_getwinversion(struct win_utsname* __sys__);

// Get windows release stores it in win_utsname structure
void win_getrelease(struct win_utsname* __sys__);

/* Virtualization supported
   checks if virtualization is enabled and accessible by the operating system
   returns true if enabled otherwise 0
*/
bool win_vtsupported();

/* Detecting Processor Architecture
   From: https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
   PROCESSOR_ARCHITECTURE_AMD64 (9)
   PROCESSOR_ARCHITECTURE_ARM (5)
   PROCESSOR_ARCHITECTURE_ARM64 (12)
   PROCESSOR_ARCHITECTURE_IA64 (6)
   PROCESSOR_ARCHITECTURE_INTEL (0)
   PROCESSOR_ARCHITECTURE_UNKNOWN(0xffff or 65535)
*/
void win_getarchitecture(struct win_utsname* __sys__);
// Gets Processor name
void win_getprocessor(struct win_utsname* __sys__);
// Gets system information stores information in structure win_utsname
int win_uname(struct win_utsname* __sys__);

//#define UTSNAME_LENGTH 256
//struct utsname {
//	char sysname[UTSNAME_LENGTH];
//	char nodename[UTSNAME_LENGTH];
//	char release[UTSNAME_LENGTH];
//	char version[UTSNAME_LENGTH];
//	char machine[UTSNAME_LENGTH];
//};
//
//int uname(struct utsname *name);

#endif//UTSNAME_H