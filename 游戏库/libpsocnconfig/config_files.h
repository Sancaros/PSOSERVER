#pragma once
/* �����ļ��������� */
enum Config_Defines {
	// LOGIN_CONFIG_
	// PATCH_CONFIG_
	PATCH_CONFIG_WINDOWS_SH_SWITCH = 0x00,
	PATCH_CONFIG_UPDATE_DIR,
	PATCH_CONFIG_UPDATE_MAXBYTES,

	// ���Ƿ����½����˺Ͳ�������˵�����
	// SHIP_CONFIG_
	// 
	// Load_Config_File() login

	WEB_SERVER_CONFIG_HOST = 0x00,
	WEB_SERVER_CONFIG_PORT,
	WEB_SERVER_CONFIG_PATCH_WELCOME_MESSAGE_FILE,
	WEB_SERVER_CONFIG_LOGIN_WELCOME_MESSAGE_FILE,
	WEB_SERVER_CONFIG_HELP_PLAYER_FILE = 0x02,
	WEB_SERVER_CONFIG_HELP_GM_FILE,

	GLOBAL_CONFIG_LOGIN_SERVER_HOST = 0x00,
	GLOBAL_CONFIG_LOGIN_SERVER_WELCOME_MESSAGE,
	GLOBAL_CONFIG_LOGIN_SERVER_PORT,
	GLOBAL_CONFIG_CLIENT_VERSION_STRING,
	GLOBAL_CONFIG_CLIENT_VERSION,
	GLOBAL_CONFIG_CLIENT_MAX_CONNECTION,
	GLOBAL_CONFIG_SHIP_SERVER_MAX_CONNECTION,
	GLOBAL_CONFIG_WEB_SERVER,
	GLOBAL_CONFIG_WINDOWS_SH_SWITCH,
	GLOBAL_CONFIG_CONSOLE_LOG_SHOW,
	GLOBAL_CONFIG_MAX,

	GLOBAL_CONFIG_RATE_APPEARANCES_HILDEBEAR = 0x00,
	GLOBAL_CONFIG_RATE_APPEARANCES_RAPPY,
	GLOBAL_CONFIG_RATE_APPEARANCES_LILY,
	GLOBAL_CONFIG_RATE_APPEARANCES_SLIME,
	GLOBAL_CONFIG_RATE_APPEARANCES_MERISSA,
	GLOBAL_CONFIG_RATE_APPEARANCES_PAZUZU,
	GLOBAL_CONFIG_RATE_APPEARANCES_DORPHON_ECLAIR,
	GLOBAL_CONFIG_RATE_APPEARANCES_KONDRIEU,
	GLOBAL_CONFIG_NAME_COLOR_GLOBAL,
	GLOBAL_CONFIG_NAME_COLOR_LOCAL,
	GLOBAL_CONFIG_NAME_COLOR_NORMAL,
	GLOBAL_CONFIG_E7BASE_MESETA,

	// 
	// Load_Config_File() ship
	SHIP_CONFIG_HOST = 0x00,
	SHIP_CONFIG_RECONNECT_TIME,
	SHIP_CONFIG_PORT,
	SHIP_CONFIG_BLOKS,
	SHIP_CONFIG_MAX_CONNECTION,
	SHIP_CONFIG_LOGIN_SERVER_HOST,
	SHIP_CONFIG_NAME,
	SHIP_CONFIG_KEY_PATH,
	SHIP_CONFIG_RESPONSE_TIMES,
	SHIP_CONFIG_CHARACTER_BACKUP_TIMES,
	SHIP_CONFIG_EVENT,
	SHIP_CONFIG_AUTO_EVENT,
	SHIP_CONFIG_NIGHTS_SKIN,
	SHIP_CONFIG_BP_KAIGUAN,
	SHIP_CONFIG_WEB_SERVER,
	SHIP_CONFIG_WINDOWS_SH_SWITCH,
	SHIP_CONFIG_CONSOLE_LOG_SHOW,
	SHIP_CONFIG_MAIN_MAX,

	// Load_Log_Setting()
	/*CONFIG_CONSOLE_LOG_SHOW_LOGIN = 0x00,
	CONFIG_CONSOLE_LOG_SHOW_DEBUG,
	CONFIG_CONSOLE_LOG_SHOW_SHIP,
	CONFIG_CONSOLE_LOG_SHOW_QUESTERR,
	CONFIG_CONSOLE_LOG_SHOW_GM,
	CONFIG_CONSOLE_LOG_SHOW_LOGIN,
	CONFIG_CONSOLE_LOG_SHOW_ERROR,
	SHIP_CONFIG_MAIN_MAX,*/

	SHIP_CONFIG_PARAM_HOSPITAL_HEAL_MESETA = 0x00,
	SHIP_CONFIG_PARAM_DEATH_PUNISH,
	SHIP_CONFIG_PARAM_DEATH_MESETA,
	SHIP_CONFIG_PARAM_DEATH_DROP_ITEM,
	SHIP_CONFIG_PARAM_DEATH_DROP_ITEM_COUNT,
	SHIP_CONFIG_PARAM_TEKKING_ITEM_MESETA,
	SHIP_CONFIG_PARAM_MAX,

	SHIP_CONFIG_RATE_EXPERIENCE = 0x00,
	SHIP_CONFIG_RATE_DROP_WEAPON,
	SHIP_CONFIG_RATE_DROP_ARMOR,
	SHIP_CONFIG_RATE_DROP_MAG,
	SHIP_CONFIG_RATE_DROP_TOOL,
	SHIP_CONFIG_RATE_DROP_MESETA,
	SHIP_CONFIG_RATE_RARE_BOX_MULTIPLIER,
	SHIP_CONFIG_RATE_RARE_MOB_BOX_MULTIPLIER,
	SHIP_CONFIG_RATE_RARE_DROP_GLOBAL_MULTIPLIER,
	SHIP_CONFIG_RATE_RARE_GLOBAL_MOB_APPEARANCES_MULTIPLIER,
	SHIP_CONFIG_RATE_APPEARANCES_HILDEBEAR,
	SHIP_CONFIG_RATE_APPEARANCES_RAPPY,
	SHIP_CONFIG_RATE_APPEARANCES_LILY,
	SHIP_CONFIG_RATE_APPEARANCES_SLIME,
	SHIP_CONFIG_RATE_APPEARANCES_MERISSA,
	SHIP_CONFIG_RATE_APPEARANCES_PAZUZU,
	SHIP_CONFIG_RATE_APPEARANCES_DORPHON_ECLAIR,
	SHIP_CONFIG_RATE_APPEARANCES_KONDRIEU,
	SHIP_CONFIG_EVENT_PARAM_MAX,

	SHIP_CONFIG_EVENT_0 = 0x00,
	SHIP_CONFIG_EVENT_1,
	SHIP_CONFIG_EVENT_2,
	SHIP_CONFIG_EVENT_3,
	SHIP_CONFIG_EVENT_4,
	SHIP_CONFIG_EVENT_5,
	SHIP_CONFIG_EVENT_6,
	SHIP_CONFIG_EVENT_7,
	SHIP_CONFIG_EVENT_8,
	SHIP_CONFIG_EVENT_9,
	SHIP_CONFIG_EVENT_10,
	SHIP_CONFIG_EVENT_11,
	SHIP_CONFIG_EVENT_12,
	SHIP_CONFIG_EVENT_13,
	SHIP_CONFIG_EVENT_14,
	SHIP_CONFIG_EVENT_MAX,
};

/* ���������ָ�� */
enum Server_Command {
	// LOGIN_COMMAND_
	// PATCH_COMMAND_
	// SHIP_COMMAND_
	// ��ѭ���ַ�����������
	// 
	// ShipSend0D login
	LOGIN_COMMAND_SEND_SHIP_LIST = 0x00,
	LOGIN_COMMAND_NEW_E7 = 0x00E7,
	// 
	// 
	// 
	// 
	// 
	// 
	// 

	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// 
	// SendEA ship
	SHIP_COMMAND_UNK1 = 0x02,
	SHIP_COMMAND_UNK2 = 0x04,
	SHIP_COMMAND_UNK3 = 0x0E,
	SHIP_COMMAND_UNK4 = 0x10,
	SHIP_COMMAND_UNK5 = 0x11,
	SHIP_COMMAND_UNK6 = 0x12,
	SHIP_COMMAND_UNK7 = 0x13,
	SHIP_COMMAND_UNK8 = 0x18,
	SHIP_COMMAND_UNK9 = 0x19,
	SHIP_COMMAND_UNK10 = 0x1A,
	SHIP_COMMAND_UNK11 = 0x1D,
	SHIP_COMMAND_UNK14 = 0x14,
	// Ship_Client_Data_Send04 ship
	SHIP_COMMAND_REQUEST_CDATA = 0x00,
	SHIP_COMMAND_SEND_CDATA = 0x02,
	SHIP_COMMAND_SEND_CMDATA = 0x03,
	SHIP_COMMAND_SEND_Q2DATA = 0x04,
	// ShipSend0E ship
	SHIP_COMMAND_SEND_0E = 0x0E,
	// ShipSend0D ship
	SHIP_COMMAND_REQUEST_SHIP_LIST = 0x00,
	// ShipSend0B ship
	SHIP_COMMAND_SEND_0B = 0x0B,
	//SHIP_COMMAND_UNK =,
	//SHIP_COMMAND_UNK =,

};

const static char* config_files[] = {
	"Config\\Config_Global.ini",//ok
	"Config\\Config_Global_Param.ini",//ok
	"Config\\Config_Mysql.ini",//ok
	"Config\\Config_Web_Server.ini",//δ���
	"Config\\Config_Lang.ini",//ok
	"Config\\Config_LocalGM.ini",//ok
	"Config\\Config_Log.ini",//ok
	"Config\\Config_Patch.ini",//ok
	"Config\\Config_Ship.ini",//ok
	"Config\\Config_Ship_Quest.ini",//ok
	"Config\\Config_Ship_Param.ini",//ok
	"Config\\Config_Ship_Drop.ini",//ok
	"Config\\Config_Ship_BattleParam_Up.ini",//ok
	"Config\\Config_Ship_Event.ini",//ok
};

enum Config_Files_Num {
	CONFIG_FILE_GLOBAL,
	CONFIG_FILE_GLOBAL_PARAM,
	CONFIG_FILE_MYSQL,
	CONFIG_FILE_WEB_SERVER,
	CONFIG_FILE_LANG,
	CONFIG_FILE_LOCALGM,
	CONFIG_FILE_LOG,
	CONFIG_FILE_PATCH,
	CONFIG_FILE_SHIP,
	CONFIG_FILE_SHIP_Q,
	CONFIG_FILE_SHIP_P,
	CONFIG_FILE_SHIP_DROP,
	CONFIG_FILE_SHIP_BP,
	CONFIG_FILE_SHIP_EVENT,
	CONFIG_FILE_MAX,
};

/*
const static char* ship_event_name[] = {
	"��ͨ1", //��� 0 ��ͨ Ĭ��
	"ʥ����", //��� 1 ʥ���� 12��25�� ���� 1��
	"��ͨ2", //��� 2 ��ͨ����������0��ͬ��Ĭ��2
	"���˽�", //��� 3 ���˽� 2��14�� ���� 1��
	"������", //��� 4 ������ 4��3�� ���� 3��
	"��ʥ��", //��� 5 ��ʥ�� 10��31�� ���� 1����
	"����˼�����", //��� 6 ����˼����� 6��23�� ���� 1��
	"����", //��� 7 ���� 1��1�� ���� 3����
	"����", //��� 8 ������ӣ����3��1����5��1�� ���� 2����
	"��ɫ���˽�", //��� 9 ��ɫ���˽� 3��14�� ���� 1��
	"����", //��� 10 ������������ ���� 1�ܣ����ֶ�������
	"�＾", //��� 11 �＾����㣩8��23�� �� 11��22�� ���� 3����
	"��Ϧ��1", //��� 12 ��Ϧ�� 8��14�� ���� 1��
	"��Ϧ��2", //��� 13 ��Ϧ�� ����������12��ͬ,12��13����� 8��14�� ���� 1��
	"��ͨ3", //��� 14 ��ͨ����������0��ͬ��Ĭ��3
};*/

const static char* ship_event_files[] = {
	"Config\\Event\\Config_Ship_Event_0.ini", //��� 0 ��ͨ1
	"Config\\Event\\Config_Ship_Event_1.ini", //��� 1 ʥ����
	"Config\\Event\\Config_Ship_Event_2.ini", //��� 2 ��ͨ2
	"Config\\Event\\Config_Ship_Event_3.ini", //��� 3 ���˽�
	"Config\\Event\\Config_Ship_Event_4.ini", //��� 4 ������
	"Config\\Event\\Config_Ship_Event_5.ini", //��� 5 ��ʥ��
	"Config\\Event\\Config_Ship_Event_6.ini", //��� 6 ����˼�����
	"Config\\Event\\Config_Ship_Event_7.ini", //��� 7 ����
	"Config\\Event\\Config_Ship_Event_8.ini", //��� 8 ����
	"Config\\Event\\Config_Ship_Event_9.ini", //��� 9 ��ɫ���˽�
	"Config\\Event\\Config_Ship_Event_10.ini", //��� 10 ����
	"Config\\Event\\Config_Ship_Event_11.ini", //��� 11 �＾
	"Config\\Event\\Config_Ship_Event_12.ini", //��� 12 ��Ϧ��1
	"Config\\Event\\Config_Ship_Event_13.ini", //��� 13 ��Ϧ��2
	"Config\\Event\\Config_Ship_Event_14.ini", //��� 14 ��ͨ3
};

enum month_Num {
	JANUARY = 1,
	FEBRUARY,
	MARCH,
	APRIL,
	MAY,
	JUNE,
	JULY,
	AUGUST,
	SEPTEMBER,
	OCTOBER,
	NOVEMBER,
	DECEMBER,
};

const static char* login_lang_dir[] = {
	"Config/messages/login/messages_",//ok
	"Config/messages/login/messages.ini",//ok
};

const static char* ship_lang_dir[] = {
	"Config/messages/ship/messages_",//ok
	"Config/messages/ship/messages.ini",//ok
};

