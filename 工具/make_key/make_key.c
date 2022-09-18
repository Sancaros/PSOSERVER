//#include "Share_Defines.h"
//
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <mysql.h>
#include "Config_files.h"
#include "database.h"
#include <mtwist.h>

/* random number functions */

extern void		mt_bestseed(void);
extern void		mt_seed(void);	/* Choose seed from random input. */
extern uint32_t	mt_lrand(void);	/* Generate 32-bit random value */

int main()
{
	unsigned ch;
	unsigned ship_index;
	unsigned char ship_key[128] = { 0 };
	char ship_string[512] = { 0 };
	FILE* fp;
	MYSQL* DBconn;
	char myQuery[255] = { 0 };
	char mySQL_Host[255] = { 0 };
	char mySQL_Username[255] = { 0 };
	char mySQL_Password[255] = { 0 };
	char mySQL_Database[255] = { 0 };
	unsigned int mySQL_Port;
	int config_index = 0;
	char config_data[255] = { 0 };

	init_genrand((uint32_t)time(NULL));
	//sfmt_init_gen_rand(&sfmt, (uint32_t)time(NULL));

	fopen_s(&fp, config_files[CONFIG_FILE_MYSQL], "r");
	if (fp == NULL)
	{
		printf("文件 %s 缺失.\n", config_files[CONFIG_FILE_MYSQL]);
		return 1;
	}
	else
		while (fgets(&config_data[0], 255, fp) != NULL)
		{
			if (config_data[0] != 0x23)
			{
				if (config_index < 0x04)
				{
					ch = strlen(&config_data[0]);
					if (config_data[ch - 1] == 0x0A)
						config_data[ch--] = 0x00;
					config_data[ch] = 0;
				}
				switch (config_index)
				{
				case MYSQL_CONFIG_HOST:
					// MySQL 地址
					memcpy(&mySQL_Host[0], &config_data[0], ch + 1);
					break;
				case MYSQL_CONFIG_USER:
					// MySQL 用户名
					memcpy(&mySQL_Username[0], &config_data[0], ch + 1);
					break;
				case MYSQL_CONFIG_PASS:
					// MySQL 密码
					memcpy(&mySQL_Password[0], &config_data[0], ch + 1);
					break;
				case MYSQL_CONFIG_DATABASE:
					// MySQL 数据库
					memcpy(&mySQL_Database[0], &config_data[0], ch + 1);
					break;
				case MYSQL_CONFIG_PORT:
					// MySQL 端口
					mySQL_Port = atoi(&config_data[0]);
					break;
				default:
					break;
				}
				config_index++;
			}
		}
	fclose(fp);

	if (config_index < MYSQL_CONFIG_MAX)
	{
		printf("文件 %s 已损坏.\n", config_files[CONFIG_FILE_MYSQL]);
		return 1;
	}

	if ((DBconn = mysql_init((MYSQL*)0)) &&
		mysql_real_connect(DBconn, &mySQL_Host[0], &mySQL_Username[0], &mySQL_Password[0], NULL, mySQL_Port,
			NULL, 0))
	{
		if (mysql_select_db(DBconn, &mySQL_Database[0]) < 0) {
			printf("无法选择数据 %s !\n", mySQL_Database);
			mysql_close(DBconn);
			return 2;
		}
	}
	else
	{
		printf("无法连接至数据库 (%s) 端口 %d !\nmysql_error = %s\n",
			mySQL_Host, mySQL_Port, mysql_error(DBconn));

		mysql_close(DBconn);
		return 1;
	}
	printf("梦幻之星中国 舰船服务器 密钥生成器\n");
	printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");

	for (ch = 0; ch < 128; ch++)
		ship_key[ch] = genrand_int32() % 256;
	mysql_real_escape_string(DBconn, &ship_string[0], &ship_key[0], 0x80);

	sprintf_s(myQuery, _countof(myQuery), "INSERT into %s (rc4key) VALUES ('%s')", SERVER_SHIPS_DATA, ship_string);
	if (!mysql_query(DBconn, &myQuery[0]))
	{
		ship_index = (unsigned)mysql_insert_id(DBconn);
		printf("密钥已成功添加入数据库! 编号: %u\n", ship_index);
	}
	else
	{
		printf("无法查询 MySQL 服务器.\n");
		return 1;
	}

	mysql_close(DBconn);

	//char* keyfilename[255];
	//strcpy(keyfilename, "ShipKey\\ship_key_");
	//strcat(keyfilename, (char*)ship_index);
	//strcat(keyfilename, ".bin");
	//printf("写入 %s... ", keyfilename);
	printf("写入 ship_key_%d.bin... \n", ship_index);

	errno_t fps = fopen_s(&fp, "System\\ShipKey\\ship_key.bin", "wb");
	if (fps)
	{
		printf("无法写入 System\\ShipKey\\ship_key.bin \n");//, keyfilename);
	}
	else
	{
		fwrite(&ship_index, 1, 4, fp);
		fwrite(&ship_key, 1, 128, fp);
		fclose(fp);
		printf("完成!!!\n");
	}
	printf("请按 [回车键] \n");
	//getchar();
	return 0;
}
