#include "f_logs.h"

#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <direct.h>
#include "Software_Defines.h"
#include "WinSock_Defines.h"
#include "psoconfig.h"

#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#ifdef WIN32
int gettimeofday(struct timeval* timevaltmp, void* tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	timevaltmp->tv_sec = (long)clock;
	timevaltmp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}
#endif

/////////////////////////////////////////
// 日志
int32_t console_log_hide_or_show;

int32_t patch_log_console_show;
int32_t auth_log_console_show;
int32_t ships_log_console_show;
int32_t blocks_log_console_show;
int32_t lobbys_log_console_show;
int32_t sgate_log_console_show;

int32_t login_log_console_show;
int32_t item_log_console_show;
int32_t mysqlerr_log_console_show;
int32_t questerr_log_console_show;
int32_t gm_log_console_show;
int32_t debug_log_console_show;
int32_t error_log_console_show;
int32_t file_log_console_show;
int32_t host_log_console_show;
int32_t unknow_packet_log_console_show;
int32_t undone_packet_log_console_show;
int32_t unused_log_show;
int32_t disconnect_log_console_show;
int32_t dont_send_log_console_show;
int32_t test_log_console_show;
int32_t monster_error_log_console_show;
int32_t script_log_console_show;
int32_t config_log_console_show;

uint8_t datadp[65535];

void print_hex_ascii_line(uint8_t* payload, int len, int offset) {

	int i;
	int gap;
	uint8_t* ch;

	/* offset */
	printf("( %08X )   ", offset);

	/* hex */
	ch = payload;
	for (i = 0; i < len; i++) {
		printf("%02X ", *ch);
		ch++;
		/* print extra space after 8th byte for visual aid */
		if (i == 7)
			printf(" ");
	}
	/* print space to handle line less than 8 bytes */
	if (len < 8)
		printf(" ");

	/* fill hex gap with spaces if not full line */
	if (len < 16) {
		gap = 16 - len;
		for (i = 0; i < gap; i++) {
			printf("   ");
		}
	}
	printf("   ");

	/* ascii (if printable) */
	ch = payload;
	for (i = 0; i < len; i++) {
		if (isprint(*ch))
			printf("%c", *ch);
		else
			printf(".");
		ch++;
	}

	printf("\n");

	return;
}

void print_payload(uint8_t* payload, int len) {

	int len_rem = len;
	int line_width = 16;			/* number of bytes per line */
	int line_len;
	int offset = 0;					/* zero-based offset counter */
	uint8_t* ch = payload;

	if (len <= 0)
		return;

	/* data fits on one line */
	if (len <= line_width) {
		print_hex_ascii_line(ch, len, offset);
		return;
	}

	/* data spans multiple lines */
	for (;; ) {
		/* compute current line length */
		line_len = line_width % len_rem;
		/* print line */
		print_hex_ascii_line(ch, line_len, offset);
		/* compute total remaining */
		len_rem = len_rem - line_len;
		/* shift pointer to remaining bytes to print */
		ch = ch + line_len;
		/* add offset */
		offset = offset + line_width;
		/* check if we have line width chars or less */
		if (len_rem <= line_width) {
			/* print last line and get out */
			print_hex_ascii_line(ch, len_rem, offset);
			break;
		}
	}

	return;
}

void packet_to_text(uint8_t* buf, int32_t len)
{
	int32_t c, c2, c3, c4;

	c = c2 = c3 = c4 = 0;

	for (c = 0; c < len; c++)
	{
		if (c3 == 16)
		{
			for (; c4 < c; c4++)
				if (buf[c4] >= 0x20)
					dp[c2++] = buf[c4];
				else
					dp[c2++] = 0x2E;
			c3 = 0;
			sprintf(&dp[c2++], "\n");
		}

		if ((c == 0) || !(c % 16))
		{
			sprintf(&dp[c2], "( %08X )   ", c);
			c2 += 15;
		}

		sprintf(&dp[c2], "%02X ", buf[c]);
		c2 += 3;

		if (c3 == 7) {
			sprintf(&dp[c2], " ");
			c2 += 1;
		}

		c3++;
	}

	if (len % 16)
	{
		c3 = len;
		while (c3 % 16)
		{
			sprintf(&dp[c2], "   ");
			c2 += 3;
			c3++;
		}
	}

	for (; c4 < c; c4++)
		if (buf[c4] >= 0x20)
			dp[c2++] = buf[c4];
		else
			dp[c2++] = 0x2E;

	dp[c2] = 0;
}

//显示数据用
void display_packet(uint8_t* buf, int32_t len)
{
	packet_to_text(buf, len);
	printf("%s\n\n", &dp[0]);
}

/* 日志设置 */
void load_log_config(void)
{
	int32_t config_index = 0;
	char config_data[255] = { 0 };
	uint32_t ch;
	//int32_t spot = 0;

	FILE* fp;
	const char* Log_config = "Config\\Config_Log.ini";
	//printf("加载日志设置文件 %s ", Log_config);
	errno_t err = fopen_s(&fp, Log_config, "r");
	//printf(".");
	if (err)
	{
		printf("设置文件 %s 缺失了.\n", Log_config); //12.22
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(EXIT_FAILURE);
	}
	else {
		while (fgets(&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)  // 如果不是#行
			{
				// If IP 设置, IP 地址, or 舰船名称
				if ((config_index == 0x00) || (config_index == 0x04) || (config_index == 0x05))
				{
					// 删除换行符和回车符
					ch = strlen(&config_data[0]);
					if (config_data[ch - 1] == 0x0A)
						config_data[ch--] = 0x00;
					config_data[ch] = 0;
				}
				//printf("%d \n", atoi(&config_data[0]));
				switch (config_index) // 解析行数
				{
				case PATCH_LOG:
					// 控制台LOG显示开关
					patch_log_console_show = atoi(&config_data[0]);
					break;

				case AUTH_LOG:
					auth_log_console_show = atoi(&config_data[0]);
					break;

				case SHIPS_LOG:
					// 控制台LOG显示开关
					ships_log_console_show = atoi(&config_data[0]);
					break;

				case BLOCK_LOG:
					// 控制台LOG显示开关
					blocks_log_console_show = atoi(&config_data[0]);
					break;

				case ERR_LOG:
					// 控制台LOG显示开关
					error_log_console_show = atoi(&config_data[0]);
					break;

				case LOBBY_LOG:
					// 控制台LOG显示开关
					lobbys_log_console_show = atoi(&config_data[0]);
					break;

				case SGATE_LOG:
					// 控制台LOG显示开关
					sgate_log_console_show = atoi(&config_data[0]);
					break;

				case LOGIN_LOG:
					// 控制台LOG显示开关
					login_log_console_show = atoi(&config_data[0]);
					break;

				case ITEM_LOG:
					// 控制台LOG显示开关
					item_log_console_show = atoi(&config_data[0]);
					break;

				case MYSQLERR_LOG:
					// 控制台LOG显示开关
					mysqlerr_log_console_show = atoi(&config_data[0]);
					break;

				case QUESTERR_LOG:
					// 控制台LOG显示开关
					questerr_log_console_show = atoi(&config_data[0]);
					break;

				case GM_LOG:
					// 控制台LOG显示开关
					gm_log_console_show = atoi(&config_data[0]);
					break;

				case DEBUG_LOG:
					// 控制台LOG显示开关
					debug_log_console_show = atoi(&config_data[0]);
					break;

				case FILE_LOG:
					// 控制台LOG显示开关
					file_log_console_show = atoi(&config_data[0]);
					break;

				case HOST_LOG:
					// 控制台LOG显示开关
					host_log_console_show = atoi(&config_data[0]);
					break;

				case UNKNOW_PACKET_LOG:
					// 控制台LOG显示开关
					unknow_packet_log_console_show = atoi(&config_data[0]);
					break;

				case UNDONE_PACKET_LOG:
					// 控制台LOG显示开关
					undone_packet_log_console_show = atoi(&config_data[0]);
					break;

				case UNUSED:
					// 控制台LOG显示开关
					unused_log_show = atoi(&config_data[0]);
					break;

				case DC_LOG:
					// 控制台LOG显示开关
					disconnect_log_console_show = atoi(&config_data[0]);
					break;

				case DONT_SEND_LOG:
					// 控制台LOG显示开关
					dont_send_log_console_show = atoi(&config_data[0]);
					break;

				case TEST_LOG:
					// 控制台LOG显示开关
					test_log_console_show = atoi(&config_data[0]);
					break;

				case MONSTERID_ERR_LOG:
					// 控制台LOG显示开关
					monster_error_log_console_show = atoi(&config_data[0]);
					break;

				case CONFIG_LOG:
					// 控制台LOG显示开关
					config_log_console_show = atoi(&config_data[0]);
					break;

				case SCRIPT_LOG:
					// 控制台LOG显示开关
					script_log_console_show = atoi(&config_data[0]);
					break;

				case LOG:
					console_log_hide_or_show = atoi(&config_data[0]);
					break;

				default:
					break;
				}
				config_index++;
			}
		}
		//printf(".");
		fclose(fp);
	}

	if (config_index < LOG_FILES_MAX)
	{
		printf("%s 文件貌似已损坏.\n", Log_config);
		printf("按下 [回车键] 退出");
		gets_s(&dp[0], 0);
		exit(EXIT_FAILURE);
	}
	//printf(".");
	//printf(" 完成!\n");
}

void color(uint32_t x)	//自定义函根据参数改变颜色 
{
	if (x % 16 == 0)
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	else
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), x);

	//if (x >= 0 && x <= 15)//参数在0-15的范围颜色
	//	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), x);	//只有一个参数，改变字体颜色 
	//else//默认的颜色白色
	//	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void Logs(int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...)
{
	va_list args;
	char mes[4096] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//strcat(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Log\\%u年%02u月%02u日", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u年%02u月%02u日 日志目录创建成功", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u年%02u月%02u日 日志目录已存在", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	strcat(logfile, "\\");
	strcat(logfile, log_server_name[log_server_name_num]);
	strcat(logfile, "_");
	strcat(logfile, log_header[files_num]);
	strcat(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[files_num], codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num]);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_server_name[log_server_name_num], log_header[files_num], codeline, mes))
		{
			color(files_num);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[files_num], codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num]);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[files_num], codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void err_Logs(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...)
{
	va_list args;
	char mes[4096] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//strcat(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Error\\%u年%02u月%02u日", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u年%02u月%02u日 日志目录创建成功", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u年%02u月%02u日 日志目录已存在", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	strcat(logfile, "\\");
	strcat(logfile, log_server_name[log_server_name_num]);
	strcat(logfile, "_");
	strcat(logfile, log_header[files_num]);
	strcat(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[files_num],
			func, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num]);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_server_name[log_server_name_num], log_header[files_num], func, codeline, mes))
		{
			color(4);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[files_num],
				func, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num]);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(4);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/,
					log_header[files_num], func, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void dbg_Logs(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...)
{
	va_list args;
	char mes[4096] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//strcat(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Debug\\%u年%02u月%02u日", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u年%02u月%02u日 日志目录创建成功", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u年%02u月%02u日 日志目录已存在", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	strcat(logfile, "\\");
	strcat(logfile, log_server_name[log_server_name_num]);
	strcat(logfile, "_");
	strcat(logfile, log_header[files_num]);
	strcat(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[files_num],
			func, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num]);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_server_name[log_server_name_num], log_header[files_num], func, codeline, mes))
		{
			color(4);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[files_num],
				func, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num]);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/,
					log_header[files_num], func, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void Logs_undone(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...)
{
	va_list args;
	char mes[4096] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//strcat(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "未完成指令日志\\%u年%02u月%02u日", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u年%02u月%02u日 日志目录创建成功", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u年%02u月%02u日 日志目录已存在", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	strcat(logfile, "\\");
	strcat(logfile, files_name);
	strcat(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[UNDONE_PACKET_LOG], codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, files_name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_server_name[log_server_name_num], log_header[UNDONE_PACKET_LOG], codeline, mes))
		{
			color(UNDONE_PACKET_LOG);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[UNDONE_PACKET_LOG], codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[UNDONE_PACKET_LOG]);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(UNDONE_PACKET_LOG);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[UNDONE_PACKET_LOG], codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void Logs_unknow(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...)
{
	va_list args;
	char mes[4096] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//strcat(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "未处理指令日志\\%u年%02u月%02u日", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u年%02u月%02u日 日志目录创建成功", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u年%02u月%02u日 日志目录已存在", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	strcat(logfile, "\\");
	strcat(logfile, files_name);
	strcat(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[UNKNOW_PACKET_LOG], codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, files_name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_server_name[log_server_name_num], log_header[UNKNOW_PACKET_LOG], codeline, mes))
		{
			color(UNKNOW_PACKET_LOG);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[UNKNOW_PACKET_LOG], codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[UNKNOW_PACKET_LOG]);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(UNKNOW_PACKET_LOG);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds/*, log_server_name[log_server_name_num]*/, log_header[UNKNOW_PACKET_LOG], codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

/* 截取舰船未处理的数据包 */
void unk_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename)
{
	uint16_t size;
	size = *(uint16_t*)&pkt[0];
	Logs(codeline, error_log_console_show, ERR_LOG, "%s %d 行 %s 指令 0x%02X%02X 未处理. (数据如下)", filename, codeline, cmd, pkt[3], pkt[2]);
	packet_to_text(pkt, size);
	Logs(codeline, error_log_console_show, ERR_LOG, "\n%s\n", &dp[0]);
}

/* 截取舰船未完成的数据包 */
void udone_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename)
{
	uint16_t size;
	size = *(uint16_t*)&pkt[0];
	Logs_undone(codeline, undone_packet_log_console_show, cmd, "%s %d 行 %s 指令 0x%02X%02X 未完成. (数据如下)", filename, codeline, cmd, pkt[3], pkt[2]);
	packet_to_text(pkt, size);
	Logs_undone(codeline, undone_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

/* 截取客户端未处理的数据包 */
void unk_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename)
{
	uint16_t size;
	size = *(uint16_t*)&pkt[0];
	Logs_unknow(codeline, unknow_packet_log_console_show, cmd, "%s %d 行 %s 指令 0x%02X%02X 未处理. (数据如下)", filename, codeline, cmd, pkt[3], pkt[2]);
	packet_to_text(pkt, size);
	Logs_unknow(codeline, unknow_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

/* 截取客户端未处理的数据包 */
void udone_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename)
{
	uint16_t size;
	size = *(uint16_t*)&pkt[0];
	Logs_undone(codeline, undone_packet_log_console_show, cmd, "%s %d 行 %s 指令 0x%02X%02X 未完成. (数据如下)", filename, codeline, cmd, pkt[3], pkt[2]);
	packet_to_text(pkt, size);
	Logs_undone(codeline, undone_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

void cpd_log(int32_t codeline, int code, void* data, int flag) {
	switch (flag) {
	case 0:
		unk_cpd(c_cmd_name(code, 0), data, codeline, __FILE__);
		break;

	case 1:
		udone_cpd(c_cmd_name(code, 0), data, codeline, __FILE__);
		break;
	}
}

void spd_log(int32_t codeline, int code, void* data, int flag) {
	switch (flag) {
	case 0:
		unk_spd(c_cmd_name(code, 0), data, codeline, __FILE__);
		break;

	case 1:
		udone_spd(c_cmd_name(code, 0), data, codeline, __FILE__);
		break;
	}
}