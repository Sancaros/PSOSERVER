#ifndef ZLMEDIAKIT_STRPTIME_WIN_H
#define ZLMEDIAKIT_STRPTIME_WIN_H

#include <time.h>
#ifdef _WIN32
//window���Լ�ʵ��strptime������linux�Ѿ��ṩstrptime
//strptime����windowsƽ̨��ʵ��
char* strptime(const char* buf, const char* fmt, struct tm* tm);
#endif
#endif //ZLMEDIAKIT_STRPTIME_WIN_H