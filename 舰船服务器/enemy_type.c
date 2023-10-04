/*
    梦幻之星中国 舰船服务器 敌人类型
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

#include "f_logs.h"
#include "pso_text.h"
#include "pso_StringReader.h"

#include "enemy_type.h"

const char* name_for_enum(EnemyType type) {
    switch (type) {
    case ENEMY_NONE:
        return "NONE";
    case ENEMY_UNKNOWN:
        return "UNKNOWN";
    case ENEMY_AL_RAPPY:
        return "AL_RAPPY";
    case ENEMY_ASTARK:
        return "ASTARK";
    case ENEMY_BA_BOOTA:
        return "BA_BOOTA";
    case ENEMY_BARBA_RAY:
        return "BARBA_RAY";
    case ENEMY_BARBAROUS_WOLF:
        return "BARBAROUS_WOLF";
    case ENEMY_BEE_L:
        return "BEE_L";
    case ENEMY_BEE_R:
        return "BEE_R";
    case ENEMY_BOOMA:
        return "BOOMA";
    case ENEMY_BOOTA:
        return "BOOTA";
    case ENEMY_BULCLAW:
        return "BULCLAW";
    case ENEMY_CANADINE:
        return "CANADINE";
    case ENEMY_CANADINE_GROUP:
        return "CANADINE_GROUP";
    case ENEMY_CANANE:
        return "CANANE";
    case ENEMY_CHAOS_BRINGER:
        return "CHAOS_BRINGER";
    case ENEMY_CHAOS_SORCERER:
        return "CHAOS_SORCERER";
    case ENEMY_CLAW:
        return "CLAW";
    case ENEMY_DARK_BELRA:
        return "DARK_BELRA";
    case ENEMY_DARK_FALZ_1:
        return "DARK_FALZ_1";
    case ENEMY_DARK_FALZ_2:
        return "DARK_FALZ_2";
    case ENEMY_DARK_FALZ_3:
        return "DARK_FALZ_3";
    case ENEMY_DARK_GUNNER:
        return "DARK_GUNNER";
    case ENEMY_DARVANT:
        return "DARVANT";
    case ENEMY_DE_ROL_LE:
        return "DE_ROL_LE";
    case ENEMY_DE_ROL_LE_BODY:
        return "DE_ROL_LE_BODY";
    case ENEMY_DE_ROL_LE_MINE:
        return "DE_ROL_LE_MINE";
    case ENEMY_DEATH_GUNNER:
        return "DEATH_GUNNER";
    case ENEMY_DEL_LILY:
        return "DEL_LILY";
    case ENEMY_DEL_RAPPY:
        return "DEL_RAPPY";
    case ENEMY_DEL_RAPPY_ALT:
        return "DEL_RAPPY_ALT";
    case ENEMY_DELBITER:
        return "DELBITER";
    case ENEMY_DELDEPTH:
        return "DELDEPTH";
    case ENEMY_DELSABER:
        return "DELSABER";
    case ENEMY_DIMENIAN:
        return "DIMENIAN";
    case ENEMY_DOLMDARL:
        return "DOLMDARL";
    case ENEMY_DOLMOLM:
        return "DOLMOLM";
    case ENEMY_DORPHON:
        return "DORPHON";
    case ENEMY_DORPHON_ECLAIR:
        return "DORPHON_ECLAIR";
    case ENEMY_DRAGON:
        return "DRAGON";
    case ENEMY_DUBCHIC:
        return "DUBCHIC";
    case ENEMY_DUBWITCH:
        return "DUBWITCH";
    case ENEMY_EGG_RAPPY:
        return "EGG_RAPPY";
    case ENEMY_EPSIGUARD:
        return "EPSIGUARD";
    case ENEMY_EPSILON:
        return "EPSILON";
    case ENEMY_EVIL_SHARK:
        return "EVIL_SHARK";
    case ENEMY_GAEL:
        return "GAEL";
    case ENEMY_GAL_GRYPHON:
        return "GAL_GRYPHON";
    case ENEMY_GARANZ:
        return "GARANZ";
    case ENEMY_GEE:
        return "GEE";
    case ENEMY_GI_GUE:
        return "GI_GUE";
    case ENEMY_GIBBLES:
        return "GIBBLES";
    case ENEMY_GIGOBOOMA:
        return "GIGOBOOMA";
    case ENEMY_GILLCHIC:
        return "GILLCHIC";
    case ENEMY_GIRTABLULU:
        return "GIRTABLULU";
    case ENEMY_GOBOOMA:
        return "GOBOOMA";
    case ENEMY_GOL_DRAGON:
        return "GOL_DRAGON";
    case ENEMY_GORAN:
        return "GORAN";
    case ENEMY_GORAN_DETONATOR:
        return "GORAN_DETONATOR";
    case ENEMY_GRASS_ASSASSIN:
        return "GRASS_ASSASSIN";
    case ENEMY_GUIL_SHARK:
        return "GUIL_SHARK";
    case ENEMY_HALLO_RAPPY:
        return "HALLO_RAPPY";
    case ENEMY_HIDOOM:
        return "HIDOOM";
    case ENEMY_HILDEBEAR:
        return "HILDEBEAR";
    case ENEMY_HILDEBLUE:
        return "HILDEBLUE";
    case ENEMY_ILL_GILL:
        return "ILL_GILL";
    case ENEMY_KONDRIEU:
        return "KONDRIEU";
    case ENEMY_LA_DIMENIAN:
        return "LA_DIMENIAN";
    case ENEMY_LOVE_RAPPY:
        return "LOVE_RAPPY";
    case ENEMY_MERICAROL:
        return "MERICAROL";
    case ENEMY_MERICUS:
        return "MERICUS";
    case ENEMY_MERIKLE:
        return "MERIKLE";
    case ENEMY_MERILLIA:
        return "MERILLIA";
    case ENEMY_MERILTAS:
        return "MERILTAS";
    case ENEMY_MERISSA_A:
        return "MERISSA_A";
    case ENEMY_MERISSA_AA:
        return "MERISSA_AA";
    case ENEMY_MIGIUM:
        return "MIGIUM";
    case ENEMY_MONEST:
        return "MONEST";
    case ENEMY_MORFOS:
        return "MORFOS";
    case ENEMY_MOTHMANT:
        return "MOTHMANT";
    case ENEMY_NANO_DRAGON:
        return "NANO_DRAGON";
    case ENEMY_NAR_LILY:
        return "NAR_LILY";
    case ENEMY_OLGA_FLOW_1:
        return "OLGA_FLOW_1";
    case ENEMY_OLGA_FLOW_2:
        return "OLGA_FLOW_2";
    case ENEMY_PAL_SHARK:
        return "PAL_SHARK";
    case ENEMY_PAN_ARMS:
        return "PAN_ARMS";
    case ENEMY_PAZUZU:
        return "PAZUZU";
    case ENEMY_PAZUZU_ALT:
        return "PAZUZU_ALT";
    case ENEMY_PIG_RAY:
        return "PIG_RAY";
    case ENEMY_POFUILLY_SLIME:
        return "POFUILLY_SLIME";
    case ENEMY_POUILLY_SLIME:
        return "POUILLY_SLIME";
    case ENEMY_POISON_LILY:
        return "POISON_LILY";
    case ENEMY_PYRO_GORAN:
        return "PYRO_GORAN";
    case ENEMY_RAG_RAPPY:
        return "RAG_RAPPY";
    case ENEMY_RECOBOX:
        return "RECOBOX";
    case ENEMY_RECON:
        return "RECON";
    case ENEMY_SAINT_MILLION:
        return "SAINT_MILLION";
    case ENEMY_SAINT_RAPPY:
        return "SAINT_RAPPY";
    case ENEMY_SAND_RAPPY:
        return "SAND_RAPPY";
    case ENEMY_SAND_RAPPY_ALT:
        return "SAND_RAPPY_ALT";
    case ENEMY_SATELLITE_LIZARD:
        return "SATELLITE_LIZARD";
    case ENEMY_SATELLITE_LIZARD_ALT:
        return "SATELLITE_LIZARD_ALT";
    case ENEMY_SAVAGE_WOLF:
        return "SAVAGE_WOLF";
    case ENEMY_SHAMBERTIN:
        return "SHAMBERTIN";
    case ENEMY_SINOW_BEAT:
        return "SINOW_BEAT";
    case ENEMY_SINOW_BERILL:
        return "SINOW_BERILL";
    case ENEMY_SINOW_GOLD:
        return "SINOW_GOLD";
    case ENEMY_SINOW_SPIGELL:
        return "SINOW_SPIGELL";
    case ENEMY_SINOW_ZELE:
        return "SINOW_ZELE";
    case ENEMY_SINOW_ZOA:
        return "SINOW_ZOA";
    case ENEMY_SO_DIMENIAN:
        return "SO_DIMENIAN";
    case ENEMY_UL_GIBBON:
        return "UL_GIBBON";
    case ENEMY_VOL_OPT_1:
        return "VOL_OPT_1";
    case ENEMY_VOL_OPT_2:
        return "VOL_OPT_2";
    case ENEMY_VOL_OPT_AMP:
        return "VOL_OPT_AMP";
    case ENEMY_VOL_OPT_CORE:
        return "VOL_OPT_CORE";
    case ENEMY_VOL_OPT_MONITOR:
        return "VOL_OPT_MONITOR";
    case ENEMY_VOL_OPT_PILLAR:
        return "VOL_OPT_PILLAR";
    case ENEMY_YOWIE:
        return "YOWIE";
    case ENEMY_YOWIE_ALT:
        return "YOWIE_ALT";
    case ENEMY_ZE_BOOTA:
        return "ZE_BOOTA";
    case ENEMY_ZOL_GIBBON:
        return "ZOL_GIBBON";
    case ENEMY_ZU:
        return "ZU";
    case ENEMY_ZU_ALT:
        return "ZU_ALT";
    default:
        ERR_LOG("无效敌人 %d", type);
        return "无效敌人";
    }
}

EnemyType name_for_enemy_type(const char* name) {
    static const EnemyTypeMapping names[] = {
      {"NONE", ENEMY_NONE},
      {"UNKNOWN", ENEMY_UNKNOWN},
      {"AL_RAPPY", ENEMY_AL_RAPPY},
      {"ASTARK", ENEMY_ASTARK},
      {"BA_BOOTA", ENEMY_BA_BOOTA},
      {"BARBA_RAY", ENEMY_BARBA_RAY},
      {"BARBAROUS_WOLF", ENEMY_BARBAROUS_WOLF},
      {"BEE_L", ENEMY_BEE_L},
      {"BEE_R", ENEMY_BEE_R},
      {"BOOMA", ENEMY_BOOMA},
      {"BOOTA", ENEMY_BOOTA},
      {"BULCLAW", ENEMY_BULCLAW},
      {"CANADINE", ENEMY_CANADINE},
      {"CANADINE_GROUP", ENEMY_CANADINE_GROUP},
      {"CANANE", ENEMY_CANANE},
      {"CHAOS_BRINGER", ENEMY_CHAOS_BRINGER},
      {"CHAOS_SORCERER", ENEMY_CHAOS_SORCERER},
      {"CLAW", ENEMY_CLAW},
      {"DARK_BELRA", ENEMY_DARK_BELRA},
      {"DARK_FALZ_1", ENEMY_DARK_FALZ_1},
      {"DARK_FALZ_2", ENEMY_DARK_FALZ_2},
      {"DARK_FALZ_3", ENEMY_DARK_FALZ_3},
      {"DARK_GUNNER", ENEMY_DARK_GUNNER},
      {"DARVANT", ENEMY_DARVANT},
      {"DARVANT_ULTIMATE", ENEMY_DARVANT_ULTIMATE},
      {"DE_ROL_LE", ENEMY_DE_ROL_LE},
      {"DE_ROL_LE_BODY", ENEMY_DE_ROL_LE_BODY},
      {"DE_ROL_LE_MINE", ENEMY_DE_ROL_LE_MINE},
      {"DEATH_GUNNER", ENEMY_DEATH_GUNNER},
      {"DEL_LILY", ENEMY_DEL_LILY},
      {"DEL_RAPPY", ENEMY_DEL_RAPPY},
      {"DEL_RAPPY_ALT", ENEMY_DEL_RAPPY_ALT},
      {"DELBITER", ENEMY_DELBITER},
      {"DELDEPTH", ENEMY_DELDEPTH},
      {"DELSABER", ENEMY_DELSABER},
      {"DIMENIAN", ENEMY_DIMENIAN},
      {"DOLMDARL", ENEMY_DOLMDARL},
      {"DOLMOLM", ENEMY_DOLMOLM},
      {"DORPHON", ENEMY_DORPHON},
      {"DORPHON_ECLAIR", ENEMY_DORPHON_ECLAIR},
      {"DRAGON", ENEMY_DRAGON},
      {"DUBCHIC", ENEMY_DUBCHIC},
      {"DUBWITCH", ENEMY_DUBWITCH},
      {"EGG_RAPPY", ENEMY_EGG_RAPPY},
      {"EPSIGUARD", ENEMY_EPSIGUARD},
      {"EPSILON", ENEMY_EPSILON},
      {"EVIL_SHARK", ENEMY_EVIL_SHARK},
      {"GAEL", ENEMY_GAEL},
      {"GAL_GRYPHON", ENEMY_GAL_GRYPHON},
      {"GARANZ", ENEMY_GARANZ},
      {"GEE", ENEMY_GEE},
      {"GI_GUE", ENEMY_GI_GUE},
      {"GIBBLES", ENEMY_GIBBLES},
      {"GIGOBOOMA", ENEMY_GIGOBOOMA},
      {"GILLCHIC", ENEMY_GILLCHIC},
      {"GIRTABLULU", ENEMY_GIRTABLULU},
      {"GOBOOMA", ENEMY_GOBOOMA},
      {"GOL_DRAGON", ENEMY_GOL_DRAGON},
      {"GORAN", ENEMY_GORAN},
      {"GORAN_DETONATOR", ENEMY_GORAN_DETONATOR},
      {"GRASS_ASSASSIN", ENEMY_GRASS_ASSASSIN},
      {"GUIL_SHARK", ENEMY_GUIL_SHARK},
      {"HALLO_RAPPY", ENEMY_HALLO_RAPPY},
      {"HIDOOM", ENEMY_HIDOOM},
      {"HILDEBEAR", ENEMY_HILDEBEAR},
      {"HILDEBLUE", ENEMY_HILDEBLUE},
      {"ILL_GILL", ENEMY_ILL_GILL},
      {"KONDRIEU", ENEMY_KONDRIEU},
      {"LA_DIMENIAN", ENEMY_LA_DIMENIAN},
      {"LOVE_RAPPY", ENEMY_LOVE_RAPPY},
      {"MERICAROL", ENEMY_MERICAROL},
      {"MERICUS", ENEMY_MERICUS},
      {"MERIKLE", ENEMY_MERIKLE},
      {"MERILLIA", ENEMY_MERILLIA},
      {"MERILTAS", ENEMY_MERILTAS},
      {"MERISSA_A", ENEMY_MERISSA_A},
      {"MERISSA_AA", ENEMY_MERISSA_AA},
      {"MIGIUM", ENEMY_MIGIUM},
      {"MONEST", ENEMY_MONEST},
      {"MORFOS", ENEMY_MORFOS},
      {"MOTHMANT", ENEMY_MOTHMANT},
      {"NANO_DRAGON", ENEMY_NANO_DRAGON},
      {"NAR_LILY", ENEMY_NAR_LILY},
      {"OLGA_FLOW_1", ENEMY_OLGA_FLOW_1},
      {"OLGA_FLOW_2", ENEMY_OLGA_FLOW_2},
      {"PAL_SHARK", ENEMY_PAL_SHARK},
      {"PAN_ARMS", ENEMY_PAN_ARMS},
      {"PAZUZU", ENEMY_PAZUZU},
      {"PAZUZU_ALT", ENEMY_PAZUZU_ALT},
      {"PIG_RAY", ENEMY_PIG_RAY},
      {"POFUILLY_SLIME", ENEMY_POFUILLY_SLIME},
      {"POUILLY_SLIME", ENEMY_POUILLY_SLIME},
      {"POISON_LILY", ENEMY_POISON_LILY},
      {"PYRO_GORAN", ENEMY_PYRO_GORAN},
      {"RAG_RAPPY", ENEMY_RAG_RAPPY},
      {"RECOBOX", ENEMY_RECOBOX},
      {"RECON", ENEMY_RECON},
      {"SAINT_MILLION", ENEMY_SAINT_MILLION},
      {"SAINT_RAPPY", ENEMY_SAINT_RAPPY},
      {"SAND_RAPPY", ENEMY_SAND_RAPPY},
      {"SAND_RAPPY_ALT", ENEMY_SAND_RAPPY_ALT},
      {"SATELLITE_LIZARD", ENEMY_SATELLITE_LIZARD},
      {"SATELLITE_LIZARD_ALT", ENEMY_SATELLITE_LIZARD_ALT},
      {"SAVAGE_WOLF", ENEMY_SAVAGE_WOLF},
      {"SHAMBERTIN", ENEMY_SHAMBERTIN},
      {"SINOW_BEAT", ENEMY_SINOW_BEAT},
      {"SINOW_BERILL", ENEMY_SINOW_BERILL},
      {"SINOW_GOLD", ENEMY_SINOW_GOLD},
      {"SINOW_SPIGELL", ENEMY_SINOW_SPIGELL},
      {"SINOW_ZELE", ENEMY_SINOW_ZELE},
      {"SINOW_ZOA", ENEMY_SINOW_ZOA},
      {"SO_DIMENIAN", ENEMY_SO_DIMENIAN},
      {"UL_GIBBON", ENEMY_UL_GIBBON},
      {"VOL_OPT_1", ENEMY_VOL_OPT_1},
      {"VOL_OPT_2", ENEMY_VOL_OPT_2},
      {"VOL_OPT_AMP", ENEMY_VOL_OPT_AMP},
      {"VOL_OPT_CORE", ENEMY_VOL_OPT_CORE},
      {"VOL_OPT_MONITOR", ENEMY_VOL_OPT_MONITOR},
      {"VOL_OPT_PILLAR", ENEMY_VOL_OPT_PILLAR},
      {"YOWIE", ENEMY_YOWIE},
      {"YOWIE_ALT", ENEMY_YOWIE_ALT},
      {"ZE_BOOTA", ENEMY_ZE_BOOTA},
      {"ZOL_GIBBON", ENEMY_ZOL_GIBBON},
      {"ZU", ENEMY_ZU},
      {"ZU_ALT", ENEMY_ZU_ALT},
    };

    int numNames = sizeof(names) / sizeof(names[0]);

    for (int i = 0; i < numNames; i++) {
        if (strcmp(names[i].name, name) == 0) {
            return names[i].type;
        }
    }

    ERR_LOG("Error: Unknown enemy type %s", name);
    return 0;
}

char* enemy_str(game_enemy_t* enemy) {
    snprintf(enemy_desc, sizeof(enemy_desc), "[Map::game_enemy %s clients_hit=%02hhX 最后击倒ID=%hu]",
        name_for_enum(enemy->bp_entry), enemy->clients_hit, enemy->last_client);
    return enemy_desc;
}

uint8_t rare_table_index_for_enemy_type(EnemyType enemy_type) {
    switch (enemy_type) {
    case ENEMY_AL_RAPPY:
        return 0x06;
    case ENEMY_ASTARK:
        return 0x01;
    case ENEMY_BA_BOOTA:
        return 0x0B;
    case ENEMY_BARBA_RAY:
        return 0x49;
    case ENEMY_BARBAROUS_WOLF:
        return 0x08;
    case ENEMY_BOOMA:
        return 0x09;
    case ENEMY_BOOTA:
        return 0x09;
    case ENEMY_BULCLAW:
        return 0x28;
    case ENEMY_CANADINE:
    case ENEMY_CANADINE_GROUP:
        return 0x03;
    case ENEMY_CANANE:
        return 0x07;
    case ENEMY_CHAOS_BRINGER:
        return 0x45;
    case ENEMY_CHAOS_SORCERER:
        return 0x46;
    case ENEMY_CLAW:
        return 0x44;
    case ENEMY_DARK_BELRA:
        return 0x2E;
    case ENEMY_DARK_FALZ_1:
        return 0x51;
    case ENEMY_DARK_FALZ_2:
        return 0x5A;
    case ENEMY_DARK_FALZ_3:
        return 0x60;
    case ENEMY_DARK_GUNNER:
        return 0x34;
    case ENEMY_DARVANT:
        return 0x0A;
    case ENEMY_DARVANT_ULTIMATE:
        return 0x29;
    case ENEMY_DE_ROL_LE:
        return 0x16;
    case ENEMY_DE_ROL_LE_BODY:
        return 0x16;
    case ENEMY_DE_ROL_LE_MINE:
        return 0x16;
    case ENEMY_DEATH_GUNNER:
        return 0x35;
    case ENEMY_DEL_LILY:
        return 0x3B;
    case ENEMY_DEL_RAPPY:
    case ENEMY_DEL_RAPPY_ALT:
        return 0x30;
    case ENEMY_DELBITER:
        return 0x31;
    case ENEMY_DELDEPTH:
        return 0x27;
    case ENEMY_DELSABER:
        return 0x32;
    case ENEMY_DIMENIAN:
        return 0x39;
    case ENEMY_DOLMDARL:
        return 0x25;
    case ENEMY_DOLMOLM:
        return 0x26;
    case ENEMY_DORPHON:
        return 0x2A;
    case ENEMY_DORPHON_ECLAIR:
        return 0x2A;
    case ENEMY_DRAGON:
        return 0x13;
    case ENEMY_DUBCHIC:
        return 0x36;
    case ENEMY_EGG_RAPPY:
        return 0x04;
    case ENEMY_EPSIGUARD:
        return 0x2F;
    case ENEMY_EPSILON:
        return 0x0C;
    case ENEMY_EVIL_SHARK:
        return 0x3C;
    case ENEMY_GAEL:
        return 0x37;
    case ENEMY_GAL_GRYPHON:
        return 0x21;
    case ENEMY_GARANZ:
        return 0x4D;
    case ENEMY_GEE:
        return 0x05;
    case ENEMY_GI_GUE:
        return 0x11;
    case ENEMY_GIBBLES:
        return 0x40;
    case ENEMY_GIGOBOOMA:
        return 0x0D;
    case ENEMY_GILLCHIC:
        return 0x38;
    case ENEMY_GIRTABLULU:
        return 0x23;
    case ENEMY_GOBOOMA:
        return 0x09;
    case ENEMY_GOL_DRAGON:
        return 0x15;
    case ENEMY_GORAN:
        return 0x1E;
    case ENEMY_GORAN_DETONATOR:
        return 0x1E;
    case ENEMY_GRASS_ASSASSIN:
        return 0x43;
    case ENEMY_GUIL_SHARK:
        return 0x3D;
    case ENEMY_HALLO_RAPPY:
        return 0x03;
    case ENEMY_HIDOOM:
        return 0x1D;
    case ENEMY_HILDEBEAR:
        return 0x19;
    case ENEMY_HILDEBLUE:
        return 0x1A;
    case ENEMY_ILL_GILL:
        return 0x42;
    case ENEMY_KONDRIEU:
        return 0x47;
    case ENEMY_LA_DIMENIAN:
        return 0x3A;
    case ENEMY_LOVE_RAPPY:
        return 0x02;
    case ENEMY_MERICAROL:
        return 0x48;
    case ENEMY_MERICUS:
        return 0x22;
    case ENEMY_MERIKLE:
        return 0x4C;
    case ENEMY_MERILLIA:
        return 0x4B;
    case ENEMY_MERILTAS:
        return 0x4A;
    case ENEMY_MERISSA_A:
        return 0x24;
    case ENEMY_MERISSA_AA:
        return 0x24;
    case ENEMY_MIGIUM:
        return 0x41;
    case ENEMY_MONEST:
        return 0x18;
    case ENEMY_MORFOS:
        return 0x12;
    case ENEMY_MOTHMANT:
        return 0x3F;
    case ENEMY_NANO_DRAGON:
        return 0x14;
    case ENEMY_NAR_LILY:
        return 0x3E;
    case ENEMY_OLGA_FLOW_1:
        return 0x54;
    case ENEMY_OLGA_FLOW_2:
        return 0x57;
    case ENEMY_PAL_SHARK:
        return 0x3D;
    case ENEMY_PAN_ARMS:
        return 0x50;
    case ENEMY_PAZUZU:
    case ENEMY_PAZUZU_ALT:
        return 0x20;
    case ENEMY_PIG_RAY:
        return 0x1C;
    case ENEMY_POFUILLY_SLIME:
        return 0x10;
    case ENEMY_POUILLY_SLIME:
        return 0x0F;
    case ENEMY_POISON_LILY:
        return 0x0E;
    case ENEMY_PYRO_GORAN:
        return 0x1B;
    case ENEMY_RAG_RAPPY:
        return 0x05;
    case ENEMY_RECOBOX:
        return 0x52;
    case ENEMY_RECON:
        return 0x33;
    case ENEMY_SAINT_MILLION:
        return 0x4E;
    case ENEMY_SAINT_RAPPY:
        return 0x07;
    case ENEMY_SAND_RAPPY:
    case ENEMY_SAND_RAPPY_ALT:
        return 0x01;
    case ENEMY_SATELLITE_LIZARD:
    case ENEMY_SATELLITE_LIZARD_ALT:
        return 0x17;
    case ENEMY_SAVAGE_WOLF:
        return 0x0A;
    case ENEMY_SHAMBERTIN:
        return 0x4F;
    case ENEMY_SINOW_BEAT:
        return 0x1F;
    case ENEMY_SINOW_BERILL:
        return 0x4;
    case ENEMY_SINOW_GOLD:
        return 0x1F;
    case ENEMY_SINOW_SPIGELL:
        return 0x4;
    case ENEMY_SINOW_ZELE:
        return 0x53;
    case ENEMY_SINOW_ZOA:
        return 0x10;
    case ENEMY_SO_DIMENIAN:
        return 0x39;
    case ENEMY_UL_GIBBON:
        return 0x52;
    case ENEMY_VOL_OPT_1:
        return 0x55;
    case ENEMY_VOL_OPT_2:
    case ENEMY_VOL_OPT_MONITOR:
        return 0x56;
    case ENEMY_VOL_OPT_AMP:
        return 0x5B;
    case ENEMY_VOL_OPT_CORE:
        return 0x59;
    case ENEMY_VOL_OPT_PILLAR:
        return 0x58;
    case ENEMY_YOWIE:
    case ENEMY_YOWIE_ALT:
        return 0x0C;
    case ENEMY_ZE_BOOTA:
        return 0x09;
    case ENEMY_ZOL_GIBBON:
        return 0x2B;
    case ENEMY_ZU:
    case ENEMY_ZU_ALT:
        return 0x0A;
    default:
        return 0xFF;
    }
}

bool enemy_type_valid_for_episode(Episode episode, EnemyType enemy_type) {
    switch (episode) {
    case GAME_TYPE_EPISODE_1:
        switch (enemy_type) {
        case ENEMY_MOTHMANT:
        case ENEMY_MONEST:
        case ENEMY_SAVAGE_WOLF:
        case ENEMY_BARBAROUS_WOLF:
        case ENEMY_POISON_LILY:
        case ENEMY_NAR_LILY:
        case ENEMY_SINOW_BEAT:
        case ENEMY_CANADINE:
        case ENEMY_CANADINE_GROUP:
        case ENEMY_CANANE:
        case ENEMY_CHAOS_SORCERER:
        case ENEMY_CHAOS_BRINGER:
        case ENEMY_DARK_BELRA:
        case ENEMY_DE_ROL_LE:
        case ENEMY_DRAGON:
        case ENEMY_SINOW_GOLD:
        case ENEMY_RAG_RAPPY:
        case ENEMY_AL_RAPPY:
        case ENEMY_NANO_DRAGON:
        case ENEMY_DUBCHIC:
        case ENEMY_GILLCHIC:
        case ENEMY_GARANZ:
        case ENEMY_DARK_GUNNER:
        case ENEMY_BULCLAW:
        case ENEMY_CLAW:
        case ENEMY_VOL_OPT_2:
        case ENEMY_POUILLY_SLIME:
        case ENEMY_POFUILLY_SLIME:
        case ENEMY_PAN_ARMS:
        case ENEMY_HIDOOM:
        case ENEMY_MIGIUM:
        case ENEMY_DARVANT:
        case ENEMY_DARVANT_ULTIMATE:
        case ENEMY_DARK_FALZ_1:
        case ENEMY_DARK_FALZ_2:
        case ENEMY_DARK_FALZ_3:
        case ENEMY_HILDEBEAR:
        case ENEMY_HILDEBLUE:
        case ENEMY_BOOMA:
        case ENEMY_GOBOOMA:
        case ENEMY_GIGOBOOMA:
        case ENEMY_GRASS_ASSASSIN:
        case ENEMY_EVIL_SHARK:
        case ENEMY_PAL_SHARK:
        case ENEMY_GUIL_SHARK:
        case ENEMY_DELSABER:
        case ENEMY_DIMENIAN:
        case ENEMY_LA_DIMENIAN:
        case ENEMY_SO_DIMENIAN:
            return true;
        default:
            return false;
        }
    case GAME_TYPE_EPISODE_2:
        switch (enemy_type) {
        case ENEMY_MOTHMANT:
        case ENEMY_MONEST:
        case ENEMY_SAVAGE_WOLF:
        case ENEMY_BARBAROUS_WOLF:
        case ENEMY_POISON_LILY:
        case ENEMY_NAR_LILY:
        case ENEMY_SINOW_BERILL:
        case ENEMY_GEE:
        case ENEMY_CHAOS_SORCERER:
        case ENEMY_DELBITER:
        case ENEMY_DARK_BELRA:
        case ENEMY_BARBA_RAY:
        case ENEMY_GOL_DRAGON:
        case ENEMY_SINOW_SPIGELL:
        case ENEMY_RAG_RAPPY:
        case ENEMY_LOVE_RAPPY:
        case ENEMY_SAINT_RAPPY:
        case ENEMY_EGG_RAPPY:
        case ENEMY_HALLO_RAPPY:
        case ENEMY_GI_GUE:
        case ENEMY_DUBCHIC:
        case ENEMY_GILLCHIC:
        case ENEMY_GARANZ:
        case ENEMY_GAL_GRYPHON:
        case ENEMY_EPSILON:
        case ENEMY_DEL_LILY:
        case ENEMY_ILL_GILL:
        case ENEMY_OLGA_FLOW_1:
        case ENEMY_OLGA_FLOW_2:
        case ENEMY_GAEL:
        case ENEMY_DELDEPTH:
        case ENEMY_PAN_ARMS:
        case ENEMY_HIDOOM:
        case ENEMY_MIGIUM:
        case ENEMY_MERICAROL:
        case ENEMY_UL_GIBBON:
        case ENEMY_ZOL_GIBBON:
        case ENEMY_GIBBLES:
        case ENEMY_MORFOS:
        case ENEMY_RECOBOX:
        case ENEMY_RECON:
        case ENEMY_SINOW_ZOA:
        case ENEMY_SINOW_ZELE:
        case ENEMY_MERIKLE:
        case ENEMY_MERICUS:
        case ENEMY_HILDEBEAR:
        case ENEMY_HILDEBLUE:
        case ENEMY_MERILLIA:
        case ENEMY_MERILTAS:
        case ENEMY_GRASS_ASSASSIN:
        case ENEMY_DOLMOLM:
        case ENEMY_DOLMDARL:
        case ENEMY_DELSABER:
        case ENEMY_DIMENIAN:
        case ENEMY_LA_DIMENIAN:
        case ENEMY_SO_DIMENIAN:
            return true;
        default:
            return false;
        }
    case GAME_TYPE_EPISODE_4:
        switch (enemy_type) {
        case ENEMY_BOOTA:
        case ENEMY_ZE_BOOTA:
        case ENEMY_BA_BOOTA:
        case ENEMY_SAND_RAPPY:
        case ENEMY_DEL_RAPPY:
        case ENEMY_ZU:
        case ENEMY_PAZUZU:
        case ENEMY_ASTARK:
        case ENEMY_SATELLITE_LIZARD:
        case ENEMY_YOWIE:
        case ENEMY_DORPHON:
        case ENEMY_DORPHON_ECLAIR:
        case ENEMY_GORAN:
        case ENEMY_PYRO_GORAN:
        case ENEMY_GORAN_DETONATOR:
        case ENEMY_SAND_RAPPY_ALT:
        case ENEMY_DEL_RAPPY_ALT:
        case ENEMY_MERISSA_A:
        case ENEMY_MERISSA_AA:
        case ENEMY_ZU_ALT:
        case ENEMY_PAZUZU_ALT:
        case ENEMY_SATELLITE_LIZARD_ALT:
        case ENEMY_YOWIE_ALT:
        case ENEMY_GIRTABLULU:
        case ENEMY_SAINT_MILLION:
        case ENEMY_SHAMBERTIN:
        case ENEMY_KONDRIEU:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

uint8_t battle_param_index_for_enemy_type(Episode episode, EnemyType enemy_type) {
    switch (episode) {
    case GAME_TYPE_EPISODE_1:
        switch (enemy_type) {
        case ENEMY_MOTHMANT:
            return 0x00;
        case ENEMY_MONEST:
            return 0x01;
        case ENEMY_SAVAGE_WOLF:
            return 0x02;
        case ENEMY_BARBAROUS_WOLF:
            return 0x03;
        case ENEMY_POISON_LILY:
            return 0x04;
        case ENEMY_NAR_LILY:
            return 0x05;
        case ENEMY_SINOW_BEAT:
            return 0x06;
        case ENEMY_CANADINE:
            return 0x07;
        case ENEMY_CANADINE_GROUP:
            return 0x08;
        case ENEMY_CANANE:
            return 0x09;
        case ENEMY_CHAOS_SORCERER:
            return 0x0A;
        case ENEMY_CHAOS_BRINGER:
            return 0x0D;
        case ENEMY_DARK_BELRA:
            return 0x0E;
        case ENEMY_DE_ROL_LE:
            return 0x0F;
        case ENEMY_DRAGON:
            return 0x12;
        case ENEMY_SINOW_GOLD:
            return 0x13;
        case ENEMY_RAG_RAPPY:
            return 0x18;
        case ENEMY_AL_RAPPY:
            return 0x19;
        case ENEMY_NANO_DRAGON:
            return 0x1A;
        case ENEMY_DUBCHIC:
            return 0x1B;
        case ENEMY_GILLCHIC:
            return 0x1C;
        case ENEMY_GARANZ:
            return 0x1D;
        case ENEMY_DARK_GUNNER:
            return 0x1E;
        case ENEMY_BULCLAW:
            return 0x1F;
        case ENEMY_CLAW:
            return 0x20;
        case ENEMY_VOL_OPT_2:
            return 0x25;
        case ENEMY_POUILLY_SLIME:
            return 0x2F;
        case ENEMY_POFUILLY_SLIME:
            return 0x30;
        case ENEMY_PAN_ARMS:
            return 0x31;
        case ENEMY_HIDOOM:
            return 0x32;
        case ENEMY_MIGIUM:
            return 0x33;
        case ENEMY_DARVANT:
            return 0x35;
        case ENEMY_DARVANT_ULTIMATE:
            return 0x39;
        case ENEMY_DARK_FALZ_1:
            return 0x36;
        case ENEMY_DARK_FALZ_2:
            return 0x37;
        case ENEMY_DARK_FALZ_3:
            return 0x38;
        case ENEMY_HILDEBEAR:
            return 0x49;
        case ENEMY_HILDEBLUE:
            return 0x4A;
        case ENEMY_BOOMA:
            return 0x4B;
        case ENEMY_GOBOOMA:
            return 0x4C;
        case ENEMY_GIGOBOOMA:
            return 0x4D;
        case ENEMY_GRASS_ASSASSIN:
            return 0x4E;
        case ENEMY_EVIL_SHARK:
            return 0x4F;
        case ENEMY_PAL_SHARK:
            return 0x50;
        case ENEMY_GUIL_SHARK:
            return 0x51;
        case ENEMY_DELSABER:
            return 0x52;
        case ENEMY_DIMENIAN:
            return 0x53;
        case ENEMY_LA_DIMENIAN:
            return 0x54;
        case ENEMY_SO_DIMENIAN:
            return 0x55;
        default:
            ERR_LOG(string_printf("%s does not have battle parameters in Episode 1", name_for_enum(enemy_type)));
            return 0xFF;
        }
        break;
    case GAME_TYPE_EPISODE_2:
        switch (enemy_type) {
        case ENEMY_MOTHMANT:
            return 0x00;
        case ENEMY_MONEST:
            return 0x01;
        case ENEMY_SAVAGE_WOLF:
            return 0x02;
        case ENEMY_BARBAROUS_WOLF:
            return 0x03;
        case ENEMY_POISON_LILY:
            return 0x04;
        case ENEMY_NAR_LILY:
            return 0x05;
        case ENEMY_SINOW_BERILL:
            return 0x06;
        case ENEMY_GEE:
            return 0x07;
        case ENEMY_CHAOS_SORCERER:
            return 0x0A;
        case ENEMY_DELBITER:
            return 0x0D;
        case ENEMY_DARK_BELRA:
            return 0x0E;
        case ENEMY_BARBA_RAY:
            return 0x0F;
        case ENEMY_GOL_DRAGON:
            return 0x12;
        case ENEMY_SINOW_SPIGELL:
            return 0x13;
        case ENEMY_RAG_RAPPY:
            return 0x18;
        case ENEMY_LOVE_RAPPY:
        case ENEMY_SAINT_RAPPY:
        case ENEMY_EGG_RAPPY:
        case ENEMY_HALLO_RAPPY:
            return 0x19;
        case ENEMY_GI_GUE:
            return 0x1A;
        case ENEMY_DUBCHIC:
            return 0x1B;
        case ENEMY_GILLCHIC:
            return 0x1C;
        case ENEMY_GARANZ:
            return 0x1D;
        case ENEMY_GAL_GRYPHON:
            return 0x1E;
        case ENEMY_EPSILON:
            return 0x23;
        case ENEMY_DEL_LILY:
            return 0x25;
        case ENEMY_ILL_GILL:
            return 0x26;
        case ENEMY_OLGA_FLOW_1:
            return 0x2B;
        case ENEMY_OLGA_FLOW_2:
            return 0x2C;
        case ENEMY_GAEL:
            return 0x2E;
        case ENEMY_DELDEPTH:
            return 0x30;
        case ENEMY_PAN_ARMS:
            return 0x31;
        case ENEMY_HIDOOM:
            return 0x32;
        case ENEMY_MIGIUM:
            return 0x33;
        case ENEMY_MERICAROL:
            return 0x3A;
        case ENEMY_UL_GIBBON:
            return 0x3B;
        case ENEMY_ZOL_GIBBON:
            return 0x3C;
        case ENEMY_GIBBLES:
            return 0x3D;
        case ENEMY_MORFOS:
            return 0x40;
        case ENEMY_RECOBOX:
            return 0x41;
        case ENEMY_RECON:
            return 0x42;
        case ENEMY_SINOW_ZOA:
            return 0x43;
        case ENEMY_SINOW_ZELE:
            return 0x44;
        case ENEMY_MERIKLE:
            return 0x45;
        case ENEMY_MERICUS:
            return 0x46;
        case ENEMY_HILDEBEAR:
            return 0x49;
        case ENEMY_HILDEBLUE:
            return 0x4A;
        case ENEMY_MERILLIA:
            return 0x4B;
        case ENEMY_MERILTAS:
            return 0x4C;
        case ENEMY_GRASS_ASSASSIN:
            return 0x4E;
        case ENEMY_DOLMOLM:
            return 0x4F;
        case ENEMY_DOLMDARL:
            return 0x50;
        case ENEMY_DELSABER:
            return 0x52;
        case ENEMY_DIMENIAN:
            return 0x53;
        case ENEMY_LA_DIMENIAN:
            return 0x54;
        case ENEMY_SO_DIMENIAN:
            return 0x55;
        default:
            ERR_LOG(string_printf("%s does not have battle parameters in Episode 2", name_for_enum(enemy_type)));
            return 0xFF;
        }
        break;
    case GAME_TYPE_EPISODE_4:
        switch (enemy_type) {
        case ENEMY_BOOTA:
            return 0x00;
        case ENEMY_ZE_BOOTA:
            return 0x01;
        case ENEMY_BA_BOOTA:
            return 0x03;
        case ENEMY_SAND_RAPPY:
            return 0x05;
        case ENEMY_DEL_RAPPY:
            return 0x06;
        case ENEMY_ZU:
            return 0x07;
        case ENEMY_PAZUZU:
            return 0x08;
        case ENEMY_ASTARK:
            return 0x09;
        case ENEMY_SATELLITE_LIZARD:
            return 0x0D;
        case ENEMY_YOWIE:
            return 0x0E;
        case ENEMY_DORPHON:
            return 0x0F;
        case ENEMY_DORPHON_ECLAIR:
            return 0x10;
        case ENEMY_GORAN:
            return 0x11;
        case ENEMY_PYRO_GORAN:
            return 0x12;
        case ENEMY_GORAN_DETONATOR:
            return 0x13;
        case ENEMY_SAND_RAPPY_ALT:
            return 0x17;
        case ENEMY_DEL_RAPPY_ALT:
            return 0x18;
        case ENEMY_MERISSA_A:
            return 0x19;
        case ENEMY_MERISSA_AA:
            return 0x1A;
        case ENEMY_ZU_ALT:
            return 0x1B;
        case ENEMY_PAZUZU_ALT:
            return 0x1C;
        case ENEMY_SATELLITE_LIZARD_ALT:
            return 0x1D;
        case ENEMY_YOWIE_ALT:
            return 0x1E;
        case ENEMY_GIRTABLULU:
            return 0x1F;
        case ENEMY_SAINT_MILLION:
        case ENEMY_SHAMBERTIN:
        case ENEMY_KONDRIEU:
            return 0x22;
        default:
            ERR_LOG(string_printf("%s does not have battle parameters in Episode 4", name_for_enum(enemy_type)));
            return 0xFF;
        }
        break;
    default:
        ERR_LOG("incorrect episode in battle param lookup");
        return 0xFF;
    }
    ERR_LOG("fallthrough case in battle param lookup");
    return 0xFF;
}

uint8_t fix_bp_entry_index() {

    return 0;
}

const char* get_lobby_mob_describe(lobby_t* l, uint8_t pt_index, uint8_t language) {
    /* PT表位是从NULL为0 客户端是从第一个怪物为0 少一位 */
    uint8_t fix_pt_index = pt_index;
    const char* enemy_name_cn = pt_index_raw_mobnames_cn[fix_pt_index];
    const char* enemy_name_en = pt_index_raw_mobnames[fix_pt_index];

    if (l->difficulty == 3) {
        enemy_name_cn = pt_index_raw_mobnames_ult_cn[fix_pt_index];
        enemy_name_en = pt_index_raw_mobnames_ult[fix_pt_index];
    }

    if (l->episode == 3) {
        fix_pt_index = pt_index + 1;

        if (l->difficulty == 3) {
            enemy_name_cn = pt_index_raw_mobnames_ep4_ult_cn[fix_pt_index];
            enemy_name_en = pt_index_raw_mobnames_ep4_ult[fix_pt_index];
        }
        else{
            enemy_name_cn = pt_index_raw_mobnames_ep4_cn[fix_pt_index];
            enemy_name_en = pt_index_raw_mobnames_ep4[fix_pt_index];
        }

    }

    switch (language) {
    case 0:
        return enemy_name_cn;

    default:
        return enemy_name_en;
    }
}

char* get_enemy_describe(lobby_t* l, uint8_t pt_index, const char* enemy_name_cn, const char* enemy_name_en, game_enemy_t* enemy) {
    memset(enemy_desc, 0, sizeof(enemy_desc));

    snprintf(enemy_desc, sizeof(enemy_desc), "怪物 %s (%s PT%d RT%d BP%d) 击杀:%s",
        enemy_name_cn, enemy_name_en, pt_index, enemy->rt_index, enemy->bp_entry/*, enemy->clients_hit*/, get_player_describe(l->clients[enemy->last_client]));

    return enemy_desc;
}

const char* get_lobby_enemy_pt_name_with_enemy(lobby_t* l, uint8_t pt_index, game_enemy_t* enemy) {
    /* 很奇怪 客户端返回的数值 缺少了表格中0表位 直接读取了1表位 太恶心了 */
    const char* enemy_name_cn = get_lobby_mob_describe(l, pt_index, 0);
    const char* enemy_name_en = get_lobby_mob_describe(l, pt_index, 1);

    return get_enemy_describe(l, pt_index, enemy_name_cn, enemy_name_en, enemy);
}

const char* get_lobby_enemy_pt_name_with_mid(lobby_t* l, uint8_t pt_index, uint16_t mid) {
    game_enemy_t* enemy = &l->map_enemies->enemies[mid];

    return get_lobby_enemy_pt_name_with_enemy(l, pt_index, enemy);
}