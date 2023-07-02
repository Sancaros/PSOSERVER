/****************************************************************/
/*	Author:	Sodaboy												*/
/*	Date:	07/22/2008											*/
/*	accountadd.c :  Adds an account to the Tethealla PSO		*/
/*			server...											*/
/*																*/
/*	History:													*/
/*		07/22/2008  TC  First version...						*/
/****************************************************************/
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mysql.h"
#include "Config_files.h"
#include "database.h"

/********************************************************
**
**		main  :-
**
********************************************************/

int
main( int argc, char * argv[] )
{
	char inputstr[255] = {0};
	MYSQL * DBconn;
	char myQuery[255] = {0};
	MYSQL_ROW myRow ;
	MYSQL_RES * myResult;
	int num_rows;
	unsigned gc_num, slot;

	char mySQL_Host[255] = {0};
	char mySQL_Username[255] = {0};
	char mySQL_Password[255] = {0};
	char mySQL_Database[255] = {0};
	unsigned int mySQL_Port;
	int config_index = 0;
	char config_data[255] = { 0 };
	unsigned ch;

	FILE* fp;
	FILE* cf;

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

		if ( (DBconn = mysql_init((MYSQL*) 0)) && 
			mysql_real_connect( DBconn, &mySQL_Host[0], &mySQL_Username[0], &mySQL_Password[0], NULL, mySQL_Port,
			NULL, 0 ) )
		{
			if ( mysql_select_db( DBconn, &mySQL_Database[0] ) < 0 ) {
				printf( "Can't select the %s database !\n", mySQL_Database ) ;
				mysql_close( DBconn ) ;
				return 2 ;
			}
		}
		else {
			printf( "Can't connect to the mysql server (%s) on port %d !\nmysql_error = %s\n",
				mySQL_Host, mySQL_Port, mysql_error(DBconn) ) ;

			mysql_close( DBconn ) ;
			return 1 ;
		}

		printf ("梦幻之星中国服务端玩家角色导出工具\n");
		printf ("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
		printf ("Guild card #: ");
		if (scanf("%s", inputstr))
		{
			exit(1);
		}
		gc_num = atoi ( inputstr );
		printf ("Slot #: ");
		if (scanf ("%s", inputstr ))
		{
		exit(1);
		}
		slot = atoi ( inputstr );
		sprintf (&myQuery[0], "SELECT * from %s WHERE guildcard='%u' AND slot='%u'", CHARACTER, gc_num, slot );
		// Check to see if that character exists.
		//printf ("Executing MySQL query: %s\n", myQuery );
		if ( ! mysql_query ( DBconn, &myQuery[0] ) )
		{
			myResult = mysql_store_result ( DBconn );
			num_rows = (int) mysql_num_rows ( myResult );
			if (num_rows)
			{
				myRow = psocn_db_result_fetch( myResult );
				cf = fopen ("exported.dat", "wb");
				fwrite (myRow[2], 1, 14752, cf);
				fclose ( cf );
				printf ("Character exported!\n");
			}
			else
				printf ("Character not found.\n");
			mysql_free_result ( myResult );
		}
		else
		{
			printf ("Couldn't query the MySQL server.\n");
			return 1;
		}
		mysql_close( DBconn ) ;
		return 0;
}
