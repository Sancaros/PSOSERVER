/*
    ?λ?֮???й? ??????????
    ??Ȩ (C) 2022 Sancaros

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

#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <f_logs.h>
#include <debug.h>

#include "admin.h"
#include "block.h"
#include "clients.h"
#include "ship.h"
#include "ship_packets.h"
#include "utils.h"

int kill_guildcard(ship_client_t *c, uint32_t gc, const char *reason) {
    block_t *b;
    ship_client_t *i;
    uint32_t j;

    /* Make sure we don't have anyone trying to escalate their privileges. */
    if(!LOCAL_GM(c)) {
        return -1;
    }

    /* Look through all the blocks for the requested user, and kick the first
       instance we happen to find (there shouldn't be more than one). */
    for(j = 0; j < ship->cfg->blocks; ++j) {
        b = ship->blocks[j];

        if(b && b->run) {
            pthread_rwlock_rdlock(&b->lock);

            /* Look for the requested user */
            TAILQ_FOREACH(i, b->clients, qentry) {
                pthread_mutex_lock(&i->mutex);

                /* Disconnect them if we find them */
                if(i->guildcard == gc) {
                    if(c->privilege <= i->privilege) {
                        pthread_mutex_unlock(&i->mutex);
                        pthread_rwlock_unlock(&b->lock);
                        return send_txt(c, "%s", __(c, "\tE\tC7???ܰ???Ŷ."));
                    }

                    if(reason) {
                        send_msg_box(i, "%s\n\n%s\n%s",
                                         __(i, "\tE???ѱ?GM?߳???Ϸ."),
                                         __(i, "ԭ??:"), reason);
                    }
                    else {
                        send_msg_box(i, "%s",
                                         __(i, "\tE???ѱ?GM?߳???Ϸ."));
                    }

                    i->flags |= CLIENT_FLAG_DISCONNECTED;
                    pthread_mutex_unlock(&i->mutex);
                    pthread_rwlock_unlock(&b->lock);
                    return 0;
                }

                pthread_mutex_unlock(&i->mutex);
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    /* If the requester is a global GM, forward the request to the shipgate,
       since it wasn't able to be done on this ship. */
    if(GLOBAL_GM(c)) {
        shipgate_send_kick(&ship->sg, c->guildcard, gc, reason);
    }

    return 0;
}

int load_quests(ship_t *s, psocn_ship_t *cfg, int initial) {
    psocn_quest_list_t qlist[CLIENT_VERSION_ALL][CLIENT_LANG_ALL];
    quest_map_t qmap;
    int i, j;
    char fn[512];

    TAILQ_INIT(&qmap);

    /* Read the quest files in... */
    if(cfg->quests_dir && cfg->quests_dir[0]) {
        for(i = 0; i < CLIENT_VERSION_ALL; ++i) {
            for(j = 0; j < CLIENT_LANG_ALL; ++j) {
                sprintf(fn, "%s/%s/quests_%s.xml", cfg->quests_dir,
                    client_type[i]->ver_name, language_codes[j]);
                if(!psocn_quests_read(fn, &qlist[i][j])) {
                    if(!quest_map(&qmap, &qlist[i][j], i, j)) {
                        SHIPS_LOG("??ȡ???? %s ???? %s",
                            client_type[i]->ver_name, language_codes[j]);
                    }
                    else {
                        SHIPS_LOG("?޷????????? %s ???? %s",
                            client_type[i]->ver_name, language_codes[j]);
                        psocn_quests_destroy(&qlist[i][j]);
                    }
                }
            }
        }

        /* Lock the mutex to prevent anyone from trying anything funny. */
        pthread_rwlock_wrlock(&s->qlock);

        /* Out with the old, and in with the new. */
        if(!initial) {
            for(i = 0; i < CLIENT_VERSION_ALL; ++i) {
                for(j = 0; j < CLIENT_LANG_ALL; ++j) {
                    psocn_quests_destroy(&s->qlist[i][j]);
                    s->qlist[i][j] = qlist[i][j];
                }
            }
        }
        else {
            for(i = 0; i < CLIENT_VERSION_ALL; ++i) {
                for(j = 0; j < CLIENT_LANG_ALL; ++j) {
                    s->qlist[i][j] = qlist[i][j];
                }
            }
        }

        if(!initial)
            quest_cleanup(&s->qmap);

        s->qmap = qmap;

        /* XXXX: Hopefully this doesn't fail... >_> */
        if(quest_cache_maps(s, &s->qmap, cfg->quests_dir))
            SHIPS_LOG("?޷?????????????????!");

        /* Unlock the lock, we're done. */
        pthread_rwlock_unlock(&s->qlock);

        return 0;
    }

    SHIPS_LOG("%s: δ?????????ļ?!", cfg->name);
    s->qmap = qmap;
    return -1;
}

void clean_quests(ship_t *s) {
    int i, j;

    for(i = 0; i < CLIENT_VERSION_ALL; ++i) {
        for(j = 0; j < CLIENT_LANG_ALL; ++j) {
            psocn_quests_destroy(&s->qlist[i][j]);
        }
    }

    quest_cleanup(&s->qmap);
}

int refresh_quests(ship_client_t *c, msgfunc f) {
    /* Make sure we don't have anyone trying to escalate their privileges. */
    if(!LOCAL_GM(c))
        return -1;

    if(!load_quests(ship, ship->cfg, 0))
        return f(c, "%s", __(c, "\tE\tC7?ɹ?ˢ???????ļ?."));
    else
        return f(c, "%s", __(c, "\tE\tC7δ?????????ļ?2."));
}

int refresh_gms(ship_client_t *c, msgfunc f) {
    /* Make sure we don't have anyone trying to escalate their privileges. */
    if(!LOCAL_ROOT(c)) {
        return -1;
    }

    if(ship->cfg->gm_file && ship->cfg->gm_file[0]) {
        /* Try to read the GM file. This will clean out the old list as
         well, if needed. */
        if(gm_list_read(ship->cfg->gm_file, ship)) {
            return f(c, "%s", __(c, "\tE\tC7?޷???ȡ GM ?б?."));
        }

        return f(c, "%s", __(c, "\tE\tC7?ɹ?ˢ?? GM ?б?."));
    }
    else {
        return f(c, "%s", __(c, "\tE\tC7δ???? GM ?б?."));
    }
}

int refresh_limits(ship_client_t *c, msgfunc f) {
    psocn_limits_t *l, *def = NULL;
    int i;
    struct limits_queue lq;
    psocn_ship_t *s = ship->cfg;
    limits_entry_t *ent;

    /* Make sure we don't have anyone trying to escalate their privileges. */
    if(!LOCAL_GM(c))
        return -1;

    /* Make sure we had limits configured in the first place... */
    if(!s->limits_count)
        return f(c, "%s", __(c, "\tE\tC7δ???? limits."));

    TAILQ_INIT(&lq);

    /* First, read in all the new files. That way, if something goes wrong, we
       don't clear out existing lists... */
    for(i = 0; i < s->limits_count; ++i) {
        if(psocn_read_limits(s->limits[i].filename, &l)) {
            ERR_LOG("%s: ?޷???ȡ limits ?ļ? %s: %s",
                  s->name, s->limits[i].name, s->limits[i].filename);
            goto err;
        }

        if(!(ent = malloc(sizeof(limits_entry_t)))) {
            ERR_LOG("%s: %s", s->name, strerror(errno));
            psocn_free_limits(l);
            goto err;
        }

        if(s->limits[i].name) {
            if(!(ent->name = _strdup(s->limits[i].name))) {
                ERR_LOG("%s: %s", s->name, strerror(errno));
                psocn_free_limits(l);
                free_safe(ent);
                goto err;
            }

            if(!(l->name = _strdup(s->limits[i].name))) {
                ERR_LOG("%s: %s", s->name, strerror(errno));
                psocn_free_limits(l);
                free_safe(ent->name);
                free_safe(ent);
                goto err;
            }
        }
        else {
            ent->name = NULL;
            l->name = NULL;
        }

        ent->limits = l;
        TAILQ_INSERT_TAIL(&lq, ent, qentry);

        if(s->limits[i].enforce)
            def = l;
    }

    /* If we get here, then everything has at least been read in successfully,
       go ahead and replace the data in the ship's structure. */
    pthread_rwlock_wrlock(&ship->llock);
    ship_free_limits_ex(&ship->all_limits);
    ship->all_limits = lq;
    ship->def_limits = def;
    pthread_rwlock_unlock(&ship->llock);

    return f(c, "%s", __(c, "\tE\tC7?ɹ?ˢ?? limits."));

err:
    ship_free_limits_ex(&lq);
    return f(c, "%s", __(c, "\tE\tC7ˢ?? limits ????????."));
}

int broadcast_message(ship_client_t *c, const char *message, int prefix) {
    block_t *b;
    uint32_t i;
    ship_client_t *i2;

    /* Make sure we don't have anyone trying to escalate their privileges. */
    if(c && !LOCAL_GM(c)) {
        return -1;
    }

    /* Go through each block and send the message to anyone that is alive. */
    for(i = 0; i < ship->cfg->blocks; ++i) {
        b = ship->blocks[i];

        if(b && b->run) {
            pthread_rwlock_rdlock(&b->lock);

            /* Send the message to each player. */
            TAILQ_FOREACH(i2, b->clients, qentry) {
                pthread_mutex_lock(&i2->mutex);

                if(i2->pl) {
                    if(prefix) {
                        send_txt(i2, "%s", __(i2, "\tE\tC7ȫ????Ϣ:"));
                    }

                    send_txt(i2, "%s", message);
                }

                pthread_mutex_unlock(&i2->mutex);
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

int schedule_shutdown(ship_client_t *c, uint32_t when, int restart, msgfunc f) {
    ship_client_t *i2;
    block_t *b;
    uint32_t i;
    extern int restart_on_shutdown;     /* in ship_server.c */

    /* Make sure we don't have anyone trying to escalate their privileges. */
    if(c && !LOCAL_ROOT(c)) {
        return -1;
    }

    /* Go through each block and send a notification to everyone. */
    for(i = 0; i < ship->cfg->blocks; ++i) {
        b = ship->blocks[i];

        if(b && b->run) {
            pthread_rwlock_rdlock(&b->lock);

            /* Send the message to each player. */
            TAILQ_FOREACH(i2, b->clients, qentry) {
                pthread_mutex_lock(&i2->mutex);

                if(i2->pl) {
                    if(i2 != c) {
                        send_txt(i2, "%s %" PRIu32 " %s%s.",
                            __(i2, "\tE\tC7????????"), when,
                            __(i2, "???Ӻ?"), restart ? "????" : "?ر?");
                       /* if(restart) {
                            send_txt(i2, "%s %" PRIu32 " %s",
                                     __(i2, "\tE\tC7????????"), when,
                                     __(i2, "???Ӻ?????."));
                        }
                        else {
                            send_txt(i2, "%s %" PRIu32 " %s%s.",
                                     __(i2, "\tE\tC7????????"), when,
                                     __(i2, "???Ӻ?", restart ? "????" : "?ر?"));
                        }*/
                    }
                    else {
                        f(i2, "%s %" PRIu32 " %s%s.",
                            __(i2, "\tE\tC7????????"), when, __(i2, "???Ӻ?"), restart ? "????" : "?ر?");
                        //if(restart) {
                        //    f(i2, "%s %" PRIu32 " %s%s.",
                        //        __(i2, "\tE\tC7????????"), when, __(i2, "???Ӻ?"), restart ? "????" : "?ر?");
                        //}
                        //else {
                        //    f(i2, "%s %" PRIu32 " %s",
                        //      __(i2, "\tE\tC7????????"), when, __(i2, "???Ӻ??ر?."));
                        //}
                    }
                }

                pthread_mutex_unlock(&i2->mutex);
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    /* Log the event to the log file */
    if(c) {
        SHIPS_LOG("?????ƻ??? %" PRIu32 " ???Ӻ? %s ????:"
              "%"PRIu32".", when, restart ? "????" : "?ر?",
              c->guildcard);
    }
    else {
        SHIPS_LOG("???????????յ??ر?ָ??.");
    }

    restart_on_shutdown = restart;
    ship_server_shutdown(ship, time(NULL) + ((int)when * 60));

    return 0;
}

int global_ban(ship_client_t *c, uint32_t gc, uint32_t l, const char *reason) {
    const char *len = NULL;
    block_t *b;
    ship_client_t *i;
    uint32_t j;

    /* Make sure we don't have anyone trying to escalate their privileges. */
    if(!GLOBAL_GM(c)) {
        return -1;
    }

    /* Set the ban with the shipgate first. */
    if(shipgate_send_ban(&ship->sg, SHDR_TYPE_GCBAN, c->guildcard, gc, l,
                         reason)) {
        return send_txt(c, "%s", __(c, "\tE\tC7???÷???????."));
    }

    /* Look through all the blocks for the requested user, and kick the first
       instance we happen to find, if any (there shouldn't be more than one). */
    for(j = 0; j < ship->cfg->blocks; ++j) {
        b = ship->blocks[j];

        if(b && b->run) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if(i->guildcard == gc) {
                    pthread_mutex_lock(&i->mutex);

                    /* Make sure we're not trying something dirty (the gate
                       should also have blocked the ban if this happens, in
                       most cases anyway) */
                    if(c->privilege <= i->privilege) {
                        pthread_mutex_unlock(&i->mutex);
                        pthread_rwlock_unlock(&b->lock);
                        return send_txt(c, "%s", __(c, "\tE\tC7???ܰ???Ŷ."));
                    }

                    /* Handle the common cases... */
                    switch(l) {
                        case 0xFFFFFFFF:
                            len = __(i, "Forever");
                            break;

                        case 2592000:
                            len = __(i, "30 days");
                            break;

                        case 604800:
                            len = __(i, "1 week");
                            break;

                        case 86400:
                            len = __(i, "1 day");
                            break;

                        /* Other cases just don't have a length on them... */
                    }

                    /* Send the user a message telling them they're banned. */
                    if(reason && len) {
                        send_msg_box(i, "%s\n%s %s\n%s\n%s",
                                         __(i, "\tEYou have been banned by a "
                                            "GM."), __(i, "Ban Length:"),
                                         len, __(i, "Reason:"), reason);
                    }
                    else if(len) {
                        send_msg_box(i, "%s\n%s %s",
                                         __(i, "\tEYou have been banned by a "
                                            "GM."), __(i, "Ban Length:"),
                                         len);
                    }
                    else if(reason) {
                        send_msg_box(i, "%s\n%s\n%s",
                                         __(i, "\tEYou have been banned by a "
                                            "GM."), __(i, "Reason:"), reason);
                    }
                    else {
                        send_msg_box(i, "%s", __(i, "\tEYou have been "
                                                     "banned by a GM."));
                    }

                    i->flags |= CLIENT_FLAG_DISCONNECTED;

                    /* The ban setter will get a message telling them the ban has been
                       set (or an error happened). */
                    pthread_mutex_unlock(&i->mutex);
                    pthread_rwlock_unlock(&b->lock);
                    return 0;
                }

                pthread_mutex_unlock(&i->mutex);
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    /* Since the requester is a global GM, forward the kick request to the
       shipgate, since it wasn't able to be done on this ship. */
    if(GLOBAL_GM(c)) {
        shipgate_send_kick(&ship->sg, c->guildcard, gc, reason);
    }

    return 0;
}
