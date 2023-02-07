/****************************************************************/
/*	Author:	Sodaboy												*/
/*	Date:	07/22/2008											*/
/*	accountadd.c :  Adds an account to the Tethealla PSO		*/
/*			server...											*/
/*																*/
/*	History:													*/
/*		07/22/2008  TC  First version...						*/
/****************************************************************/

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <mysql.h>
#include <md5.h>
#include <config_files.h>
#include <database.h>

void UpdateDataFile(const char* filename, unsigned count, void* data, unsigned record_size, int new_record)
{
	FILE* fp;
	unsigned fs;

	fopen_s(&fp, filename, "r+b");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		fs = ftell(fp);
		if ((count * record_size) <= fs)
		{
			fseek(fp, count * record_size, SEEK_SET);
			fwrite(data, 1, record_size, fp);
		}
		else
			printf("无查询到 %s\n", filename);
		fclose(fp);
	}
	else
	{
		fopen_s(&fp, filename, "wb");
		if (fp)
		{
			fwrite(data, 1, record_size, fp); // Has to be the first record...
			fclose(fp);
		}
		else
			printf("无法打开修改 %s !\n", filename);
	}
}

void LoadDataFile(const char* filename, unsigned* count, void** data, unsigned record_size)
{
	FILE* fp;
	unsigned ch;

	printf("读取 \"%s\" ... ", filename);
	fopen_s(&fp, filename, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		*count = ftell(fp) / record_size;
		fseek(fp, 0, SEEK_SET);
		for (ch = 0; ch < *count; ch++)
		{
			data[ch] = malloc(record_size);
			if (!data[ch])
			{
				printf("内存不足!\n按下 [回车键] 退出");
				exit(EXIT_FAILURE);
			}
			fread(data[ch], 1, record_size, fp);
		}
		fclose(fp);
	}
	printf("读取完成!\n");
}


/********************************************************
**
**		main  :-
**
********************************************************/

int main(int argc, char* argv[])
{
	char inputstr[255] = { 0 };
	char username[17] = { 0 };
	char password[34] = { 0 };
	char password_check[17] = { 0 };
	char md5password[34] = { 0 };
	char email[255] = { 0 };
	char email_check[255] = { 0 };
	size_t ch;
	time_t regtime;
	unsigned reg_seconds;

	MYSQL* DBconn;
	char myQuery[255] = { 0 };
	MYSQL_ROW myRow;
	MYSQL_RES* myResult;
	unsigned char max_fields;
	int num_rows, pw_ok, pw_same;
	unsigned guildcard_number;

	char mySQL_Host[255] = { 0 };
	char mySQL_Username[255] = { 0 };
	char mySQL_Password[255] = { 0 };
	char mySQL_Database[255] = { 0 };
	unsigned int mySQL_Port;
	int config_index = 0;
	char config_data[255] = { 0 };

	unsigned char MDBuffer[17] = { 0 };

	FILE* fp;

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
			printf("无法选择数据表 %s !\n", mySQL_Database);
			mysql_close(DBconn);
			return 2;
		}
	}
	else {
		printf("无法连接至MySQL (%s) 端口 %d !\nmysql_error = %s\n",
			mySQL_Host, mySQL_Port, mysql_error(DBconn));

		mysql_close(DBconn);
		return 1;
	}

	printf("梦幻之星中国 服务器 账户 新增程序\n");
	printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	pw_ok = 0;
	while (!pw_ok)
	{
		printf("数据表名称: %s \n", AUTH_ACCOUNT);
		printf("新的账户名称: ");
		scanf_s("%254s", inputstr, _countof(inputstr));
		if (strlen(inputstr) < 17)
		{
			sprintf_s(myQuery, _countof(myQuery), "SELECT * from %s WHERE username='%s'", AUTH_ACCOUNT, inputstr);
			// Check to see if that account already exists.
			//printf ("Executing MySQL query: %s\n", myQuery );
			if (!mysql_query(DBconn, &myQuery[0]))
			{
				myResult = mysql_store_result(DBconn);
				num_rows = (int)mysql_num_rows(myResult);
				if (num_rows)
				{
					printf("服务器上已经有一个该名称的帐户.\n");
					myRow = psocn_db_result_fetch(myResult);
					max_fields = mysql_num_fields(myResult);
					printf("输出数据:\n");
					printf("-=-=-=-=-\n");
					for (ch = 0; ch < max_fields; ch++)
						printf("字段 %u = %s\n", ch, myRow[ch]);
				}
				else
					pw_ok = 1;
				mysql_free_result(myResult);
			}
			else
			{
				printf("无法查询 MySQL 服务器.\n");
				return 1;
			}
		}
		else
			printf("账户名称长度应低于16个字符.\n");
	}
	memcpy(&username[0], &inputstr[0], strlen(inputstr) + 1);
	// Gunna use this to salt it up
	regtime = time(NULL);
	pw_ok = 0;
	while (!pw_ok)
	{
		printf("新账户密码: ");
		scanf_s("%254s", inputstr, _countof(inputstr));
		if ((strlen(inputstr) < 17) || (strlen(inputstr) < 8))
		{
			memcpy(&password[0], &inputstr[0], 17);
			printf("确认密码: ");
			scanf_s("%254s", inputstr, _countof(inputstr));
			memcpy(&password_check[0], &inputstr[0], 17);
			pw_same = 1;
			for (ch = 0; ch < 16; ch++)
			{
				if (password[ch] != password_check[ch])
					pw_same = 0;
			}
			if (pw_same)
				pw_ok = 1;
			else
				printf("输入的密码不匹配.\n");
		}
		else
			printf("密码长度应低于16个字符.\n");
	}
	pw_ok = 0;
	while (!pw_ok)
	{
		printf("新账户邮箱地址: ");
		scanf_s("%254s", inputstr, _countof(inputstr));
		memcpy(&email[0], &inputstr[0], strlen(inputstr) + 1);
		// Check to see if the e-mail address has already been registered to an account.
		sprintf_s(myQuery, _countof(myQuery), "SELECT * from %s WHERE email='%s'", AUTH_ACCOUNT, email);
		//printf ("Executing MySQL query: %s\n", myQuery );
		if (!mysql_query(DBconn, &myQuery[0]))
		{
			myResult = mysql_store_result(DBconn);
			num_rows = (int)mysql_num_rows(myResult);
			mysql_free_result(myResult);
			if (num_rows)
				printf("该邮箱已在服务器中注册使用.\n");
		}
		else
		{
			printf("无法查询 MySQL 数据库.\n");
			return 1;
		}
		if (!num_rows)
		{
			printf("确认邮件地址: ");
			scanf_s("%254s", inputstr, _countof(inputstr));
			memcpy(&email_check[0], &inputstr[0], strlen(inputstr) + 1);
			pw_same = 1;
			for (ch = 0; ch < strlen(email); ch++)
			{
				if (email[ch] != email_check[ch])
					pw_same = 0;
			}
			if (pw_same)
				pw_ok = 1;
			else
				printf("输入的电子邮件地址不匹配.\n");
		}
	}
	// Check to see if any accounts already registered in the database at all.
	sprintf_s(myQuery, _countof(myQuery), "SELECT * from %s", AUTH_ACCOUNT);
	//printf ("Executing MySQL query: %s\n", myQuery );
	// Check to see if the e-mail address has already been registered to an account.
	if (!mysql_query(DBconn, &myQuery[0]))
	{
		myResult = mysql_store_result(DBconn);
		num_rows = (int)mysql_num_rows(myResult);
		mysql_free_result(myResult);
		printf("当前服务器已注册有 %i 个账户.\n", num_rows);
	}
	else
	{
		printf("无法查询 MySQL 服务器.\n");
		return 1;
	}
	reg_seconds = (unsigned)regtime;
	ch = (unsigned char)strlen(&password[0]);

	errno_t err = _itoa_s(reg_seconds, config_data, sizeof(config_data), 10);
	if (err)
		return 1;
	//Throw some salt in the game ;)

	printf("新密码为 = %s reg_seconds = %d\n", password, reg_seconds);

	sprintf_s(&password[strlen(password)], _countof(password) - strlen(password), "_%s_salt", &config_data[0]);
	md5((unsigned char*)password, strlen(password), MDBuffer);
	//md5_file(&password[0], &MDBuffer[0]);
	for (ch = 0; ch < 16; ch++)
		sprintf_s(&md5password[ch * 2], 
			_countof(md5password) - (ch * 2), "%02x", (unsigned char)MDBuffer[ch]);
	md5password[32] = 0;
	if (!num_rows)
	{
		/* First account created is always GM. */
		guildcard_number = 00000001;

		sprintf_s(myQuery, _countof(myQuery), "INSERT into %s (username,password,email,regtime,guildcard,isgm,isactive) VALUES ('%s','%s','%s','%u','%u','1','1')", AUTH_ACCOUNT, username, md5password, email, reg_seconds, guildcard_number);
	}
	else
	{
		sprintf_s(myQuery, _countof(myQuery), "INSERT into %s (username,password,email,regtime,isactive) VALUES ('%s','%s','%s','%u','1')", AUTH_ACCOUNT, username, md5password, email, reg_seconds);
	}
	// Insert into table.
	//printf ("Executing MySQL query: %s\n", myQuery );

	if (!mysql_query(DBconn, &myQuery[0]))
		printf("账户已成功添加至数据库!");
	else
	{
		printf("无法查询 MySQL 服务器.\n");
		return 1;
	}

	for (;;) {

	}

	mysql_close(DBconn);
	return 0;
}
