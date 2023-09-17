/*
    梦幻之星中国 数据库
    版权 (C) 2022, 2023 Sancaros

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

#define SUPPORT_STMT

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

    uint32_t show_set = (uint32_t)strtoul(dbcfg->show_setting, NULL, 10);

    /* 设置数据库 */
    if (show_set) {
        CONFIG_LOG("数据库连接参数");
        CONFIG_LOG("数据库类型: %s", dbcfg->type);
        CONFIG_LOG("数据库地址: %s", dbcfg->host);
        CONFIG_LOG("数据库端口: %u", dbcfg->port);
        if (dbcfg->unix_socket) {
            CONFIG_LOG("数据库代理: %s", dbcfg->unix_socket);
        }
        CONFIG_LOG("数据库用户: %s", dbcfg->user);
        CONFIG_LOG("数据库密码: %s", dbcfg->pass);
        CONFIG_LOG("数据库表: %s", dbcfg->db);
        CONFIG_LOG("数据库重连: %s", dbcfg->auto_reconnect);
        CONFIG_LOG("数据库字符集: %s", dbcfg->char_set);
        CONFIG_LOG("显示数据库设置: %s", dbcfg->show_setting);
    }

#ifdef SUPPORT_STMT
    my_bool stmt_set = 1;
#endif

    /* Attempt to connect to the MySQL server. */
    if (!mysql_real_connect(mysql, dbcfg->host, dbcfg->user, dbcfg->pass, NULL,
        dbcfg->port, dbcfg->unix_socket, 0)) {
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

#ifdef SUPPORT_STMT
    if (stmt_set) {
        if (mysql_options(mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN, &rc)) {
            mysql_close(mysql);
            conn->conndata = NULL;
            return -7;
        }
    }
#endif

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

void* psocn_db_result_use(psocn_dbconn_t* conn) {
    if (!conn || !conn->conndata) {
        return NULL;
    }

    return (void*)mysql_use_result((MYSQL*)conn->conndata);
}
    
void psocn_db_result_free(void* result) {
    if (!result) {
        return;
    }

    mysql_free_result((MYSQL_RES*)result);
}

void psocn_db_next_result_free(psocn_dbconn_t* conn, void* result) {
    while (!mysql_next_result((MYSQL*)conn->conndata)) {
        result = psocn_db_result_store(conn);
        psocn_db_result_free(result);
    }
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
        return "数据库未连接";
    }

    return mysql_error((MYSQL*)conn->conndata);
}

int psocn_db_commit(psocn_dbconn_t* conn) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    return mysql_commit((MYSQL*)conn->conndata);
}

unsigned long long psocn_db_affected_rows(psocn_dbconn_t* conn) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    return mysql_affected_rows((MYSQL*)conn->conndata);
}

/// <参数化查询>
/// //////////////////////////////////////////////////////////////////////////////////////////
/// </参数化查询>

int psocn_db_stmt_query(psocn_dbconn_t* conn, MYSQL_STMT* stmt, const char* str, unsigned long length, MYSQL_BIND* params) {
    if (!conn || !conn->conndata || !stmt || !params) {
        return -42;
    }

    if (mysql_stmt_prepare(stmt, str, length)) {
        return -2;
    }

    if (mysql_stmt_bind_param(stmt, params)) {
        return -3;
    }

    if (mysql_stmt_execute(stmt)) {
        return -4;
    }

    return 0;
}

int psocn_db_squery(psocn_dbconn_t* conn, const char* str, MYSQL_BIND* params) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    MYSQL_STMT* stmt = mysql_stmt_init((MYSQL*)conn->conndata);
    if (!stmt) {
        return -1;
    }

    unsigned long length = strlen(str);

    int result = psocn_db_stmt_query(conn, stmt, str, length, params);

    mysql_stmt_close(stmt);
    return result;
}

int psocn_db_real_squery(psocn_dbconn_t* conn, const char* str, MYSQL_BIND* params) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    MYSQL_STMT* stmt = mysql_stmt_init((MYSQL*)conn->conndata);
    if (!stmt) {
        return -1;
    }

    unsigned long length = strlen(str);

    int result = psocn_db_stmt_query(conn, stmt, str, length, params);

    mysql_stmt_close(stmt);
    return result;
}

char** psocn_db_result_sfetch(void* result, int num_fields) {
    if (!result) {
        return NULL;
    }

    MYSQL_STMT* stmt = (MYSQL_STMT*)result;

    MYSQL_BIND* bind = (MYSQL_BIND*)malloc(num_fields * sizeof(MYSQL_BIND));
    memset(bind, 0, num_fields * sizeof(MYSQL_BIND));

    for (int i = 0; i < num_fields; i++) {
        bind[i].buffer_type = MYSQL_TYPE_STRING;
        bind[i].buffer = malloc(1024);
        bind[i].buffer_length = 1024;
        bind[i].is_null = malloc(sizeof(my_bool));
        bind[i].length = malloc(sizeof(unsigned long));
    }

    if (mysql_stmt_bind_result(stmt, bind)) {
        for (int i = 0; i < num_fields; i++) {
            free(bind[i].buffer);
            free(bind[i].is_null);
            free(bind[i].length);
        }
        free(bind);
        return NULL;
    }

    if (mysql_stmt_fetch(stmt)) {
        for (int i = 0; i < num_fields; i++) {
            free(bind[i].buffer);
            free(bind[i].is_null);
            free(bind[i].length);
        }
        free(bind);
        return NULL;
    }

    char** rows = (char**)malloc((num_fields + 1) * sizeof(char*));
    for (int i = 0; i < num_fields; i++) {
        rows[i] = _strdup((char*)bind[i].buffer);
        free(bind[i].buffer);
        free(bind[i].is_null);
        free(bind[i].length);
    }
    rows[num_fields] = NULL;

    free(bind);
    return rows;
}

// 销毁参数对象
void psocn_destroy_db_param(psocn_db_param_t* param) {
    if (param->value != NULL) {
        free(param->value);
    }
    memset(param, 0, sizeof(psocn_db_param_t));
}

int psocn_set_db_param_value(psocn_db_param_t* param, psocn_param_type_t type, void* value, size_t length) {
    switch (type) {
    case PARAM_TYPE_UINT8: {
        uint8_t* uint8_value = malloc(sizeof(uint8_t));
        if (uint8_value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        *uint8_value = *(uint8_t*)value;
        param->type = MYSQL_TYPE_LONGLONG;
        param->value = uint8_value;
        param->length = sizeof(uint8_t);
        break;
    }
    case PARAM_TYPE_UINT16: {
        uint16_t* uint16_value = malloc(sizeof(uint16_t));
        if (uint16_value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        *uint16_value = *(uint16_t*)value;
        param->type = MYSQL_TYPE_SHORT;
        param->value = uint16_value;
        param->length = sizeof(uint16_t);
        break;
    }
    case PARAM_TYPE_UINT32: {
        uint32_t* uint32_value = malloc(sizeof(uint32_t));
        if (uint32_value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        *uint32_value = *(uint32_t*)value;
        param->type = MYSQL_TYPE_LONGLONG;
        param->value = uint32_value;
        param->length = sizeof(uint32_t);
        break;
    }
    case PARAM_TYPE_FLOAT: {
        float* float_value = malloc(sizeof(float));
        if (float_value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        *float_value = *(float*)value;
        param->type = MYSQL_TYPE_FLOAT;
        param->value = float_value;
        param->length = sizeof(float);
        break;
    }
    case PARAM_TYPE_CHAR: {
        char* char_value = malloc(sizeof(char));
        if (char_value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        *char_value = *(char*)value;
        param->type = MYSQL_TYPE_STRING;
        param->value = char_value;
        param->length = sizeof(char);
        break;
    }
    case PARAM_TYPE_WCHAR: {
        wchar_t* wchar_value = malloc(length * sizeof(wchar_t));
        if (wchar_value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        wcsncpy(wchar_value, (wchar_t*)value, length - 1);
        wchar_value[length - 1] = L'\0';
        param->type = MYSQL_TYPE_STRING;
        param->value = wchar_value;
        param->length = length * sizeof(wchar_t);
        break;
    }
    case PARAM_TYPE_STRING: {
        char* string_value = malloc(length);
        if (string_value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        strncpy(string_value, (char*)value, length);
        param->type = MYSQL_TYPE_STRING;
        param->value = string_value;
        param->length = length;
        break;
    }
    case PARAM_TYPE_STRUCT: {
        param->type = MYSQL_TYPE_BIT;
        param->value = malloc(length);
        if (param->value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        memcpy(param->value, value, length);
        param->length = length;
        break;
    }
    case PARAM_TYPE_BLOB: {
        param->type = MYSQL_TYPE_BIT;
        param->value = malloc(length);
        if (param->value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        memcpy(param->value, value, length);
        param->length = length;
        break;
    }
    case PARAM_TYPE_UNKNOWN: {
        param->type = MYSQL_TYPE_NULL;
        param->value = malloc(length);
        if (param->value == NULL) {
            return -1; // 错误码：内存分配失败
        }
        memcpy(param->value, value, length);
        param->length = length;
        break;
    }
    default:
        return -2; // 错误码：无效的参数
    }
    return 0; // 成功
}

void psocn_set_uint8_param(psocn_db_param_t* param, uint8_t value) {
    psocn_set_db_param_value(param, PARAM_TYPE_UINT8, &value, sizeof(uint8_t));
}

void psocn_set_uint16_param(psocn_db_param_t* param, uint16_t value) {
    psocn_set_db_param_value(param, PARAM_TYPE_UINT16, &value, sizeof(uint16_t));
}

void psocn_set_uint32_param(psocn_db_param_t* param, uint32_t value) {
    psocn_set_db_param_value(param, PARAM_TYPE_UINT32, &value, sizeof(uint32_t));
}

void psocn_set_float_param(psocn_db_param_t* param, float value) {
    psocn_set_db_param_value(param, PARAM_TYPE_FLOAT, &value, sizeof(float));
}

void psocn_set_char_param(psocn_db_param_t* param, char value) {
    psocn_set_db_param_value(param, PARAM_TYPE_CHAR, &value, sizeof(char));
}

void psocn_set_wchar_param(psocn_db_param_t* param, wchar_t* value) {
    psocn_set_db_param_value(param, PARAM_TYPE_WCHAR, (void*)value, wcslen(value) * sizeof(wchar_t));
}

void psocn_set_string_param(psocn_db_param_t* param, const char* value) {
    psocn_set_db_param_value(param, PARAM_TYPE_STRING, (void*)value, strlen(value));
}

void psocn_set_struct_param(psocn_db_param_t* param, void* value, size_t size) {
    psocn_set_db_param_value(param, PARAM_TYPE_STRUCT, value, size);
}

void psocn_set_blob_param(psocn_db_param_t* param, void* value, size_t size) {
    psocn_set_db_param_value(param, PARAM_TYPE_BLOB, value, size);
}

void psocn_set_unknown_param(psocn_db_param_t* param, void* value, size_t size) {
    psocn_set_db_param_value(param, PARAM_TYPE_UNKNOWN, value, size);
}

// 输出参数值
void psocn_output_param_value(const psocn_db_param_t* param) {
    switch (param->type) {
    case MYSQL_TYPE_SHORT:
        printf("MYSQL_TYPE_SHORT 参数值：%u\n", *(uint16_t*)param->value);
        break;
    case MYSQL_TYPE_LONGLONG:
        printf("MYSQL_TYPE_LONGLONG 参数值：%u\n", *(uint32_t*)param->value);
        break;
    case MYSQL_TYPE_FLOAT:
        printf("浮点数参数值：%f\n", *(float*)param->value);
        break;
    case MYSQL_TYPE_STRING:
        if (param->length > 0 && param->value != NULL) {
            printf("字符串参数值：%s\n", (char*)param->value);
        }
        else {
            printf("空字符串参数\n");
        }
        break;
    case MYSQL_TYPE_BIT:
        printf("二进制数据参数\n");
        // TODO: 输出二进制数据内容
        break;
    case MYSQL_TYPE_NULL:
        printf("未知参数类型\n");
        break;
    default:
        printf("无效的参数类型\n");
        break;
    }
}

int psocn_db_param_query(psocn_dbconn_t* conn, const char* query, const psocn_db_param_t* params, int param_count) {
    if (!conn || !conn->conndata) {
        return -42;
    }

    MYSQL_STMT* stmt = mysql_stmt_init((MYSQL*)conn->conndata);
    if (!stmt) {
        printf("无法创建参数化查询语句: %s\n", mysql_stmt_error(stmt));
        return -1;
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        printf("无法准备参数化查询语句.Error: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return -2;
    }

    MYSQL_BIND* bind_params = (MYSQL_BIND*)calloc(param_count, sizeof(MYSQL_BIND));
    if (!bind_params) {
        mysql_stmt_close(stmt);
        return -3;
    }

    memset(bind_params, 0, param_count * sizeof(MYSQL_BIND));

    // 绑定参数
    for (int i = 0; i < param_count; i++) {
        bind_params[i].buffer_type = params[i].type;

        if (params[i].value) {
            bind_params[i].buffer = params[i].value;
            bind_params[i].buffer_length = params[i].length;
            bind_params[i].is_null = (my_bool*)0;
        }
        else {
            bind_params[i].is_null = (my_bool*)1;
        }

        if (mysql_stmt_bind_param(stmt, &bind_params[i])) {
            printf("无法绑定参数: %s 参数 %d 类型: %d\n", mysql_stmt_error(stmt), i, bind_params[i].buffer_type);
            free(bind_params);
            mysql_stmt_close(stmt);
            return -4;
        }
    }

    if (mysql_stmt_execute(stmt)) {
        printf("无法执行查询: %s\n", mysql_stmt_error(stmt));
        free(bind_params);
        mysql_stmt_close(stmt);
        return -5;
    }

    free(bind_params);
    mysql_stmt_close(stmt);
    return 0;
}