/*
    �λ�֮���й� ��֤������ ���ݿ����
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
#include "packets_bb.h"

/* ���ݿ����� */
extern psocn_dbconn_t conn;

//�Ƿ��� db_update_bb_char_guild �����ص��ˣ���������
static int db_updata_bb_char_guild_data(uint32_t guild_id, uint32_t gc) {

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
        "guild_id = '%" PRIu32"' "
        "WHERE guildcard = '%" PRIu32 "'"
        , CLIENTS_GUILD
        , guild_id
        , gc);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷����½�ɫ %s ��������!", CLIENTS_GUILD);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_get_bb_char_guild_data(uint32_t gc) {

}

static int db_del_guild_data(uint32_t guild_id) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "DELETE FROM %s "
        "WHERE "
        "guild_id = '%" PRIu32 "'"
        , CLIENTS_GUILD
        , guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷�����ɹ������� (GUILD ID %"
            PRIu32 "):\n%s", guild_id,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

static int db_update_account_guild_data(uint32_t guild_id, uint32_t new_guild_id, uint32_t new_guild_priv_level) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s"
        " SET "
        "guild_id = '%" PRIu32 "', guild_priv_level = '%" PRIu32 "'"
        " WHERE "
        "guild_id = '%" PRIu32 "'"
        , AUTH_ACCOUNT
        , new_guild_id, new_guild_priv_level
        , guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷������˺Ź������� (GUILD ID %"
            PRIu32 "):\n%s", guild_id,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

static int db_initialize_account_guild_data(uint32_t guild_id) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s"
        " SET "
        "guild_id = '-1', guild_priv_level = '0'"
        " WHERE "
        "guild_id = '%" PRIu32 "'"
        , AUTH_ACCOUNT
        , guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ʼ���˺Ź������� (GUILD ID %"
            PRIu32 "):\n%s", guild_id,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

/* ��ȡ BB �Ĺ������� */
psocn_bb_db_guild_t db_get_bb_char_guild(uint32_t gc) {
    void* result;
    char** row;
    psocn_bb_db_guild_t guild;
    uint32_t guild_id, guild_priv_level;

    memset(&guild, 0, sizeof(bb_guild_t));

    memset(myquery, 0, sizeof(myquery));

    /* ��ѯ���ݱ� */
    sprintf(myquery, "SELECT guild_id, guild_priv_level FROM %s WHERE "
        "guildcard = '%" PRIu32 "'", AUTH_ACCOUNT, gc);

    if (!psocn_db_real_query(&conn, myquery)) {

        result = psocn_db_result_store(&conn);
        if (psocn_db_result_rows(result)) {
            row = psocn_db_result_fetch(result);
            guild_id = (uint32_t)strtoul(row[0], NULL, 0);
            guild_priv_level = (uint32_t)strtoul(row[1], NULL, 0);
            psocn_db_result_free(result);
        }
        else {
            psocn_db_result_free(result);
            return guild;
        }

#ifdef DEBUG
        DBG_LOG("%u %d %d", gc, guild_id, guild_priv_level);
#endif // DEBUG

        if (guild_id != -1) {

            /* ͬ����ɫ�˺����ɫ�������ݵĹ���ID�빫��ȼ� */
            if (db_updata_bb_char_guild_data(guild_id, gc))
                return guild;

            /* ��ѯ���ݱ� */
            sprintf(myquery, "SELECT * FROM %s WHERE "
                "guild_id = '%" PRIu32 "'", CLIENTS_GUILD, guild_id);

            if (!psocn_db_real_query(&conn, myquery)) {
                result = psocn_db_result_store(&conn);
                /* �����Ƿ������� */
                if (psocn_db_result_rows(result)) {
                    row = psocn_db_result_fetch(result);
                    guild.guild_data.guild_owner_gc = (uint32_t)strtoul(row[0], NULL, 0);
                    guild.guild_data.guild_id = guild_id;
                    memcpy(&guild.guild_data.guild_info, row[2], sizeof(guild.guild_data.guild_info));
                    guild.guild_data.guild_priv_level = guild_priv_level;

                    /* ����������ɫ���� */
                    guild.guild_data.guild_name[0] = 0x0009;
                    guild.guild_data.guild_name[1] = 0x0045;
                    memcpy(&guild.guild_data.guild_name, row[4], sizeof(guild.guild_data.guild_name) - 4);

                    /* TODO ����ȼ�δʵ�� */
                    guild.guild_data.guild_rank = (uint32_t)strtoul(row[5], NULL, 0);

                    memcpy(&guild.guild_data.guild_flag, row[6], sizeof(guild.guild_data.guild_flag));
                    guild.guild_data.guild_rewards = (uint32_t)strtoul(row[7], NULL, 0);
                }

                psocn_db_result_free(result);
            }
            else {
                /* ��ʼ��Ĭ������ */
                guild.guild_data.guild_owner_gc = gc;
                guild.guild_data.guild_id = guild_id;
                guild.guild_data.guild_priv_level = guild_priv_level;

                //guild.guild_data.guild_rank = 0x00986C84; // ?? Ӧ�������а�δ��ɵĲ�����

                guild.guild_data.guild_rank = 0x00000000; // ?? Ӧ�������а�δ��ɵĲ�����

                /* TODO ��������δ��ó�ʼ���� ���Դ�Ĭ�ϵ�������ɫ�����л�ȡ��ʼ����*/
                guild.guild_data.guild_rewards = 0xFFFFFFFF; //�͸������й�

                sprintf(myquery, "INSERT INTO %s ("
                    "guildcard, guild_id, guild_priv_level, "
                    "guild_rank, guild_rewards, "
                    "guild_info, guild_name, guild_flag"
                    ") VALUES ("
                    "'%" PRIu32"', '%u', '%u', "
                    "'%u', '%u', '", CLIENTS_GUILD,
                    guild.guild_data.guild_owner_gc, guild.guild_data.guild_id, guild.guild_data.guild_priv_level,
                    guild.guild_data.guild_rank, guild.guild_data.guild_rewards
                );

                psocn_db_escape_str(&conn, myquery + strlen(myquery),
                    (char*)&guild.guild_data.guild_info, sizeof(guild.guild_data.guild_info));

                strcat(myquery, "', '");

                psocn_db_escape_str(&conn, myquery + strlen(myquery),
                    (char*)&guild.guild_data.guild_name, sizeof(guild.guild_data.guild_name));

                strcat(myquery, "', '");

                psocn_db_escape_str(&conn, myquery + strlen(myquery),
                    (char*)&guild.guild_data.guild_flag, sizeof(guild.guild_data.guild_flag));

                strcat(myquery, "')");

                if (psocn_db_real_query(&conn, myquery)) {
                    SQLERR_LOG("�޷������������� "
                        "guildcard %" PRIu32 "", gc);
                    SQLERR_LOG("%s", psocn_db_error(&conn));
                }
            }
        }
    }

    return guild;
}

int db_insert_bb_char_guild(uint16_t* guild_name, uint8_t* default_guild_flag, uint32_t gc, uint32_t version) {
    void* result;
    //char** row;
    bb_guild_t* g_data;
    uint8_t create_res;
    uint32_t guild_exists = 0, guild_id;
    char guild_name_text[24];

    g_data = (bb_guild_t*)malloc(sizeof(bb_guild_t));

    if (!g_data) {
        ERR_LOG("�޷����乫�����ݵ��ڴ�ռ�");
        ERR_LOG("%s", strerror(errno));
        return -1;
    }

    memcpy(g_data->guild_name, guild_name, sizeof(g_data->guild_name));
    memcpy(g_data->guild_flag, default_guild_flag, sizeof(g_data->guild_flag));

    istrncpy16_raw(ic_utf16_to_utf8, guild_name_text, &g_data->guild_name[2], 24, sizeof(g_data->guild_name) - 4);

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "SELECT * FROM %s WHERE guild_name_text = '%s'",
        CLIENTS_GUILD, guild_name_text);
    if (!psocn_db_real_query(&conn, myquery))
    {
        result = psocn_db_result_store(&conn);
        guild_exists = (uint32_t)psocn_db_result_rows(result);
        psocn_db_result_free(result);
    }
    else
        create_res = 1;

    if (!guild_exists)
    {
        memset(myquery, 0, sizeof(myquery));

        // It doesn't... but it will now. :)
        sprintf_s(myquery, _countof(myquery), "INSERT INTO %s "
            "(guildcard, version, guild_name_text, guild_name, guild_flag)"
            " VALUES "
            "('%" PRIu32 "', '%u', '%s', '",
            CLIENTS_GUILD,
            gc, version, guild_name_text);

        psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&g_data->guild_name,
            sizeof(g_data->guild_name));

        strcat(myquery, "', '");

        psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&g_data->guild_flag,
            sizeof(g_data->guild_flag));

        strcat(myquery, "')");

        if (!psocn_db_real_query(&conn, myquery))
        {
            memset(myquery, 0, sizeof(myquery));

            guild_id = (uint32_t)psocn_db_insert_id(&conn);
            sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
                "guild_id = '%u', guild_priv_level = '%u'"
                " WHERE "
                "guildcard = '%" PRIu32 "'"
                , AUTH_ACCOUNT
                , guild_id, 0x00000040
                , gc
            );

            if (psocn_db_real_query(&conn, myquery))
                create_res = 1;
            else
                create_res = 0;
        }
        else
            create_res = 1;
    }
    else
        create_res = 2;

    free_safe(g_data);

    return create_res;
}

int db_update_bb_guild_member_add(uint32_t guild_id, uint32_t gc_target) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
        "guild_id = '%u',guild_priv_level = '0' WHERE guildcard = '%" PRIu32"'",
        AUTH_ACCOUNT, guild_id, gc_target);
    if (!psocn_db_real_query(&conn, myquery))
        return 0;

    return 1;
}

int db_update_bb_guild_flag(uint8_t* guild_flag, uint32_t guild_id) {
    memset(myquery, 0, sizeof(myquery));
    uint8_t flag_slashes[4098] = { 0 };

    psocn_db_escape_str(&conn, &flag_slashes[0], guild_flag, 0x800);

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET guild_flag = '%s' WHERE guild_id = '%u'",
        CLIENTS_GUILD, (char*)&flag_slashes[0], guild_id);

    if (!psocn_db_real_query(&conn, myquery))
        return 0;

    return 1;
}

int db_dissolve_bb_guild(uint32_t guild_id) {

    if (db_del_guild_data(guild_id))
        return 1;

    if (db_initialize_account_guild_data(guild_id))
        return 2;

    return 0;
}

int db_update_bb_char_guild(psocn_bb_db_guild_t guild, uint32_t gc) {
    //DBG_LOG("���� guild ����");

    memset(myquery, 0, sizeof(myquery));

    /* Build the db query */
    sprintf(myquery, "UPDATE %s SET guild_info = '", CLIENTS_GUILD);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&guild.guild_data.guild_info,
        sizeof(guild.guild_data.guild_info));

    strcat(myquery, "', guild_name = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&guild.guild_data.guild_name,
        sizeof(guild.guild_data.guild_name));

    strcat(myquery, "', guild_flag = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&guild.guild_data.guild_flag,
        sizeof(guild.guild_data.guild_flag));

    sprintf(myquery + strlen(myquery),
        "', guildcard = '%u', guild_id = '%u', guild_priv_level = '%u'"
        ", guild_rank = '%u', guild_rewards = '%u'"
        " WHERE "
        "guildcard = '%" PRIu32 "'",
        guild.guild_data.guild_owner_gc, guild.guild_data.guild_id, guild.guild_data.guild_priv_level,
        guild.guild_data.guild_rank, guild.guild_data.guild_rewards, 
        gc
    );

    /* Execute the query */
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷����½�ɫ %s ��������!", CLIENTS_GUILD);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}
