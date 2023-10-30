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

	// �����ַ�����ֱ�������ַ���������'\0'
	for (int i = 0; str[i] != '\0'; i++) {
		// �ж��ַ��Ƿ�Ϊ�ո���Ʊ���ȷǿ��ַ�
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
	set_console_title("�λ�֮���й� %s %s�汾 Ver%s ���� Sancaros", servername, PSOBBCN_PLATFORM_STR, ver);
	//system("color F7");
	printf(" <Ctrl-C> �رճ���.\n");
	printf(" ______                                ______\n");
	printf(" __    �λ�֮���й� %s    __\n", servername);
	printf("      http://www.phantasystaronline.cn:88    \n");
	printf(" ______       %s - %s�汾       ______\n", PSOBBCN_PLATFORM_STR, PSOBBCN_LINKAGE_TYPE_STR);
	printf("        %s       \n", ver);
	printf(" ______    ���� sancaros@msn.cn        ______\n\n");
	//printf("\n�λ�֮���й� %s %s-%s�汾 %s\n", servername, PSOBBCN_PLATFORM_STR, PSOBBCN_LINKAGE_TYPE_STR, ver);
	////printf("\n��Ȩ���� (C) 2008  Terry Chatman Jr.\n");
	//printf("\n���±��� Sancaros. %s\n\n", __DATE__);
	//printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	//printf("����δ��֤����ȫ����: ����μ�˵��\n");
	//printf("�����GPL-3.0.TXT�еĵ�15��\n");
	//printf("����������,��ӭ�����·���\n");
	//printf("�����������GPL-3.0.TXT.\n");
	printf("\n");

	char* localLanguage = setlocale(LC_ALL, "");
	if (localLanguage == NULL)
	{
		printf("��ȡ������������ʧ��\n");
	}
	setlocale(LC_CTYPE, localLanguage);
	if (setlocale(LC_CTYPE, "") == NULL)
	{ /*����Ϊ���ػ������������locale*/
		fprintf(stderr, "�޷����ñ�������\n �Ѹ�ΪUT8��������");
		setlocale(LC_CTYPE, "utf8");
	}
	printf("������������Ϊ %s\n\n", localLanguage);

}
