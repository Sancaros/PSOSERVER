#include "Software_Defines.h"
#include <locale.h> //12.22
#include <stdio.h>
#include <string.h>

void load_program_info(const char* servername, const char* ver)
{
	memset(&windows[0], 0, sizeof(dp));
	strcat(&windows[0], "�λ�֮���й� ");
	strcat(&windows[0], servername);
	strcat(&windows[0], " �汾 ");
	strcat(&windows[0], ver);
	strcat(&windows[0], " ���� Sancaros");
	SetConsoleTitle((LPCSTR)&windows[0]);
	//system("color F7");
	printf(" <Ctrl-C> �رճ���.\n");
	printf(" ______                                ______\n");
	printf(" __        �λ�֮���й� %s         __\n", servername);
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
	printf("������������Ϊ %s\n", localLanguage);

}
