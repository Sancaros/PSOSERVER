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

//#include <WinSock2.h>
//#include <Windows.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <versionhelpers.h>
//#include "utsname.h"
//
//int uname(struct utsname *name) {
//#pragma warning(disable: 4996)
//    struct utsname *ret = (struct utsname*)malloc(sizeof(struct utsname));
//    OSVERSIONINFO versionInfo;
//    SYSTEM_INFO sysInfo;
//    errno_t err;
//    // Get Windows version info
//    ZeroMemory(&versionInfo, sizeof(OSVERSIONINFO));
//    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO); // wtf
//    GetVersionEx(&versionInfo);
//    
//    // Get hardware info
//    ZeroMemory(&sysInfo, sizeof(SYSTEM_INFO));
//    GetSystemInfo(&sysInfo);
//
//    err = strcpy_s(name->sysname, sizeof(name->sysname), "Windows");
//    if (err)
//        return -1;
//
//    err = _itoa_s(versionInfo.dwBuildNumber, name->release, sizeof(name->release), 10);
//    if (err)
//        return -1;
//
//    sprintf_s(name->version, sizeof(name->version), "%i.%i", versionInfo.dwMajorVersion, versionInfo.dwMinorVersion);
//
//    if (gethostname(name->nodename, UTSNAME_LENGTH) != 0) {
//        if (WSAGetLastError() == WSANOTINITIALISED) { // WinSock not initialized
//            WSADATA WSAData;
//            err = WSAStartup(MAKEWORD(1, 0), &WSAData);
//            if (err)
//                return WSAGetLastError();
//            gethostname(name->nodename, UTSNAME_LENGTH);
//            WSACleanup();
//        } else
//            return WSAGetLastError();
//    }
//
//    switch(sysInfo.wProcessorArchitecture) {
//    case PROCESSOR_ARCHITECTURE_AMD64:
//        strcpy(name->machine, "x86_64");
//        break;
//    case PROCESSOR_ARCHITECTURE_IA64:
//        strcpy(name->machine, "ia64");
//        break;
//    case PROCESSOR_ARCHITECTURE_INTEL:
//        strcpy(name->machine, "x86");
//        break;
//    case PROCESSOR_ARCHITECTURE_UNKNOWN:
//    default:
//        strcpy(name->machine, "δ֪�汾");
//    }
//
//    return 0;
//}

/*
   USES WIN32API
   Written By catsanddogs
*/

#include "utsname.h"

// Get Operating System Version
void win_getfullversion(struct win_utsname* __sys__) {
	memset(__sys__->full_version, 0, 65);
	int i = 0;
	char c;
	FILE* f = _popen("ver", "r");
	while ((c = fgetc(f)) != EOF) { if (c != '\n') { __sys__->full_version[i] = c; i++; } }
	fclose(f);
}
/* # Examples of VER command output:
   #   Windows 2000:  Microsoft Windows 2000 [Version 5.00.2195]
   #   Windows XP:    Microsoft Windows XP [Version 5.1.2600]
   #   Windows Vista: Microsoft Windows [Version 6.0.6002]
   #
   # Note that the "Version" string gets localized on different
   # Windows versions.
*/

void win_getversion(struct win_utsname* __sys__) {
	win_getfullversion(__sys__);
	memset(__sys__->version, 0, 65);
	regex_t preg;
	const char* pattern = "[0-9]+.[0-9]+.[0-9]+.[0-9]+", * string = __sys__->full_version;
	int x = 0, size = strlen(string);
	regmatch_t pmatch[65];
	regcomp(&preg, pattern, REG_EXTENDED);
	regexec(&preg, string, size + 1, pmatch, REG_NOTBOL);
	for (int i = pmatch[0].rm_so; i < pmatch[0].rm_eo; i++) { __sys__->version[x] = string[i]; x++; }
	regfree(&preg);
}

/* Gets windows release version
   Examples:
   Version 21H1
   Version 20H1
   Version 2004
   Version 1909
*/
void win_getwinversion(struct win_utsname* __sys__) {
	HKEY hkey;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hkey);
	memset(__sys__->win_version, 0, 65);
	long unsigned int size = 65;
	RegQueryValueEx(hkey, "DisplayVersion", NULL, NULL, (LPBYTE)__sys__->win_version, &size);
	RegCloseKey(hkey);
}

// Get windows release stores it in win_utsname structure
void win_getrelease(struct win_utsname* __sys__) {
	win_getfullversion(__sys__);
	regex_t preg;
	int x = 0;
	const char* pattern = "[0-9]+", * string = __sys__->full_version;
	memset(__sys__->release, 0, 65);
	regmatch_t pmatch[65];
	regcomp(&preg, pattern, REG_EXTENDED);
	regexec(&preg, string, strlen(string) + 1, pmatch, REG_NOTBOL);
	for (int i = pmatch[0].rm_so; i < pmatch[0].rm_eo; i++) { __sys__->release[x] = string[i]; x++; }
	regfree(&preg);
}

/* Virtualization supported
   checks if virtualization is enabled and accessible by the operating system
   returns true if enabled otherwise 0
*/
bool win_vtsupported() {
	switch (IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED)) {
	case true: return true;
	default: return false;
	}
}

/* Detecting Processor Architecture
   From: https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
   PROCESSOR_ARCHITECTURE_AMD64 (9)
   PROCESSOR_ARCHITECTURE_ARM (5)
   PROCESSOR_ARCHITECTURE_ARM64 (12)
   PROCESSOR_ARCHITECTURE_IA64 (6)
   PROCESSOR_ARCHITECTURE_INTEL (0)
   PROCESSOR_ARCHITECTURE_UNKNOWN(0xffff or 65535)
*/
void win_getarchitecture(struct win_utsname* __sys__) {
	SYSTEM_INFO sys;
	GetNativeSystemInfo(&sys);
	switch (sys.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64: strcpy(__sys__->machine, "AMD64"); break;
	case PROCESSOR_ARCHITECTURE_ARM: strcpy(__sys__->machine, "ARM32"); break;
#ifdef PROCESSOR_ARCHITECTURE_ARM64
	case PROCESSOR_ARCHITECTURE_ARM64: strcpy(__sys__->machine, "ARM64"); break;
#endif
	case PROCESSOR_ARCHITECTURE_IA64: strcpy(__sys__->machine, "IA-64"); break;
	case PROCESSOR_ARCHITECTURE_INTEL: strcpy(__sys__->machine, "x86"); break;
	default: strcpy(__sys__->machine, "UNKNOWN");
	}
}
// Gets Processor name
void win_getprocessor(struct win_utsname* __sys__) {
	HKEY hkey;
	long unsigned int size = 130;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, KEY_QUERY_VALUE, &hkey);
	RegQueryValueEx(hkey, "PROCESSOR_IDENTIFIER", NULL, NULL, (LPBYTE)__sys__->processor, &size);
	RegCloseKey(hkey);
}
// Gets system information stores information in structure win_utsname
int win_uname(struct win_utsname* __sys__) {
	win_getarchitecture(__sys__);
	win_getfullversion(__sys__);
	win_getversion(__sys__);
	win_getrelease(__sys__);
	win_getwinversion(__sys__);
	win_getprocessor(__sys__);
	if (SOCKET_ERROR == gethostname(__sys__->nodename, 256)) {
		return 1;
	}
	strcpy(__sys__->sysname, platform);
	__sys__->vt_supported = win_vtsupported();
	return 0;
}