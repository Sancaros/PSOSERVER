/*
	梦幻之星中国 舰船服务器 Item RT RE MAP
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

#ifndef RT_RE_MAP
#define RT_RE_MAP

static const int tool_base[28] = {
	Item_Monomate,
	Item_Dimate,
	Item_Trimate,
	Item_Monofluid,
	Item_Difluid,
	Item_Trifluid,
	Item_Antidote,
	Item_Antiparalysis,
	Item_Sol_Atomizer,
	Item_Moon_Atomizer,
	Item_Star_Atomizer,
	Item_Telepipe,
	Item_Trap_Vision,
	Item_Monogrinder,
	Item_Digrinder,
	Item_Trigrinder,
	Item_Power_Material,
	Item_Mind_Material,
	Item_Evade_Material,
	Item_HP_Material,
	Item_TP_Material,
	Item_Def_Material,
	Item_Hit_Material,
	Item_Luck_Material,
	Item_Scape_Doll,
	Item_Disk_Lv01,
	Item_Photon_Drop,
	Item_NoSuchItem
};

#define EPSILON 0.001f

// For remapping Episode IV monsters to Episode I counterparts...
static const int ep2_rtremap[] = {
	0x00, 0x00, // Pioneer 2
	0x01, 0x01, // VR Temple Alpha
	0x02, 0x02, // VR Temple Beta
	0x03, 0x03, // VR Spaceship Alpha
	0x04, 0x04, // VR Spaceship Beta
	0x05, 0x08, // CCA
	0x06, 0x05, // Jungle North
	0x07, 0x06, // Jungle East
	0x08, 0x07, // Mountain
	0x09, 0x08, // Seaside
	0x0A, 0x09, // Seabed Upper
	0x0B, 0x0A, // Seabed Lower
};

static const int ep4_rtremap[] = {
	0x00, 0x00, // None to None
	0x01, 0x01, // Remap Astark to Hildebear
	0x02, 0x08, // Remap Yowie to Barbarous Wolf
	0x03, 0x07, // Remap Satellite Lizard to Savage Wolf
	0x04, 0x13, // Remap Merissa A to Poufilly Slime
	0x05, 0x14, // Remap Merissa AA to Pouilly Slime
	0x06, 0x38, // Remap Girtablulu to Mericarol
	0x07, 0x37, // Remap Zu to Gi Gue
	0x08, 0x02, // Remap Pazuzu to Hildeblue
	0x09, 0x09, // Remap Boota to Booma
	0x0A, 0x0A, // Remap Ze Boota to Gobooma
	0x0B, 0x0B, // Remap Ba Boota to Gigobooma
	0x0C, 0x48, // Remap Dorphon to Delbiter
	0x0D, 0x02, // Remap Dorphon Eclair to Hildeblue
	0x0E, 0x29, // Remap Goran to Dimenian
	0x0F, 0x2B, // Remap Goran Detonator to So Dimenian
	0x10, 0x2A, // Remap Pyro Goran to La Dimenian
	0x11, 0x05, // Remap Sand Rappy to Rappy
	0x12, 0x06, // Remap Del Rappy to Al Rappy
	0x13, 0x2F, // Remap Saint Million to Dark Falz
	0x14, 0x2F, // Remap Shambertin to Dark Falz
	0x15, 0x2F, // Remap Kondrieu to Dark Falz
};

#endif // !RT_RE_MAP