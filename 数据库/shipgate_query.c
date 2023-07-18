/*
    �λ�֮���й� ��բ������ ���ݿ����
    ��Ȩ (C) 2022, 2023 Sancaros

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

/* ��ʼ�����ݿ����� */
extern psocn_dbconn_t conn;

uint32_t db_get_char_data_length(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    unsigned long* len;
    uint32_t data_length;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data, size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("δ�ҵ�����Ľ�ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the length of the character data */
    if (!(len = psocn_db_result_lengths(result))) {
        psocn_db_result_free(result);
        SQLERR_LOG("�޷���ȡ��ɫ���ݵĳ���");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    data_length = len[0];

    psocn_db_result_free(result);
    return data_length;
}

uint32_t db_get_char_data_size(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    uint32_t data_size;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("δ�ҵ�����Ľ�ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    data_size = (uint32_t)atoi(row[0]);

    psocn_db_result_free(result);
    return data_size;
}

uint32_t db_get_char_data_play_time(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    uint32_t play_time;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT play_time FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("δ�ҵ�����Ľ�ɫ���� (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    play_time = (uint32_t)atoi(row[0]);

    psocn_db_result_free(result);
    return play_time;
}

char* db_get_char_raw_data(uint32_t gc, uint8_t slot, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ���� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ���� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("δ�ҵ�����Ľ�ɫ���� (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return 0;
    }

    return row[0];
}

int db_initialize() {
    char query[256] = { 0 };

    SGATE_LOG("��ʼ�����߽������ݱ�"/*, SERVER_SHIPS_ONLINE*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_SHIPS_ONLINE);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("��ʼ�� %s ���ݱ����,�������ݿ�", SERVER_SHIPS_ONLINE);
        return -1;
    }

    SGATE_LOG("��ʼ������������ݱ�"/*, SERVER_CLIENTS_ONLINE*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_CLIENTS_ONLINE);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("��ʼ�� %s ���ݱ����,�������ݿ�", SERVER_CLIENTS_ONLINE);
        return -1;
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", AUTH_ACCOUNT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("��ʼ�� %s ���ݱ����,�������ݿ�", AUTH_ACCOUNT);
        return -1;
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", CHARACTER);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("��ʼ�� %s ���ݱ����,�������ݿ�", CHARACTER);
        return -1;
    }

    SGATE_LOG("��ʼ����ʱ������ݱ�"/*, SERVER_CLIENTS_TRANSIENT*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_CLIENTS_TRANSIENT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("��ʼ�� %s ���ݱ����,�������ݿ�", SERVER_CLIENTS_TRANSIENT);
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