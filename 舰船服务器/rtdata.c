/*
    �λ�֮���й� ����������
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <f_logs.h>
#include <mtwist.h>

#include <AFS.h>
#include <GSL.h>

#include "rtdata.h"
#include "ship_packets.h"

/* This function based on information from a couple of different sources, namely
   Fuzziqer's newserv and information from Lee (through Aleron Ives). */
static double expand_rate(uint8_t rate) {
    int tmp = (rate >> 3) - 4;
    uint32_t expd;

    if(tmp < 0)
        tmp = 0;

    expd = (2 << tmp) * ((rate & 7) + 7);
    return (double)expd / (double)0x100000000ULL;
}

int rt_read_v2(const char *fn) {
    FILE *fp;
    uint8_t buf[30];
    int rv = 0, i, j, k;
    uint32_t offsets[40], tmp;
    rt_entry_t ent;

    have_v2rt = 0;

    /* Open up the file */
    if(!(fp = fopen(fn, "rb"))) {
        ERR_LOG("�޷��� %s �ļ�: %s", fn, strerror(errno));
        return -1;
    }

    /* Make sure that it looks like a sane AFS file. */
    if(fread(buf, 1, 4, fp) != 4) {
        ERR_LOG("��ȡ�ļ�����: %s", strerror(errno));
        rv = -2;
        goto out;
    }

    if(buf[0] != 0x41 || buf[1] != 0x46 || buf[2] != 0x53 || buf[3] != 0x00) {
        ERR_LOG("%s is not an AFS archive!", fn);
        rv = -3;
        goto out;
    }

    /* Make sure there are exactly 40 entries */
    if(fread(buf, 1, 4, fp) != 4) {
        ERR_LOG("��ȡ�ļ�����: %s", strerror(errno));
        rv = -2;
        goto out;
    }

    if(buf[0] != 40 || buf[1] != 0 || buf[2] != 0 || buf[3] != 0) {
        ERR_LOG("%s does not appear to be an ItemRT.afs file", fn);
        rv = -4;
        goto out;
    }

    /* Read in the offsets and lengths */
    for(i = 0; i < 40; ++i) {
        if(fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("��ȡ�ļ�����: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        offsets[i] = (buf[0]) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

        if(fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("��ȡ�ļ�����: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        if(buf[0] != 0x80 || buf[1] != 0x02 || buf[2] != 0 || buf[3] != 0) {
            ERR_LOG("Invalid sized entry in ItemRT.afs!");
            rv = 5;
            goto out;
        }
    }

    /* Now, parse each entry... */
    for(i = 0; i < 4; ++i) {
        for(j = 0; j < 10; ++j) {
            if(fseek(fp, (long)offsets[i * 10 + j], SEEK_SET)) {
                ERR_LOG("fseek����: %s", strerror(errno));
                rv = -2;
                goto out;
            }

            /* Read in the enemy entries */
            for(k = 0; k < 0x65; ++k) {
                if(fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                   sizeof(rt_entry_t)) {
                    ERR_LOG("��ȡ ItemRT �ļ�����: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                    (ent.item_data[2] << 16);
                v2_rtdata[i][j].enemy_rares[k].prob = expand_rate(ent.prob);
                v2_rtdata[i][j].enemy_rares[k].item_data = tmp;
                v2_rtdata[i][j].enemy_rares[k].area = 0; /* Unused */
            }

            /* Read in the box entries */
            if(fread(buf, 1, 30, fp) != 30) {
                ERR_LOG("��ȡ ItemRT �ļ�����: %s", strerror(errno));
                rv = -2;
                goto out;
            }

            for(k = 0; k < 30; ++k) {
                if(fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                   sizeof(rt_entry_t)) {
                    ERR_LOG("��ȡ ItemRT �ļ�����: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                    (ent.item_data[2] << 16);
                v2_rtdata[i][j].box_rares[k].prob = expand_rate(ent.prob);
                v2_rtdata[i][j].box_rares[k].item_data = tmp;
                v2_rtdata[i][j].box_rares[k].area = buf[k];
            }
        }
    }

    have_v2rt = 1;

out:
    fclose(fp);
    return rv;
}

int rt_read_gc(const char *fn) {
    FILE *fp;
    uint8_t buf[30];
    int rv = 0, i, j, k, l;
    uint32_t offsets[80], tmp;
    rt_entry_t ent;

    have_gcrt = 0;

    /* Open up the file */
    if(!(fp = fopen(fn, "rb"))) {
        ERR_LOG("�޷��� ItemRT �ļ� %s: %s", fn, strerror(errno));
        return -1;
    }

    /* Read in the offsets and lengths for the Episode I & II data. */
    for(i = 0; i < 80; ++i) {
        if(fseek(fp, 32, SEEK_CUR)) {
            ERR_LOG("fseek����: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        if(fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("��ȡ�ļ�����: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        /* The offsets are in 2048 byte blocks. */
        offsets[i] = (buf[3]) | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);
        offsets[i] <<= 11;

        if(fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("��ȡ�ļ�����: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        if(buf[0] != 0 || buf[1] != 0 || buf[2] != 0x02 || buf[3] != 0x80) {
            ERR_LOG("ItemRT.gsl �е���Ŀ��С��Ч!");
            rv = 5;
            goto out;
        }

        /* Skip over the padding. */
        if(fseek(fp, 8, SEEK_CUR)) {
            ERR_LOG("fseek����: %s", strerror(errno));
            rv = -2;
            goto out;
        }
    }

    /* Now, parse each entry... */
    for(i = 0; i < 2; ++i) {
        for(j = 0; j < 4; ++j) {
            for(k = 0; k < 10; ++k) {
                if(fseek(fp, (long)offsets[i * 40 + j * 10 + k], SEEK_SET)) {
                    ERR_LOG("fseek����: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                /* Read in the enemy entries */
                for(l = 0; l < 0x65; ++l) {
                    if(fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                       sizeof(rt_entry_t)) {
                        ERR_LOG("��ȡ ItemRT �ļ�����: %s",
                              strerror(errno));
                        rv = -2;
                        goto out;
                    }

                    tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                        (ent.item_data[2] << 16);
                    gc_rtdata[i][j][k].enemy_rares[l].prob =
                        expand_rate(ent.prob);
                    gc_rtdata[i][j][k].enemy_rares[l].item_data = tmp;
                    gc_rtdata[i][j][k].enemy_rares[l].area = 0; /* Unused */
                }

                /* Read in the box entries */
                if(fread(buf, 1, 30, fp) != 30) {
                    ERR_LOG("��ȡ ItemRT �ļ�����: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                for(l = 0; l < 30; ++l) {
                    if(fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                       sizeof(rt_entry_t)) {
                        ERR_LOG("��ȡ ItemRT �ļ�����: %s",
                              strerror(errno));
                        rv = -2;
                        goto out;
                    }

                    tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                        (ent.item_data[2] << 16);
                    gc_rtdata[i][j][k].box_rares[l].prob =
                        expand_rate(ent.prob);
                    gc_rtdata[i][j][k].box_rares[l].item_data = tmp;
                    gc_rtdata[i][j][k].box_rares[l].area = buf[k];
                }
            }
        }
    }

    have_gcrt = 1;

out:
    fclose(fp);
    return rv;
}

int rt_read_bb(const char* fn) {
    FILE* fp;
    uint8_t buf[30];
    int rv = 0, i, j, k, l;
    uint32_t offsets[160], tmp;
    rt_entry_t ent;

    have_bbrt = 0;

    /* Open up the file */
    if (!(fp = fopen(fn, "rb"))) {
        ERR_LOG("�޷��� ItemRT �ļ� %s: %s", fn, strerror(errno));
        return -1;
    }

    /* Read in the offsets and lengths for the Episode I & II & IV data. */
    for (i = 0; i < 160; ++i) {
        if (fseek(fp, 32, SEEK_CUR)) {
            ERR_LOG("fseek����: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        if (fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("��ȡ�ļ�����: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        /* The offsets are in 2048 byte blocks. */
        offsets[i] = (buf[3]) | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);
        offsets[i] <<= 11;

        if (fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("��ȡ�ļ�����: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        if (buf[0] != 0 || buf[1] != 0 || buf[2] != 0x02 || buf[3] != 0x80) {
            ERR_LOG("ItemRT.gsl �е���Ŀ��С��Ч!");
            rv = 5;
            goto out;
        }

        /* Skip over the padding. */
        if (fseek(fp, 8, SEEK_CUR)) {
            ERR_LOG("fseek����: %s", strerror(errno));
            rv = -2;
            goto out;
        }
    }

    //EP1 0  NULL / EP2 1  l /  CHALLENGE 2 c / EP4 3 bb

    /* ��ʼ����ÿһ��ʵ������.. */
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            for (k = 0; k < 10; ++k) {
                if (fseek(fp, (long)offsets[i * 40 + j * 10 + k], SEEK_SET)) {
                    ERR_LOG("fseek����: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                /* Read in the enemy entries */
                for (l = 0; l < 0x65; ++l) {
                    if (fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                        sizeof(rt_entry_t)) {
                        ERR_LOG("��ȡ ItemRT �ļ�����: %s",
                            strerror(errno));
                        rv = -2;
                        goto out;
                    }
					
                    tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                        (ent.item_data[2] << 16);
                    bb_rtdata[i][j][k].enemy_rares[l].prob =
                        expand_rate(ent.prob);
                    bb_rtdata[i][j][k].enemy_rares[l].item_data = tmp;
                    bb_rtdata[i][j][k].enemy_rares[l].area = 0; /* Unused */
                }

                /* Read in the box entries */
                if (fread(buf, 1, 30, fp) != 30) {
                    ERR_LOG("��ȡ ItemRT �ļ�����: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                for (l = 0; l < 30; ++l) {
                    if (fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                        sizeof(rt_entry_t)) {
                        ERR_LOG("��ȡ ItemRT �ļ�����: %s",
                            strerror(errno));
                        rv = -2;
                        goto out;
                    }
					
                    tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                        (ent.item_data[2] << 16);
                    bb_rtdata[i][j][k].box_rares[l].prob =
                        expand_rate(ent.prob);
                    bb_rtdata[i][j][k].box_rares[l].item_data = tmp;
                    bb_rtdata[i][j][k].box_rares[l].area = buf[k];
                }
            }
        }
    }

    have_gcrt = 1;

out:
    fclose(fp);
    return rv;
}

int rt_v2_enabled(void) {
    return have_v2rt;
}

int rt_gc_enabled(void) {
    return have_gcrt;
}

int rt_bb_enabled(void) {
    return have_bbrt;
}

uint32_t rt_generate_v2_rare(ship_client_t *c, lobby_t *l, int rt_index,
                             int area) {
    struct mt19937_state *rng = &c->cur_block->rng;
    double rnd;
    rt_set_t *set;
    int i;
    int section = l->clients[l->leader_id]->pl->v1.character.disp.dress_data.section;

    /* Make sure we read in a rare table and we have a sane index */
    if(!have_v2rt)
        return 0;

    if(rt_index < -1 || rt_index > 100)
        return -1;

    /* Grab the rare set for the game */
    set = &v2_rtdata[l->difficulty][section];

    /* Are we doing a drop for an enemy or a box? */
    if(rt_index >= 0) {
        rnd = mt19937_genrand_real1(rng);

        if (rnd < set->enemy_rares[rt_index].prob)
            return set->enemy_rares[rt_index].item_data;
    }
    else {
        for (i = 0; i < 30; ++i) {
            if (set->box_rares[i].area == area) {
                rnd = mt19937_genrand_real1(rng);

                if(rnd < set->box_rares[i].prob)
                    return set->box_rares[i].item_data;
            }
        }
    }

    return 0;
}

uint32_t rt_generate_gc_rare(ship_client_t *c, lobby_t *l, int rt_index,
                             int area) {
    struct mt19937_state *rng = &c->cur_block->rng;
    double rnd;
    rt_set_t *set;
    int i;
    int section = l->clients[l->leader_id]->pl->v1.character.disp.dress_data.section;

    /* Make sure we read in a rare table and we have a sane index */
    if(!have_gcrt)
        return 0;

    if(rt_index < -1 || rt_index > 100)
        return -1;

    /* Grab the rare set for the game */
    set = &gc_rtdata[l->episode - 1][l->difficulty][section];

    /* Are we doing a drop for an enemy or a box? */
    if(rt_index >= 0) {
        rnd = mt19937_genrand_real1(rng);

        if (rnd < set->enemy_rares[rt_index].prob)
            return set->enemy_rares[rt_index].item_data;
    }
    else {
        for (i = 0; i < 30; ++i) {
            if (set->box_rares[i].area == area) {
                rnd = mt19937_genrand_real1(rng);

                if(rnd < set->box_rares[i].prob)
                    return set->box_rares[i].item_data;
            }
        }
    }

    return 0;
}

uint32_t rt_generate_bb_rare(ship_client_t* c, lobby_t* l, int rt_index,
    int area) {
    struct mt19937_state* rng = &c->cur_block->rng;
    double rnd;
    rt_set_t* set;
    int i;
    int section = l->clients[l->leader_id]->pl->v1.character.disp.dress_data.section;
    uint8_t game_type = 0;//����Ϸ�½��������

    /* Make sure we read in a rare table and we have a sane index */
    if (!have_bbrt)
        return 0;

    if (rt_index < -1 || rt_index > 100)
        return -1;

    /* Grab the rare set for the game */
    //EP1 0  NULL / EP2 1  l /  CHALLENGE 2 c / EP4 3 bb

    switch (l->episode)
    {
    case GAME_TYPE_EPISODE_1:
        if (l->challenge)
            game_type = 2;
        else
            game_type = 0;
        break;
    case GAME_TYPE_EPISODE_2:
        if (l->challenge)
            game_type = 2;
        else
            game_type = 1;
        break;
    case GAME_TYPE_EPISODE_4:
        game_type = 3;
        break;
    default:
        game_type = 0;
        break;
    }

    set = &bb_rtdata[game_type][l->difficulty][section];

    /* Are we doing a drop for an enemy or a box? */
    if (rt_index >= 0) {
        rnd = mt19937_genrand_real1(rng);

        if (rnd < set->enemy_rares[rt_index].prob)
            return set->enemy_rares[rt_index].item_data;
    }
    else {
        for (i = 0; i < 30; ++i) {
            if (set->box_rares[i].area == area) {
                rnd = mt19937_genrand_real1(rng);

                if (rnd < set->box_rares[i].prob)
                    return set->box_rares[i].item_data;
            }
        }
    }

    return 0;
}