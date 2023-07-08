/*
    �λ�֮���й� ���������� ��Ʒ����
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

#ifndef PSO_ITEMS_H
#define PSO_ITEMS_H

#include "pso_player.h"
#include "pso_item_list.h"

// item_equip_flags ְҵװ����־ ����ʶ��ͬ����
#define EQUIP_FLAGS_NONE     1
#define EQUIP_FLAGS_OK       0
#define EQUIP_FLAGS_HUNTER   0x01   // Bit 1 ����
#define EQUIP_FLAGS_RANGER   0x02   // Bit 2 ǹ��
#define EQUIP_FLAGS_FORCE    0x04   // Bit 3 ��ʦ
#define EQUIP_FLAGS_HUMAN    0x08   // Bit 4 ����
#define EQUIP_FLAGS_DROID    0x10   // Bit 5 ������
#define EQUIP_FLAGS_NEWMAN   0x20   // Bit 6 ������
#define EQUIP_FLAGS_MALE     0x40   // Bit 7 ����
#define EQUIP_FLAGS_FEMALE   0x80   // Bit 8 Ů��
#define EQUIP_FLAGS_MAX      8

#define MAX_LOBBY_SAVED_ITEMS      3000

/* Item buckets. Each item gets put into one of these buckets when in the list,
   in order to make searching the list slightly easier. These are all based on
   the least significant byte of the item code. */
#define ITEM_TYPE_WEAPON                0x00
#define ITEM_TYPE_GUARD                 0x01
#define ITEM_TYPE_MAG                   0x02
#define ITEM_TYPE_TOOL                  0x03
#define ITEM_TYPE_MESETA                0x04

#define MESETA_IDENTIFIER               0x00040000

   /* ITEM_TYPE_GUARD items are actually slightly more specialized, and fall into
      three subtypes of their own. These are the second least significant byte in
      the item code. */
#define ITEM_SUBTYPE_FRAME              0x01
#define ITEM_SUBTYPE_BARRIER            0x02
#define ITEM_SUBTYPE_UNIT               0x03

      /* ITEM_TYPE_TOOL subtype*/
#define ITEM_SUBTYPE_MATE               0x00
#define ITEM_SUBTYPE_FLUID              0x01
#define ITEM_SUBTYPE_DISK               0x02
#define ITEM_SUBTYPE_SOL_ATOMIZER       0x03
#define ITEM_SUBTYPE_MOON_ATOMIZER      0x04
#define ITEM_SUBTYPE_STAR_ATOMIZER      0x05
#define ITEM_SUBTYPE_ANTI               0x06
#define ITEM_SUBTYPE_TELEPIPE           0x07
#define ITEM_SUBTYPE_TRAP_VISION        0x08
#define ITEM_SUBTYPE_GRINDER            0x0A
#define ITEM_SUBTYPE_MATERIAL           0x0B
#define ITEM_SUBTYPE_MAG_CELL1          0x0C
#define ITEM_SUBTYPE_MONSTER_LIMBS      0x0D
#define ITEM_SUBTYPE_MAG_CELL2          0x0E
#define ITEM_SUBTYPE_ADD_SLOT           0x0F
#define ITEM_SUBTYPE_PHOTON             0x10

/* Default behaviors for the item lists. ITEM_DEFAULT_ALLOW means to accept any
   things NOT in the list read in by default, whereas ITEM_DEFAULT_REJECT causes
   unlisted items to be rejected. */
#define ITEM_DEFAULT_ALLOW      1
#define ITEM_DEFAULT_REJECT     0

   /* Version codes, as used in this part of the code. */
#define ITEM_VERSION_V1         0x01
#define ITEM_VERSION_V2         0x02
#define ITEM_VERSION_GC         0x04
#define ITEM_VERSION_XBOX       0x08
#define ITEM_VERSION_BB         0x10

/* ��ȡ��Ʒ���� */
const char* item_get_name(item_t* item, int version);

/* ��ӡ��Ʒ���� */
void print_item_data(item_t* item, int version);

/* ��ӡ������Ʒ���� */
void print_iitem_data(iitem_t* iitem, int item_index, int version);

/* ��ӡ������Ʒ���� */
void print_bitem_data(bitem_t* iitem, int item_index, int version);

void print_biitem_data(void* data, int item_index, int version, int inv, int err);

#endif /* !PSO_ITEMS_H */