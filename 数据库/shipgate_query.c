/*
    梦幻之星中国 船闸服务器 数据库操作
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
#include "database_query.h"

int db_initialize() {
    char query[256] = { 0 };

    SGATE_LOG("初始化在线舰船数据表"/*, SERVER_SHIPS_ONLINE*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_SHIPS_ONLINE);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_SHIPS_ONLINE);
        return -1;
    }

    SGATE_LOG("初始化在线玩家数据表"/*, SERVER_CLIENTS_ONLINE*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_CLIENTS_ONLINE);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_CLIENTS_ONLINE);
        return -1;
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", AUTH_ACCOUNT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", AUTH_ACCOUNT);
        return -1;
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", CHARACTER);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", CHARACTER);
        return -1;
    }

    SGATE_LOG("初始化临时玩家数据表"/*, SERVER_CLIENTS_TRANSIENT*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_CLIENTS_TRANSIENT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_CLIENTS_TRANSIENT);
        return -1;
    }

    return 0;
}

int db_update_ship_ipv4(uint32_t ip, uint16_t key_idx) {

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET ip = '%lu' WHERE ship_id = '%u'",
        SERVER_SHIPS_ONLINE, ntohl(ip), key_idx);
    if (!psocn_db_real_query(&conn, myquery))
        return -1;

    return 0;
}