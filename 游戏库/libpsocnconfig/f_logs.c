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
// ��־
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
		sprintf(dp, "��ָ�����ݰ�����Ч���� % d ���ݰ�.", length);
		goto done;
	}

	uint8_t* buff = (uint8_t*)data;

	if (is_all_zero(buff, length)) {
		sprintf(dp, "�����ݰ� ���� %d.", length);
		goto done;
	}

	strcpy(dp, "���ݰ�����:\n\r");	

	for (i = 0; i < length; i++) {
		if (i % 16 == 0) {
			if (i != 0) {
				SAFE_STRCAT(dp, "\n");
			}
			sprintf(dp + strlen(dp), "(%08X)", (unsigned int)i);
		}
		sprintf(dp + strlen(dp), " %02X", (unsigned char)buff[i]);

		if (i % 8 == 7 && i % 16 != 15) {
			SAFE_STRCAT(dp, " "); // ��8�������ƺ�����һ���ո�
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

	if (strlen(dp) + 2 + 1 <= MAX_PACKET_BUFF) { // ��鳤���Ƿ��㹻
		SAFE_STRCAT(dp, "\n\r"); // ����������з�
	}
	else {
		print_method("���������ɻ��з�");
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

		if ((c + 1) % 8 == 0) {  // ÿ8���ֽڿ�һ��
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
		// ��ӡ���
		char* pch = dp;
		while (*pch != '\0') {
			putchar(*pch);
			pch++;
		}
		printf("\n");
	}
}

//��ʾ������
void display_packet_old(const void* buf, size_t len) {
	uint8_t* buff = (uint8_t*)buf;

	if (!buff)
		ERR_LOG("�޷���ʾ���ݰ�");
	else
		packet_to_text(buff, len, true);
}

/* ��־���� */
void load_log_config(void)
{
	int32_t config_index = 0;
	char config_data[255] = { 0 };
	uint32_t ch;
	//int32_t spot = 0;

	FILE* fp;
	const char* Log_config = "Config\\Config_Log.ini";
	//printf("������־�����ļ� %s ", Log_config);
	errno_t err = fopen_s(&fp, Log_config, "r");
	//printf(".");
	if (err)
	{
		printf("�����ļ� %s ȱʧ��.\n", Log_config); //12.22
		printf("���� [�س���] �˳�");
		gets_s(&dp[0], 0);
		exit(EXIT_FAILURE);
	}
	else {
		while (fgets(&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)  // �������#��
			{
				// If IP ����, IP ��ַ, or ��������
				if ((config_index == 0x00) || (config_index == 0x04) || (config_index == 0x05))
				{
					// ɾ�����з��ͻس���
					ch = strlen(&config_data[0]);
					if (config_data[ch - 1] == 0x0A)
						config_data[ch--] = 0x00;
					config_data[ch] = 0;
				}
				//printf("%d \n", atoi(&config_data[0]));
				switch (config_index) // ��������
				{
				case PATCH_LOG:
					// ����̨LOG��ʾ����
					patch_log_console_show = atoi(&config_data[0]);
					break;

				case AUTH_LOG:
					auth_log_console_show = atoi(&config_data[0]);
					break;

				case SHIPS_LOG:
					// ����̨LOG��ʾ����
					ships_log_console_show = atoi(&config_data[0]);
					break;

				case BLOCK_LOG:
					// ����̨LOG��ʾ����
					blocks_log_console_show = atoi(&config_data[0]);
					break;

				case ERR_LOG:
					// ����̨LOG��ʾ����
					error_log_console_show = atoi(&config_data[0]);
					break;

				case LOBBY_LOG:
					// ����̨LOG��ʾ����
					lobbys_log_console_show = atoi(&config_data[0]);
					break;

				case SGATE_LOG:
					// ����̨LOG��ʾ����
					sgate_log_console_show = atoi(&config_data[0]);
					break;

				case LOGIN_LOG:
					// ����̨LOG��ʾ����
					login_log_console_show = atoi(&config_data[0]);
					break;

				case ITEMS_LOG:
					// ����̨LOG��ʾ����
					item_log_console_show = atoi(&config_data[0]);
					break;

				case MYSQLERR_LOG:
					// ����̨LOG��ʾ����
					mysqlerr_log_console_show = atoi(&config_data[0]);
					break;

				case QUESTERR_LOG:
					// ����̨LOG��ʾ����
					questerr_log_console_show = atoi(&config_data[0]);
					break;

				case GMC_LOG:
					// ����̨LOG��ʾ����
					gmc_log_console_show = atoi(&config_data[0]);
					break;

				case DEBUG_LOG:
					// ����̨LOG��ʾ����
					debug_log_console_show = atoi(&config_data[0]);
					break;

				case FILE_LOG:
					// ����̨LOG��ʾ����
					file_log_console_show = atoi(&config_data[0]);
					break;

				case HOST_LOG:
					// ����̨LOG��ʾ����
					host_log_console_show = atoi(&config_data[0]);
					break;

				case UNKNOW_PACKET_LOG:
					// ����̨LOG��ʾ����
					unknow_packet_log_console_show = atoi(&config_data[0]);
					break;

				case UNDONE_PACKET_LOG:
					// ����̨LOG��ʾ����
					undone_packet_log_console_show = atoi(&config_data[0]);
					break;

				case UNUSED:
					// ����̨LOG��ʾ����
					unused_log_show = atoi(&config_data[0]);
					break;

				case DC_LOG:
					// ����̨LOG��ʾ����
					disconnect_log_console_show = atoi(&config_data[0]);
					break;

				case DONT_SEND_LOG:
					// ����̨LOG��ʾ����
					dont_send_log_console_show = atoi(&config_data[0]);
					break;

				case TEST_LOG:
					// ����̨LOG��ʾ����
					test_log_console_show = atoi(&config_data[0]);
					break;

				case MONSTERID_ERR_LOG:
					// ����̨LOG��ʾ����
					monster_error_log_console_show = atoi(&config_data[0]);
					break;

				case CONFIG_LOG:
					// ����̨LOG��ʾ����
					config_log_console_show = atoi(&config_data[0]);
					break;

				case SCRIPT_LOG:
					// ����̨LOG��ʾ����
					script_log_console_show = atoi(&config_data[0]);
					break;

				case DNS_LOG:
					// ����̨LOG��ʾ����
					dns_log_console_show = atoi(&config_data[0]);
					break;

				case CRASH_LOG:
					// ����̨LOG��ʾ����
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
		printf("%s �ļ�ò������.\n", Log_config);
		printf("���� [�س���] �˳�");
		gets_s(&dp[0], 0);
		exit(EXIT_FAILURE);
	}
	//printf(".");
	//printf(" ���!\n");
}

void color(uint32_t x)	//�Զ��庯���ݲ����ı���ɫ 
{
	if (x % 16 == 0)
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	else
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), x);

	//if (x >= 0 && x <= 15)//������0-15�ķ�Χ��ɫ
	//	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), x);	//ֻ��һ���������ı�������ɫ 
	//else//Ĭ�ϵ���ɫ��ɫ
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Log\\%u��%02u��%02u��", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
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
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, codeline, mes))
		{
			color(files_num);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[files_num].name);
		}

		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Log\\%u��%02u��%02u��", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
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
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
			func, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, func, codeline, mes))
		{
			color(files_num);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
				func, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[files_num].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "%s\\%u��%02u��%02u��.", file, rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
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
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, codeline, mes))
		{
			color(files_num);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[files_num].name);
		}

		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Error\\%u��%02u��%02u��", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
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
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
			func, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, func, codeline, mes))
		{
			color(4);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
				func, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[files_num].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(4);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Crash\\%u��%02u��%02u��", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
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
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
			func, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, func, codeline, mes))
		{
			color(4);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
				func, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[files_num].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(4);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	mes[MAX_PACKET_BUFF - 1] = '\0';
	va_end(args);

	//strcpy(logfile, "Log\\");
	sprintf(logdir, "Debug\\%u��%02u��%02u��", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
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
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
			func, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, log_header[files_num].name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[files_num].name, func, codeline, mes))
		{
			color(4);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[files_num].name,
				func, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[files_num].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(files_num);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%s %04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "δ���ָ����־\\%u��%02u��%02u��", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, files_name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNDONE_PACKET_LOG].name, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, files_name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[UNDONE_PACKET_LOG].name, codeline, mes))
		{
			color(UNDONE_PACKET_LOG);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNDONE_PACKET_LOG].name, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[UNDONE_PACKET_LOG].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(UNDONE_PACKET_LOG);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "δ����ָ����־\\%u��%02u��%02u��", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, files_name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, files_name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[UNKNOW_PACKET_LOG].name, codeline, mes))
		{
			color(UNKNOW_PACKET_LOG);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[UNKNOW_PACKET_LOG].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(UNKNOW_PACKET_LOG);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
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

	//snprintf(headermes, sizeof(headermes), "[%u��%02u��%02u�� %02u:%02u] ", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);
	//SAFE_STRCAT(headermes, log_header[files]);
	va_start(args, fmt);
	strcpy(mes + vsprintf(mes, fmt, args), "\r\n");
	va_end(args);
	//strcpy(logfile, "Log\\");
	sprintf(logdir, "�������ݰ�\\%u��%02u��%02u��", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	if (!_mkdir(logdir)) {
		//printf("%u��%02u��%02u�� ��־Ŀ¼�����ɹ�", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	else
	{
		//printf("%u��%02u��%02u�� ��־Ŀ¼�Ѵ���", rawtime.wYear, rawtime.wMonth, rawtime.wDay);
	}
	strcpy(logfile, logdir);
	SAFE_STRCAT(logfile, "\\");
	SAFE_STRCAT(logfile, files_name);
	SAFE_STRCAT(logfile, ".log");
	errno_t err = fopen_s(&fp, logfile, "a");
	if (err) {
		color(4);
		printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
		printf("���� %d ��,�洢 %s.log ��־��������\n", codeline, files_name);
	}
	else
	{
		if (!fprintf(fp, "[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
			rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, server_name[server_name_num].name, log_header[UNKNOW_PACKET_LOG].name, codeline, mes))
		{
			color(UNKNOW_PACKET_LOG);
			printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay, rawtime.wHour,
				rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
			printf("���� %d ��,��¼ %s.log ��־��������\n", codeline, log_header[UNKNOW_PACKET_LOG].name);
		}
		if (console_log_hide_or_show)
		{
			if (consoleshow)
			{
				color(UNKNOW_PACKET_LOG);
				printf("[%u��%02u��%02u�� %02u:%02u:%02u:%03u] %s(%04d): %s", rawtime.wYear, rawtime.wMonth, rawtime.wDay,
					rawtime.wHour, rawtime.wMinute, rawtime.wSecond, rawtime.wMilliseconds, log_header[UNKNOW_PACKET_LOG].name, codeline, mes);
			}
		}
		color(16);
		fclose(fp);
	}
}

/* ��ȡ����δ��������ݰ� */
void unk_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog(codeline, error_log_console_show, ERR_LOG, "%s %s ָ�� 0x%02X%02X δ����.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog(codeline, error_log_console_show, ERR_LOG, "\n%s\n", &dp[0]);
}

/* ��ȡ����δ��ɵ����ݰ� */
void udone_spd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog_undone(codeline, undone_packet_log_console_show, cmd, "%s %s ָ�� 0x%02X%02X δ���.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog_undone(codeline, undone_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

/* ��ȡ������������ݰ� */
void err_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog_err_packet(codeline, undone_packet_log_console_show, cmd, "%s %s ָ�� 0x%02X%02X ���ݴ���.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog_err_packet(codeline, undone_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

/* ��ȡ�ͻ���δ��������ݰ� */
void unk_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog_unknow(codeline, unknow_packet_log_console_show, cmd, "%s %s ָ�� 0x%02X%02X δ����.", filename, cmd, pkt[3], pkt[2]);
	packet_to_text(&pkt[0], size, false);
	flog_unknow(codeline, unknow_packet_log_console_show, cmd, "\n%s\n", &dp[0]);
}

/* ��ȡ�ͻ���δ��������ݰ� */
void udone_cpd(const char* cmd, uint8_t* pkt, int32_t codeline, char* filename) {
	uint16_t size = *(uint16_t*)&pkt[0];

	flog_undone(codeline, undone_packet_log_console_show, cmd, "%s %s ָ�� 0x%02X%02X δ���.", filename, cmd, pkt[3], pkt[2]);
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
		ERR_LOG("�޷����ļ��У�%s", path);
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
					ERR_LOG("�޷�ɾ���ļ���%s", file_path);
					return -1;
				}
			}
		}
	} while (FindNextFile(find_handle, &find_data) != 0);

	FindClose(find_handle);

	if (!RemoveDirectory(path)) {
		ERR_LOG("�޷�ɾ���ļ��У�%s", path);
		return -1;
	}

#ifdef DEBUG

	DBG_LOG("�ɹ�ɾ���ļ��У�%s", path);

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