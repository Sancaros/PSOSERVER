/*
    梦幻之星中国 数据库
    版权 (C) 2022 Sancaros

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License version 3
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "database.h"
#include "f_logs.h"

#include <stdio.h>

#if !defined(MARIADB_BASE_VERSION)
/* MySQL 8 deprecated the my_bool type. */
#   if MYSQL_VERSION_ID > 80000
#       #define my_bool int
#    endif
#endif

int psocn_db_open(psocn_dbconfig_t* dbcfg, psocn_dbconn_t* conn) {
    MYSQL* mysql;

    psocn_read_db_config(NULL, &dbcfg);

    if (!dbcfg || !conn) {
        return -42;
    }

    /* Set up the MYSQL object to connect to the database. */
    mysql = mysql_init(NULL);

    if (!mysql) {
        conn->conndata = NULL;
        return -1;
    }

    uint32_t show_set = (uint32_t)strtoul(dbcfg->show_setting, NULL, 0);

    /* 设置数据库 */
    if (show_set) {
        CONFIG_LOG("MySQL 数据库连接参数");
        CONFIG_LOG("数据库地址: %s", dbcfg->host);
        CONFIG_LOG("数据库端口: %u", dbcfg->port);
        CONFIG_LOG("数据库用户: %s", dbcfg->user);
        CONFIG_LOG("数据库密码: %s", dbcfg->pass);
        CONFIG_LOG("数据库表: %s", dbcfg->db);
        CONFIG_LOG("数据库重连: %s", dbcfg->auto_reconnect);
        CONFIG_LOG("数据库字符集: %s", dbcfg->char_set);
        CONFIG_LOG("显示数据库设置: %s", dbcfg->show_setting);
    }

    /* Attempt to connect to the MySQL server. */
    if (!mysql_real_connect(mysql, dbcfg->host, dbcfg->user, dbcfg->pass, NULL,
        dbcfg->port, NULL, 0)) {
        mysql_close(mysql);
        conn->conndata = NULL;
        return -2;
    }

    if (mysql_set_character_set(mysql, dbcfg->char_set)) {
        mysql_close(mysql);
        conn->conndata = NULL;
        return -3;
    }

    /* Attempt to select the database requested. */
    if (mysql_select_db(mysql, dbcfg->db) < 0) {
        mysql_close(mysql);
        conn->conndata = NULL;
        return -4;
    }

    my_bool rc = (my_bool)dbcfg->auto_reconnect;

    /* Configure to automatically reconnect if the connection is dropped. */
    if (mysql_options(mysql, MYSQL_OPT_RECONNECT, &rc)) {
        mysql_close(mysql);
        conn->conndata = NULL;
        return -5;
    }

    if (show_set) {
        MY_CHARSET_INFO charset;
        mysql_get_character_set_info(mysql, &charset);
        CONFIG_LOG("数据库字符集序号:%d", charset.number);
        CONFIG_LOG("数据库字符集状态:%d", charset.state);
        CONFIG_LOG("数据库字符排序规则:%s", charset.csname);
        CONFIG_LOG("数据库字符集:%s", charset.name);
        CONFIG_LOG("数据库字符解析:%s", charset.comment);
        CONFIG_LOG("数据库字符集目录:%s", charset.dir);
        CONFIG_LOG("数据库字符最小长度:%d", charset.mbminlen);
        CONFIG_LOG("数据库字符最大长度:%d", charset.mbmaxlen);
    }

    conn->conndata = (void*)mysql;
    return 0;
}

void psocn_db_close(psocn_dbconn_t* conn) {
    if (!conn || !conn->conndata) {
        return;
    }

    mysql_close((MYSQL*)conn->conndata);
}

int psocn_db_query(psocn_dbconn_t* conn, const char* str) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    return mysql_query((MYSQL*)conn->conndata, str);
}

int psocn_db_real_query(psocn_dbconn_t* conn, const char* str) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    unsigned long length = strlen(str);

    return mysql_real_query((MYSQL*)conn->conndata, str, length);
}

void* psocn_db_result_store(psocn_dbconn_t* conn) {
    if (!conn || !conn->conndata) {
        return NULL;
    }

    return (void*)mysql_store_result((MYSQL*)conn->conndata);
}

void psocn_db_result_free(void* result) {
    if (!result) {
        return;
    }

    mysql_free_result((MYSQL_RES*)result);
}

long long int psocn_db_result_rows(void* result) {
    if (!result) {
        return -42;
    }

    return (long long int)mysql_num_rows((MYSQL_RES*)result);
}

unsigned int psocn_db_result_fields(psocn_dbconn_t* conn) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    return mysql_field_count((MYSQL*)conn->conndata);
}

char** psocn_db_result_fetch(void* result) {
    if (!result) {
        return NULL;
    }

    return mysql_fetch_row((MYSQL_RES*)result);
}

unsigned long* psocn_db_result_lengths(void* result) {
    if (!result) {
        return NULL;
    }

    return mysql_fetch_lengths((MYSQL_RES*)result);
}

long long int psocn_db_insert_id(psocn_dbconn_t* conn) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    return (long long int)mysql_insert_id((MYSQL*)conn->conndata);
}

unsigned long psocn_db_escape_str(psocn_dbconn_t* conn, char* to,
    const char* from, unsigned long len) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    return mysql_real_escape_string((MYSQL*)conn->conndata, to, from, len);
}

unsigned long psocn_db_str(psocn_dbconn_t* conn, char* q, const char* str, unsigned long len) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    return mysql_real_escape_string((MYSQL*)conn->conndata, q + strlen(q), str, len);
}

const char* psocn_db_error(psocn_dbconn_t* conn) {
    if (!conn || !conn->conndata) {
        return "No Connection";
    }

    return mysql_error((MYSQL*)conn->conndata);
}
