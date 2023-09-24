/*
    �λ�֮���й� �����ṹ

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

#ifndef PSO_STRUCT_TECHNIQUES_H
#define PSO_STRUCT_TECHNIQUES_H

#include <stdint.h>

/* �Ƽ�����ID */
/* ������ */
#define TECHNIQUE_FOIE              0
/* ��ǽ�� */
#define TECHNIQUE_GIFOIE            1
/* ������ */
#define TECHNIQUE_RAFOIE            2
/* ������ */
#define TECHNIQUE_BARTA             3
/* ������ */
#define TECHNIQUE_GIBARTA           4
/* ������ */
#define TECHNIQUE_RABARTA           5
/* ������ */
#define TECHNIQUE_ZONDE             6
/* Ⱥ���� */
#define TECHNIQUE_GIZONDE           7
/* �ױ��� */
#define TECHNIQUE_RAZONDE           8
/* ʥ���� */
#define TECHNIQUE_GRANTS            9
/* ǿ���� */
#define TECHNIQUE_DEBAND            10
/* ������ */
#define TECHNIQUE_JELLEN            11
/* ������ */
#define TECHNIQUE_ZALURE            12
/* ǿ���� */
#define TECHNIQUE_SHIFTA            13
/* ˲���� */
#define TECHNIQUE_RYUKER            14
/* ʥȪ�� */
#define TECHNIQUE_RESTA             15
/* ״̬�� */
#define TECHNIQUE_ANTI              16
/* �ػ��� */
#define TECHNIQUE_REVERSER          17
/* ������ */
#define TECHNIQUE_MEGID             18

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct player_techniques {
    union {
        struct {
            /* ������ */ uint8_t foie;
            /* ��ǽ�� */ uint8_t gifoie;
            /* ������ */ uint8_t rafoie;
            /* ������ */ uint8_t barta;
            /* ������ */ uint8_t gibarta;
            /* ������ */ uint8_t rabarta;
            /* ������ */ uint8_t zonde;
            /* Ⱥ���� */ uint8_t gizonde;
            /* �ױ��� */ uint8_t razonde;
            /* ʥ���� */ uint8_t grants;
            /* ǿ���� */ uint8_t deband;
            /* ������ */ uint8_t jellen;
            /* ������ */ uint8_t zalure;
            /* ǿ���� */ uint8_t shifta;
            /* ˲���� */ uint8_t ryuker;
            /* ʥȪ�� */ uint8_t resta;
            /* ״̬�� */ uint8_t anti;
            /* �ػ��� */ uint8_t reverser;
            /* ������ */ uint8_t megid;
            /* ?????? */ uint8_t unused;
        }PACKED;
        uint8_t all[0x14];
    }PACKED;
} PACKED techniques_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

static inline const char* get_technique_comment(int index) {
    switch (index) {
    case TECHNIQUE_FOIE:
        return "������";
    case TECHNIQUE_GIFOIE:
        return "��ǽ��";
    case TECHNIQUE_RAFOIE:
        return "������";
    case TECHNIQUE_BARTA:
        return "������";
    case TECHNIQUE_GIBARTA:
        return "������";
    case TECHNIQUE_RABARTA:
        return "������";
    case TECHNIQUE_ZONDE:
        return "������";
    case TECHNIQUE_GIZONDE:
        return "Ⱥ����";
    case TECHNIQUE_RAZONDE:
        return "�ױ���";
    case TECHNIQUE_GRANTS:
        return "ʥ����";
    case TECHNIQUE_DEBAND:
        return "ǿ����";
    case TECHNIQUE_JELLEN:
        return "������";
    case TECHNIQUE_ZALURE:
        return "������";
    case TECHNIQUE_SHIFTA:
        return "ǿ����";
    case TECHNIQUE_RYUKER:
        return "˲����";
    case TECHNIQUE_RESTA:
        return "ʥȪ��";
    case TECHNIQUE_ANTI:
        return "״̬��";
    case TECHNIQUE_REVERSER:
        return "�ػ���";
    case TECHNIQUE_MEGID:
        return "������";
    default:
        return "δ֪����";
    }
}

#endif // !PSO_STRUCT_TECHNIQUES_H
