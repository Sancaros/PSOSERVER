/*
    ÃÎ»ÃÖ®ÐÇÖÐ¹ú pthread Ïß³ÌËøºê
    °æÈ¨ (C) 2022, 2023 Sancaros

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

#include <pthread.h>

#define LOCK_RWLOCK(lock) \
    do { \
        int ret = pthread_rwlock_rdlock(lock); \
        if (ret != 0) { \
            ERR_LOG("pthread_rwlock_rdlock ´íÎó: %d", ret); \
        } \
    } while (0)

#define UNLOCK_RWLOCK(lock) \
    do { \
        int ret = pthread_rwlock_unlock(lock); \
        if (ret != 0) { \
            ERR_LOG("pthread_rwlock_unlock ´íÎó: %d", ret); \
        } \
    } while (0)

#define LOCK_LMUTEX(lobby) \
    do { \
        int ret = pthread_mutex_lock(&(lobby)->mutex); \
        if (ret != 0) { \
            ERR_LOG("%s pthread_mutex_lock ´íÎó: %d", get_lobby_describe((lobby_t*)lobby), ret); \
        } \
    } while (0)

#define UNLOCK_LMUTEX(lobby) \
    do { \
        int ret = pthread_mutex_unlock(&(lobby)->mutex); \
        if (ret != 0) { \
            ERR_LOG("%s pthread_mutex_unlock ´íÎó: %d", get_lobby_describe((lobby_t*)lobby), ret); \
        } \
    } while (0)

#define LOCK_CMUTEX(client) \
    do { \
        int ret = pthread_mutex_lock(&(client)->mutex); \
        if (ret != 0) { \
            ERR_LOG("%s pthread_mutex_lock ´íÎó: %d", get_player_describe((ship_client_t*)client), ret); \
        } \
    } while (0)

#define UNLOCK_CMUTEX(client) \
    do { \
        int ret = pthread_mutex_unlock(&(client)->mutex); \
        if (ret != 0) { \
            ERR_LOG("%s pthread_mutex_unlock ´íÎó: %d", get_player_describe((ship_client_t*)client), ret); \
        } \
    } while (0)
