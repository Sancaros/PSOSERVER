/*
    �λ�֮���й� ���ŷ����� ���ݿ����
    ��Ȩ (C) 2022 Sancaros

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

/* ��ʼ�����ݿ����� */
extern psocn_dbconn_t conn;

uint32_t db_get_char_data_length(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    unsigned long* len;
    uint32_t data_length;

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data, size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER_DATA, gc, slot);

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

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER_DATA, gc, slot);

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

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT play_time FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER_DATA, gc, slot);

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

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER_DATA, gc, slot);

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
        if (check) {
            SQLERR_LOG("δ�ҵ�����Ľ�ɫ���� (%u: %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return 0;
    }

    return row[0];
}

int db_update_bb_char_option(psocn_bb_db_opts_t opts, uint32_t gc) {
    static char query[sizeof(psocn_bb_db_opts_t) * 2 + 256 + 1248 * 2];
    //DBG_LOG("��������");

    /* Build the db query */
    sprintf(query, "UPDATE %s SET key_config='", CLIENTS_BLUEBURST_OPTION);

    psocn_db_escape_str(&conn, query + strlen(query), (char*)&opts.key_cfg.key_config,
        sizeof(opts.key_cfg.key_config));

    strcat(query, "', joystick_config = '");

    psocn_db_escape_str(&conn, query + strlen(query), (char*)&opts.key_cfg.joystick_config,
        sizeof(opts.key_cfg.joystick_config));

    strcat(query, "', symbol_chats = '");

    psocn_db_escape_str(&conn, query + strlen(query), (char*)&opts.symbol_chats,
        sizeof(opts.symbol_chats));

    strcat(query, "', shortcuts = '");

    psocn_db_escape_str(&conn, query + strlen(query), (char*)&opts.shortcuts,
        sizeof(opts.shortcuts));

    strcat(query, "', guild_name = '");

    psocn_db_escape_str(&conn, query + strlen(query), (char*)&opts.guild_name,
        sizeof(opts.guild_name));

    sprintf(query + strlen(query), "', option_flags = '%d' WHERE guildcard='%" PRIu32 "'", opts.option_flags, gc);

    /* Execute the query */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

/* ������һ������������ݿ� */
int db_update_char_challenge(psocn_bb_db_char_t* char_data, uint32_t gc, uint8_t slot, uint32_t flag) {
    static char query[sizeof(psocn_bb_db_char_t) * 2 + 256];
    char name[64];
    char class_name[64];

    istrncpy16_raw(ic_utf16_to_utf8, name, &char_data->character.name[2], 64, 14);

    istrncpy(ic_gbk_to_utf8, class_name, classes_cn[char_data->character.disp.dress_data.ch_class], 64);

    if (flag & PSOCN_DB_SAVE_CHAR) {
        sprintf(query, "INSERT INTO %s(guildcard, slot, name, class_name, version, data) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%s', '%" PRIu8 "', '", CHARACTER_DATA_CHALLENGE, gc, slot
        , name, class_name, char_data->character.disp.dress_data.version);

        psocn_db_escape_str(&conn, query + strlen(query), (char*)char_data->challenge_data,
            sizeof(char_data->challenge_data));

        strcat(query, "')");

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("�޷�������ս���ݱ� %s (GC %" PRIu32 ", "
                "��λ %" PRIu8 "):\n%s", CHARACTER_DATA_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            return 0;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        sprintf(query, "DELETE FROM %s WHERE guildcard="
            "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_DATA_CHALLENGE, gc,
            slot);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("�޷��������� %s ���� (GC %"
                PRIu32 ", ��λ %" PRIu8 "):\n%s", CHARACTER_DATA_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
            return -1;
        }

        sprintf(query, "INSERT INTO %s(guildcard, slot, name, class_name, version, data) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%s', '%" PRIu8 "', '", CHARACTER_DATA_CHALLENGE, gc, slot
            , name, class_name, char_data->character.disp.dress_data.version);

        psocn_db_escape_str(&conn, query + strlen(query), (char*)char_data->challenge_data,
            sizeof(char_data->challenge_data));

        strcat(query, "')");

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("�޷�������ս���ݱ� %s (GC %" PRIu32 ", "
                "��λ %" PRIu8 "):\n%s", CHARACTER_DATA_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            return 0;
        }
    }

    return 0;
}
