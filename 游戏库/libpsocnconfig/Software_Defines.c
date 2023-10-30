#include "Software_Defines.h"
#include <locale.h> //12.22
#include <stdio.h>
#include <string.h>

void parse_version(uint8_t* maj, uint8_t* min, uint8_t* mic,
	const char* ver) {
	int v1, v2, v3;

	if (!sscanf(ver, "%d.%d.%d", &v1, &v2, &v3))
		return;

	*maj = (uint8_t)v1;
	*min = (uint8_t)v2;
	*mic = (uint8_t)v3;
}

int get_nonempty_length(const char* str) {
	int length = 0;

	// 遍历字符串，直到遇到字符串结束符'\0'
	for (int i = 0; str[i] != '\0'; i++) {
		// 判断字符是否为空格或制表符等非空字符
		if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
			length++;
		}
	}

	return length;
}

void set_console_title(const char* fmt, ...) {
	va_list args;

	va_start(args, fmt);

	memset(&windows[0], 0, sizeof(windows));
	vsnprintf(&windows[0], 4096, fmt, args);
	windows[4095] = '\0';

	va_end(args);

	SetConsoleTitle((LPCSTR)&windows[0]);
}

void load_program_info(const char* servername, const char* ver)
{
	set_console_title("梦幻之星中国 %s %s版本 Ver%s 作者 Sancaros", servername, PSOBBCN_PLATFORM_STR, ver);
	//system("color F7");
	printf(" <Ctrl-C> 关闭程序.\n");
	printf(" ______                                ______\n");
	printf(" __    梦幻之星中国 %s    __\n", servername);
	printf("      http://www.phantasystaronline.cn:88    \n");
	printf(" ______       %s - %s版本       ______\n", PSOBBCN_PLATFORM_STR, PSOBBCN_LINKAGE_TYPE_STR);
	printf("        %s       \n", ver);
	printf(" ______    作者 sancaros@msn.cn        ______\n\n");
	//printf("\n梦幻之星中国 %s %s-%s版本 %s\n", servername, PSOBBCN_PLATFORM_STR, PSOBBCN_LINKAGE_TYPE_STR, ver);
	////printf("\n版权作者 (C) 2008  Terry Chatman Jr.\n");
	//printf("\n重新编译 Sancaros. %s\n\n", __DATE__);
	//printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	//printf("程序未保证可完全运行: 详情参见说明\n");
	//printf("请参阅GPL-3.0.TXT中的第15节\n");
	//printf("这是免费软件,欢迎您重新发布\n");
	//printf("其他条款详见GPL-3.0.TXT.\n");
	printf("\n");

	char* localLanguage = setlocale(LC_ALL, "");
	if (localLanguage == NULL)
	{
		printf("获取本地语言类型失败\n");
	}
	setlocale(LC_CTYPE, localLanguage);
	if (setlocale(LC_CTYPE, "") == NULL)
	{ /*设置为本地环境变量定义的locale*/
		fprintf(stderr, "无法设置本地语言\n 已改为UT8编码语言");
		setlocale(LC_CTYPE, "utf8");
	}
	printf("本地语言类型为 %s\n\n", localLanguage);

}
