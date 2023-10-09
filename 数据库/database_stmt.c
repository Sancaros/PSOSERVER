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

#ifdef SUPPORT_STMT

psocn_db_stmt_query_t* psocn_db_stmt_query_init(MYSQL* connection) {
    if (!connection) {
        return NULL;
    }

    psocn_db_stmt_query_t* query = (psocn_db_stmt_query_t*)malloc(sizeof(psocn_db_stmt_query_t));
    if (!query) {
        return NULL;
    }

    query->connection = connection;
    query->statement = mysql_stmt_init(connection);
    if (!query->statement) {
        free_safe(query);
        return NULL;
    }

    return query;
}

void psocn_db_stmt_query_free(psocn_db_stmt_query_t* query) {
    if (query) {
        if (query->statement) {
            mysql_stmt_close(query->statement);
        }

        free_safe(query);
    }
}

int psocn_db_stmt_add_param(enum enum_field_types type, void* value, unsigned long length) {
    if (param_count >= MAX_PARAMS) {
        fprintf(stderr, "Exceeded maximum number of parameters\n");
        return -1;
    }

    stmt_param[param_count].type = type;
    stmt_param[param_count].value = value;
    stmt_param[param_count].length = length;
    param_count++;

    return 0;
}

void psocn_db_stmt_clear_params() {
    param_count = 0;
    memset(stmt_param, 0, sizeof(stmt_param));
}

int psocn_db_stmt_query_exec(psocn_db_stmt_query_t* query, const char* sql, psocn_db_stmt_param_t* params, int param_count) {
    if (!query || !query->connection || !query->statement || !sql) {
        return -1;
    }

    if (mysql_stmt_prepare(query->statement, sql, strlen(sql))) {
        fprintf(stderr, "Unable to prepare query: %s\n", mysql_stmt_error(query->statement));
        return -2;
    }

    if (param_count > 0) {
        MYSQL_BIND* bind_params = (MYSQL_BIND*)calloc(param_count, sizeof(MYSQL_BIND));
        if (!bind_params) {
            return -3;
        }

        memset(bind_params, 0, param_count * sizeof(MYSQL_BIND));

        // Bind parameters
        for (int i = 0; i < param_count; i++) {
            bind_params[i].buffer_type = params[i].type;
            bind_params[i].buffer = params[i].value;
            bind_params[i].buffer_length = params[i].length;
            my_bool is_null_value = params[i].value ? 0 : 1;
            bind_params[i].is_null = &is_null_value;
        }

        if (mysql_stmt_bind_param(query->statement, bind_params)) {
            fprintf(stderr, "Unable to bind parameters: %s\n", mysql_stmt_error(query->statement));
            free_safe(bind_params);
            return -4;
        }

        free_safe(bind_params);
    }

    if (mysql_stmt_execute(query->statement)) {
        fprintf(stderr, "Unable to execute query: %s\n", mysql_stmt_error(query->statement));
        return -5;
    }

    return 0;
}

#endif // SUPPORT_STMT