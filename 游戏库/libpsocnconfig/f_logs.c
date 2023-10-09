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
	struct tm tm = { 0 };
	SYSTEMTIME rawtime;
	GetLocalTime(&rawtime);
	tm.tm_year = rawtime.wYear - 1900;
	tm.tm_mon = rawtime.wMonth - 1;
	tm.tm_mday = rawtime.wDay;
	tm.tm_hour = rawtime.wHour;
	tm.tm_min = rawtime.wMinute;
	tm.tm_sec = rawtime.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	timevaltmp->tv_sec = (long)clock;
	timevaltmp->tv_usec = rawtime.wMilliseconds * 1000;
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
int32_t dns_log_console_show;
int32_t crash_log_console_show;

int32_t login_log_console_show;
int32_t item_log_console_show;
int32_t mysqlerr_log_console_show;
int32_t questerr_log_console_show;
int32_t gmc_log_console_show;
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
int32_t config_log_console_show;
int32_t script_log_console_show;

bool is_all_zero(const char* data, size_t length) {
	for (size_t i = 0; i < length; i++) {
		if (data[i] != 0) {
			return false;
		}
	}
	return true;
}

void print_ascii_hex(void (*print_method)(const char*), const void* data, size_t length) {
	pthread_mutex_lock(&pkt_mutex);
	size_t i;
	memset(&dp[0], 0, sizeof(dp));

	if (data == NULL || length <= 0 || length > MAX_PACKET_BUFF) {
		sprintf(dp, "空指针数据包或无效长度 % d 数据包.", length);
		goto done;
	}

	uint8_t* buff = (uint8_t*)data;

	if (is_all_zero(buff, length)) {
		sprintf(dp, "空数据包 长度 %d.", length);
		goto done;
	}

	strcpy(dp, "数据包如下:\n\r");	

	for (i = 0; i < length; i++) {
		if (i % 16 == 0) {
			if (i != 0) {
				SAFE_STRCAT(dp, "\n");
			}
			sprintf(dp + strlen(dp), "(%08X)", (unsigned int)i);
		}
		sprintf(dp + strlen(dp), " %02X", (unsigned char)buff[i]);

		if (i % 8 == 7 && i % 16 != 15) {
			SAFE_STRCAT(dp, " "); // 在8个二进制后增加一个空格
		}

		if (i % 16 == 15 || i == length - 1) {
			size_t j;
			SAFE_STRCAT(dp, "    ");
			for (j = i - (i % 16); j <= i; j++) {
				if (j >= length) {
					SAFE_STRCAT(dp, " ");
				}
				else if (buff[j] >= ' ' && buff[j] <= '~') {
					char tmp_str[2] = { buff[j], '\0' };
					SAFE_STRCAT(dp, tmp_str);
				}
				else {
					SAFE_STRCAT(dp, ".");
				}
			}
		}
	}

	if (strlen(dp) + 2 + 1 <= MAX_PACKET_BUFF) { // 检查长度是否足够
		SAFE_STRCAT(dp, "\n\r"); // 添加两个换行符
	}
	else {
		print_method("不足以容纳换行符");
	}

done:
	print_method(dp);

	pthread_mutex_unlock(&pkt_mutex);
}

/* This function based on information from a couple of different sources, namely
   Fuzziqer's newserv and information from Lee (through Aleron Ives). */
double expand_rate(uint8_t rate) {
	int tmp = (rate >> 3) - 4;
	uint32_t expd;

	if (tmp < 0)
		tmp = 0;

	expd = (2 << tmp) * ((rate & 7) + 7);
	return (double)expd / (double)0x100000000ULL;
}

uint32_t byteswap(uint32_t e) {
	return (((e >> 24) & 0xFF) | (((e >> 16) & 0xFF) << 8) | (((e >> 8) & 0xFF) << 16) | ((e & 0xFF) << 24));
}

uint32_t be_convert_to_le_uint32(uint8_t item_code[3]) {
	uint32_t result = 0;

	result |= ((uint32_t)item_code[2]) << 16;
	result |= ((uint32_t)item_code[1]) << 8;
	result |= item_code[0];

	return result;
}

uint32_t little_endian_value(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
	return ((uint32_t)b4 << 24) | ((uint32_t)b3 << 16) | ((uint32_t)b2 << 8) | b1;
}

void packet_to_text(uint8_t* buf, size_t len, bool show) {
	static const char hex_digits[] = "0123456789ABCDEF";

	memset(&dp[0], 0, sizeof(dp));

	size_t c, c2, c3, c4;
	c = c2 = c3 = c4 = 0;

	for (c = 0; c < len; c++) {
		if (c3 == 16) {
			for (; c4 < c; c4++) {
				if (buf[c4] >= 0x20)
					dp[c2++] = buf[c4];
				else
					dp[c2++] = 0x2E;
			}
			c3 = 0;
			dp[c2++] = '\n';
		}

		if ((c == 0) || !(c % 16)) {
			sprintf(&dp[c2], "( %08X )   ", c);
			c2 += 15;
		}

		dp[c2++] = hex_digits[buf[c] >> 4];
		dp[c2++] = hex_digits[buf[c] & 0x0F];
		dp[c2++] = ' ';

		if (c3 == 7) {
			dp[c2++] = ' ';
		}

		if ((c + 1) % 8 == 0) {  // 每8个字节空一格
			dp[c2++] = ' ';
		}

		c3++;
	}

	if (len % 16) {
		c3 = len;
		while (c3 % 16) {
			dp[c2++] = ' ';
			dp[c2++] = ' ';
			dp[c2++] = ' ';
			c3++;
		}
	}

	for (; c4 < c; c4++) {
		if (buf[c4] >= 0x20)
			dp[c2++] = buf[c4];
		else
			dp[c2++] = 0x2E;
	}

	//dp[c2] = '\0';

	if (show) {
		// 打印结果
		char* pch = dp;
		while (*pch != '\0') {
			putchar(*pch);
			pch++;
		}
		printf("\n");
	}
}

//显示数据用
void display_packet_old(const void* buf, size_t len) {
	uint8_t* buff = (uint8_t*)buf;

	if (!buff)
		ERR_LOG("无法显示数据包");
	else
		packet_to_text(buff, len, true);
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

				case ITEMS_LOG:
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

				case GMC_LOG:
					// 控制台LOG显示开关
					gmc_log_console_show = atoi(&config_data[0]);
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

				case DNS_LOG:
					// 控制台LOG显示开关
					dns_log_console_show = atoi(&config_data[0]);
					break;

				case CRASH_LOG:
					// 控制台LOG显示开关
					crash_log_console_show = atoi(&config_data[0]);
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

void flog(int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...)
{
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
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
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, server_name[server_name_num].name);
	SAFE_STRCAT(logfile, "_");
	SAFE_STRCAT(logfile, log_header[files_num].name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, codeline, mes))
		{
			color(files_num);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
		}

		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void flog_item(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...)
{
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
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
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, server_name[server_name_num].name);
	SAFE_STRCAT(logfile, "_");
	SAFE_STRCAT(logfile, log_header[files_num].name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
			func, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, func, codeline, mes))
		{
			color(files_num);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
				func, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds,
					log_header[files_num].name, func, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void flog_file(int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* file, const char* fmt, ...) {
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "%s\\%u年%02u月%02u日.", file, rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u年%02u月%02u日 日志目录创建成功", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u年%02u月%02u日 日志目录已存在", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, server_name[server_name_num].name);
	SAFE_STRCAT(logfile, "_");
	SAFE_STRCAT(logfile, log_header[files_num].name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, codeline, mes))
		{
			color(files_num);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
		}

		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void flog_err(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...)
{
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
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
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, server_name[server_name_num].name);
	SAFE_STRCAT(logfile, "_");
	SAFE_STRCAT(logfile, log_header[files_num].name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
			func, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, func, codeline, mes))
		{
			color(4);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
				func, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(4);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds,
					log_header[files_num].name, func, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void flog_crash(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...)
{
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Crash\\%u年%02u月%02u日", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u年%02u月%02u日 日志目录创建成功", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u年%02u月%02u日 日志目录已存在", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, server_name[server_name_num].name);
	SAFE_STRCAT(logfile, "_");
	SAFE_STRCAT(logfile, log_header[files_num].name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
			func, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, func, codeline, mes))
		{
			color(4);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
				func, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(4);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds,
					log_header[files_num].name, func, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void flog_debug(const char* func, int32_t codeline, uint32_t consoleshow, uint32_t files_num, const char* fmt, ...)
{
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;
	int fmt_size = 0;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	mes[MAX_PACKET_BUFF - 1] = '\0';
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
	strncat_s(logfile, sizeof(logfile), "\\", 1);
	strncat_s(logfile, sizeof(logfile), server_name[server_name_num].name, strlen(server_name[server_name_num].name));
	strncat_s(logfile, sizeof(logfile), "_", 1);
	strncat_s(logfile, sizeof(logfile), log_header[files_num].name, strlen(log_header[files_num].name));
	strncat_s(logfile, sizeof(logfile), ".log", 4);
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
			func, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, func, codeline, mes))
		{
			color(4);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
				func, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[files_num].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds,
					log_header[files_num].name, func, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void flog_undone(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...)
{
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
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
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, files_name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNDONE_PACKET_LOG].name, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, files_name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[UNDONE_PACKET_LOG].name, codeline, mes))
		{
			color(UNDONE_PACKET_LOG);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNDONE_PACKET_LOG].name, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[UNDONE_PACKET_LOG].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(UNDONE_PACKET_LOG);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNDONE_PACKET_LOG].name, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void flog_unknow(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...)
{
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
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
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, files_name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, files_name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[UNKNOW_PACKET_LOG].name, codeline, mes))
		{
			color(UNKNOW_PACKET_LOG);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[UNKNOW_PACKET_LOG].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(UNKNOW_PACKET_LOG);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

void flog_err_packet(int32_t codeline, uint32_t consoleshow, const char* files_name, const char* fmt, ...)
{
	va_list args;
	char mes[MAX_PACKET_BUFF] = { 0 };
	//char headermes[128] = { 0 };
	//char text[4096] = { 0 };
	SYSTEMTIME rawtime;

	FILE* fp;
	char logdir[64] = { 0 };
	char logfile[256] = { 0 };
	GetLocalTime(&rawtime);

	//snprintf(headermes, sizeof(headermes), "[%u年%02u月%02u日 %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "错误数据包\\%u年%02u月%02u日", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u年%02u月%02u日 日志目录创建成功", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u年%02u月%02u日 日志目录已存在", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, files_name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
		printf("代码 %d 行,存储 %s.log 日志发生错误\n", codeline, files_name);
	}
	else
	{
		if (!fprintf(fp, "[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[UNKNOW_PACKET_LOG].name, codeline, mes))
		{
			color(UNKNOW_PACKET_LOG);
			printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
			printf("代码 %d 行,记录 %s.log 日志发生错误\n", codeline, log_header[UNKNOW_PACKET_LOG].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(UNKNOW_PACKET_LOG);
				printf("[%u年%02u月%02u日 %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

/* 截取舰船未处理的数据包 */
void unk_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog(codeline, error_log_console_show, ERR_LOG, "%s %s 指令 0x%02X%02X 未处理.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog(codeline, error_log_console_show, ERR_LOG, "\n%s\n", &dp[0]);
}

/* 截取舰船未完成的数据包 */
void udone_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog_undone(codeline, undone_packet_log_console_show, cmd, "%s %s 指令 0x%02X%02X 未完成.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog_undone(codeline, undone_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

/* 截取舰船错误的数据包 */
void err_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog_err_packet(codeline, undone_packet_log_console_show, cmd, "%s %s 指令 0x%02X%02X 数据错误.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog_err_packet(codeline, undone_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

/* 截取客户端未处理的数据包 */
void unk_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog_unknow(codeline, unknow_packet_log_console_show, cmd, "%s %s 指令 0x%02X%02X 未处理.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog_unknow(codeline, unknow_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

/* 截取客户端未处理的数据包 */
void udone_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog_undone(codeline, undone_packet_log_console_show, cmd, "%s %s 指令 0x%02X%02X 未完成.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog_undone(codeline, undone_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
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

int remove_directory(const char* path) {
	WIN32_FIND_DATA find_data;
	HANDLE find_handle = INVALID_HANDLE_VALUE;
	char file_path[MAX_PATH];

	snprintf(file_path, MAX_PATH, "%s\\*", path);

	find_handle = FindFirstFile(file_path, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE) {
		ERR_LOG("无法打开文件夹：%s", path);
		return -1;
	}

	do {
		if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
			snprintf(file_path, MAX_PATH, "%s\\%s", path, find_data.cFileName);
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				remove_directory(file_path);
			}
			else {
				if (!DeleteFile(file_path)) {
					ERR_LOG("无法删除文件：%s", file_path);
					return -1;
				}
			}
		}
	} while (FindNextFile(find_handle, &find_data) != 0);

	FindClose(find_handle);

	if (!RemoveDirectory(path)) {
		ERR_LOG("无法删除文件夹：%s", path);
		return -1;
	}

#ifdef DEBUG

	DBG_LOG("成功删除文件夹：%s", path);

#endif // DEBUG
	return 0;
}

ssize_t clamp(ssize_t value, ssize_t min, ssize_t max) {
	if (value < min) {
		return min;
	}
	else if (value > max) {
		return max;
	}
	return value;
}