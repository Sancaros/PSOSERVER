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
#include <string.h>

#include <f_logs.h>

#include "items.h"

/* We need LE32 down below... so get it from packets.h */
#define PACKETS_H_HEADERS_ONLY
#include "packets.h"
#include "pmtdata.h"
#include "ptdata.h"
#include "rtdata.h"

/* ��Ʒ�������嵥. ���� PSOPC. */
static item_map_t item_list[] = {
    { Item_Meseta, "Meseta" },
    { Item_Saber, "Saber" },
    { Item_Brand, "Brand" },
    { Item_Buster, "Buster" },
    { Item_Pallasch, "Pallasch" },
    { Item_Gladius, "Gladius" },
    { Item_DBS_SABER, "DB'S SABER" },
    { Item_KALADBOLG, "KALADBOLG" },
    { Item_DURANDAL, "DURANDAL" },
    { Item_Sword, "Sword" },
    { Item_Gigush, "Gigush" },
    { Item_Breaker, "Breaker" },
    { Item_Claymore, "Claymore" },
    { Item_Calibur, "Calibur" },
    { Item_FLOWENS_SWORD, "FLOWEN'S SWORD" },
    { Item_LAST_SURVIVOR, "LAST SURVIVOR" },
    { Item_DRAGON_SLAYER, "DRAGON SLAYER" },
    { Item_Dagger, "Dagger" },
    { Item_Knife, "Knife" },
    { Item_Blade, "Blade" },
    { Item_Edge, "Edge" },
    { Item_Ripper, "Ripper" },
    { Item_BLADE_DANCE, "BLADE DANCE" },
    { Item_BLOODY_ART, "BLOODY ART" },
    { Item_CROSS_SCAR, "CROSS SCAR" },
    { Item_Partisan, "Partisan" },
    { Item_Halbert, "Halbert" },
    { Item_Glaive, "Glaive" },
    { Item_Berdys, "Berdys" },
    { Item_Gungnir, "Gungnir" },
    { Item_BRIONAC, "BRIONAC" },
    { Item_VJAYA, "VJAYA" },
    { Item_GAE_BOLG, "GAE BOLG" },
    { Item_Slicer, "Slicer" },
    { Item_Spinner, "Spinner" },
    { Item_Cutter, "Cutter" },
    { Item_Sawcer, "Sawcer" },
    { Item_Diska, "Diska" },
    { Item_SLICER_OF_ASSASSIN, "SLICER OF ASSASSIN" },
    { Item_DISKA_OF_LIBERATOR, "DISKA OF LIBERATOR" },
    { Item_DISKA_OF_BRAVEMAN, "DISKA OF BRAVEMAN" },
    { Item_Handgun, "Handgun" },
    { Item_Autogun, "Autogun" },
    { Item_Lockgun, "Lockgun" },
    { Item_Railgun, "Railgun" },
    { Item_Raygun, "Raygun" },
    { Item_VARISTA, "VARISTA" },
    { Item_CUSTOM_RAY_ver_OO, "CUSTOM RAY ver.OO" },
    { Item_BRAVACE, "BRAVACE" },
    { Item_Rifle, "Rifle" },
    { Item_Sniper, "Sniper" },
    { Item_Blaster, "Blaster" },
    { Item_Beam, "Beam" },
    { Item_Laser, "Laser" },
    { Item_VISK_235W, "VISK-235W" },
    { Item_WALS_MK2, "WALS-MK2" },
    { Item_JUSTY_23ST, "JUSTY-23ST" },
    { Item_Mechgun, "Mechgun" },
    { Item_Assault, "Assault" },
    { Item_Repeater, "Repeater" },
    { Item_Gatling, "Gatling" },
    { Item_Vulcan, "Vulcan" },
    { Item_M_and_A60_VISE, "M&A60 VISE" },
    { Item_H_and_S25_JUSTICE, "H&S25 JUSTICE" },
    { Item_L_and_K14_COMBAT, "L&K14 COMBAT" },
    { Item_Shot, "Shot" },
    { Item_Spread, "Spread" },
    { Item_Cannon, "Cannon" },
    { Item_Launcher, "Launcher" },
    { Item_Arms, "Arms" },
    { Item_CRUSH_BULLET, "CRUSH BULLET" },
    { Item_METEOR_SMASH, "METEOR SMASH" },
    { Item_FINAL_IMPACT, "FINAL IMPACT" },
    { Item_Cane, "Cane" },
    { Item_Stick, "Stick" },
    { Item_Mace, "Mace" },
    { Item_Club, "Club" },
    { Item_CLUB_OF_LACONIUM, "CLUB OF LACONIUM" },
    { Item_MACE_OF_ADAMAN, "MACE OF ADAMAN" },
    { Item_CLUB_OF_ZUMIURAN, "CLUB OF ZUMIURAN" },
    { Item_Rod, "Rod" },
    { Item_Pole, "Pole" },
    { Item_Pillar, "Pillar" },
    { Item_Striker, "Striker" },
    { Item_BATTLE_VERGE, "BATTLE VERGE" },
    { Item_BRAVE_HAMMER, "BRAVE HAMMER" },
    { Item_ALIVE_AQHU, "ALIVE AQHU" },
    { Item_Wand, "Wand" },
    { Item_Staff, "Staff" },
    { Item_Baton, "Baton" },
    { Item_Scepter, "Scepter" },
    { Item_FIRE_SCEPTER_AGNI, "FIRE SCEPTER:AGNI" },
    { Item_ICE_STAFF_DAGON, "ICE STAFF:DAGON" },
    { Item_STORM_WAND_INDRA, "STORM WAND:INDRA" },
    { Item_PHOTON_CLAW, "PHOTON CLAW" },
    { Item_SILENCE_CLAW, "SILENCE CLAW" },
    { Item_NEIS_CLAW, "NEI'S CLAW" },
    { Item_DOUBLE_SABER, "DOUBLE SABER" },
    { Item_STAG_CUTLERY, "STAG CUTLERY" },
    { Item_TWIN_BRAND, "TWIN BRAND" },
    { Item_BRAVE_KNUCKLE, "BRAVE KNUCKLE" },
    { Item_ANGRY_FIST, "ANGRY FIST" },
    { Item_GOD_HAND, "GOD HAND" },
    { Item_SONIC_KNUCKLE, "SONIC KNUCKLE" },
    { Item_OROTIAGITO_alt, "OROTIAGITO (alt)" },
    { Item_OROTIAGITO, "OROTIAGITO" },
    { Item_AGITO_1975, "AGITO (AUW 1975)" },
    { Item_AGITO_1983, "AGITO (AUW 1983)" },
    { Item_AGITO_2001, "AGITO (AUW 2001)" },
    { Item_AGITO_1991, "AGITO (AUW 1991)" },
    { Item_AGITO_1977, "AGITO (AUW 1977)" },
    { Item_AGITO_1980, "AGITO (AUW 1980)" },
    { Item_SOUL_EATER, "SOUL EATER" },
    { Item_SOUL_BANISH, "SOUL BANISH" },
    { Item_SPREAD_NEEDLE, "SPREAD NEEDLE" },
    { Item_HOLY_RAY, "HOLY RAY" },
    { Item_INFERNO_BAZOOKA, "INFERNO BAZOOKA" },
    { Item_FLAME_VISIT, "FLAME VISIT" },
    { Item_AKIKOS_FRYING_PAN, "AKIKO'S FRYING PAN" },
    { Item_C_SORCERERS_CANE, "C-SORCERER'S CANE" },
    { Item_S_BEATS_BLADE, "S-BEAT'S BLADE" },
    { Item_P_ARMSS_BLADE, "P-ARMS'S BLADE" },
    { Item_DELSABERS_BUSTER, "DELSABER'S BUSTER" },
    { Item_C_BRINGERS_RIFLE, "C-BRINGER'S RIFLE" },
    { Item_EGG_BLASTER, "EGG BLASTER" },
    { Item_PSYCHO_WAND, "PSYCHO WAND" },
    { Item_HEAVEN_PUNISHER, "HEAVEN PUNISHER" },
    { Item_LAVIS_CANNON, "LAVIS CANNON" },
    { Item_VICTOR_AXE, "VICTOR AXE" },
    { Item_CHAIN_SAWD, "CHAIN SAWD" },
    { Item_CADUCEUS, "CADUCEUS" },
    { Item_STING_TIP, "STING TIP" },
    { Item_MAGICAL_PIECE, "MAGICAL PIECE" },
    { Item_TECHNICAL_CROZIER, "TECHNICAL CROZIER" },
    { Item_SUPPRESSED_GUN, "SUPPRESSED GUN" },
    { Item_ANCIENT_SABER, "ANCIENT SABER" },
    { Item_HARISEN_BATTLE_FAN, "HARISEN BATTLE FAN" },
    { Item_YAMIGARASU, "YAMIGARASU" },
    { Item_AKIKOS_WOK, "AKIKO'S WOK" },
    { Item_TOY_HAMMER, "TOY HAMMER" },
    { Item_ELYSION, "ELYSION" },
    { Item_RED_SABER, "RED SABER" },
    { Item_METEOR_CUDGEL, "METEOR CUDGEL" },
    { Item_MONKEY_KING_BAR, "MONKEY KING BAR" },
    { Item_DOUBLE_CANNON, "DOUBLE CANNON" },
    { Item_HUGE_BATTLE_FAN, "HUGE BATTLE FAN" },
    { Item_TSUMIKIRI_J_SWORD, "TSUMIKIRI J-SWORD" },
    { Item_SEALED_J_SWORD, "SEALED J-SWORD" },
    { Item_RED_SWORD, "RED SWORD" },
    { Item_CRAZY_TUNE, "CRAZY TUNE" },
    { Item_TWIN_CHAKRAM, "TWIN CHAKRAM" },
    { Item_WOK_OF_AKIKOS_SHOP, "WOK OF AKIKO'S SHOP" },
    { Item_LAVIS_BLADE, "LAVIS BLADE" },
    { Item_RED_DAGGER, "RED DAGGER" },
    { Item_MADAMS_PARASOL, "MADAM'S PARASOL" },
    { Item_MADAMS_UMBRELLA, "MADAM'S UMBRELLA" },
    { Item_IMPERIAL_PICK, "IMPERIAL PICK" },
    { Item_BERDYSH, "BERDYSH" },
    { Item_RED_PARTISAN, "RED PARTISAN" },
    { Item_FLIGHT_CUTTER, "FLIGHT CUTTER" },
    { Item_FLIGHT_FAN, "FLIGHT FAN" },
    { Item_RED_SLICER, "RED SLICER" },
    { Item_HANDGUN_GULD, "HANDGUN:GULD" },
    { Item_HANDGUN_MILLA, "HANDGUN:MILLA" },
    { Item_RED_HANDGUN, "RED HANDGUN" },
    { Item_FROZEN_SHOOTER, "FROZEN SHOOTER" },
    { Item_ANTI_ANDROID_RIFLE, "ANTI ANDROID RIFLE" },
    { Item_ROCKET_PUNCH, "ROCKET PUNCH" },
    { Item_SAMBA_MARACAS, "SAMBA MARACAS" },
    { Item_TWIN_PSYCHOGUN, "TWIN PSYCHOGUN" },
    { Item_DRILL_LAUNCHER, "DRILL LAUNCHER" },
    { Item_GULD_MILLA, "GULD MILLA" },
    { Item_RED_MECHGUN, "RED MECHGUN" },
    { Item_BERLA_CANNON, "BERLA CANNON" },
    { Item_PANZER_FAUST, "PANZER FAUST" },
    { Item_SUMMIT_MOON, "SUMMIT MOON" },
    { Item_WINDMILL, "WINDMILL" },
    { Item_EVIL_CURST, "EVIL CURST" },
    { Item_FLOWER_CANE, "FLOWER CANE" },
    { Item_HILDEBEARS_CANE, "HILDEBEAR'S CANE" },
    { Item_HILDEBLUES_CANE, "HILDEBLUE'S CANE" },
    { Item_RABBIT_WAND, "RABBIT WAND" },
    { Item_PLANTAIN_LEAF, "PLANTAIN LEAF" },
    { Item_DEMONIC_FORK, "DEMONIC FORK" },
    { Item_STRIKER_OF_CHAO, "STIRKER OF CHAO" },
    { Item_BROOM, "BROOM" },
    { Item_PROPHETS_OF_MOTAV, "PROPHETS OF MOTAV" },
    { Item_THE_SIGH_OF_A_GOD, "THE SIGH OF A GOD" },
    { Item_TWINKLE_STAR, "TWINKLE STAR" },
    { Item_PLANTAIN_FAN, "PLANTAIN FAN" },
    { Item_TWIN_BLAZE, "TWIN BLAZE" },
    { Item_MARINAS_BAG, "MARINA'S BAG" },
    { Item_DRAGONS_CLAW, "DRAGON'S CLAW" },
    { Item_PANTHERS_CLAW, "PANTHER'S CLAW" },
    { Item_S_REDS_BLADE, "S-RED'S BLADE" },
    { Item_PLANTAIN_HUGE_FAN, "PLANTAIN HUGE FAN" },
    { Item_CHAMELEON_SCYTHE, "CHAMELEON SCYTHE" },
    { Item_YASMINKOV_3000R, "YASMINKOV 3000R" },
    { Item_ANO_RIFLE, "ANO RIFLE" },
    { Item_BARANZ_LAUNCHER, "BARANZ LAUNCHER" },
    { Item_BRANCH_OF_PAKUPAKU, "BRANCH OF PAKUPAKU" },
    { Item_HEART_OF_POUMN, "HEART OF POUMN" },
    { Item_YASMINKOV_2000H, "YASMINKOV 2000H" },
    { Item_YASMINKOV_7000V, "YASMINKOV 7000V" },
    { Item_YASMINKOV_9200M, "YASMINKOV 9200M" },
    { Item_MASER_BEAM, "MASER BEAM" },
    { Item_GAME_MAGAZNE, "GAME MAGAZNE" },
    { Item_FLOWER_BOUQUET, "FLOWER BOUQUET" },
    { Item_SRANK_SABER, "SABER" },
    { Item_SRANK_SWORD, "SWORD" },
    { Item_SRANK_BLADE, "BLADE" },
    { Item_SRANK_PARTISAN, "PARTISAN" },
    { Item_SRANK_SLICER, "SLICER" },
    { Item_SRANK_GUN, "GUN" },
    { Item_SRANK_RIFLE, "RIFLE" },
    { Item_SRANK_MECHGUN, "MECHGUN" },
    { Item_SRANK_SHOT, "SHOT" },
    { Item_SRANK_CANE, "CANE" },
    { Item_SRANK_ROD, "ROD" },
    { Item_SRANK_WAND, "WAND" },
    { Item_SRANK_TWIN, "TWIN" },
    { Item_SRANK_CLAW, "CLAW" },
    { Item_SRANK_BAZOOKA, "BAZOOKA" },
    { Item_SRANK_NEEDLE, "NEEDLE" },
    { Item_SRANK_SCYTHE, "SCYTHE" },
    { Item_SRANK_HAMMER, "HAMMER" },
    { Item_SRANK_MOON, "MOON" },
    { Item_SRANK_PSYCHOGUN, "PSYCHOGUN" },
    { Item_SRANK_PUNCH, "PUNCH" },
    { Item_SRANK_WINDMILL, "WINDMILL" },
    { Item_SRANK_HARISEN, "HARISEN" },
    { Item_SRANK_J_BLADE, "J-BLADE" },
    { Item_SRANK_J_CUTTER, "J-CUTTER" },
    { Item_Frame, "Frame" },
    { Item_Armor, "Armor" },
    { Item_Psy_Armor, "Psy Armor" },
    { Item_Giga_Frame, "Giga Frame" },
    { Item_Soul_Frame, "Soul Frame" },
    { Item_Cross_Armor, "Cross Armor" },
    { Item_Solid_Frame, "Solid Frame" },
    { Item_Brave_Armor, "Brace Armor" },
    { Item_Hyper_Frame, "Hyper Frame" },
    { Item_Grand_Armor, "Grand Armor" },
    { Item_Shock_Frame, "Shock Frame" },
    { Item_Kings_Frame, "King's Frame" },
    { Item_Dragon_Frame, "Dragon Frame" },
    { Item_Absorb_Armor, "Absorb Armor" },
    { Item_Protect_Frame, "Protect Frame" },
    { Item_General_Armor, "General Armor" },
    { Item_Perfect_Frame, "Perfect Frame" },
    { Item_Valiant_Frame, "Valiant Frame" },
    { Item_Imperial_Armor, "Imperial Armor" },
    { Item_Holiness_Armor, "Holiness Armor" },
    { Item_Guardian_Armor, "Guardian Armor" },
    { Item_Divinity_Armor, "Divinity Armor" },
    { Item_Ultimate_Frame, "Ultimate Frame" },
    { Item_Celestial_Armor, "Celestial Armor" },
    { Item_HUNTER_FIELD, "HUNTER FIELD" },
    { Item_RANGER_FIELD, "RANGER FIELD" },
    { Item_FORCE_FIELD, "FORCE FIELD" },
    { Item_REVIVAL_GARMENT, "REVIVAL GARMENT" },
    { Item_SPIRIT_GARMENT, "SPIRIT GARMENT" },
    { Item_STINK_FRAME, "STINK FRAME" },
    { Item_D_PARTS_ver1_01, "D-PARTS ver1.01" },
    { Item_D_PARTS_ver2_10, "D-PARTS ver2.10" },
    { Item_PARASITE_WEAR_De_Rol, "PARASITE WEAR:De Rol" },
    { Item_PARASITE_WEAR_Nelgal, "PARASITE WEAR:Nelgal" },
    { Item_PARASITE_WEAR_Vajulla, "PARASITE WEAR:Vajulla" },
    { Item_SENSE_PLATE, "SENSE PLATE" },
    { Item_GRAVITON_PLATE, "GRAVITON PLATE" },
    { Item_ATTRIBUTE_PLATE, "ATTRIBUTE PLATE" },
    { Item_FLOWENS_FRAME, "FLOWEN'S FRAME" },
    { Item_CUSTOM_FRAME_ver_OO, "CUSTOM FRAME ver.OO" },
    { Item_DBS_ARMOR, "DB'S ARMOR" },
    { Item_GUARD_WAVE, "GUARD WAVE" },
    { Item_DF_FIELD, "DF FIELD" },
    { Item_LUMINOUS_FIELD, "LUMINOUS FIELD" },
    { Item_CHU_CHU_FEVER, "CHU CHU FEVER" },
    { Item_LOVE_HEART, "LOVE HEART" },
    { Item_FLAME_GARMENT, "FLAME GARMENT" },
    { Item_VIRUS_ARMOR_Lafuteria, "VIRUS ARMOR:Lafuteria" },
    { Item_BRIGHTNESS_CIRCLE, "BRIGHTNESS CIRCLE" },
    { Item_AURA_FIELD, "AURA FIELD" },
    { Item_ELECTRO_FRAME, "ELECTRO FRAME" },
    { Item_SACRED_CLOTH, "SACRED CLOTH" },
    { Item_SMOKING_PLATE, "SMOKING PLATE" },
    { Item_Barrier, "Barrier" },
    { Item_Shield, "Shield" },
    { Item_Core_Shield, "Core Shield" },
    { Item_Giga_Shield, "Giga Shield" },
    { Item_Soul_Barrier, "Soul Barrier" },
    { Item_Hard_Shield, "Hard Shield" },
    { Item_Brave_Barrier, "Brave Barrier" },
    { Item_Solid_Shield, "Solid Shield" },
    { Item_Flame_Barrier, "Flame Barrier" },
    { Item_Plasma_Barrier, "Plasma Barrier" },
    { Item_Freeze_Barrier, "Freeze Barrier" },
    { Item_Psychic_Barrier, "Psychic Barrier" },
    { Item_General_Shield, "General Shield" },
    { Item_Protect_Barrier, "Protect Barrier" },
    { Item_Glorious_Shield, "Glorious Shield" },
    { Item_Imperial_Barrier, "Imperial Barrier" },
    { Item_Guardian_Shield, "Guardian Shield" },
    { Item_Divinity_Barrier, "Divinity Barrier" },
    { Item_Ultimate_Shield, "Ultimate Shield" },
    { Item_Spiritual_Shield, "Spiritual Shield" },
    { Item_Celestial_Shield, "Celestial Shield" },
    { Item_INVISIBLE_GUARD, "INVISIBLE GUARD" },
    { Item_SACRED_GUARD, "SACRED GUARD" },
    { Item_S_PARTS_ver1_16, "S-PARTS ver1.16" },
    { Item_S_PARTS_ver2_01, "S-PARTS ver2.01" },
    { Item_LIGHT_RELIEF, "LIGHT RELIEF" },
    { Item_SHIELD_OF_DELSABER, "SHIELD OF DELSABER" },
    { Item_FORCE_WALL, "FORCE WALL" },
    { Item_RANGER_WALL, "RANGER WALL" },
    { Item_HUNTER_WALL, "HUNTER WALL" },
    { Item_ATTRIBUTE_WALL, "ATTRIBUTE WALL" },
    { Item_SECRET_GEAR, "SECRET GEAR" },
    { Item_COMBAT_GEAR, "COMBAT GEAR" },
    { Item_PROTO_REGENE_GEAR, "PROTO REGENE GEAR" },
    { Item_REGENERATE_GEAR, "REGENERATE GEAR" },
    { Item_REGENE_GEAR_ADV, "REGENE GEAR ADV" },
    { Item_FLOWENS_SHIELD, "FLOWEN'S SHIELD" },
    { Item_CUSTOM_BARRIER_ver_OO, "CUSTOM BARRIER ver.OO" },
    { Item_DBS_SHIELD, "DB'S SHIELD" },
    { Item_RED_RING, "RED RING" },
    { Item_TRIPOLIC_SHIELD, "TRIPOLIC SHIELD" },
    { Item_STANDSTILL_SHIELD, "STANDSTILL SHIELD" },
    { Item_SAFETY_HEART, "SAFETY HEART" },
    { Item_KASAMI_BRACER, "KASAMI BRACER" },
    { Item_GODS_SHIELD_SUZAKU, "GODS SHIELD SUZAKU" },
    { Item_GODS_SHIELD_GENBU, "GODS SHIELD GENBU" },
    { Item_GODS_SHIELD_BYAKKO, "GODS SHIELD BYAKKO" },
    { Item_GODS_SHIELD_SEIRYU, "GODS SHIELD SEIRYU" },
    { Item_HANTERS_SHELL, "HANTER'S SHELL" },
    { Item_RIKOS_GLASSES, "RIKO'S GLASSES" },
    { Item_RIKOS_EARRING, "RIKO'S EARRING" },
    { Item_BLUE_RING, "BLUE RING" },
    { Item_YELLOW_RING, "YELLOW RING" },
    { Item_SECURE_FEET, "SECURE FEET" },
    { Item_PURPLE_RING, "PURPLE RING" },
    { Item_GREEN_RING, "GREEN RING" },
    { Item_BLACK_RING, "BLACK RING" },
    { Item_WHITE_RING, "WHITE RING" },
    { Item_Knight_Power, "Knight/Power" },
    { Item_General_Power, "General/Power" },
    { Item_Ogre_Power, "Ogre/Power" },
    { Item_God_Power, "God/Power" },
    { Item_Priest_Mind, "Priest/Mind" },
    { Item_General_Mind, "General/Mind" },
    { Item_Angel_Mind, "Angel/Mind" },
    { Item_God_Mind, "God/Mind" },
    { Item_Marksman_Arm, "Marksman/Arm" },
    { Item_General_Arm, "General/Arm" },
    { Item_Elf_Arm, "Elf/Arm" },
    { Item_God_Arm, "God/Arm" },
    { Item_Thief_Legs, "Thief/Legs" },
    { Item_General_Legs, "General/Legs" },
    { Item_Elf_Legs, "Elf/Legs" },
    { Item_God_Legs, "God/Legs" },
    { Item_Digger_HP, "Digger/HP" },
    { Item_General_HP, "General/HP" },
    { Item_Dragon_HP, "Dragon/HP" },
    { Item_God_HP, "God/HP" },
    { Item_Magician_TP, "Magician/TP" },
    { Item_General_TP, "General/TP" },
    { Item_Angel_TP, "Angel/TP" },
    { Item_God_TP, "God/TP" },
    { Item_Warrior_Body, "Warrior/Body" },
    { Item_General_Body, "General/Body" },
    { Item_Metal_Body, "Metal/Body" },
    { Item_God_Body, "God/Body", },
    { Item_Angel_Luck, "Angel/Luck" },
    { Item_God_Luck, "God/Luck" },
    { Item_Master_Ability, "Master/Ability" },
    { Item_Hero_Ability, "Hero/Ability" },
    { Item_God_Ability, "God/Ability" },
    { Item_Resist_Fire, "Resist/Fire" },
    { Item_Resist_Flame, "Resist/Flame" },
    { Item_Resist_Burning, "Resist/Burning" },
    { Item_Resist_Cold, "Resist/Cold" },
    { Item_Resist_Freeze, "Resist/Freeze" },
    { Item_Resist_Blizzard, "Resist/Blizzard" },
    { Item_Resist_Shock, "Resist/Shock" },
    { Item_Resist_Thunder, "Resist/Thunder" },
    { Item_Resist_Storm, "Resist/Storm" },
    { Item_Resist_Light, "Resist/Light" },
    { Item_Resist_Saint, "Resist/Saint" },
    { Item_Resist_Holy, "Resist/Holy" },
    { Item_Resist_Dark, "Resist/Dark" },
    { Item_Resist_Evil, "Resist/Evil" },
    { Item_Resist_Devil, "Resist/Devil" },
    { Item_All_Resist, "All/Resist" },
    { Item_Super_Resist, "Super/Resist" },
    { Item_Perfect_Resist, "Perfect/Resist" },
    { Item_HP_Restorate, "HP/Restorate" },
    { Item_HP_Generate, "HP/Generate" },
    { Item_HP_Revival, "HP/Revival" },
    { Item_TP_Restorate, "TP/Restorate" },
    { Item_TP_Generate, "TP/Generate" },
    { Item_TP_Revival, "TP/Revival" },
    { Item_PB_Amplifier, "PB/Amplifier" },
    { Item_PB_Generate, "PB/Generate" },
    { Item_PB_Create, "PB/Create" },
    { Item_Wizard_Technique, "Wizard/Technique" },
    { Item_Devil_Technique, "Devil/Technique" },
    { Item_God_Technique, "God/Technique" },
    { Item_General_Battle, "General/Battle" },
    { Item_Devil_Battle, "Devil/Battle" },
    { Item_God_Battle, "God/Battle" },
    { Item_State_Maintenance, "State/Maintenance" },
    { Item_Trap_Search, "Trap/Search" },
    { Item_Mag, "Mag" },
    { Item_Varuna, "Varuna" },
    { Item_Mitra, "Mitra" },
    { Item_Surya, "Surya" },
    { Item_Vayu, "Vayu" },
    { Item_Varaha, "Varaha" },
    { Item_Kama, "Kama" },
    { Item_Ushasu, "Ushasu" },
    { Item_Apsaras, "Apsaras" },
    { Item_Kumara, "Kumara" },
    { Item_Kaitabha, "Kaitabha" },
    { Item_Tapas, "Tapas" },
    { Item_Bhirava, "Bhirava" },
    { Item_Kalki, "Kalki" },
    { Item_Rudra, "Rudra" },
    { Item_Marutah, "Marutah" },
    { Item_Yaksa, "Yaksa" },
    { Item_Sita, "Sita" },
    { Item_Garuda, "Garuda" },
    { Item_Nandin, "Nandin" },
    { Item_Ashvinau, "Ashvinau" },
    { Item_Ribhava, "Ribhava" },
    { Item_Soma, "Soma" },
    { Item_Ila, "Ila" },
    { Item_Durga, "Durga" },
    { Item_Vritra, "Vritra" },
    { Item_Namuci, "Namuci" },
    { Item_Sumba, "Sumba" },
    { Item_Naga, "Naga" },
    { Item_Pitri, "Pitri" },
    { Item_Kabanda, "Kabanda" },
    { Item_Ravana, "Ravana" },
    { Item_Marica, "Marica" },
    { Item_Soniti, "Soniti" },
    { Item_Preta, "Preta" },
    { Item_Andhaka, "Andhaka" },
    { Item_Bana, "Bana" },
    { Item_Naraka, "Naraka" },
    { Item_Madhu, "Madhu" },
    { Item_Churel, "Churel" },
    { Item_ROBOCHAO, "ROBOCHAO" },
    { Item_OPA_OPA, "OPA-OPA" },
    { Item_PIAN, "PIAN" },
    { Item_CHAO, "CHAO" },
    { Item_CHU_CHU, "CHU CHU" },
    { Item_KAPU_KAPU, "KAPU KAPU" },
    { Item_ANGELS_WING, "ANGEL'S WING" },
    { Item_DEVILS_WING, "DEVIL'S WING" },
    { Item_ELENOR, "ELENOR" },
    { Item_MARK3, "MARK3" },
    { Item_MASTER_SYSTEM, "MASTER SYSTEM" },
    { Item_GENESIS, "GENESIS" },
    { Item_SEGA_SATURN, "SEGA SATURN" },
    { Item_DREAMCAST, "DREAMCAST" },
    { Item_HAMBURGER, "HAMBURGER" },
    { Item_PANZERS_TAIL, "PANZER'S TAIL" },
    { Item_DAVILS_TAIL, "DAVIL'S TAIL" },
    { Item_Monomate, "Monomate" },
    { Item_Dimate, "Dimate" },
    { Item_Trimate, "Trimate" },
    { Item_Monofluid, "Monofluid" },
    { Item_Difluid, "Difluid" },
    { Item_Trifluid, "Trifluid" },
    { Item_Disk_Lv01, "Disk:Lv.1" },
    { Item_Disk_Lv02, "Disk:Lv.2" },
    { Item_Disk_Lv03, "Disk:Lv.3" },
    { Item_Disk_Lv04, "Disk:Lv.4" },
    { Item_Disk_Lv05, "Disk:Lv.5" },
    { Item_Disk_Lv06, "Disk:Lv.6" },
    { Item_Disk_Lv07, "Disk:Lv.7" },
    { Item_Disk_Lv08, "Disk:Lv.8" },
    { Item_Disk_Lv09, "Disk:Lv.9" },
    { Item_Disk_Lv10, "Disk:Lv.10" },
    { Item_Disk_Lv11, "Disk:Lv.11" },
    { Item_Disk_Lv12, "Disk:Lv.12" },
    { Item_Disk_Lv13, "Disk:Lv.13" },
    { Item_Disk_Lv14, "Disk:Lv.14" },
    { Item_Disk_Lv15, "Disk:Lv.15" },
    { Item_Disk_Lv16, "Disk:Lv.16" },
    { Item_Disk_Lv17, "Disk:Lv.17" },
    { Item_Disk_Lv18, "Disk:Lv.18" },
    { Item_Disk_Lv19, "Disk:Lv.19" },
    { Item_Disk_Lv20, "Disk:Lv.20" },
    { Item_Disk_Lv21, "Disk:Lv.21" },
    { Item_Disk_Lv22, "Disk:Lv.22" },
    { Item_Disk_Lv23, "Disk:Lv.23" },
    { Item_Disk_Lv24, "Disk:Lv.24" },
    { Item_Disk_Lv25, "Disk:Lv.25" },
    { Item_Disk_Lv26, "Disk:Lv.26" },
    { Item_Disk_Lv27, "Disk:Lv.27" },
    { Item_Disk_Lv28, "Disk:Lv.28" },
    { Item_Disk_Lv29, "Disk:Lv.29" },
    { Item_Disk_Lv30, "Disk:Lv.30" },
    { Item_Sol_Atomizer, "Sol Atomizer" },
    { Item_Moon_Atomizer, "Moon Atomizer" },
    { Item_Star_Atomizer, "Star Atomizer" },
    { Item_Antidote, "Antidote" },
    { Item_Antiparalysis, "Antiparalysis" },
    { Item_Telepipe, "Telepipe" },
    { Item_Trap_Vision, "Trap Vision" },
    { Item_Scape_Doll, "Scape Doll" },
    { Item_Monogrinder, "Monogrinder" },
    { Item_Digrinder, "Digrinder" },
    { Item_Trigrinder, "Trigrinder" },
    { Item_Power_Material, "Power Material" },
    { Item_Mind_Material, "Mind Material" },
    { Item_Evade_Material, "Evade Material" },
    { Item_HP_Material, "HP Material" },
    { Item_TP_Material, "TP Material" },
    { Item_Def_Material, "Def Material" },
    { Item_Hit_Material, "Hit Material" },
    { Item_Luck_Material, "Luck Material" },
    { Item_Cell_of_MAG_502, "Cell of MAG 502" },
    { Item_Cell_of_MAG_213, "Cell of MAG 213" },
    { Item_Parts_of_RoboChao, "Parts of RoboChao" },
    { Item_Heart_of_Opa_Opa, "Heart of Opa Opa" },
    { Item_Heart_of_Pian, "Heart of Pian" },
    { Item_Heart_of_Chao, "Heart of Chao" },
    { Item_Sorcerers_Right_Arm, "Sorcerer's Right Arm" },
    { Item_S_beats_Arms, "S-beat's Arms" },
    { Item_P_arms_Arms, "P-arm's Arms" },
    { Item_Delsabers_Right_Arm, "Delsaber's Right Arm" },
    { Item_C_bringers_Right_Arm, "C-bringer's Right Arm" },
    { Item_Delsabres_Left_Arm, "Delsabre's Left Arm" },
    { Item_Book_of_KATANA1, "Book of KATANA1" },
    { Item_Book_of_KATANA2, "Book of KATANA2" },
    { Item_Book_of_KATANA3, "Book of KATANA3" },
    { Item_S_reds_Arms, "S-red's Arms" },
    { Item_Dragons_Claw, "Dragon's Claw" },
    { Item_Hildebears_Head, "Hildebear's Head" },
    { Item_Hildeblues_Head, "Hildeblue's Head" },
    { Item_Parts_of_Baranz, "Parts of Baranz" },
    { Item_Belras_Right_Arm, "Belra's Right Arm" },
    { Item_Joint_Parts, "Joint Parts" },
    { Item_Weapons_Bronze_Badge, "Weapons Bronze Badge" },
    { Item_Weapons_Silver_Badge, "Weapons Silver Badge" },
    { Item_Weapons_Gold_Badge, "Weapons Gold Badge" },
    { Item_Weapons_Crystal_Badge, "Weapons Crystal Badge" },
    { Item_Weapons_Steel_Badge, "Weapons Steel Badge" },
    { Item_Weapons_Aluminum_Badge, "Weapons Aluminum Badge" },
    { Item_Weapons_Leather_Badge, "Weapons Leather Badge" },
    { Item_Weapons_Bone_Badge, "Weapons Bone Badge" },
    { Item_Letter_of_appreciation, "Letter of appreciation" },
    { Item_Autograph_Album, "Autograph Album" },
    { Item_High_level_Mag_Cell_Eno, "High-level Mag Cell, Eno" },
    { Item_High_level_Mag_Armor_Uru, "High-level Mag Armor, Uru" },
    { Item_Special_Gene_Flou, "Special Gene Flou" },
    { Item_Sound_Source_FM, "Sound Source FM" },
    { Item_Parts_of_68000, "Parts of \"68000\"" },
    { Item_SH2, "SH2" },
    { Item_SH4, "SH4" },
    { Item_Modem, "Modem" },
    { Item_Power_VR, "Power VR" },
    { Item_Glory_in_the_past, "Glory in the past" },
    { Item_Valentines_Chocolate, "Valentine's Chocolate" },
    { Item_New_Years_Card, "New Year's Card" },
    { Item_Christmas_Card, "Christmas Card" },
    { Item_Birthday_Card, "Birthday Card" },
    { Item_Proof_of_Sonic_Team, "Proof of Sonic Team" },
    { Item_Special_Event_Ticket, "Special Event Ticket" },
    { Item_Flower_Bouquet, "Flower Bouquet" },
    { Item_Cake, "Cake" },
    { Item_Accessories, "Accessories" },
    { Item_Mr_Nakas_Business_Card, "Mr.Naka's Business Card" },
    { Item_NoSuchItem, "" }
};

const char* item_get_name_by_code(item_code_t code, int version) {
    item_map_t* cur = &item_list[0];
    //TODO δ��ɶ�������Ʒ����

    (void)version;

    /* Take care of mags so that we'll match them properly... */
    if ((code & 0xFF) == 0x02) {
        code &= 0xFFFF;
    }

    /* Look through the list for the one we want */
    while (cur->code != Item_NoSuchItem) {
        if (cur->code == code) {
            return cur->name;
        }

        ++cur;
    }

    /* No item found... */
    return NULL;
}

/* ��Ʒ�������嵥. Ӣ��. */
static bbitem_map_t bbitem_list_en[] = {
    { BBItem_Saber0, "Saber0" },
    { BBItem_Saber, "Saber" },
    { BBItem_Brand, "Brand" },
    { BBItem_Buster, "Buster" },
    { BBItem_Pallasch, "Pallasch" },
    { BBItem_Gladius, "Gladius" },
    { BBItem_DBS_SABER, "DBS SABER" },
    { BBItem_KALADBOLG, "KALADBOLG" },
    { BBItem_DURANDAL, "DURANDAL" },
    { BBItem_GALATINE, "GALATINE" },
    { BBItem_Sword, "Sword" },
    { BBItem_Gigush, "Gigush" },
    { BBItem_Breaker, "Breaker" },
    { BBItem_Claymore, "Claymore" },
    { BBItem_Calibur, "Calibur" },
    { BBItem_FLOWENS_SWORD, "FLOWENS SWORD" },
    { BBItem_LAST_SURVIVOR, "LAST SURVIVOR" },
    { BBItem_DRAGON_SLAYER, "DRAGON SLAYER" },
    { BBItem_Dagger, "Dagger" },
    { BBItem_Knife, "Knife" },
    { BBItem_Blade, "Blade" },
    { BBItem_Edge, "Edge" },
    { BBItem_Ripper, "Ripper" },
    { BBItem_BLADE_DANCE, "BLADE DANCE" },
    { BBItem_BLOODY_ART, "BLOODY ART" },
    { BBItem_CROSS_SCAR, "CROSS SCAR" },
    { BBItem_ZERO_DIVIDE, "ZERO DIVIDE" },
    { BBItem_TWO_KAMUI, "TWO KAMUI" },
    { BBItem_Partisan, "Partisan" },
    { BBItem_Halbert, "Halbert" },
    { BBItem_Glaive, "Glaive" },
    { BBItem_Berdys, "Berdys" },
    { BBItem_Gungnir, "Gungnir" },
    { BBItem_BRIONAC, "BRIONAC" },
    { BBItem_VJAYA, "VJAYA" },
    { BBItem_GAE_BOLG, "GAE BOLG" },
    { BBItem_ASTERON_BELT, "ASTERON BELT" },
    { BBItem_Slicer, "Slicer" },
    { BBItem_Spinner, "Spinner" },
    { BBItem_Cutter, "Cutter" },
    { BBItem_Sawcer, "Sawcer" },
    { BBItem_Diska, "Diska" },
    { BBItem_SLICER_OF_ASSASSIN, "SLICER OF ASSASSIN" },
    { BBItem_DISKA_OF_LIBERATOR, "DISKA OF LIBERATOR" },
    { BBItem_DISKA_OF_BRAVEMAN, "DISKA OF BRAVEMAN" },
    { BBItem_IZMAELA, "IZMAELA" },
    { BBItem_Handgun, "Handgun" },
    { BBItem_Autogun, "Autogun" },
    { BBItem_Lockgun, "Lockgun" },
    { BBItem_Railgun, "Railgun" },
    { BBItem_Raygun, "Raygun" },
    { BBItem_VARISTA, "VARISTA" },
    { BBItem_CUSTOM_RAY_ver_OO, "CUSTOM RAY ver.OO" },
    { BBItem_BRAVACE, "BRAVACE" },
    { BBItem_TENSION_BLASTER, "TENSION BLASTER" },
    { BBItem_Rifle, "Rifle" },
    { BBItem_Sniper, "Sniper" },
    { BBItem_Blaster, "Blaster" },
    { BBItem_Beam, "Beam" },
    { BBItem_Laser, "Laser" },
    { BBItem_VISK_235W, "VISK-235W" },
    { BBItem_WALS_MK2, "WALS-MK2" },
    { BBItem_JUSTY_23ST, "JUSTY-23ST" },
    { BBItem_RIANOV_303SNR, "RIANOV 303SNR" },
    { BBItem_RIANOV_303SNR_1, "RIANOV 303SNR-1" },
    { BBItem_RIANOV_303SNR_2, "RIANOV 303SNR-2" },
    { BBItem_RIANOV_303SNR_3, "RIANOV 303SNR-3" },
    { BBItem_RIANOV_303SNR_4, "RIANOV 303SNR-4" },
    { BBItem_RIANOV_303SNR_5, "RIANOV 303SNR-5" },
    { BBItem_Mechgun, "Mechgun" },
    { BBItem_Assault, "Assault" },
    { BBItem_Repeater, "Repeater" },
    { BBItem_Gatling, "Gatling" },
    { BBItem_Vulcan, "Vulcan" },
    { BBItem_M_and_A60_VISE, "M&A60 VISE" },
    { BBItem_H_and_S25_JUSTICE, "H&S25 JUSTICE" },
    { BBItem_L_and_K14_COMBAT, "L&K14 COMBAT" },
    { BBItem_Shot, "Shot" },
    { BBItem_Spread, "Spread" },
    { BBItem_Cannon, "Cannon" },
    { BBItem_Launcher, "Launcher" },
    { BBItem_Arms, "Arms" },
    { BBItem_CRUSH_BULLET, "CRUSH BULLET" },
    { BBItem_METEOR_SMASH, "METEOR SMASH" },
    { BBItem_FINAL_IMPACT, "FINAL IMPACT" },
    { BBItem_Cane, "Cane" },
    { BBItem_Stick, "Stick" },
    { BBItem_Mace, "Mace" },
    { BBItem_Club, "Club" },
    { BBItem_CLUB_OF_LACONIUM, "CLUB OF LACONIUM" },
    { BBItem_MACE_OF_ADAMAN, "MACE OF ADAMAN" },
    { BBItem_CLUB_OF_ZUMIURAN, "CLUB OF ZUMIURAN" },
    { BBItem_LOLIPOP, "LOLIPOP" },
    { BBItem_Rod, "Rod" },
    { BBItem_Pole, "Pole" },
    { BBItem_Pillar, "Pillar" },
    { BBItem_Striker, "Striker" },
    { BBItem_BATTLE_VERGE, "BATTLE VERGE" },
    { BBItem_BRAVE_HAMMER, "BRAVE HAMMER" },
    { BBItem_ALIVE_AQHU, "ALIVE AQHU" },
    { BBItem_VALKYRIE, "VALKYRIE" },
    { BBItem_Wand, "Wand" },
    { BBItem_Staff, "Staff" },
    { BBItem_Baton, "Baton" },
    { BBItem_Scepter, "Scepter" },
    { BBItem_FIRE_SCEPTER_AGNI, "FIRE SCEPTER:AGNI" },
    { BBItem_ICE_STAFF_DAGON, "ICE STAFF:DAGON" },
    { BBItem_STORM_WAND_INDRA, "STORM WAND:INDRA" },
    { BBItem_EARTH_WAND_BROWNIE, "EARTH WAND BROWNIE" },
    { BBItem_PHOTON_CLAW, "PHOTON CLAW" },
    { BBItem_SILENCE_CLAW, "SILENCE CLAW" },
    { BBItem_NEIS_CLAW_2, "NEIS CLAW" },
    { BBItem_PHOENIX_CLAW, "PHOENIX CLAW" },
    { BBItem_DOUBLE_SABER, "DOUBLE SABER" },
    { BBItem_STAG_CUTLERY, "STAG CUTLERY" },
    { BBItem_TWIN_BRAND, "TWIN BRAND" },
    { BBItem_BRAVE_KNUCKLE, "BRAVE KNUCKLE" },
    { BBItem_ANGRY_FIST, "ANGRY FIST" },
    { BBItem_GOD_HAND, "GOD HAND" },
    { BBItem_SONIC_KNUCKLE, "SONIC KNUCKLE" },
    { BBItem_OROTIAGITO_alt, "OROTIAGITO_alt" },
    { BBItem_OROTIAGITO, "OROTIAGITO" },
    { BBItem_AGITO_1975, "AGITO 1975" },
    { BBItem_AGITO_1983, "AGITO 1983" },
    { BBItem_AGITO_2001, "AGITO 2001" },
    { BBItem_AGITO_1991, "AGITO 1991" },
    { BBItem_AGITO_1977, "AGITO 1977" },
    { BBItem_AGITO_1980, "AGITO 1980" },
    { BBItem_RAIKIRI, "RAIKIRI" },
    { BBItem_SOUL_EATER, "SOUL EATER" },
    { BBItem_SOUL_BANISH, "SOUL BANISH" },
    { BBItem_SPREAD_NEEDLE, "SPREAD NEEDLE" },
    { BBItem_HOLY_RAY, "HOLY RAY" },
    { BBItem_INFERNO_BAZOOKA, "INFERNO BAZOOKA" },
    { BBItem_RAMBLING_MAY, "RAMBLING MAY" },
    { BBItem_LK38_COMBAT, "L&K38 COMBAT" },
    { BBItem_FLAME_VISIT, "FLAME VISIT" },
    { BBItem_BURNING_VISIT, "BURNING VISIT" },
    { BBItem_AKIKOS_FRYING_PAN, "AKIKOS FRYING PAN" },
    { BBItem_SORCERERS_CANE, "SORCERERS CANE" },
    { BBItem_S_BEATS_BLADE, "S-BEATS BLADE" },
    { BBItem_P_ARMSS_BLADE, "P-ARMSS BLADE" },
    { BBItem_DELSABERS_BUSTER, "DELSABERS BUSTER" },
    { BBItem_BRINGERS_RIFLE, "BRINGERS RIFLE" },
    { BBItem_EGG_BLASTER, "EGG BLASTER" },
    { BBItem_PSYCHO_WAND, "PSYCHO WAND" },
    { BBItem_HEAVEN_PUNISHER, "HEAVEN PUNISHER" },
    { BBItem_LAVIS_CANNON, "LAVIS CANNON" },
    { BBItem_VICTOR_AXE, "VICTOR AXE" },
    { BBItem_LACONIUM_AXE, "LACONIUM AXE" },
    { BBItem_CHAIN_SAWD, "CHAIN SAWD" },
    { BBItem_CADUCEUS, "CADUCEUS" },
    { BBItem_MERCURIUS_ROD, "MERCURIUS ROD" },
    { BBItem_STING_TIP, "STING TIP" },
    { BBItem_MAGICAL_PIECE, "MAGICAL PIECE" },
    { BBItem_TECHNICAL_CROZIER, "TECHNICAL CROZIER" },
    { BBItem_SUPPRESSED_GUN, "SUPPRESSED GUN" },
    { BBItem_ANCIENT_SABER, "ANCIENT SABER" },
    { BBItem_HARISEN_BATTLE_FAN, "HARISEN BATTLE FAN" },
    { BBItem_YAMIGARASU, "YAMIGARASU" },
    { BBItem_AKIKOS_WOK, "AKIKOS WOK" },
    { BBItem_TOY_HAMMER, "TOY HAMMER" },
    { BBItem_ELYSION, "ELYSION" },
    { BBItem_RED_SABER, "RED SABER" },
    { BBItem_METEOR_CUDGEL, "METEOR CUDGEL" },
    { BBItem_MONKEY_KING_BAR, "MONKEY KING BAR" },
    { BBItem_BLACK_KING_BAR, "BLACK KING BAR" },
    { BBItem_DOUBLE_CANNON, "DOUBLE CANNON" },
    { BBItem_GIRASOLE, "GIRASOLE" },
    { BBItem_HUGE_BATTLE_FAN, "HUGE BATTLE FAN" },
    { BBItem_TSUMIKIRI_J_SWORD, "TSUMIKIRI J-SWORD" },
    { BBItem_SEALED_J_SWORD, "SEALED J-SWORD" },
    { BBItem_RED_SWORD, "RED SWORD" },
    { BBItem_CRAZY_TUNE, "CRAZY TUNE" },
    { BBItem_TWIN_CHAKRAM, "TWIN CHAKRAM" },
    { BBItem_WOK_OF_AKIKOS_SHOP, "WOK OF AKIKOS SHOP" },
    { BBItem_LAVIS_BLADE, "LAVIS BLADE" },
    { BBItem_RED_DAGGER, "RED DAGGER" },
    { BBItem_MADAMS_PARASOL, "MADAMS PARASOL" },
    { BBItem_MADAMS_UMBRELLA, "MADAMS UMBRELLA" },
    { BBItem_IMPERIAL_PICK, "IMPERIAL PICK" },
    { BBItem_BERDYSH, "BERDYSH" },
    { BBItem_RED_PARTISAN, "RED PARTISAN" },
    { BBItem_FLIGHT_CUTTER, "FLIGHT CUTTER" },
    { BBItem_FLIGHT_FAN, "FLIGHT FAN" },
    { BBItem_RED_SLICER, "RED SLICER" },
    { BBItem_HANDGUN_GULD, "HANDGUN:GULD" },
    { BBItem_MASTER_RAVEN, "MASTER RAVEN" },
    { BBItem_HANDGUN_MILLA, "HANDGUN:MILLA" },
    { BBItem_LAST_SWAN, "LAST SWAN" },
    { BBItem_RED_HANDGUN, "RED HANDGUN" },
    { BBItem_FROZEN_SHOOTER, "FROZEN SHOOTER" },
    { BBItem_SNOW_QUEEN, "SNOW QUEEN" },
    { BBItem_ANTI_ANDROID_RIFLE, "ANTI ANDROID RIFLE" },
    { BBItem_ROCKET_PUNCH, "ROCKET PUNCH" },
    { BBItem_SAMBA_MARACAS, "SAMBA MARACAS" },
    { BBItem_TWIN_PSYCHOGUN, "TWIN PSYCHOGUN" },
    { BBItem_DRILL_LAUNCHER, "DRILL LAUNCHER" },
    { BBItem_GULD_MILLA, "GULD MILLA" },
    { BBItem_DUAL_BIRD, "DUAL BIRD" },
    { BBItem_RED_MECHGUN, "RED MECHGUN" },
    { BBItem_BELRA_CANNON, "BELRA CANNON" },
    { BBItem_PANZER_FAUST, "PANZER FAUST" },
    { BBItem_IRON_FAUST, "IRON FAUST" },
    { BBItem_SUMMIT_MOON, "SUMMIT MOON" },
    { BBItem_WINDMILL, "WINDMILL" },
    { BBItem_EVIL_CURST, "EVIL CURST" },
    { BBItem_FLOWER_CANE, "FLOWER CANE" },
    { BBItem_HILDEBEARS_CANE, "HILDEBEARS CANE" },
    { BBItem_HILDEBLUES_CANE, "HILDEBLUES CANE" },
    { BBItem_RABBIT_WAND, "RABBIT WAND" },
    { BBItem_PLANTAIN_LEAF, "PLANTAIN LEAF" },
    { BBItem_FATSIA, "FATSIA" },
    { BBItem_DEMONIC_FORK, "DEMONIC FORK" },
    { BBItem_STRIKER_OF_CHAO, "STRIKER OF CHAO" },
    { BBItem_BROOM, "BROOM" },
    { BBItem_PROPHETS_OF_MOTAV, "PROPHETS OF MOTAV" },
    { BBItem_THE_SIGH_OF_A_GOD, "THE SIGH OF A GOD" },
    { BBItem_TWINKLE_STAR, "TWINKLE STAR" },
    { BBItem_PLANTAIN_FAN, "PLANTAIN FAN" },
    { BBItem_TWIN_BLAZE, "TWIN BLAZE" },
    { BBItem_MARINAS_BAG, "MARINAS BAG" },
    { BBItem_DRAGONS_CLAW_0, "DRAGONS CLAW" },
    { BBItem_PANTHERS_CLAW, "PANTHERS CLAW" },
    { BBItem_S_REDS_BLADE, "S-REDS BLADE" },
    { BBItem_PLANTAIN_HUGE_FAN, "PLANTAIN HUGE FAN" },
    { BBItem_CHAMELEON_SCYTHE, "CHAMELEON SCYTHE" },
    { BBItem_YASMINKOV_3000R, "YASMINKOV 3000R" },
    { BBItem_ANO_RIFLE, "ANO RIFLE" },
    { BBItem_BARANZ_LAUNCHER, "BARANZ LAUNCHER" },
    { BBItem_BRANCH_OF_PAKUPAKU, "BRANCH OF PAKUPAKU" },
    { BBItem_HEART_OF_POUMN, "HEART OF POUMN" },
    { BBItem_YASMINKOV_2000H, "YASMINKOV 2000H" },
    { BBItem_YASMINKOV_7000V, "YASMINKOV 7000V" },
    { BBItem_YASMINKOV_9000M, "YASMINKOV 9000M" },
    { BBItem_MASER_BEAM, "MASER BEAM" },
    { BBItem_POWER_MASER, "POWER MASER" },
    { BBItem_GAME_MAGAZNE, "GAME MAGAZNE" },
    { BBItem_LOGiN, "LOGiN" },
    { BBItem_FLOWER_BOUQUET_0, "FLOWER BOUQUET" },
    { BBItem_SRANK_SABER_0, "SRANK SABER 0" },
    { BBItem_SRANK_SABER_1, "SRANK SABER 1" },
    { BBItem_SRANK_SABER_2, "SRANK SABER 2" },
    { BBItem_SRANK_SABER_3, "SRANK SABER 3" },
    { BBItem_SRANK_SABER_4, "SRANK SABER 4" },
    { BBItem_SRANK_SABER_5, "SRANK SABER 5" },
    { BBItem_SRANK_SABER_6, "SRANK SABER 6" },
    { BBItem_SRANK_SABER_7, "SRANK SABER 7" },
    { BBItem_SRANK_SABER_8, "SRANK SABER 8" },
    { BBItem_SRANK_SABER_9, "SRANK SABER 9" },
    { BBItem_SRANK_SABER_10, "SRANK SABER 10" },
    { BBItem_SRANK_SABER_11, "SRANK SABER 11" },
    { BBItem_SRANK_SABER_12, "SRANK SABER 12" },
    { BBItem_SRANK_SABER_13, "SRANK SABER 13" },
    { BBItem_SRANK_SABER_14, "SRANK SABER 14" },
    { BBItem_SRANK_SABER_15, "SRANK SABER 15" },
    { BBItem_SRANK_SABER_16, "SRANK SABER 16" },
    { BBItem_SRANK_SWORD_0, "SRANK SWORD 0" },
    { BBItem_SRANK_SWORD_1, "SRANK SWORD 1" },
    { BBItem_SRANK_SWORD_2, "SRANK SWORD 2" },
    { BBItem_SRANK_SWORD_3, "SRANK SWORD 3" },
    { BBItem_SRANK_SWORD_4, "SRANK SWORD 4" },
    { BBItem_SRANK_SWORD_5, "SRANK SWORD 5" },
    { BBItem_SRANK_SWORD_6, "SRANK SWORD 6" },
    { BBItem_SRANK_SWORD_7, "SRANK SWORD 7" },
    { BBItem_SRANK_SWORD_8, "SRANK SWORD 8" },
    { BBItem_SRANK_SWORD_9, "SRANK SWORD 9" },
    { BBItem_SRANK_SWORD_10, "SRANK SWORD 10" },
    { BBItem_SRANK_SWORD_11, "SRANK SWORD 11" },
    { BBItem_SRANK_SWORD_12, "SRANK SWORD 12" },
    { BBItem_SRANK_SWORD_13, "SRANK SWORD 13" },
    { BBItem_SRANK_SWORD_14, "SRANK SWORD 14" },
    { BBItem_SRANK_SWORD_15, "SRANK SWORD 15" },
    { BBItem_SRANK_SWORD_16, "SRANK SWORD 16" },
    { BBItem_SRANK_BLADE_0, "SRANK BLADE 0" },
    { BBItem_SRANK_BLADE_1, "SRANK BLADE 1" },
    { BBItem_SRANK_BLADE_2, "SRANK BLADE 2" },
    { BBItem_SRANK_BLADE_3, "SRANK BLADE 3" },
    { BBItem_SRANK_BLADE_4, "SRANK BLADE 4" },
    { BBItem_SRANK_BLADE_5, "SRANK BLADE 5" },
    { BBItem_SRANK_BLADE_6, "SRANK BLADE 6" },
    { BBItem_SRANK_BLADE_7, "SRANK BLADE 7" },
    { BBItem_SRANK_BLADE_8, "SRANK BLADE 8" },
    { BBItem_SRANK_BLADE_9, "SRANK BLADE 9" },
    { BBItem_SRANK_BLADE_10, "SRANK BLADE 10" },
    { BBItem_SRANK_BLADE_11, "SRANK BLADE 11" },
    { BBItem_SRANK_BLADE_12, "SRANK BLADE 12" },
    { BBItem_SRANK_BLADE_13, "SRANK BLADE 13" },
    { BBItem_SRANK_BLADE_14, "SRANK BLADE 14" },
    { BBItem_SRANK_BLADE_15, "SRANK BLADE 15" },
    { BBItem_SRANK_BLADE_16, "SRANK BLADE 16" },
    { BBItem_SRANK_PARTISAN_0, "SRANK PARTISAN 0" },
    { BBItem_SRANK_PARTISAN_1, "SRANK PARTISAN 1" },
    { BBItem_SRANK_PARTISAN_2, "SRANK PARTISAN 2" },
    { BBItem_SRANK_PARTISAN_3, "SRANK PARTISAN 3" },
    { BBItem_SRANK_PARTISAN_4, "SRANK PARTISAN 4" },
    { BBItem_SRANK_PARTISAN_5, "SRANK PARTISAN 5" },
    { BBItem_SRANK_PARTISAN_6, "SRANK PARTISAN 6" },
    { BBItem_SRANK_PARTISAN_7, "SRANK PARTISAN 7" },
    { BBItem_SRANK_PARTISAN_8, "SRANK PARTISAN 8" },
    { BBItem_SRANK_PARTISAN_9, "SRANK PARTISAN 9" },
    { BBItem_SRANK_PARTISAN_10, "SRANK PARTISAN 10" },
    { BBItem_SRANK_PARTISAN_11, "SRANK PARTISAN 11" },
    { BBItem_SRANK_PARTISAN_12, "SRANK PARTISAN 12" },
    { BBItem_SRANK_PARTISAN_13, "SRANK PARTISAN 13" },
    { BBItem_SRANK_PARTISAN_14, "SRANK PARTISAN 14" },
    { BBItem_SRANK_PARTISAN_15, "SRANK PARTISAN 15" },
    { BBItem_SRANK_PARTISAN_16, "SRANK PARTISAN 16" },
    { BBItem_SRANK_SLICER_0, "SRANK SLICER 0" },
    { BBItem_SRANK_SLICER_1, "SRANK SLICER 1" },
    { BBItem_SRANK_SLICER_2, "SRANK SLICER 2" },
    { BBItem_SRANK_SLICER_3, "SRANK SLICER 3" },
    { BBItem_SRANK_SLICER_4, "SRANK SLICER 4" },
    { BBItem_SRANK_SLICER_5, "SRANK SLICER 5" },
    { BBItem_SRANK_SLICER_6, "SRANK SLICER 6" },
    { BBItem_SRANK_SLICER_7, "SRANK SLICER 7" },
    { BBItem_SRANK_SLICER_8, "SRANK SLICER 8" },
    { BBItem_SRANK_SLICER_9, "SRANK SLICER 9" },
    { BBItem_SRANK_SLICER_10, "SRANK SLICER 10" },
    { BBItem_SRANK_SLICER_11, "SRANK SLICER 11" },
    { BBItem_SRANK_SLICER_12, "SRANK SLICER 12" },
    { BBItem_SRANK_SLICER_13, "SRANK SLICER 13" },
    { BBItem_SRANK_SLICER_14, "SRANK SLICER 14" },
    { BBItem_SRANK_SLICER_15, "SRANK SLICER 15" },
    { BBItem_SRANK_SLICER_16, "SRANK SLICER 16" },
    { BBItem_SRANK_GUN_0, "SRANK GUN 0" },
    { BBItem_SRANK_GUN_1, "SRANK GUN 1" },
    { BBItem_SRANK_GUN_2, "SRANK GUN 2" },
    { BBItem_SRANK_GUN_3, "SRANK GUN 3" },
    { BBItem_SRANK_GUN_4, "SRANK GUN 4" },
    { BBItem_SRANK_GUN_5, "SRANK GUN 5" },
    { BBItem_SRANK_GUN_6, "SRANK GUN 6" },
    { BBItem_SRANK_GUN_7, "SRANK GUN 7" },
    { BBItem_SRANK_GUN_8, "SRANK GUN 8" },
    { BBItem_SRANK_GUN_9, "SRANK GUN 9" },
    { BBItem_SRANK_GUN_10, "SRANK GUN 10" },
    { BBItem_SRANK_GUN_11, "SRANK GUN 11" },
    { BBItem_SRANK_GUN_12, "SRANK GUN 12" },
    { BBItem_SRANK_GUN_13, "SRANK GUN 13" },
    { BBItem_SRANK_GUN_14, "SRANK GUN 14" },
    { BBItem_SRANK_GUN_15, "SRANK GUN 15" },
    { BBItem_SRANK_GUN_16, "SRANK GUN 16" },
    { BBItem_SRANK_RIFLE_0, "SRANK RIFLE 0" },
    { BBItem_SRANK_RIFLE_1, "SRANK RIFLE 1" },
    { BBItem_SRANK_RIFLE_2, "SRANK RIFLE 2" },
    { BBItem_SRANK_RIFLE_3, "SRANK RIFLE 3" },
    { BBItem_SRANK_RIFLE_4, "SRANK RIFLE 4" },
    { BBItem_SRANK_RIFLE_5, "SRANK RIFLE 5" },
    { BBItem_SRANK_RIFLE_6, "SRANK RIFLE 6" },
    { BBItem_SRANK_RIFLE_7, "SRANK RIFLE 7" },
    { BBItem_SRANK_RIFLE_8, "SRANK RIFLE 8" },
    { BBItem_SRANK_RIFLE_9, "SRANK RIFLE 9" },
    { BBItem_SRANK_RIFLE_10, "SRANK RIFLE 10" },
    { BBItem_SRANK_RIFLE_11, "SRANK RIFLE 11" },
    { BBItem_SRANK_RIFLE_12, "SRANK RIFLE 12" },
    { BBItem_SRANK_RIFLE_13, "SRANK RIFLE 13" },
    { BBItem_SRANK_RIFLE_14, "SRANK RIFLE 14" },
    { BBItem_SRANK_RIFLE_15, "SRANK RIFLE 15" },
    { BBItem_SRANK_RIFLE_16, "SRANK RIFLE 16" },
    { BBItem_SRANK_MECHGUN_0, "SRANK MECHGUN 0" },
    { BBItem_SRANK_MECHGUN_1, "SRANK MECHGUN 1" },
    { BBItem_SRANK_MECHGUN_2, "SRANK MECHGUN 2" },
    { BBItem_SRANK_MECHGUN_3, "SRANK MECHGUN 3" },
    { BBItem_SRANK_MECHGUN_4, "SRANK MECHGUN 4" },
    { BBItem_SRANK_MECHGUN_5, "SRANK MECHGUN 5" },
    { BBItem_SRANK_MECHGUN_6, "SRANK MECHGUN 6" },
    { BBItem_SRANK_MECHGUN_7, "SRANK MECHGUN 7" },
    { BBItem_SRANK_MECHGUN_8, "SRANK MECHGUN 8" },
    { BBItem_SRANK_MECHGUN_9, "SRANK MECHGUN 9" },
    { BBItem_SRANK_MECHGUN_10, "SRANK MECHGUN 10" },
    { BBItem_SRANK_MECHGUN_11, "SRANK MECHGUN 11" },
    { BBItem_SRANK_MECHGUN_12, "SRANK MECHGUN 12" },
    { BBItem_SRANK_MECHGUN_13, "SRANK MECHGUN 13" },
    { BBItem_SRANK_MECHGUN_14, "SRANK MECHGUN 14" },
    { BBItem_SRANK_MECHGUN_15, "SRANK MECHGUN 15" },
    { BBItem_SRANK_MECHGUN_16, "SRANK MECHGUN 16" },
    { BBItem_SRANK_SHOT_0, "SRANK SHOT 0" },
    { BBItem_SRANK_SHOT_1, "SRANK SHOT 1" },
    { BBItem_SRANK_SHOT_2, "SRANK SHOT 2" },
    { BBItem_SRANK_SHOT_3, "SRANK SHOT 3" },
    { BBItem_SRANK_SHOT_4, "SRANK SHOT 4" },
    { BBItem_SRANK_SHOT_5, "SRANK SHOT 5" },
    { BBItem_SRANK_SHOT_6, "SRANK SHOT 6" },
    { BBItem_SRANK_SHOT_7, "SRANK SHOT 7" },
    { BBItem_SRANK_SHOT_8, "SRANK SHOT 8" },
    { BBItem_SRANK_SHOT_9, "SRANK SHOT 9" },
    { BBItem_SRANK_SHOT_10, "SRANK SHOT 10" },
    { BBItem_SRANK_SHOT_11, "SRANK SHOT 11" },
    { BBItem_SRANK_SHOT_12, "SRANK SHOT 12" },
    { BBItem_SRANK_SHOT_13, "SRANK SHOT 13" },
    { BBItem_SRANK_SHOT_14, "SRANK SHOT 14" },
    { BBItem_SRANK_SHOT_15, "SRANK SHOT 15" },
    { BBItem_SRANK_SHOT_16, "SRANK SHOT 16" },
    { BBItem_SRANK_CANE_0, "SRANK CANE 0" },
    { BBItem_SRANK_CANE_1, "SRANK CANE 1" },
    { BBItem_SRANK_CANE_2, "SRANK CANE 2" },
    { BBItem_SRANK_CANE_3, "SRANK CANE 3" },
    { BBItem_SRANK_CANE_4, "SRANK CANE 4" },
    { BBItem_SRANK_CANE_5, "SRANK CANE 5" },
    { BBItem_SRANK_CANE_6, "SRANK CANE 6" },
    { BBItem_SRANK_CANE_7, "SRANK CANE 7" },
    { BBItem_SRANK_CANE_8, "SRANK CANE 8" },
    { BBItem_SRANK_CANE_9, "SRANK CANE 9" },
    { BBItem_SRANK_CANE_10, "SRANK CANE 10" },
    { BBItem_SRANK_CANE_11, "SRANK CANE 11" },
    { BBItem_SRANK_CANE_12, "SRANK CANE 12" },
    { BBItem_SRANK_CANE_13, "SRANK CANE 13" },
    { BBItem_SRANK_CANE_14, "SRANK CANE 14" },
    { BBItem_SRANK_CANE_15, "SRANK CANE 15" },
    { BBItem_SRANK_CANE_16, "SRANK CANE 16" },
    { BBItem_SRANK_ROD_0, "SRANK ROD 0" },
    { BBItem_SRANK_ROD_1, "SRANK ROD 1" },
    { BBItem_SRANK_ROD_2, "SRANK ROD 2" },
    { BBItem_SRANK_ROD_3, "SRANK ROD 3" },
    { BBItem_SRANK_ROD_4, "SRANK ROD 4" },
    { BBItem_SRANK_ROD_5, "SRANK ROD 5" },
    { BBItem_SRANK_ROD_6, "SRANK ROD 6" },
    { BBItem_SRANK_ROD_7, "SRANK ROD 7" },
    { BBItem_SRANK_ROD_8, "SRANK ROD 8" },
    { BBItem_SRANK_ROD_9, "SRANK ROD 9" },
    { BBItem_SRANK_ROD_10, "SRANK ROD 10" },
    { BBItem_SRANK_ROD_11, "SRANK ROD 11" },
    { BBItem_SRANK_ROD_12, "SRANK ROD 12" },
    { BBItem_SRANK_ROD_13, "SRANK ROD 13" },
    { BBItem_SRANK_ROD_14, "SRANK ROD 14" },
    { BBItem_SRANK_ROD_15, "SRANK ROD 15" },
    { BBItem_SRANK_ROD_16, "SRANK ROD 16" },
    { BBItem_SRANK_WAND_0, "SRANK WAND 0" },
    { BBItem_SRANK_WAND_1, "SRANK WAND 1" },
    { BBItem_SRANK_WAND_2, "SRANK WAND 2" },
    { BBItem_SRANK_WAND_3, "SRANK WAND 3" },
    { BBItem_SRANK_WAND_4, "SRANK WAND 4" },
    { BBItem_SRANK_WAND_5, "SRANK WAND 5" },
    { BBItem_SRANK_WAND_6, "SRANK WAND 6" },
    { BBItem_SRANK_WAND_7, "SRANK WAND 7" },
    { BBItem_SRANK_WAND_8, "SRANK WAND 8" },
    { BBItem_SRANK_WAND_9, "SRANK WAND 9" },
    { BBItem_SRANK_WAND_10, "SRANK WAND 10" },
    { BBItem_SRANK_WAND_11, "SRANK WAND 11" },
    { BBItem_SRANK_WAND_12, "SRANK WAND 12" },
    { BBItem_SRANK_WAND_13, "SRANK WAND 13" },
    { BBItem_SRANK_WAND_14, "SRANK WAND 14" },
    { BBItem_SRANK_WAND_15, "SRANK WAND 15" },
    { BBItem_SRANK_WAND_16, "SRANK WAND 16" },
    { BBItem_SRANK_TWIN_0, "SRANK TWIN 0" },
    { BBItem_SRANK_TWIN_1, "SRANK TWIN 1" },
    { BBItem_SRANK_TWIN_2, "SRANK TWIN 2" },
    { BBItem_SRANK_TWIN_3, "SRANK TWIN 3" },
    { BBItem_SRANK_TWIN_4, "SRANK TWIN 4" },
    { BBItem_SRANK_TWIN_5, "SRANK TWIN 5" },
    { BBItem_SRANK_TWIN_6, "SRANK TWIN 6" },
    { BBItem_SRANK_TWIN_7, "SRANK TWIN 7" },
    { BBItem_SRANK_TWIN_8, "SRANK TWIN 8" },
    { BBItem_SRANK_TWIN_9, "SRANK TWIN 9" },
    { BBItem_SRANK_TWIN_10, "SRANK TWIN 10" },
    { BBItem_SRANK_TWIN_11, "SRANK TWIN 11" },
    { BBItem_SRANK_TWIN_12, "SRANK TWIN 12" },
    { BBItem_SRANK_TWIN_13, "SRANK TWIN 13" },
    { BBItem_SRANK_TWIN_14, "SRANK TWIN 14" },
    { BBItem_SRANK_TWIN_15, "SRANK TWIN 15" },
    { BBItem_SRANK_TWIN_16, "SRANK TWIN 16" },
    { BBItem_SRANK_CLAW_0, "SRANK CLAW 0" },
    { BBItem_SRANK_CLAW_1, "SRANK CLAW 1" },
    { BBItem_SRANK_CLAW_2, "SRANK CLAW 2" },
    { BBItem_SRANK_CLAW_3, "SRANK CLAW 3" },
    { BBItem_SRANK_CLAW_4, "SRANK CLAW 4" },
    { BBItem_SRANK_CLAW_5, "SRANK CLAW 5" },
    { BBItem_SRANK_CLAW_6, "SRANK CLAW 6" },
    { BBItem_SRANK_CLAW_7, "SRANK CLAW 7" },
    { BBItem_SRANK_CLAW_8, "SRANK CLAW 8" },
    { BBItem_SRANK_CLAW_9, "SRANK CLAW 9" },
    { BBItem_SRANK_CLAW_10, "SRANK CLAW 10" },
    { BBItem_SRANK_CLAW_11, "SRANK CLAW 11" },
    { BBItem_SRANK_CLAW_12, "SRANK CLAW 12" },
    { BBItem_SRANK_CLAW_13, "SRANK CLAW 13" },
    { BBItem_SRANK_CLAW_14, "SRANK CLAW 14" },
    { BBItem_SRANK_CLAW_15, "SRANK CLAW 15" },
    { BBItem_SRANK_CLAW_16, "SRANK CLAW 16" },
    { BBItem_SRANK_BAZOOKA_0, "SRANK BAZOOKA 0" },
    { BBItem_SRANK_BAZOOKA_1, "SRANK BAZOOKA 1" },
    { BBItem_SRANK_BAZOOKA_2, "SRANK BAZOOKA 2" },
    { BBItem_SRANK_BAZOOKA_3, "SRANK BAZOOKA 3" },
    { BBItem_SRANK_BAZOOKA_4, "SRANK BAZOOKA 4" },
    { BBItem_SRANK_BAZOOKA_5, "SRANK BAZOOKA 5" },
    { BBItem_SRANK_BAZOOKA_6, "SRANK BAZOOKA 6" },
    { BBItem_SRANK_BAZOOKA_7, "SRANK BAZOOKA 7" },
    { BBItem_SRANK_BAZOOKA_8, "SRANK BAZOOKA 8" },
    { BBItem_SRANK_BAZOOKA_9, "SRANK BAZOOKA 9" },
    { BBItem_SRANK_BAZOOKA_10, "SRANK BAZOOKA 10" },
    { BBItem_SRANK_BAZOOKA_11, "SRANK BAZOOKA 11" },
    { BBItem_SRANK_BAZOOKA_12, "SRANK BAZOOKA 12" },
    { BBItem_SRANK_BAZOOKA_13, "SRANK BAZOOKA 13" },
    { BBItem_SRANK_BAZOOKA_14, "SRANK BAZOOKA 14" },
    { BBItem_SRANK_BAZOOKA_15, "SRANK BAZOOKA 15" },
    { BBItem_SRANK_BAZOOKA_16, "SRANK BAZOOKA 16" },
    { BBItem_SRANK_NEEDLE_0, "SRANK NEEDLE 0" },
    { BBItem_SRANK_NEEDLE_1, "SRANK NEEDLE 1" },
    { BBItem_SRANK_NEEDLE_2, "SRANK NEEDLE 2" },
    { BBItem_SRANK_NEEDLE_3, "SRANK NEEDLE 3" },
    { BBItem_SRANK_NEEDLE_4, "SRANK NEEDLE 4" },
    { BBItem_SRANK_NEEDLE_5, "SRANK NEEDLE 5" },
    { BBItem_SRANK_NEEDLE_6, "SRANK NEEDLE 6" },
    { BBItem_SRANK_NEEDLE_7, "SRANK NEEDLE 7" },
    { BBItem_SRANK_NEEDLE_8, "SRANK NEEDLE 8" },
    { BBItem_SRANK_NEEDLE_9, "SRANK NEEDLE 9" },
    { BBItem_SRANK_NEEDLE_10, "SRANK NEEDLE 10" },
    { BBItem_SRANK_NEEDLE_11, "SRANK NEEDLE 11" },
    { BBItem_SRANK_NEEDLE_12, "SRANK NEEDLE 12" },
    { BBItem_SRANK_NEEDLE_13, "SRANK NEEDLE 13" },
    { BBItem_SRANK_NEEDLE_14, "SRANK NEEDLE 14" },
    { BBItem_SRANK_NEEDLE_15, "SRANK NEEDLE 15" },
    { BBItem_SRANK_NEEDLE_16, "SRANK NEEDLE 16" },
    { BBItem_SRANK_SCYTHE_0, "SRANK SCYTHE 0" },
    { BBItem_SRANK_SCYTHE_1, "SRANK SCYTHE 1" },
    { BBItem_SRANK_SCYTHE_2, "SRANK SCYTHE 2" },
    { BBItem_SRANK_SCYTHE_3, "SRANK SCYTHE 3" },
    { BBItem_SRANK_SCYTHE_4, "SRANK SCYTHE 4" },
    { BBItem_SRANK_SCYTHE_5, "SRANK SCYTHE 5" },
    { BBItem_SRANK_SCYTHE_6, "SRANK SCYTHE 6" },
    { BBItem_SRANK_SCYTHE_7, "SRANK SCYTHE 7" },
    { BBItem_SRANK_SCYTHE_8, "SRANK SCYTHE 8" },
    { BBItem_SRANK_SCYTHE_9, "SRANK SCYTHE 9" },
    { BBItem_SRANK_SCYTHE_10, "SRANK SCYTHE 10" },
    { BBItem_SRANK_SCYTHE_11, "SRANK SCYTHE 11" },
    { BBItem_SRANK_SCYTHE_12, "SRANK SCYTHE 12" },
    { BBItem_SRANK_SCYTHE_13, "SRANK SCYTHE 13" },
    { BBItem_SRANK_SCYTHE_14, "SRANK SCYTHE 14" },
    { BBItem_SRANK_SCYTHE_15, "SRANK SCYTHE 15" },
    { BBItem_SRANK_SCYTHE_16, "SRANK SCYTHE 16" },
    { BBItem_SRANK_HAMMER_0, "SRANK HAMMER 0" },
    { BBItem_SRANK_HAMMER_1, "SRANK HAMMER 1" },
    { BBItem_SRANK_HAMMER_2, "SRANK HAMMER 2" },
    { BBItem_SRANK_HAMMER_3, "SRANK HAMMER 3" },
    { BBItem_SRANK_HAMMER_4, "SRANK HAMMER 4" },
    { BBItem_SRANK_HAMMER_5, "SRANK HAMMER 5" },
    { BBItem_SRANK_HAMMER_6, "SRANK HAMMER 6" },
    { BBItem_SRANK_HAMMER_7, "SRANK HAMMER 7" },
    { BBItem_SRANK_HAMMER_8, "SRANK HAMMER 8" },
    { BBItem_SRANK_HAMMER_9, "SRANK HAMMER 9" },
    { BBItem_SRANK_HAMMER_10, "SRANK HAMMER 10" },
    { BBItem_SRANK_HAMMER_11, "SRANK HAMMER 11" },
    { BBItem_SRANK_HAMMER_12, "SRANK HAMMER 12" },
    { BBItem_SRANK_HAMMER_13, "SRANK HAMMER 13" },
    { BBItem_SRANK_HAMMER_14, "SRANK HAMMER 14" },
    { BBItem_SRANK_HAMMER_15, "SRANK HAMMER 15" },
    { BBItem_SRANK_HAMMER_16, "SRANK HAMMER 16" },
    { BBItem_SRANK_MOON_0, "SRANK MOON 0" },
    { BBItem_SRANK_MOON_1, "SRANK MOON 1" },
    { BBItem_SRANK_MOON_2, "SRANK MOON 2" },
    { BBItem_SRANK_MOON_3, "SRANK MOON 3" },
    { BBItem_SRANK_MOON_4, "SRANK MOON 4" },
    { BBItem_SRANK_MOON_5, "SRANK MOON 5" },
    { BBItem_SRANK_MOON_6, "SRANK MOON 6" },
    { BBItem_SRANK_MOON_7, "SRANK MOON 7" },
    { BBItem_SRANK_MOON_8, "SRANK MOON 8" },
    { BBItem_SRANK_MOON_9, "SRANK MOON 9" },
    { BBItem_SRANK_MOON_10, "SRANK MOON 10" },
    { BBItem_SRANK_MOON_11, "SRANK MOON 11" },
    { BBItem_SRANK_MOON_12, "SRANK MOON 12" },
    { BBItem_SRANK_MOON_13, "SRANK MOON 13" },
    { BBItem_SRANK_MOON_14, "SRANK MOON 14" },
    { BBItem_SRANK_MOON_15, "SRANK MOON 15" },
    { BBItem_SRANK_MOON_16, "SRANK MOON 16" },
    { BBItem_SRANK_PSYCHOGUN_0, "SRANK PSYCHOGUN 0" },
    { BBItem_SRANK_PSYCHOGUN_1, "SRANK PSYCHOGUN 1" },
    { BBItem_SRANK_PSYCHOGUN_2, "SRANK PSYCHOGUN 2" },
    { BBItem_SRANK_PSYCHOGUN_3, "SRANK PSYCHOGUN 3" },
    { BBItem_SRANK_PSYCHOGUN_4, "SRANK PSYCHOGUN 4" },
    { BBItem_SRANK_PSYCHOGUN_5, "SRANK PSYCHOGUN 5" },
    { BBItem_SRANK_PSYCHOGUN_6, "SRANK PSYCHOGUN 6" },
    { BBItem_SRANK_PSYCHOGUN_7, "SRANK PSYCHOGUN 7" },
    { BBItem_SRANK_PSYCHOGUN_8, "SRANK PSYCHOGUN 8" },
    { BBItem_SRANK_PSYCHOGUN_9, "SRANK PSYCHOGUN 9" },
    { BBItem_SRANK_PSYCHOGUN_10, "SRANK PSYCHOGUN 10" },
    { BBItem_SRANK_PSYCHOGUN_11, "SRANK PSYCHOGUN 11" },
    { BBItem_SRANK_PSYCHOGUN_12, "SRANK PSYCHOGUN 12" },
    { BBItem_SRANK_PSYCHOGUN_13, "SRANK PSYCHOGUN 13" },
    { BBItem_SRANK_PSYCHOGUN_14, "SRANK PSYCHOGUN 14" },
    { BBItem_SRANK_PSYCHOGUN_15, "SRANK PSYCHOGUN 15" },
    { BBItem_SRANK_PSYCHOGUN_16, "SRANK PSYCHOGUN 16" },
    { BBItem_SRANK_PUNCH_0, "SRANK PUNCH 0" },
    { BBItem_SRANK_PUNCH_1, "SRANK PUNCH 1" },
    { BBItem_SRANK_PUNCH_2, "SRANK PUNCH 2" },
    { BBItem_SRANK_PUNCH_3, "SRANK PUNCH 3" },
    { BBItem_SRANK_PUNCH_4, "SRANK PUNCH 4" },
    { BBItem_SRANK_PUNCH_5, "SRANK PUNCH 5" },
    { BBItem_SRANK_PUNCH_6, "SRANK PUNCH 6" },
    { BBItem_SRANK_PUNCH_7, "SRANK PUNCH 7" },
    { BBItem_SRANK_PUNCH_8, "SRANK PUNCH 8" },
    { BBItem_SRANK_PUNCH_9, "SRANK PUNCH 9" },
    { BBItem_SRANK_PUNCH_10, "SRANK PUNCH 10" },
    { BBItem_SRANK_PUNCH_11, "SRANK PUNCH 11" },
    { BBItem_SRANK_PUNCH_12, "SRANK PUNCH 12" },
    { BBItem_SRANK_PUNCH_13, "SRANK PUNCH 13" },
    { BBItem_SRANK_PUNCH_14, "SRANK PUNCH 14" },
    { BBItem_SRANK_PUNCH_15, "SRANK PUNCH 15" },
    { BBItem_SRANK_PUNCH_16, "SRANK PUNCH 16" },
    { BBItem_SRANK_WINDMILL_0, "SRANK WINDMILL 0" },
    { BBItem_SRANK_WINDMILL_1, "SRANK WINDMILL 1" },
    { BBItem_SRANK_WINDMILL_2, "SRANK WINDMILL 2" },
    { BBItem_SRANK_WINDMILL_3, "SRANK WINDMILL 3" },
    { BBItem_SRANK_WINDMILL_4, "SRANK WINDMILL 4" },
    { BBItem_SRANK_WINDMILL_5, "SRANK WINDMILL 5" },
    { BBItem_SRANK_WINDMILL_6, "SRANK WINDMILL 6" },
    { BBItem_SRANK_WINDMILL_7, "SRANK WINDMILL 7" },
    { BBItem_SRANK_WINDMILL_8, "SRANK WINDMILL 8" },
    { BBItem_SRANK_WINDMILL_9, "SRANK WINDMILL 9" },
    { BBItem_SRANK_WINDMILL_10, "SRANK WINDMILL 10" },
    { BBItem_SRANK_WINDMILL_11, "SRANK WINDMILL 11" },
    { BBItem_SRANK_WINDMILL_12, "SRANK WINDMILL 12" },
    { BBItem_SRANK_WINDMILL_13, "SRANK WINDMILL 13" },
    { BBItem_SRANK_WINDMILL_14, "SRANK WINDMILL 14" },
    { BBItem_SRANK_WINDMILL_15, "SRANK WINDMILL 15" },
    { BBItem_SRANK_WINDMILL_16, "SRANK WINDMILL 16" },
    { BBItem_SRANK_HARISEN_0, "SRANK HARISEN 0" },
    { BBItem_SRANK_HARISEN_1, "SRANK HARISEN 1" },
    { BBItem_SRANK_HARISEN_2, "SRANK HARISEN 2" },
    { BBItem_SRANK_HARISEN_3, "SRANK HARISEN 3" },
    { BBItem_SRANK_HARISEN_4, "SRANK HARISEN 4" },
    { BBItem_SRANK_HARISEN_5, "SRANK HARISEN 5" },
    { BBItem_SRANK_HARISEN_6, "SRANK HARISEN 6" },
    { BBItem_SRANK_HARISEN_7, "SRANK HARISEN 7" },
    { BBItem_SRANK_HARISEN_8, "SRANK HARISEN 8" },
    { BBItem_SRANK_HARISEN_9, "SRANK HARISEN 9" },
    { BBItem_SRANK_HARISEN_10, "SRANK HARISEN 10" },
    { BBItem_SRANK_HARISEN_11, "SRANK HARISEN 11" },
    { BBItem_SRANK_HARISEN_12, "SRANK HARISEN 12" },
    { BBItem_SRANK_HARISEN_13, "SRANK HARISEN 13" },
    { BBItem_SRANK_HARISEN_14, "SRANK HARISEN 14" },
    { BBItem_SRANK_HARISEN_15, "SRANK HARISEN 15" },
    { BBItem_SRANK_HARISEN_16, "SRANK HARISEN 16" },
    { BBItem_SRANK_KATANA_0, "SRANK KATANA 0" },
    { BBItem_SRANK_KATANA_1, "SRANK KATANA 1" },
    { BBItem_SRANK_KATANA_2, "SRANK KATANA 2" },
    { BBItem_SRANK_KATANA_3, "SRANK KATANA 3" },
    { BBItem_SRANK_KATANA_4, "SRANK KATANA 4" },
    { BBItem_SRANK_KATANA_5, "SRANK KATANA 5" },
    { BBItem_SRANK_KATANA_6, "SRANK KATANA 6" },
    { BBItem_SRANK_KATANA_7, "SRANK KATANA 7" },
    { BBItem_SRANK_KATANA_8, "SRANK KATANA 8" },
    { BBItem_SRANK_KATANA_9, "SRANK KATANA 9" },
    { BBItem_SRANK_KATANA_10, "SRANK KATANA 10" },
    { BBItem_SRANK_KATANA_11, "SRANK KATANA 11" },
    { BBItem_SRANK_KATANA_12, "SRANK KATANA 12" },
    { BBItem_SRANK_KATANA_13, "SRANK KATANA 13" },
    { BBItem_SRANK_KATANA_14, "SRANK KATANA 14" },
    { BBItem_SRANK_KATANA_15, "SRANK KATANA 15" },
    { BBItem_SRANK_KATANA_16, "SRANK KATANA 16" },
    { BBItem_SRANK_J_CUTTER_0, "SRANK J-CUTTER 0" },
    { BBItem_SRANK_J_CUTTER_1, "SRANK J-CUTTER 1" },
    { BBItem_SRANK_J_CUTTER_2, "SRANK J-CUTTER 2" },
    { BBItem_SRANK_J_CUTTER_3, "SRANK J-CUTTER 3" },
    { BBItem_SRANK_J_CUTTER_4, "SRANK J-CUTTER 4" },
    { BBItem_SRANK_J_CUTTER_5, "SRANK J-CUTTER 5" },
    { BBItem_SRANK_J_CUTTER_6, "SRANK J-CUTTER 6" },
    { BBItem_SRANK_J_CUTTER_7, "SRANK J-CUTTER 7" },
    { BBItem_SRANK_J_CUTTER_8, "SRANK J-CUTTER 8" },
    { BBItem_SRANK_J_CUTTER_9, "SRANK J-CUTTER 9" },
    { BBItem_SRANK_J_CUTTER_10, "SRANK J-CUTTER 10" },
    { BBItem_SRANK_J_CUTTER_11, "SRANK J-CUTTER 11" },
    { BBItem_SRANK_J_CUTTER_12, "SRANK J-CUTTER 12" },
    { BBItem_SRANK_J_CUTTER_13, "SRANK J-CUTTER 13" },
    { BBItem_SRANK_J_CUTTER_14, "SRANK J-CUTTER 14" },
    { BBItem_SRANK_J_CUTTER_15, "SRANK J-CUTTER 15" },
    { BBItem_SRANK_J_CUTTER_16, "SRANK J-CUTTER 16" },
    { BBItem_MUSASHI, "MUSASHI" },
    { BBItem_YAMATO, "YAMATO" },
    { BBItem_ASUKA, "ASUKA" },
    { BBItem_SANGE_and_YASHA, "SANGE & YASHA" },
    { BBItem_SANGE, "SANGE" },
    { BBItem_YASHA, "YASHA" },
    { BBItem_KAMUI, "KAMUI" },
    { BBItem_PHOTON_LAUNCHER, "PHOTON LAUNCHER" },
    { BBItem_GUILTY_LIGHT, "GUILTY LIGHT" },
    { BBItem_RED_SCORPIO, "RED SCORPIO" },
    { BBItem_PHONON_MASER, "PHONON MASER" },
    { BBItem_TALIS, "TALIS" },
    { BBItem_MAHU, "MAHU" },
    { BBItem_HITOGATA, "HITOGATA" },
    { BBItem_DANCING_HITOGATA, "DANCING HITOGATA" },
    { BBItem_KUNAI, "KUNAI" },
    { BBItem_NUG2000_BAZOOKA, "NUG2000-BAZOOKA" },
    { BBItem_S_BERILLS_HANDS_0, "S-BERILLS HANDS #0" },
    { BBItem_S_BERILLS_HANDS_1, "S-BERILLS HANDS #1" },
    { BBItem_FLOWENS_SWORD_3060, "FLOWENS SWORD 3060" },
    { BBItem_FLOWENS_SWORD_3064, "FLOWENS SWORD 3064" },
    { BBItem_FLOWENS_SWORD_3067, "FLOWENS SWORD 3067" },
    { BBItem_FLOWENS_SWORD_3073, "FLOWENS SWORD 3073" },
    { BBItem_FLOWENS_SWORD_3077, "FLOWENS SWORD 3077" },
    { BBItem_FLOWENS_SWORD_3082, "FLOWENS SWORD 3082" },
    { BBItem_FLOWENS_SWORD_3083, "FLOWENS SWORD 3083" },
    { BBItem_FLOWENS_SWORD_3084, "FLOWENS SWORD 3084" },
    { BBItem_FLOWENS_SWORD_3079, "FLOWENS SWORD 3079" },
    { BBItem_DBS_SABER_3062, "DBS SABER 3062" },
    { BBItem_DBS_SABER_3067, "DBS SABER 3067" },
    { BBItem_DBS_SABER_3069_2, "DBS SABER 3069" },
    { BBItem_DBS_SABER_3064, "DBS SABER 3064" },
    { BBItem_DBS_SABER_3069_4, "DBS SABER 3069" },
    { BBItem_DBS_SABER_3073, "DBS SABER 3073" },
    { BBItem_DBS_SABER_3070, "DBS SABER 3070" },
    { BBItem_DBS_SABER_3075, "DBS SABER 3075" },
    { BBItem_DBS_SABER_3077, "DBS SABER 3077" },
    { BBItem_GI_GUE_BAZOOKA, "GI GUE BAZOOKA" },
    { BBItem_GUARDIANNA, "GUARDIANNA" },
    { BBItem_VIRIDIA_CARD, "VIRIDIA CARD" },
    { BBItem_GREENILL_CARD, "GREENILL CARD" },
    { BBItem_SKYLY_CARD, "SKYLY CARD" },
    { BBItem_BLUEFULL_CARD, "BLUEFULL CARD" },
    { BBItem_PURPLENUM_CARD, "PURPLENUM CARD" },
    { BBItem_PINKAL_CARD, "PINKAL CARD" },
    { BBItem_REDRIA_CARD, "REDRIA CARD" },
    { BBItem_ORAN_CARD, "ORAN CARD" },
    { BBItem_YELLOWBOZE_CARD, "YELLOWBOZE CARD" },
    { BBItem_WHITILL_CARD, "WHITILL CARD" },
    { BBItem_MORNING_GLORY, "MORNING GLORY" },
    { BBItem_PARTISAN_of_LIGHTNING, "PARTISAN of LIGHTNING" },
    { BBItem_GAL_WIND, "GAL WIND" },
    { BBItem_ZANBA, "ZANBA" },
    { BBItem_RIKAS_CLAW, "RIKAS CLAW" },
    { BBItem_ANGEL_HARP, "ANGEL HARP" },
    { BBItem_DEMOLITION_COMET, "DEMOLITION COMET" },
    { BBItem_NEIS_CLAW_0, "NEIS CLAW" },
    { BBItem_RAINBOW_BATON, "RAINBOW BATON" },
    { BBItem_DARK_FLOW, "DARK FLOW" },
    { BBItem_DARK_METEOR, "DARK METEOR" },
    { BBItem_DARK_BRIDGE, "DARK BRIDGE" },
    { BBItem_G_ASSASSINS_SABERS, "G-ASSASSINS SABERS" },
    { BBItem_RAPPYS_FAN, "RAPPYS FAN" },
    { BBItem_BOOMAS_CLAW, "BOOMAS CLAW" },
    { BBItem_GOBOOMAS_CLAW, "GOBOOMAS CLAW" },
    { BBItem_GIGOBOOMAS_CLAW, "GIGOBOOMAS CLAW" },
    { BBItem_RUBY_BULLET, "RUBY BULLET" },
    { BBItem_AMORE_ROSE, "AMORE ROSE" },
    { BBItem_SRANK_SWORDS_0, "SRANK SWORDS 0" },
    { BBItem_SRANK_SWORDS_1, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_2, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_3, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_4, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_5, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_6, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_7, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_8, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_9, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0A, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0B, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0C, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0D, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0E, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0F, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_10, "SRANK SWORDS" },
    { BBItem_SRANK_LAUNCHER_0, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_1, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_2, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_3, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_4, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_5, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_6, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_7, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_8, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_9, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0A, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0B, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0C, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0D, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0E, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0F, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_10, "SRANK LAUNCHER" },
    { BBItem_SRANK_CARDS_0, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_1, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_2, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_3, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_4, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_5, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_6, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_7, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_8, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_9, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0A, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0B, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0C, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0D, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0E, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0F, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_10, "SRANK CARDS" },
    { BBItem_SRANK_KNUCKLE_0, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_1, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_2, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_3, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_4, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_5, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_6, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_7, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_8, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_9, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0A, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0B, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0C, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0D, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0E, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0F, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_10, "SRANK KNUCKLE" },
    { BBItem_SRANK_AXE_0, "SRANK AXE" },
    { BBItem_SRANK_AXE_1, "SRANK AXE" },
    { BBItem_SRANK_AXE_2, "SRANK AXE" },
    { BBItem_SRANK_AXE_3, "SRANK AXE" },
    { BBItem_SRANK_AXE_4, "SRANK AXE" },
    { BBItem_SRANK_AXE_5, "SRANK AXE" },
    { BBItem_SRANK_AXE_6, "SRANK AXE" },
    { BBItem_SRANK_AXE_7, "SRANK AXE" },
    { BBItem_SRANK_AXE_8, "SRANK AXE" },
    { BBItem_SRANK_AXE_9, "SRANK AXE" },
    { BBItem_SRANK_AXE_0A, "SRANK AXE" },
    { BBItem_SRANK_AXE_0B, "SRANK AXE" },
    { BBItem_SRANK_AXE_0C, "SRANK AXE" },
    { BBItem_SRANK_AXE_0D, "SRANK AXE" },
    { BBItem_SRANK_AXE_0E, "SRANK AXE" },
    { BBItem_SRANK_AXE_0F, "SRANK AXE" },
    { BBItem_SRANK_AXE_10, "SRANK AXE" },
    { BBItem_SLICER_OF_FANATIC, "SLICER OF FANATIC" },
    { BBItem_LAME_DARGENT, "LAME DARGENT" },
    { BBItem_EXCALIBUR, "EXCALIBUR" },
    { BBItem_RAGE_DE_FEU_0, "RAGE DE FEU" },
    { BBItem_RAGE_DE_FEU_1, "RAGE DE FEU" },
    { BBItem_RAGE_DE_FEU_2, "RAGE DE FEU" },
    { BBItem_RAGE_DE_FEU_3, "RAGE DE FEU" },
    { BBItem_DAISY_CHAIN, "DAISY CHAIN" },
    { BBItem_OPHELIE_SEIZE, "OPHELIE SEIZE" },
    { BBItem_MILLE_MARTEAUX, "MILLE MARTEAUX" },
    { BBItem_LE_COGNEUR, "LE COGNEUR" },
    { BBItem_COMMANDER_BLADE, "COMMANDER BLADE" },
    { BBItem_VIVIENNE, "VIVIENNE" },
    { BBItem_KUSANAGI, "KUSANAGI" },
    { BBItem_SACRED_DUSTER, "SACRED DUSTER" },
    { BBItem_GUREN, "GUREN" },
    { BBItem_SHOUREN, "SHOUREN" },
    { BBItem_JIZAI, "JIZAI" },
    { BBItem_FLAMBERGE, "FLAMBERGE" },
    { BBItem_YUNCHANG, "YUNCHANG" },
    { BBItem_SNAKE_SPIRE, "SNAKE SPIRE" },
    { BBItem_FLAPJACK_FLAPPER, "FLAPJACK FLAPPER" },
    { BBItem_GETSUGASAN, "GETSUGASAN" },
    { BBItem_MAGUWA, "MAGUWA" },
    { BBItem_HEAVEN_STRIKER, "HEAVEN STRIKER" },
    { BBItem_CANNON_ROUGE, "CANNON ROUGE" },
    { BBItem_METEOR_ROUGE, "METEOR ROUGE" },
    { BBItem_SOLFERINO, "SOLFERINO" },
    { BBItem_CLIO, "CLIO" },
    { BBItem_SIREN_GLASS_HAMMER, "SIREN GLASS HAMMER" },
    { BBItem_GLIDE_DIVINE, "GLIDE DIVINE" },
    { BBItem_SHICHISHITO, "SHICHISHITO" },
    { BBItem_MURASAME, "MURASAME" },
    { BBItem_DAYLIGHT_SCAR, "DAYLIGHT SCAR" },
    { BBItem_DECALOG, "DECALOG" },
    { BBItem_5TH_ANNIV_BLADE, "5TH ANNIV. BLADE" },
    { BBItem_TYRELLS_PARASOL, "TYRELLS PARASOL" },
    { BBItem_AKIKOS_CLEAVER, "AKIKOS CLEAVER" },
    { BBItem_TANEGASHIMA, "TANEGASHIMA" },
    { BBItem_TREE_CLIPPERS, "TREE CLIPPERS" },
    { BBItem_NICE_SHOT, "NICE SHOT" },
    { BBItem_UNKNOWN3, "UNKNOWN3" },
    { BBItem_UNKNOWN4, "UNKNOWN4" },
    { BBItem_ANO_BAZOOKA, "ANO BAZOOKA" },
    { BBItem_SYNTHESIZER, "SYNTHESIZER" },
    { BBItem_BAMBOO_SPEAR, "BAMBOO SPEAR" },
    { BBItem_KANEI_TSUHO, "KANEI TSUHO" },
    { BBItem_JITTE, "JITTE" },
    { BBItem_BUTTERFLY_NET, "BUTTERFLY NET" },
    { BBItem_SYRINGE, "SYRINGE" },
    { BBItem_BATTLEDORE, "BATTLEDORE" },
    { BBItem_RACKET, "RACKET" },
    { BBItem_HAMMER, "HAMMER" },
    { BBItem_GREAT_BOUQUET, "GREAT BOUQUET" },
    { BBItem_TypeSA_SABER, "TypeSA/SABER" },
    { BBItem_TypeSL_SABER, "TypeSL/SABER" },
    { BBItem_TypeSL_SLICER, "TypeSL/SLICER" },
    { BBItem_TypeSL_CLAW, "TypeSL/CLAW" },
    { BBItem_TypeSL_KATANA, "TypeSL/KATANA" },
    { BBItem_TypeJS_SABER, "TypeJS/SABER" },
    { BBItem_TypeJS_SLICER, "TypeJS/SLICER" },
    { BBItem_TypeJS_J_SWORD, "TypeJS/J-SWORD" },
    { BBItem_TypeSW_SWORD, "TypeSW/SWORD" },
    { BBItem_TypeSW_SLICER, "TypeSW/SLICER" },
    { BBItem_TypeSW_J_SWORD, "TypeSW/J-SWORD" },
    { BBItem_TypeRO_SWORD, "TypeRO/SWORD" },
    { BBItem_TypeRO_HALBERT, "TypeRO/HALBERT" },
    { BBItem_TypeRO_ROD, "TypeRO/ROD" },
    { BBItem_TypeBL_BLADE, "TypeBL/BLADE" },
    { BBItem_TypeKN_BLADE, "TypeKN/BLADE" },
    { BBItem_TypeKN_CLAW, "TypeKN/CLAW" },
    { BBItem_TypeHA_HALBERT, "TypeHA/HALBERT" },
    { BBItem_TypeHA_ROD, "TypeHA/ROD" },
    { BBItem_TypeDS_DSABER, "TypeDS/D.SABER" },
    { BBItem_TypeDS_ROD, "TypeDS/ROD" },
    { BBItem_TypeDS, "TypeDS" },
    { BBItem_TypeCL_CLAW, "TypeCL/CLAW" },
    { BBItem_TypeSS_SW, "TypeSS/SW" },
    { BBItem_TypeGU_HAND, "TypeGU/HAND" },
    { BBItem_TypeGU_MECHGUN, "TypeGU/MECHGUN" },
    { BBItem_TypeRI_RIFLE, "TypeRI/RIFLE" },
    { BBItem_TypeME_MECHGUN, "TypeME/MECHGUN" },
    { BBItem_TypeSH_SHOT, "TypeSH/SHOT" },
    { BBItem_TypeWA_WAND, "TypeWA/WAND" },
    { BBItem_UNK_WEAPON, "UNK WEAPON" },
    { BBItem_Frame, "Frame" },
    { BBItem_Armor, "Armor" },
    { BBItem_Psy_Armor, "Psy Armor" },
    { BBItem_Giga_Frame, "Giga Frame" },
    { BBItem_Soul_Frame, "Soul Frame" },
    { BBItem_Cross_Armor, "Cross Armor" },
    { BBItem_Solid_Frame, "Solid Frame" },
    { BBItem_Brave_Armor, "Brave Armor" },
    { BBItem_Hyper_Frame, "Hyper Frame" },
    { BBItem_Grand_Armor, "Grand Armor" },
    { BBItem_Shock_Frame, "Shock Frame" },
    { BBItem_Kings_Frame, "Kings Frame" },
    { BBItem_Dragon_Frame, "Dragon Frame" },
    { BBItem_Absorb_Armor, "Absorb Armor" },
    { BBItem_Protect_Frame, "Protect Frame" },
    { BBItem_General_Armor, "General Armor" },
    { BBItem_Perfect_Frame, "Perfect Frame" },
    { BBItem_Valiant_Frame, "Valiant Frame" },
    { BBItem_Imperial_Armor, "Imperial Armor" },
    { BBItem_Holiness_Armor, "Holiness Armor" },
    { BBItem_Guardian_Armor, "Guardian Armor" },
    { BBItem_Divinity_Armor, "Divinity Armor" },
    { BBItem_Ultimate_Frame, "Ultimate Frame" },
    { BBItem_Celestial_Armor, "Celestial Armor" },
    { BBItem_HUNTER_FIELD, "HUNTER FIELD" },
    { BBItem_RANGER_FIELD, "RANGER FIELD" },
    { BBItem_FORCE_FIELD, "FORCE FIELD" },
    { BBItem_REVIVAL_GARMENT, "REVIVAL GARMENT" },
    { BBItem_SPIRIT_GARMENT, "SPIRIT GARMENT" },
    { BBItem_STINK_FRAME, "STINK FRAME" },
    { BBItem_D_PARTS_ver1_01, "D-PARTS ver1.01" },
    { BBItem_D_PARTS_ver2_10, "D-PARTS ver2.10" },
    { BBItem_PARASITE_WEAR_De_Rol, "PARASITE WEAR:De Rol" },
    { BBItem_PARASITE_WEAR_Nelgal, "PARASITE WEAR:Nelgal" },
    { BBItem_PARASITE_WEAR_Vajulla, "PARASITE WEAR:Vajulla" },
    { BBItem_SENSE_PLATE, "SENSE PLATE" },
    { BBItem_GRAVITON_PLATE, "GRAVITON PLATE" },
    { BBItem_ATTRIBUTE_PLATE, "ATTRIBUTE PLATE" },
    { BBItem_FLOWENS_FRAME, "FLOWENS FRAME" },
    { BBItem_CUSTOM_FRAME_ver_OO, "CUSTOM FRAME ver.OO" },
    { BBItem_DBS_ARMOR, "DBS ARMOR" },
    { BBItem_GUARD_WAVE, "GUARD WAVE" },
    { BBItem_DF_FIELD, "DF FIELD" },
    { BBItem_LUMINOUS_FIELD, "LUMINOUS FIELD" },
    { BBItem_CHU_CHU_FEVER, "CHU CHU FEVER" },
    { BBItem_LOVE_HEART, "LOVE HEART" },
    { BBItem_FLAME_GARMENT, "FLAME GARMENT" },
    { BBItem_VIRUS_ARMOR_Lafuteria, "VIRUS ARMOR:Lafuteria" },
    { BBItem_BRIGHTNESS_CIRCLE, "BRIGHTNESS CIRCLE" },
    { BBItem_AURA_FIELD, "AURA FIELD" },
    { BBItem_ELECTRO_FRAME, "ELECTRO FRAME" },
    { BBItem_SACRED_CLOTH, "SACRED CLOTH" },
    { BBItem_SMOKING_PLATE, "SMOKING PLATE" },
    { BBItem_STAR_CUIRASS, "STAR CUIRASS" },
    { BBItem_BLACK_HOUND_CUIRASS, "BLACK HOUND CUIRASS" },
    { BBItem_MORNING_PRAYER, "MORNING PRAYER" },
    { BBItem_BLACK_ODOSHI_DOMARU, "BLACK ODOSHI DOMARU" },
    { BBItem_RED_ODOSHI_DOMARU, "RED ODOSHI DOMARU" },
    { BBItem_BLACK_ODOSHI_RED_NIMAIDOU, "BLACK ODOSHI RED NIMAIDOU" },
    { BBItem_BLUE_ODOSHI_VIOLET_NIMAIDOU, "BLUE ODOSHI VIOLET NIMAIDOU" },
    { BBItem_DIRTY_LIFEJACKET, "DIRTY LIFEJACKET" },
    { BBItem_KROES_SWEATER, "KROES SWEATER" },
    { BBItem_WEDDING_DRESS, "WEDDING DRESS" },
    { BBItem_SONICTEAM_ARMOR, "SONICTEAM ARMOR" },
    { BBItem_RED_COAT, "RED COAT" },
    { BBItem_THIRTEEN, "THIRTEEN" },
    { BBItem_MOTHER_GARB, "MOTHER GARB" },
    { BBItem_MOTHER_GARB_PLUS, "MOTHER GARB+" },
    { BBItem_DRESS_PLATE, "DRESS PLATE" },
    { BBItem_SWEETHEART, "SWEETHEART" },
    { BBItem_IGNITION_CLOAK, "IGNITION CLOAK" },
    { BBItem_CONGEAL_CLOAK, "CONGEAL CLOAK" },
    { BBItem_TEMPEST_CLOAK, "TEMPEST CLOAK" },
    { BBItem_CURSED_CLOAK, "CURSED CLOAK" },
    { BBItem_SELECT_CLOAK, "SELECT CLOAK" },
    { BBItem_SPIRIT_CUIRASS, "SPIRIT CUIRASS" },
    { BBItem_REVIVAL_CURIASS, "REVIVAL CURIASS" },
    { BBItem_ALLIANCE_UNIFORM, "ALLIANCE UNIFORM" },
    { BBItem_OFFICER_UNIFORM, "OFFICER UNIFORM" },
    { BBItem_COMMANDER_UNIFORM, "COMMANDER UNIFORM" },
    { BBItem_CRIMSON_COAT, "CRIMSON COAT" },
    { BBItem_INFANTRY_GEAR, "INFANTRY GEAR" },
    { BBItem_LIEUTENANT_GEAR, "LIEUTENANT GEAR" },
    { BBItem_INFANTRY_MANTLE, "INFANTRY MANTLE" },
    { BBItem_LIEUTENANT_MANTLE, "LIEUTENANT MANTLE" },
    { BBItem_UNION_FIELD, "UNION FIELD" },
    { BBItem_SAMURAI_ARMOR, "SAMURAI ARMOR" },
    { BBItem_STEALTH_SUIT, "STEALTH SUIT" },
    { BBItem_UNK_ARMOR, "UNK ARMOR" },
    { BBItem_Barrier_0, "Barrier" },
    { BBItem_Shield, "Shield" },
    { BBItem_Core_Shield, "Core Shield" },
    { BBItem_Giga_Shield, "Giga Shield" },
    { BBItem_Soul_Barrier, "Soul Barrier" },
    { BBItem_Hard_Shield, "Hard Shield" },
    { BBItem_Brave_Barrier, "Brave Barrier" },
    { BBItem_Solid_Shield, "Solid Shield" },
    { BBItem_Flame_Barrier, "Flame Barrier" },
    { BBItem_Plasma_Barrier, "Plasma Barrier" },
    { BBItem_Freeze_Barrier, "Freeze Barrier" },
    { BBItem_Psychic_Barrier, "Psychic Barrier" },
    { BBItem_General_Shield, "General Shield" },
    { BBItem_Protect_Barrier, "Protect Barrier" },
    { BBItem_Glorious_Shield, "Glorious Shield" },
    { BBItem_Imperial_Barrier, "Imperial Barrier" },
    { BBItem_Guardian_Shield, "Guardian Shield" },
    { BBItem_Divinity_Barrier, "Divinity Barrier" },
    { BBItem_Ultimate_Shield, "Ultimate Shield" },
    { BBItem_Spiritual_Shield, "Spiritual Shield" },
    { BBItem_Celestial_Shield, "Celestial Shield" },
    { BBItem_INVISIBLE_GUARD, "INVISIBLE GUARD" },
    { BBItem_SACRED_GUARD, "SACRED GUARD" },
    { BBItem_S_PARTS_ver1_16, "S-PARTS ver1.16" },
    { BBItem_S_PARTS_ver2_01, "S-PARTS ver2.01" },
    { BBItem_LIGHT_RELIEF, "LIGHT RELIEF" },
    { BBItem_SHIELD_OF_DELSABER, "SHIELD OF DELSABER" },
    { BBItem_FORCE_WALL, "FORCE WALL" },
    { BBItem_RANGER_WALL, "RANGER WALL" },
    { BBItem_HUNTER_WALL, "HUNTER WALL" },
    { BBItem_ATTRIBUTE_WALL, "ATTRIBUTE WALL" },
    { BBItem_SECRET_GEAR, "SECRET GEAR" },
    { BBItem_COMBAT_GEAR, "COMBAT GEAR" },
    { BBItem_PROTO_REGENE_GEAR, "PROTO REGENE GEAR" },
    { BBItem_REGENERATE_GEAR, "REGENERATE GEAR" },
    { BBItem_REGENE_GEAR_ADV, "REGENE GEAR ADV." },
    { BBItem_FLOWENS_SHIELD, "FLOWENS SHIELD" },
    { BBItem_CUSTOM_BARRIER_ver_OO, "CUSTOM BARRIER ver.OO" },
    { BBItem_DBS_SHIELD, "DBS SHIELD" },
    { BBItem_RED_RING, "RED RING" },
    { BBItem_TRIPOLIC_SHIELD, "TRIPOLIC SHIELD" },
    { BBItem_STANDSTILL_SHIELD, "STANDSTILL SHIELD" },
    { BBItem_SAFETY_HEART, "SAFETY HEART" },
    { BBItem_KASAMI_BRACER, "KASAMI BRACER" },
    { BBItem_GODS_SHIELD_SUZAKU, "GODS SHIELD SUZAKU" },
    { BBItem_GODS_SHIELD_GENBU, "GODS SHIELD GENBU" },
    { BBItem_GODS_SHIELD_BYAKKO, "GODS SHIELD BYAKKO" },
    { BBItem_GODS_SHIELD_SEIRYU, "GODS SHIELD SEIRYU" },
    { BBItem_HANTERS_SHELL, "HUNTERS SHELL" },
    { BBItem_RIKOS_GLASSES, "RIKOS GLASSES" },
    { BBItem_RIKOS_EARRING, "RIKOS EARRING" },
    { BBItem_BLUE_RING_33, "BLUE RING" },
    { BBItem_Barrier_34, "Barrier" },
    { BBItem_SECURE_FEET, "SECURE FEET" },
    { BBItem_Barrier_36, "Barrier" },
    { BBItem_Barrier_37, "Barrier" },
    { BBItem_Barrier_38, "Barrier" },
    { BBItem_Barrier_39, "Barrier" },
    { BBItem_RESTA_MERGE, "RESTA MERGE" },
    { BBItem_ANTI_MERGE, "ANTI MERGE" },
    { BBItem_SHIFTA_MERGE, "SHIFTA MERGE" },
    { BBItem_DEBAND_MERGE, "DEBAND MERGE" },
    { BBItem_FOIE_MERGE, "FOIE MERGE" },
    { BBItem_GIFOIE_MERGE, "GIFOIE MERGE" },
    { BBItem_RAFOIE_MERGE, "RAFOIE MERGE" },
    { BBItem_RED_MERGE, "RED MERGE" },
    { BBItem_BARTA_MERGE, "BARTA MERGE" },
    { BBItem_GIBARTA_MERGE, "GIBARTA MERGE" },
    { BBItem_RABARTA_MERGE, "RABARTA MERGE" },
    { BBItem_BLUE_MERGE, "BLUE MERGE" },
    { BBItem_ZONDE_MERGE, "ZONDE MERGE" },
    { BBItem_GIZONDE_MERGE, "GIZONDE MERGE" },
    { BBItem_RAZONDE_MERGE, "RAZONDE MERGE" },
    { BBItem_YELLOW_MERGE, "YELLOW MERGE" },
    { BBItem_RECOVERY_BARRIER, "RECOVERY BARRIER" },
    { BBItem_ASSIST__BARRIER, "ASSIST  BARRIER" },
    { BBItem_RED_BARRIER, "RED BARRIER" },
    { BBItem_BLUE_BARRIER, "BLUE BARRIER" },
    { BBItem_YELLOW_BARRIER, "YELLOW BARRIER" },
    { BBItem_WEAPONS_GOLD_SHIELD, "WEAPONS GOLD SHIELD" },
    { BBItem_BLACK_GEAR, "BLACK GEAR" },
    { BBItem_WORKS_GUARD, "WORKS GUARD" },
    { BBItem_RAGOL_RING, "RAGOL RING" },
    { BBItem_BLUE_RING_53, "BLUE RING" },
    { BBItem_BLUE_RING_54, "BLUE RING" },
    { BBItem_BLUE_RING_55, "BLUE RING" },
    { BBItem_BLUE_RING_56, "BLUE RING" },
    { BBItem_BLUE_RING_57, "BLUE RING" },
    { BBItem_BLUE_RING_58, "BLUE RING" },
    { BBItem_BLUE_RING_59, "BLUE RING" },
    { BBItem_BLUE_RING_5A, "BLUE RING" },
    { BBItem_GREEN_RING_5B, "GREEN RING" },
    { BBItem_GREEN_RING_5C, "GREEN RING" },
    { BBItem_GREEN_RING_5D, "GREEN RING" },
    { BBItem_GREEN_RING_5E, "GREEN RING" },
    { BBItem_GREEN_RING_5F, "GREEN RING" },
    { BBItem_GREEN_RING_60, "GREEN RING" },
    { BBItem_GREEN_RING_61, "GREEN RING" },
    { BBItem_GREEN_RING_62, "GREEN RING" },
    { BBItem_YELLOW_RING_63, "YELLOW RING" },
    { BBItem_YELLOW_RING_64, "YELLOW RING" },
    { BBItem_YELLOW_RING_65, "YELLOW RING" },
    { BBItem_YELLOW_RING_66, "YELLOW RING" },
    { BBItem_YELLOW_RING_67, "YELLOW RING" },
    { BBItem_YELLOW_RING_68, "YELLOW RING" },
    { BBItem_YELLOW_RING_69, "YELLOW RING" },
    { BBItem_YELLOW_RING_6A, "YELLOW RING" },
    { BBItem_PURPLE_RING_6B, "PURPLE RING" },
    { BBItem_PURPLE_RING_6C, "PURPLE RING" },
    { BBItem_PURPLE_RING_6D, "PURPLE RING" },
    { BBItem_PURPLE_RING_6E, "PURPLE RING" },
    { BBItem_PURPLE_RING_6F, "PURPLE RING" },
    { BBItem_PURPLE_RING_70, "PURPLE RING" },
    { BBItem_PURPLE_RING_71, "PURPLE RING" },
    { BBItem_PURPLE_RING_72, "PURPLE RING" },
    { BBItem_ANTI_DARK_RING, "ANTI-DARK RING" },
    { BBItem_WHITE_RING_74, "WHITE RING" },
    { BBItem_WHITE_RING_75, "WHITE RING" },
    { BBItem_WHITE_RING_76, "WHITE RING" },
    { BBItem_WHITE_RING_77, "WHITE RING" },
    { BBItem_WHITE_RING_78, "WHITE RING" },
    { BBItem_WHITE_RING_79, "WHITE RING" },
    { BBItem_WHITE_RING_7A, "WHITE RING" },
    { BBItem_ANTI_LIGHT_RING, "ANTI-LIGHT RING" },
    { BBItem_BLACK_RING_7C, "BLACK RING" },
    { BBItem_BLACK_RING_7D, "BLACK RING" },
    { BBItem_BLACK_RING_7E, "BLACK RING" },
    { BBItem_BLACK_RING_7F, "BLACK RING" },
    { BBItem_BLACK_RING_80, "BLACK RING" },
    { BBItem_BLACK_RING_81, "BLACK RING" },
    { BBItem_BLACK_RING_82, "BLACK RING" },
    { BBItem_WEAPONS_SILVER_SHIELD, "WEAPONS SILVER SHIELD" },
    { BBItem_WEAPONS_COPPER_SHIELD, "WEAPONS COPPER SHIELD" },
    { BBItem_GRATIA, "GRATIA" },
    { BBItem_TRIPOLIC_REFLECTOR, "TRIPOLIC REFLECTOR" },
    { BBItem_STRIKER_PLUS, "STRIKER PLUS" },
    { BBItem_REGENERATE_GEAR_BP, "REGENERATE GEAR B.P." },
    { BBItem_RUPIKA, "RUPIKA" },
    { BBItem_YATA_MIRROR, "YATA MIRROR" },
    { BBItem_BUNNY_EARS, "BUNNY EARS" },
    { BBItem_CAT_EARS, "CAT EARS" },
    { BBItem_THREE_SEALS, "THREE SEALS" },
    { BBItem_GODS_SHIELD_KOURYU, "GODS SHIELD KOURYU" },
    { BBItem_DF_SHIELD, "DF SHIELD" },
    { BBItem_FROM_THE_DEPTHS, "FROM THE DEPTHS" },
    { BBItem_DE_ROL_LE_SHIELD, "DE ROL LE SHIELD" },
    { BBItem_HONEYCOMB_REFLECTOR, "HONEYCOMB REFLECTOR" },
    { BBItem_EPSIGUARD, "EPSIGUARD" },
    { BBItem_ANGEL_RING, "ANGEL RING" },
    { BBItem_UNION_GUARD_95, "UNION GUARD" },
    { BBItem_UNION_GUARD_96, "UNION GUARD" },
    { BBItem_UNION_GUARD_97, "UNION GUARD" },
    { BBItem_UNION_GUARD_98, "UNION GUARD" },
    { BBItem_STINK_SHIELD, "STINK SHIELD" },
    { BBItem_BLACK_GAUNTLETS, "BLACK GAUNTLETS" },
    { BBItem_GENPEI_9B, "GENPEI" },
    { BBItem_GENPEI_9C, "GENPEI" },
    { BBItem_GENPEI_9D, "GENPEI" },
    { BBItem_GENPEI_9E, "GENPEI" },
    { BBItem_GENPEI_9F, "GENPEI" },
    { BBItem_GENPEI_A0, "GENPEI" },
    { BBItem_GENPEI_A1, "GENPEI" },
    { BBItem_GENPEI_A2, "GENPEI" },
    { BBItem_GENPEI_A3, "GENPEI" },
    { BBItem_GENPEI_A4, "GENPEI" },
    { BBItem_UNK_SHIELD, "UNK SHIELD" },
    { BBItem_Mag, "Mag" },
    { BBItem_Varuna, "Varuna" },
    { BBItem_Mitra, "Mitra" },
    { BBItem_Surya, "Surya" },
    { BBItem_Vayu, "Vayu" },
    { BBItem_Varaha, "Varaha" },
    { BBItem_Kama, "Kama" },
    { BBItem_Ushasu, "Ushasu" },
    { BBItem_Apsaras, "Apsaras" },
    { BBItem_Kumara, "Kumara" },
    { BBItem_Kaitabha, "Kaitabha" },
    { BBItem_Tapas, "Tapas" },
    { BBItem_Bhirava, "Bhirava" },
    { BBItem_Kalki, "Kalki" },
    { BBItem_Rudra, "Rudra" },
    { BBItem_Marutah, "Marutah" },
    { BBItem_Yaksa, "Yaksa" },
    { BBItem_Sita, "Sita" },
    { BBItem_Garuda, "Garuda" },
    { BBItem_Nandin, "Nandin" },
    { BBItem_Ashvinau, "Ashvinau" },
    { BBItem_Ribhava, "Ribhava" },
    { BBItem_Soma, "Soma" },
    { BBItem_Ila, "Ila" },
    { BBItem_Durga, "Durga" },
    { BBItem_Vritra, "Vritra" },
    { BBItem_Namuci, "Namuci" },
    { BBItem_Sumba, "Sumba" },
    { BBItem_Naga, "Naga" },
    { BBItem_Pitri, "Pitri" },
    { BBItem_Kabanda, "Kabanda" },
    { BBItem_Ravana, "Ravana" },
    { BBItem_Marica, "Marica" },
    { BBItem_Soniti, "Soniti" },
    { BBItem_Preta, "Preta" },
    { BBItem_Andhaka, "Andhaka" },
    { BBItem_Bana, "Bana" },
    { BBItem_Naraka, "Naraka" },
    { BBItem_Madhu, "Madhu" },
    { BBItem_Churel, "Churel" },
    { BBItem_ROBOCHAO, "ROBOCHAO" },
    { BBItem_OPA_OPA, "OPA-OPA" },
    { BBItem_PIAN, "PIAN" },
    { BBItem_CHAO, "CHAO" },
    { BBItem_CHU_CHU, "CHU CHU" },
    { BBItem_KAPU_KAPU, "KAPU KAPU" },
    { BBItem_ANGELS_WING, "ANGEL'S WING" },
    { BBItem_DEVILS_WING, "DEVIL'S WING" },
    { BBItem_ELENOR, "ELENOR" },
    { BBItem_MARK3, "MARK3" },
    { BBItem_MASTER_SYSTEM, "MASTER SYSTEM" },
    { BBItem_GENESIS, "GENESIS" },
    { BBItem_SEGA_SATURN, "SEGA SATURN" },
    { BBItem_DREAMCAST, "DREAMCAST" },
    { BBItem_HAMBURGER, "HAMBURGER" },
    { BBItem_PANZERS_TAIL, "PANZER'S TAIL" },
    { BBItem_DAVILS_TAIL, "DEVIL'S TAIL" },
    { BBItem_Deva, "Deva" },
    { BBItem_Rati, "Rati" },
    { BBItem_Savitri, "Savitri" },
    { BBItem_Rukmin, "Rukmin" },
    { BBItem_Pushan, "Pushan" },
    { BBItem_Diwari, "Diwari" },
    { BBItem_Sato, "Sato" },
    { BBItem_Bhima, "Bhima" },
    { BBItem_Nidra, "Nidra" },
    { BBItem_Geung_si, "Geung-si" },
    { BBItem_Unknow1, "Unknow1" },
    { BBItem_Tellusis, "Tellusis" },
    { BBItem_Striker_Unit, "Striker Unit" },
    { BBItem_Pioneer, "Pioneer" },
    { BBItem_Puyo, "Puyo" },
    { BBItem_Moro, "Moro" },
    { BBItem_Yahoo, "Yahoo!" },
    { BBItem_Rappy, "Rappy" },
    { BBItem_Gael_Giel, "Gael Giel" },
    { BBItem_Agastya, "Agastya" },
    { BBItem_Cell_of_MAG_0503_0, "Cell of MAG 0503" },
    { BBItem_Cell_of_MAG_0504_0, "Cell of MAG 0504" },
    { BBItem_Cell_of_MAG_0505_0, "Cell of MAG 0505" },
    { BBItem_Cell_of_MAG_0506_0, "Cell of MAG 0506" },
    { BBItem_Cell_of_MAG_0507_0, "Cell of MAG 0507" },
    { BBItem_UNK_MAG, "UNK MAG" },
    { BBItem_Knight_Power, "Knight/Power" },
    { BBItem_General_Power, "General/Power" },
    { BBItem_Ogre_Power, "Ogre/Power" },
    { BBItem_God_Power, "God/Power" },
    { BBItem_Priest_Mind, "Priest/Mind" },
    { BBItem_General_Mind, "General/Mind" },
    { BBItem_Angel_Mind, "Angel/Mind" },
    { BBItem_God_Mind, "God/Mind" },
    { BBItem_Marksman_Arm, "Marksman/Arm" },
    { BBItem_General_Arm, "General/Arm" },
    { BBItem_Elf_Arm, "Elf/Arm" },
    { BBItem_God_Arm, "God/Arm" },
    { BBItem_Thief_Legs, "Thief/Legs" },
    { BBItem_General_Legs, "General/Legs" },
    { BBItem_Elf_Legs, "Elf/Legs" },
    { BBItem_God_Legs, "God/Legs" },
    { BBItem_Digger_HP, "Digger/HP" },
    { BBItem_General_HP, "General/HP" },
    { BBItem_Dragon_HP, "Dragon/HP" },
    { BBItem_God_HP, "God/HP" },
    { BBItem_Magician_TP, "Magician/TP" },
    { BBItem_General_TP, "General/TP" },
    { BBItem_Angel_TP, "Angel/TP" },
    { BBItem_God_TP, "God/TP" },
    { BBItem_Warrior_Body, "Warrior/Body" },
    { BBItem_General_Body, "General/Body" },
    { BBItem_Metal_Body, "Metal/Body" },
    { BBItem_God_Body, "God/Body" },
    { BBItem_Angel_Luck, "Angel/Luck" },
    { BBItem_God_Luck, "God/Luck" },
    { BBItem_Master_Ability, "Master/Ability" },
    { BBItem_Hero_Ability, "Hero/Ability" },
    { BBItem_God_Ability, "God/Ability" },
    { BBItem_Resist_Fire, "Resist/Fire" },
    { BBItem_Resist_Flame, "Resist/Flame" },
    { BBItem_Resist_Burning, "Resist/Burning" },
    { BBItem_Resist_Cold, "Resist/Cold" },
    { BBItem_Resist_Freeze, "Resist/Freeze" },
    { BBItem_Resist_Blizzard, "Resist/Blizzard" },
    { BBItem_Resist_Shock, "Resist/Shock" },
    { BBItem_Resist_Thunder, "Resist/Thunder" },
    { BBItem_Resist_Storm, "Resist/Storm" },
    { BBItem_Resist_Light, "Resist/Light" },
    { BBItem_Resist_Saint, "Resist/Saint" },
    { BBItem_Resist_Holy, "Resist/Holy" },
    { BBItem_Resist_Dark, "Resist/Dark" },
    { BBItem_Resist_Evil, "Resist/Evil" },
    { BBItem_Resist_Devil, "Resist/Devil" },
    { BBItem_All_Resist, "All/Resist" },
    { BBItem_Super_Resist, "Super/Resist" },
    { BBItem_Perfect_Resist, "Perfect/Resist" },
    { BBItem_HP_Restorate, "HP/Restorate" },
    { BBItem_HP_Generate, "HP/Generate" },
    { BBItem_HP_Revival, "HP/Revival" },
    { BBItem_TP_Restorate, "TP/Restorate" },
    { BBItem_TP_Generate, "TP/Generate" },
    { BBItem_TP_Revival, "TP/Revival" },
    { BBItem_PB_Amplifier, "PB/Amplifier" },
    { BBItem_PB_Generate, "PB/Generate" },
    { BBItem_PB_Create, "PB/Create" },
    { BBItem_Wizard_Technique, "Wizard/Technique" },
    { BBItem_Devil_Technique, "Devil/Technique" },
    { BBItem_God_Technique, "God/Technique" },
    { BBItem_General_Battle, "General/Battle" },
    { BBItem_Devil_Battle, "Devil/Battle" },
    { BBItem_God_Battle, "God/Battle" },
    { BBItem_Cure_Poison, "Cure/Poison" },
    { BBItem_Cure_Paralysis, "Cure/Paralysis" },
    { BBItem_Cure_Slow, "Cure/Slow" },
    { BBItem_Cure_Confuse, "Cure/Confuse" },
    { BBItem_Cure_Freeze, "Cure/Freeze" },
    { BBItem_Cure_Shock, "Cure/Shock" },
    { BBItem_YASAKANI_MAGATAMA, "YASAKANI MAGATAMA" },
    { BBItem_V101, "V101" },
    { BBItem_V501, "V501" },
    { BBItem_V502, "V502" },
    { BBItem_V801, "V801" },
    { BBItem_LIMITER, "LIMITER" },
    { BBItem_ADEPT, "ADEPT" },
    { BBItem_Sl_JORDSf_WJ_LORE, "Sl-JORDSf-WJ LORE" },
    { BBItem_PROOF_OF_SWORD_SAINT, "PROOF OF SWORD-SAINT" },
    { BBItem_SMARTLINK, "SMARTLINK" },
    { BBItem_DIVINE_PROTECTION, "DIVINE PROTECTION" },
    { BBItem_Heavenly_Battle, "Heavenly/Battle" },
    { BBItem_Heavenly_Power, "Heavenly/Power" },
    { BBItem_Heavenly_Mind, "Heavenly/Mind" },
    { BBItem_Heavenly_Arms, "Heavenly/Arms" },
    { BBItem_Heavenly_Legs, "Heavenly/Legs" },
    { BBItem_Heavenly_Body, "Heavenly/Body" },
    { BBItem_Heavenly_Luck, "Heavenly/Luck" },
    { BBItem_Heavenly_Ability, "Heavenly/Ability" },
    { BBItem_Centurion_Ability, "Centurion/Ability" },
    { BBItem_Friend_Ring, "Friend Ring" },
    { BBItem_Heavenly_HP, "Heavenly/HP" },
    { BBItem_Heavenly_TP, "Heavenly/TP" },
    { BBItem_Heavenly_Resist, "Heavenly/Resist" },
    { BBItem_Heavenly_Technique, "Heavenly/Technique" },
    { BBItem_HP_Ressurection, "HP/Ressurection" },
    { BBItem_TP_Ressurection, "TP/Ressurection" },
    { BBItem_PB_Increase, "PB/Increase" },
    { BBItem_UNK_UNIT, "UNK UNIT" },
    { BBItem_Monomate, "Monomate" },
    { BBItem_Dimate, "Dimate" },
    { BBItem_Trimate, "Trimate" },
    { BBItem_Monofluid, "Monofluid" },
    { BBItem_Difluid, "Difluid" },
    { BBItem_Trifluid, "Trifluid" },
    { BBItem_Disk_Lv01, "Disk:Lv.1" },
    { BBItem_Disk_Lv02, "Disk:Lv.2" },
    { BBItem_Disk_Lv03, "Disk:Lv.3" },
    { BBItem_Disk_Lv04, "Disk:Lv.4" },
    { BBItem_Disk_Lv05, "Disk:Lv.5" },
    { BBItem_Disk_Lv06, "Disk:Lv.6" },
    { BBItem_Disk_Lv07, "Disk:Lv.7" },
    { BBItem_Disk_Lv08, "Disk:Lv.8" },
    { BBItem_Disk_Lv09, "Disk:Lv.9" },
    { BBItem_Disk_Lv10, "Disk:Lv.10" },
    { BBItem_Disk_Lv11, "Disk:Lv.11" },
    { BBItem_Disk_Lv12, "Disk:Lv.12" },
    { BBItem_Disk_Lv13, "Disk:Lv.13" },
    { BBItem_Disk_Lv14, "Disk:Lv.14" },
    { BBItem_Disk_Lv15, "Disk:Lv.15" },
    { BBItem_Disk_Lv16, "Disk:Lv.16" },
    { BBItem_Disk_Lv17, "Disk:Lv.17" },
    { BBItem_Disk_Lv18, "Disk:Lv.18" },
    { BBItem_Disk_Lv19, "Disk:Lv.19" },
    { BBItem_Disk_Lv20, "Disk:Lv.20" },
    { BBItem_Disk_Lv21, "Disk:Lv.21" },
    { BBItem_Disk_Lv22, "Disk:Lv.22" },
    { BBItem_Disk_Lv23, "Disk:Lv.23" },
    { BBItem_Disk_Lv24, "Disk:Lv.24" },
    { BBItem_Disk_Lv25, "Disk:Lv.25" },
    { BBItem_Disk_Lv26, "Disk:Lv.26" },
    { BBItem_Disk_Lv27, "Disk:Lv.27" },
    { BBItem_Disk_Lv28, "Disk:Lv.28" },
    { BBItem_Disk_Lv29, "Disk:Lv.29" },
    { BBItem_Disk_Lv30, "Disk:Lv.30" },
    { BBItem_Sol_Atomizer, "Sol Atomizer" },
    { BBItem_Moon_Atomizer, "Moon Atomizer" },
    { BBItem_Star_Atomizer, "Star Atomizer" },
    { BBItem_Antidote, "Antidote" },
    { BBItem_Antiparalysis, "Antiparalysis" },
    { BBItem_Telepipe, "Telepipe" },
    { BBItem_Trap_Vision, "Trap Vision" },
    { BBItem_Scape_Doll, "Scape Doll" },
    { BBItem_Monogrinder, "Monogrinder" },
    { BBItem_Digrinder, "Digrinder" },
    { BBItem_Trigrinder, "Trigrinder" },
    { BBItem_Power_Material, "Power Material" },
    { BBItem_Mind_Material, "Mind Material" },
    { BBItem_Evade_Material, "Evade Material" },
    { BBItem_HP_Material, "HP Material" },
    { BBItem_TP_Material, "TP Material" },
    { BBItem_Def_Material, "Def Material" },
    { BBItem_Luck_Material, "Luck Material" },
    { BBItem_Hit_Material, "Hit Material" },
    { BBItem_Cell_of_MAG_502, "Cell of MAG 502" },
    { BBItem_Cell_of_MAG_213, "Cell of MAG 213" },
    { BBItem_Parts_of_RoboChao, "Parts of RoboChao" },
    { BBItem_Heart_of_Opa_Opa, "Heart of Opa Opa" },
    { BBItem_Heart_of_Pian, "Heart of Pian" },
    { BBItem_Heart_of_Chao, "Heart of Chao" },
    { BBItem_Sorcerers_Right_Arm, "Sorcerer's Right Arm" },
    { BBItem_S_beats_Arms, "S-beat's Arms" },
    { BBItem_P_arms_Arms, "P-arm's Arms" },
    { BBItem_Delsabers_Right_Arm, "Delsaber's Right Arm" },
    { BBItem_Bringers_Right_Arm, "Bringer's Right Arm" },
    { BBItem_Delsabers_Left_Arm, "Delsaber's Left Arm" },
    { BBItem_S_reds_Arm, "S-red's Arm" },
    { BBItem_Dragons_Claw_7, "Dragon's Claw" },
    { BBItem_Hildebears_Head, "Hildebear's Head" },
    { BBItem_Hildeblues_Head, "Hildeblue's Head" },
    { BBItem_Parts_of_Baranz, "Parts of Baranz" },
    { BBItem_Belras_Right_Arm, "Belra's Right Arm" },
    { BBItem_Gi_Gues_body, "Gi Gue's body" },
    { BBItem_Sinow_Berills_Aras, "Sinow Berill's Aras" },
    { BBItem_Grass_Assassins_Aras, "Grass Assassin's Aras" },
    { BBItem_Boomas_Right_Ara, "Booma's Right Ara" },
    { BBItem_Goboomas_Right_Arm, "Gobooma's Right Arm" },
    { BBItem_Gigoboomas_Right_Arm, "Gigobooma's Right Arm" },
    { BBItem_Gal_Gryphons_Wing, "Gal Gryphon's Wing" },
    { BBItem_Rappys_Ming, "Rappy's Ming" },
    { BBItem_Cladding_of_Epsilon, "Cladding of Epsilon" },
    { BBItem_De_Rol_Le_Shell, "De Rol Le Shell" },
    { BBItem_Berill_Photon, "Berill Photon" },
    { BBItem_Parasitic_gene_Flow, "Parasitic gene 'Flow'" },
    { BBItem_Magic_Stone_Iritista, "Magic Stone 'Iritista'" },
    { BBItem_Blue_black_stone, "Blue-black stone" },
    { BBItem_Syncesta, "Syncesta" },
    { BBItem_Magic_Water, "Magic Water" },
    { BBItem_Parasitic_cell_Type_D, "Parasitic cell Type D" },
    { BBItem_magic_rock_Heart_Key, "magic rock 'Heart Key'" },
    { BBItem_magic_rock_Moola, "magic rock 'Moola'" },
    { BBItem_Star_Amplifier, "Star Amplifier" },
    { BBItem_Book_of_HITOGATA, "Book of HITOGATA" },
    { BBItem_Heart_of_Chu_Chu, "Heart of Chu Chu" },
    { BBItem_Parts_of_EGG_BLASTER, "Parts of EGG BLASTER" },
    { BBItem_Heart_of_Angel, "Heart of Angel" },
    { BBItem_Heart_of_Devil, "Heart of Devil" },
    { BBItem_Kit_of_Hamburger, "Kit of Hamburger" },
    { BBItem_Panthers_Spirit, "Panther's Spirit" },
    { BBItem_Kit_of_MARK3, "Kit of MARK3" },
    { BBItem_Kit_of_MASTER_SYSTEM, "Kit of MASTER SYSTEM" },
    { BBItem_Kit_of_GENESIS, "Kit of GENESIS" },
    { BBItem_Kit_of_SEGA_SATURN, "Kit of SEGA SATURN" },
    { BBItem_Kit_of_DREAMCAST, "Kit of DREAMCAST" },
    { BBItem_Amplifier_of_Resta, "Amplifier of Resta" },
    { BBItem_Amplifier_of_Anti, "Amplifier of Anti" },
    { BBItem_Amplifier_of_Shift, "Amplifier of Shift*" },
    { BBItem_Amplifier_of_Deband, "Amplifier of Deband" },
    { BBItem_Amplifier_of_Foie, "Amplifier of Foie" },
    { BBItem_Amplifier_of_Gifoie, "Amplifier of Gifoie" },
    { BBItem_Amplifier_of_Rafoie, "Amplifier of Rafoie" },
    { BBItem_Amplifier_of_Barta, "Amplifier of Barta" },
    { BBItem_Amplifier_of_Gibarta, "Amplifier of Gibarta" },
    { BBItem_Amplifier_of_Rabarta, "Amplifier of Rabarta" },
    { BBItem_Amplifier_of_Zonde, "Amplifier of Zonde" },
    { BBItem_Amplifier_of_Gizonde, "Amplifier of Gizonde" },
    { BBItem_Amplifier_of_Razonde, "Amplifier of Razonde" },
    { BBItem_Amplifier_of_Red, "Amplifier of Red" },
    { BBItem_Amplifier_of_Blue, "Amplifier of Blue" },
    { BBItem_Amplifier_of_Yellow, "Amplifier of Yellow" },
    { BBItem_Heart_of_KAPU_KAPU, "Heart of KAPU KAPU" },
    { BBItem_Photon_Booster, "Photon Booster" },
    { BBItem_AddSlot, "AddSlot" },
    { BBItem_Photon_Drop, "Photon Drop" },
    { BBItem_Photon_Sphere, "Photon Sphere" },
    { BBItem_Photon_Crystal, "Photon Crystal" },
    { BBItem_Secret_Ticket, "Secret Ticket" },
    { BBItem_Photon_Ticket, "Photon Ticket" },
    { BBItem_Book_of__KATANA1, "Book of  KATANA1" },
    { BBItem_Book_of__KATANA2, "Book of  KATANA2" },
    { BBItem_Book_of__KATANA3, "Book of  KATANA3" },
    { BBItem_Weapons_Bronze_Badge, "Weapons Bronze Badge" },
    { BBItem_Weapons_Silver_Badge_1, "Weapons Silver Badge" },
    { BBItem_Weapons_Gold_Badge_2, "Weapons Gold Badge" },
    { BBItem_Weapons_Crystal_Badge_3, "Weapons Crystal Badge" },
    { BBItem_Weapons_Steel_Badge_4, "Weapons Steel Badge" },
    { BBItem_Weapons_Aluminum_Badge_5, "Weapons Aluminum Badge" },
    { BBItem_Weapons_Leather_Badge_6, "Weapons Leather Badge" },
    { BBItem_Weapons_Bone_Badge_7, "Weapons Bone Badge" },
    { BBItem_Letter_of_appreciation, "Letter of appreciation" },
    { BBItem_Item_Ticket, "Item Ticket" },
    { BBItem_Valentines_Chocolate, "Valentine's Chocolate" },
    { BBItem_New_Years_Card, "New Year's Card" },
    { BBItem_Christmas_Card, "Christmas Card" },
    { BBItem_Birthday_Card, "Birthday Card" },
    { BBItem_Proof_of_Sonic_Team, "Proof of Sonic Team" },
    { BBItem_Special_Event_Ticket, "Special Event Ticket" },
    { BBItem_Flower_Bouquet_10, "Flower Bouquet" },
    { BBItem_Cake_11, "Cake" },
    { BBItem_Accessories, "Accessories" },
    { BBItem_MrNakas_Business_Card, "Mr.Naka's Business Card" },
    { BBItem_Present, "Present" },
    { BBItem_Chocolate, "Chocolate" },
    { BBItem_Candy, "Candy" },
    { BBItem_Cake_2, "Cake" },
    { BBItem_Weapons_Silver_Badge_3, "Weapons Silver Badge" },
    { BBItem_Weapons_Gold_Badge_4, "Weapons Gold Badge" },
    { BBItem_Weapons_Crystal_Badge_5, "Weapons Crystal Badge" },
    { BBItem_Weapons_Steel_Badge_6, "Weapons Steel Badge" },
    { BBItem_Weapons_Aluminum_Badge_7, "Weapons Aluminum Badge" },
    { BBItem_Weapons_Leather_Badge_8, "Weapons Leather Badge" },
    { BBItem_Weapons_Bone_Badge_9, "Weapons Bone Badge" },
    { BBItem_Bouquet, "Bouquet" },
    { BBItem_Decoction, "Decoction" },
    { BBItem_Christmas_Present, "Christmas Present" },
    { BBItem_Easter_Egg, "Easter Egg" },
    { BBItem_Jack_0_Lantern, "Jack-0'-Lantern" },
    { BBItem_DISK_Vol1, "DISK Vol.1 'Wedding March'" },
    { BBItem_DISK_Vol2, "DISK Vol.2 'Day Light'" },
    { BBItem_DISK_Vol3, "DISK Vol.3 'Burning Rangers'" },
    { BBItem_DISK_Vol4, "DISK Vol.4 'Open Your Heart'" },
    { BBItem_DISK_Vol5, "DISK Vol.5 'Live & Learn'" },
    { BBItem_DISK_Vol6, "DISK Vol.6 'NiGHTS'" },
    { BBItem_DISK_Vol7, "DISK Vol.7 'Ending Theme��Piano ver.��'" },
    { BBItem_DISK_Vol8, "DISK Vol.8 'Heart to Heart'" },
    { BBItem_DISK_Vol9, "DISK Vol.9 'Strange Blue'" },
    { BBItem_DISK_Vol10, "DISK Vol.10 'Reunion System'" },
    { BBItem_DISK_Vol11, "DISK Vol.11 'Pinnacles'" },
    { BBItem_DISK_Vol12, "DISK Vol.12 'Fight inside the Spaceship'" },
    { BBItem_Hunters_Report_0, "Hunters Report" },
    { BBItem_Hunters_Report_1, "Hunters Report" },
    { BBItem_Hunters_Report_2, "Hunters Report" },
    { BBItem_Hunters_Report_3, "Hunters Report" },
    { BBItem_Hunters_Report_4, "Hunters Report" },
    { BBItem_Tablet, "Tablet" },
    { BBItem_UNKNOWN2, "UNKNOWN2" },
    { BBItem_Dragon_Scale, "Dragon Scale" },
    { BBItem_Heaven_Striker_Coat, "Heaven Striker Coat" },
    { BBItem_Pioneer_Parts, "Pioneer Parts" },
    { BBItem_Amities_Memo, "Amitie's Memo" },
    { BBItem_Heart_of_Morolian, "Heart of Morolian" },
    { BBItem_Rappys_Beak, "Rappy's Beak" },
    { BBItem_YahooIs_engine, "YahooI's engine" },
    { BBItem_D_Photon_Core, "D-Photon Core" },
    { BBItem_Liberta_Kit, "Liberta Kit" },
    { BBItem_Cell_of_MAG_0503_0B, "Cell of MAG 0503" },
    { BBItem_Cell_of_MAG_0504_0C, "Cell of MAG 0504" },
    { BBItem_Cell_of_MAG_0505_0D, "Cell of MAG 0505" },
    { BBItem_Cell_of_MAG_0506_0E, "Cell of MAG 0506" },
    { BBItem_Cell_of_MAG_0507_0F, "Cell of MAG 0507" },
    { BBItem_Team_Points_500, "Team Points 500" },
    { BBItem_Team_Points_1000, "Team Points 1000" },
    { BBItem_Team_Points_5000, "Team Points 5000" },
    { BBItem_Team_Points_10000, "Team Points 10000" },
    { BBItem_UNK_TOOL, "UNK TOOL" },
    { BBItem_Meseta, "Meseta" },
    { BBItem_NoSuchItem, "No Such Item" }
};

/* ��Ʒ�������嵥. ���� */
static bbitem_map_t bbitem_list_cn[] = {
    { BBItem_Saber0, "�⽣0" },
    { BBItem_Saber, "�⽣" },
    { BBItem_Brand, "ǿ���⽣" },
    { BBItem_Buster, "��ӡ�⽣" },
    { BBItem_Pallasch, "��ҫ�⽣" },
    { BBItem_Gladius, "�����⽣" },
    { BBItem_DBS_SABER, "DB֮��" },
    { BBItem_KALADBOLG, "��֮��" },
    { BBItem_DURANDAL, "��ʿ֮��" },
    { BBItem_GALATINE, "̫��֮��" },
    { BBItem_Sword, "��" },
    { BBItem_Gigush, "���д�" },
    { BBItem_Breaker, "˫�д�" },
    { BBItem_Claymore, "���д�" },
    { BBItem_Calibur, "���ߴ�" },
    { BBItem_FLOWENS_SWORD, "�����Ĵ�" },
    { BBItem_LAST_SURVIVOR, "����������R" },
    { BBItem_DRAGON_SLAYER, "ն����" },
    { BBItem_Dagger, "ذ��" },
    { BBItem_Knife, "����ذ��" },
    { BBItem_Blade, "ͻ��ذ��" },
    { BBItem_Edge, "����ذ��" },
    { BBItem_Ripper, "��ɱذ��" },
    { BBItem_BLADE_DANCE, "��֮��" },
    { BBItem_BLOODY_ART, "Ѫ֮��" },
    { BBItem_CROSS_SCAR, "ʮ�ְ�" },
    { BBItem_ZERO_DIVIDE, "�������" },
    { BBItem_TWO_KAMUI, "��������" },
    { BBItem_Partisan, "����" },
    { BBItem_Halbert, "��������" },
    { BBItem_Glaive, "��ʿ����" },
    { BBItem_Berdys, "��������" },
    { BBItem_Gungnir, "ս�񳤵�" },
    { BBItem_BRIONAC, "��������" },
    { BBItem_VJAYA, "��������" },
    { BBItem_GAE_BOLG, "ս������" },
    { BBItem_ASTERON_BELT, "ʮ����ǹ" },
    { BBItem_Slicer, "Ͷ��" },
    { BBItem_Spinner, "����Ͷ��" },
    { BBItem_Cutter, "����Ͷ��" },
    { BBItem_Sawcer, "�ɵ�Ͷ��" },
    { BBItem_Diska, "����Ͷ��" },
    { BBItem_SLICER_OF_ASSASSIN, "��ɱ�ߵ�Ͷ��" },
    { BBItem_DISKA_OF_LIBERATOR, "����ߵķɵ�" },
    { BBItem_DISKA_OF_BRAVEMAN, "���ߵķɵ�N" },
    { BBItem_IZMAELA, "�����갬��" },
    { BBItem_Handgun, "��ǹ" },
    { BBItem_Autogun, "���ܹ�ǹ" },
    { BBItem_Lockgun, "�����ǹ" },
    { BBItem_Railgun, "�ŵ��ǹ" },
    { BBItem_Raygun, "ԭ�ӹ�ǹ" },
    { BBItem_VARISTA, "����ǹ" },
    { BBItem_CUSTOM_RAY_ver_OO, "���ù�ǹ ver.00" },
    { BBItem_BRAVACE, "���õ��ǹ" },
    { BBItem_TENSION_BLASTER, "ѹ������ǹ" },
    { BBItem_Rifle, "��ǹ" },
    { BBItem_Sniper, "�ѻ���ǹ" },
    { BBItem_Blaster, "ͻ����ǹ" },
    { BBItem_Beam, "���岽ǹ" },
    { BBItem_Laser, "���ⲽǹ" },
    { BBItem_VISK_235W, "��˹��-235W" },
    { BBItem_WALS_MK2, "�߶�˹-MK2" },
    { BBItem_JUSTY_23ST, "٤˹��-23ST" },
    { BBItem_RIANOV_303SNR, "����ŵ�� 303SNR" },
    { BBItem_RIANOV_303SNR_1, "����ŵ�� 303SNR-1" },
    { BBItem_RIANOV_303SNR_2, "����ŵ�� 303SNR-2" },
    { BBItem_RIANOV_303SNR_3, "����ŵ�� 303SNR-3" },
    { BBItem_RIANOV_303SNR_4, "����ŵ�� 303SNR-4" },
    { BBItem_RIANOV_303SNR_5, "����ŵ�� 303SNR-5" },
    { BBItem_Mechgun, "��ǹ" },
    { BBItem_Assault, "ɨ���ǹ" },
    { BBItem_Repeater, "�����ǹ" },
    { BBItem_Gatling, "���ֻ�ǹ" },
    { BBItem_Vulcan, "���ٻ�ǹ" },
    { BBItem_M_and_A60_VISE, "M&A60 �ϻ�ǯ" },
    { BBItem_H_and_S25_JUSTICE, "H&S25 �����Ʋ�" },
    { BBItem_L_and_K14_COMBAT, "L&K14 ս��֮��" },
    { BBItem_Shot, "ɢ��ǹ" },
    { BBItem_Spread, "����ɢ��ǹ" },
    { BBItem_Cannon, "��ũɢ��ǹ" },
    { BBItem_Launcher, "ǿϮɢ��ǹ" },
    { BBItem_Arms, "����ɢ��ǹ" },
    { BBItem_CRUSH_BULLET, "�鵯ǹ" },
    { BBItem_METEOR_SMASH, "��ʯǹ" },
    { BBItem_FINAL_IMPACT, "���ĳ��" },
    { BBItem_Cane, "����" },
    { BBItem_Stick, "����" },
    { BBItem_Mace, "����" },
    { BBItem_Club, "Ȩ��" },
    { BBItem_CLUB_OF_LACONIUM, "������ķ������" },
    { BBItem_MACE_OF_ADAMAN, "���ʯ��" },
    { BBItem_CLUB_OF_ZUMIURAN, "ҵ��ʯ��" },
    { BBItem_LOLIPOP, "������" },
    { BBItem_Rod, "����" },
    { BBItem_Pole, "��ʦ����" },
    { BBItem_Pillar, "��˾����" },
    { BBItem_Striker, "���߳���" },
    { BBItem_BATTLE_VERGE, "ս����" },
    { BBItem_BRAVE_HAMMER, "����֮��" },
    { BBItem_ALIVE_AQHU, "������" },
    { BBItem_VALKYRIE, "�߶�����" },
    { BBItem_Wand, "ħ��" },
    { BBItem_Staff, "ħ����" },
    { BBItem_Baton, "ħ����" },
    { BBItem_Scepter, "ħϢ��" },
    { BBItem_FIRE_SCEPTER_AGNI, "���ȡ������᡹" },
    { BBItem_ICE_STAFF_DAGON, "���ȡ��ﹱ��" },
    { BBItem_STORM_WAND_INDRA, "���ȡ������ޡ�" },
    { BBItem_EARTH_WAND_BROWNIE, "���ȡ��������᡹" },
    { BBItem_PHOTON_CLAW, "����צ" },
    { BBItem_SILENCE_CLAW, "��Ĭצ" },
    { BBItem_NEIS_CLAW_2, "��֮צ" },
    { BBItem_PHOENIX_CLAW, "˫ͷ��" },
    { BBItem_DOUBLE_SABER, "˫ͷ��" },
    { BBItem_STAG_CUTLERY, "�۷�֮��" },
    { BBItem_TWIN_BRAND, "˫��ӡ" },
    { BBItem_BRAVE_KNUCKLE, "����֮ȭ" },
    { BBItem_ANGRY_FIST, "��ŭ֮ȭ" },
    { BBItem_GOD_HAND, "�ϵ�֮��" },
    { BBItem_SONIC_KNUCKLE, "�����ȭ��" },
    { BBItem_OROTIAGITO_alt, "������_alt" },
    { BBItem_OROTIAGITO, "������" },
    { BBItem_AGITO_1975, "�� 1975" },
    { BBItem_AGITO_1983, "�� 1983" },
    { BBItem_AGITO_2001, "�� 2001" },
    { BBItem_AGITO_1991, "�� 1991" },
    { BBItem_AGITO_1977, "�� 1977" },
    { BBItem_AGITO_1980, "�� 1980" },
    { BBItem_RAIKIRI, "����" },
    { BBItem_SOUL_EATER, "�ɻ���" },
    { BBItem_SOUL_BANISH, "�����" },
    { BBItem_SPREAD_NEEDLE, "ħ����" },
    { BBItem_HOLY_RAY, "ʥ��ǹ" },
    { BBItem_INFERNO_BAZOOKA, "���������" },
    { BBItem_RAMBLING_MAY, "������Ů" },
    { BBItem_LK38_COMBAT, "L&K38 ս��֮��" },
    { BBItem_FLAME_VISIT, "����������" },
    { BBItem_BURNING_VISIT, "ǿ������������" },
    { BBItem_AKIKOS_FRYING_PAN, "����������ƽ�׹�" },
    { BBItem_SORCERERS_CANE, "���編ʦ֮��" },
    { BBItem_S_BEATS_BLADE, "����˫��" },
    { BBItem_P_ARMSS_BLADE, "�����˫��" },
    { BBItem_DELSABERS_BUSTER, "��ħ֮��" },
    { BBItem_BRINGERS_RIFLE, "������ʿǹ" },
    { BBItem_EGG_BLASTER, "����ǹ" },
    { BBItem_PSYCHO_WAND, "ʥ�ȡ����" },
    { BBItem_HEAVEN_PUNISHER, "ʥǹ���췣��" },
    { BBItem_LAVIS_CANNON, "ʥ������ά˹����ũ��" },
    { BBItem_VICTOR_AXE, "ʤ����֮��" },
    { BBItem_LACONIUM_AXE, "������ķս��" },
    { BBItem_CHAIN_SAWD, "���" },
    { BBItem_CADUCEUS, "��ë��" },
    { BBItem_MERCURIUS_ROD, "��ʹ֮��" },
    { BBItem_STING_TIP, "��β��" },
    { BBItem_MAGICAL_PIECE, "������" },
    { BBItem_TECHNICAL_CROZIER, "TECHNICAL CROZIER" },
    { BBItem_SUPPRESSED_GUN, "����ǹ" },
    { BBItem_ANCIENT_SABER, "ANCIENT SABER" },
    { BBItem_HARISEN_BATTLE_FAN, "HARISEN BATTLE FAN" },
    { BBItem_YAMIGARASU, "YAMIGARASU" },
    { BBItem_AKIKOS_WOK, "AKIKOS WOK" },
    { BBItem_TOY_HAMMER, "TOY HAMMER" },
    { BBItem_ELYSION, "ELYSION" },
    { BBItem_RED_SABER, "RED SABER" },
    { BBItem_METEOR_CUDGEL, "METEOR CUDGEL" },
    { BBItem_MONKEY_KING_BAR, "MONKEY KING BAR" },
    { BBItem_BLACK_KING_BAR, "BLACK KING BAR" },
    { BBItem_DOUBLE_CANNON, "DOUBLE CANNON" },
    { BBItem_GIRASOLE, "GIRASOLE" },
    { BBItem_HUGE_BATTLE_FAN, "HUGE BATTLE FAN" },
    { BBItem_TSUMIKIRI_J_SWORD, "TSUMIKIRI J-SWORD" },
    { BBItem_SEALED_J_SWORD, "SEALED J-SWORD" },
    { BBItem_RED_SWORD, "RED SWORD" },
    { BBItem_CRAZY_TUNE, "CRAZY TUNE" },
    { BBItem_TWIN_CHAKRAM, "TWIN CHAKRAM" },
    { BBItem_WOK_OF_AKIKOS_SHOP, "WOK OF AKIKOS SHOP" },
    { BBItem_LAVIS_BLADE, "LAVIS BLADE" },
    { BBItem_RED_DAGGER, "RED DAGGER" },
    { BBItem_MADAMS_PARASOL, "MADAMS PARASOL" },
    { BBItem_MADAMS_UMBRELLA, "MADAMS UMBRELLA" },
    { BBItem_IMPERIAL_PICK, "IMPERIAL PICK" },
    { BBItem_BERDYSH, "BERDYSH" },
    { BBItem_RED_PARTISAN, "RED PARTISAN" },
    { BBItem_FLIGHT_CUTTER, "FLIGHT CUTTER" },
    { BBItem_FLIGHT_FAN, "FLIGHT FAN" },
    { BBItem_RED_SLICER, "RED SLICER" },
    { BBItem_HANDGUN_GULD, "HANDGUN:GULD" },
    { BBItem_MASTER_RAVEN, "MASTER RAVEN" },
    { BBItem_HANDGUN_MILLA, "HANDGUN:MILLA" },
    { BBItem_LAST_SWAN, "LAST SWAN" },
    { BBItem_RED_HANDGUN, "RED HANDGUN" },
    { BBItem_FROZEN_SHOOTER, "FROZEN SHOOTER" },
    { BBItem_SNOW_QUEEN, "SNOW QUEEN" },
    { BBItem_ANTI_ANDROID_RIFLE, "ANTI ANDROID RIFLE" },
    { BBItem_ROCKET_PUNCH, "ROCKET PUNCH" },
    { BBItem_SAMBA_MARACAS, "SAMBA MARACAS" },
    { BBItem_TWIN_PSYCHOGUN, "TWIN PSYCHOGUN" },
    { BBItem_DRILL_LAUNCHER, "DRILL LAUNCHER" },
    { BBItem_GULD_MILLA, "GULD MILLA" },
    { BBItem_DUAL_BIRD, "DUAL BIRD" },
    { BBItem_RED_MECHGUN, "RED MECHGUN" },
    { BBItem_BELRA_CANNON, "BELRA CANNON" },
    { BBItem_PANZER_FAUST, "PANZER FAUST" },
    { BBItem_IRON_FAUST, "IRON FAUST" },
    { BBItem_SUMMIT_MOON, "SUMMIT MOON" },
    { BBItem_WINDMILL, "WINDMILL" },
    { BBItem_EVIL_CURST, "EVIL CURST" },
    { BBItem_FLOWER_CANE, "FLOWER CANE" },
    { BBItem_HILDEBEARS_CANE, "HILDEBEARS CANE" },
    { BBItem_HILDEBLUES_CANE, "HILDEBLUES CANE" },
    { BBItem_RABBIT_WAND, "RABBIT WAND" },
    { BBItem_PLANTAIN_LEAF, "PLANTAIN LEAF" },
    { BBItem_FATSIA, "����Ҷ" },
    { BBItem_DEMONIC_FORK, "��ħ֮��" },
    { BBItem_STRIKER_OF_CHAO, "��ŷ��" },
    { BBItem_BROOM, "ɨ��" },
    { BBItem_PROPHETS_OF_MOTAV, "PROPHETS OF MOTAV" },
    { BBItem_THE_SIGH_OF_A_GOD, "THE SIGH OF A GOD" },
    { BBItem_TWINKLE_STAR, "TWINKLE STAR" },
    { BBItem_PLANTAIN_FAN, "PLANTAIN FAN" },
    { BBItem_TWIN_BLAZE, "TWIN BLAZE" },
    { BBItem_MARINAS_BAG, "MARINAS BAG" },
    { BBItem_DRAGONS_CLAW_0, "DRAGONS CLAW" },
    { BBItem_PANTHERS_CLAW, "PANTHERS CLAW" },
    { BBItem_S_REDS_BLADE, "S-REDS BLADE" },
    { BBItem_PLANTAIN_HUGE_FAN, "PLANTAIN HUGE FAN" },
    { BBItem_CHAMELEON_SCYTHE, "CHAMELEON SCYTHE" },
    { BBItem_YASMINKOV_3000R, "YASMINKOV 3000R" },
    { BBItem_ANO_RIFLE, "ANO RIFLE" },
    { BBItem_BARANZ_LAUNCHER, "BARANZ LAUNCHER" },
    { BBItem_BRANCH_OF_PAKUPAKU, "BRANCH OF PAKUPAKU" },
    { BBItem_HEART_OF_POUMN, "HEART OF POUMN" },
    { BBItem_YASMINKOV_2000H, "YASMINKOV 2000H" },
    { BBItem_YASMINKOV_7000V, "YASMINKOV 7000V" },
    { BBItem_YASMINKOV_9000M, "YASMINKOV 9000M" },
    { BBItem_MASER_BEAM, "�׹���" },
    { BBItem_POWER_MASER, "ǿ���׹���" },
    { BBItem_GAME_MAGAZNE, "�ƣ��ͣ�ͨ" },
    { BBItem_LOGiN, "LOGiN" },
    { BBItem_FLOWER_BOUQUET_0, "FLOWER BOUQUET" },
    { BBItem_SRANK_SABER_0, "SRANK SABER 0" },
    { BBItem_SRANK_SABER_1, "SRANK SABER 1" },
    { BBItem_SRANK_SABER_2, "SRANK SABER 2" },
    { BBItem_SRANK_SABER_3, "SRANK SABER 3" },
    { BBItem_SRANK_SABER_4, "SRANK SABER 4" },
    { BBItem_SRANK_SABER_5, "SRANK SABER 5" },
    { BBItem_SRANK_SABER_6, "SRANK SABER 6" },
    { BBItem_SRANK_SABER_7, "SRANK SABER 7" },
    { BBItem_SRANK_SABER_8, "SRANK SABER 8" },
    { BBItem_SRANK_SABER_9, "SRANK SABER 9" },
    { BBItem_SRANK_SABER_10, "SRANK SABER 10" },
    { BBItem_SRANK_SABER_11, "SRANK SABER 11" },
    { BBItem_SRANK_SABER_12, "SRANK SABER 12" },
    { BBItem_SRANK_SABER_13, "SRANK SABER 13" },
    { BBItem_SRANK_SABER_14, "SRANK SABER 14" },
    { BBItem_SRANK_SABER_15, "SRANK SABER 15" },
    { BBItem_SRANK_SABER_16, "SRANK SABER 16" },
    { BBItem_SRANK_SWORD_0, "SRANK SWORD 0" },
    { BBItem_SRANK_SWORD_1, "SRANK SWORD 1" },
    { BBItem_SRANK_SWORD_2, "SRANK SWORD 2" },
    { BBItem_SRANK_SWORD_3, "SRANK SWORD 3" },
    { BBItem_SRANK_SWORD_4, "SRANK SWORD 4" },
    { BBItem_SRANK_SWORD_5, "SRANK SWORD 5" },
    { BBItem_SRANK_SWORD_6, "SRANK SWORD 6" },
    { BBItem_SRANK_SWORD_7, "SRANK SWORD 7" },
    { BBItem_SRANK_SWORD_8, "SRANK SWORD 8" },
    { BBItem_SRANK_SWORD_9, "SRANK SWORD 9" },
    { BBItem_SRANK_SWORD_10, "SRANK SWORD 10" },
    { BBItem_SRANK_SWORD_11, "SRANK SWORD 11" },
    { BBItem_SRANK_SWORD_12, "SRANK SWORD 12" },
    { BBItem_SRANK_SWORD_13, "SRANK SWORD 13" },
    { BBItem_SRANK_SWORD_14, "SRANK SWORD 14" },
    { BBItem_SRANK_SWORD_15, "SRANK SWORD 15" },
    { BBItem_SRANK_SWORD_16, "SRANK SWORD 16" },
    { BBItem_SRANK_BLADE_0, "SRANK BLADE 0" },
    { BBItem_SRANK_BLADE_1, "SRANK BLADE 1" },
    { BBItem_SRANK_BLADE_2, "SRANK BLADE 2" },
    { BBItem_SRANK_BLADE_3, "SRANK BLADE 3" },
    { BBItem_SRANK_BLADE_4, "SRANK BLADE 4" },
    { BBItem_SRANK_BLADE_5, "SRANK BLADE 5" },
    { BBItem_SRANK_BLADE_6, "SRANK BLADE 6" },
    { BBItem_SRANK_BLADE_7, "SRANK BLADE 7" },
    { BBItem_SRANK_BLADE_8, "SRANK BLADE 8" },
    { BBItem_SRANK_BLADE_9, "SRANK BLADE 9" },
    { BBItem_SRANK_BLADE_10, "SRANK BLADE 10" },
    { BBItem_SRANK_BLADE_11, "SRANK BLADE 11" },
    { BBItem_SRANK_BLADE_12, "SRANK BLADE 12" },
    { BBItem_SRANK_BLADE_13, "SRANK BLADE 13" },
    { BBItem_SRANK_BLADE_14, "SRANK BLADE 14" },
    { BBItem_SRANK_BLADE_15, "SRANK BLADE 15" },
    { BBItem_SRANK_BLADE_16, "SRANK BLADE 16" },
    { BBItem_SRANK_PARTISAN_0, "SRANK PARTISAN 0" },
    { BBItem_SRANK_PARTISAN_1, "SRANK PARTISAN 1" },
    { BBItem_SRANK_PARTISAN_2, "SRANK PARTISAN 2" },
    { BBItem_SRANK_PARTISAN_3, "SRANK PARTISAN 3" },
    { BBItem_SRANK_PARTISAN_4, "SRANK PARTISAN 4" },
    { BBItem_SRANK_PARTISAN_5, "SRANK PARTISAN 5" },
    { BBItem_SRANK_PARTISAN_6, "SRANK PARTISAN 6" },
    { BBItem_SRANK_PARTISAN_7, "SRANK PARTISAN 7" },
    { BBItem_SRANK_PARTISAN_8, "SRANK PARTISAN 8" },
    { BBItem_SRANK_PARTISAN_9, "SRANK PARTISAN 9" },
    { BBItem_SRANK_PARTISAN_10, "SRANK PARTISAN 10" },
    { BBItem_SRANK_PARTISAN_11, "SRANK PARTISAN 11" },
    { BBItem_SRANK_PARTISAN_12, "SRANK PARTISAN 12" },
    { BBItem_SRANK_PARTISAN_13, "SRANK PARTISAN 13" },
    { BBItem_SRANK_PARTISAN_14, "SRANK PARTISAN 14" },
    { BBItem_SRANK_PARTISAN_15, "SRANK PARTISAN 15" },
    { BBItem_SRANK_PARTISAN_16, "SRANK PARTISAN 16" },
    { BBItem_SRANK_SLICER_0, "SRANK SLICER 0" },
    { BBItem_SRANK_SLICER_1, "SRANK SLICER 1" },
    { BBItem_SRANK_SLICER_2, "SRANK SLICER 2" },
    { BBItem_SRANK_SLICER_3, "SRANK SLICER 3" },
    { BBItem_SRANK_SLICER_4, "SRANK SLICER 4" },
    { BBItem_SRANK_SLICER_5, "SRANK SLICER 5" },
    { BBItem_SRANK_SLICER_6, "SRANK SLICER 6" },
    { BBItem_SRANK_SLICER_7, "SRANK SLICER 7" },
    { BBItem_SRANK_SLICER_8, "SRANK SLICER 8" },
    { BBItem_SRANK_SLICER_9, "SRANK SLICER 9" },
    { BBItem_SRANK_SLICER_10, "SRANK SLICER 10" },
    { BBItem_SRANK_SLICER_11, "SRANK SLICER 11" },
    { BBItem_SRANK_SLICER_12, "SRANK SLICER 12" },
    { BBItem_SRANK_SLICER_13, "SRANK SLICER 13" },
    { BBItem_SRANK_SLICER_14, "SRANK SLICER 14" },
    { BBItem_SRANK_SLICER_15, "SRANK SLICER 15" },
    { BBItem_SRANK_SLICER_16, "SRANK SLICER 16" },
    { BBItem_SRANK_GUN_0, "SRANK GUN 0" },
    { BBItem_SRANK_GUN_1, "SRANK GUN 1" },
    { BBItem_SRANK_GUN_2, "SRANK GUN 2" },
    { BBItem_SRANK_GUN_3, "SRANK GUN 3" },
    { BBItem_SRANK_GUN_4, "SRANK GUN 4" },
    { BBItem_SRANK_GUN_5, "SRANK GUN 5" },
    { BBItem_SRANK_GUN_6, "SRANK GUN 6" },
    { BBItem_SRANK_GUN_7, "SRANK GUN 7" },
    { BBItem_SRANK_GUN_8, "SRANK GUN 8" },
    { BBItem_SRANK_GUN_9, "SRANK GUN 9" },
    { BBItem_SRANK_GUN_10, "SRANK GUN 10" },
    { BBItem_SRANK_GUN_11, "SRANK GUN 11" },
    { BBItem_SRANK_GUN_12, "SRANK GUN 12" },
    { BBItem_SRANK_GUN_13, "SRANK GUN 13" },
    { BBItem_SRANK_GUN_14, "SRANK GUN 14" },
    { BBItem_SRANK_GUN_15, "SRANK GUN 15" },
    { BBItem_SRANK_GUN_16, "SRANK GUN 16" },
    { BBItem_SRANK_RIFLE_0, "SRANK RIFLE 0" },
    { BBItem_SRANK_RIFLE_1, "SRANK RIFLE 1" },
    { BBItem_SRANK_RIFLE_2, "SRANK RIFLE 2" },
    { BBItem_SRANK_RIFLE_3, "SRANK RIFLE 3" },
    { BBItem_SRANK_RIFLE_4, "SRANK RIFLE 4" },
    { BBItem_SRANK_RIFLE_5, "SRANK RIFLE 5" },
    { BBItem_SRANK_RIFLE_6, "SRANK RIFLE 6" },
    { BBItem_SRANK_RIFLE_7, "SRANK RIFLE 7" },
    { BBItem_SRANK_RIFLE_8, "SRANK RIFLE 8" },
    { BBItem_SRANK_RIFLE_9, "SRANK RIFLE 9" },
    { BBItem_SRANK_RIFLE_10, "SRANK RIFLE 10" },
    { BBItem_SRANK_RIFLE_11, "SRANK RIFLE 11" },
    { BBItem_SRANK_RIFLE_12, "SRANK RIFLE 12" },
    { BBItem_SRANK_RIFLE_13, "SRANK RIFLE 13" },
    { BBItem_SRANK_RIFLE_14, "SRANK RIFLE 14" },
    { BBItem_SRANK_RIFLE_15, "SRANK RIFLE 15" },
    { BBItem_SRANK_RIFLE_16, "SRANK RIFLE 16" },
    { BBItem_SRANK_MECHGUN_0, "SRANK MECHGUN 0" },
    { BBItem_SRANK_MECHGUN_1, "SRANK MECHGUN 1" },
    { BBItem_SRANK_MECHGUN_2, "SRANK MECHGUN 2" },
    { BBItem_SRANK_MECHGUN_3, "SRANK MECHGUN 3" },
    { BBItem_SRANK_MECHGUN_4, "SRANK MECHGUN 4" },
    { BBItem_SRANK_MECHGUN_5, "SRANK MECHGUN 5" },
    { BBItem_SRANK_MECHGUN_6, "SRANK MECHGUN 6" },
    { BBItem_SRANK_MECHGUN_7, "SRANK MECHGUN 7" },
    { BBItem_SRANK_MECHGUN_8, "SRANK MECHGUN 8" },
    { BBItem_SRANK_MECHGUN_9, "SRANK MECHGUN 9" },
    { BBItem_SRANK_MECHGUN_10, "SRANK MECHGUN 10" },
    { BBItem_SRANK_MECHGUN_11, "SRANK MECHGUN 11" },
    { BBItem_SRANK_MECHGUN_12, "SRANK MECHGUN 12" },
    { BBItem_SRANK_MECHGUN_13, "SRANK MECHGUN 13" },
    { BBItem_SRANK_MECHGUN_14, "SRANK MECHGUN 14" },
    { BBItem_SRANK_MECHGUN_15, "SRANK MECHGUN 15" },
    { BBItem_SRANK_MECHGUN_16, "SRANK MECHGUN 16" },
    { BBItem_SRANK_SHOT_0, "SRANK SHOT 0" },
    { BBItem_SRANK_SHOT_1, "SRANK SHOT 1" },
    { BBItem_SRANK_SHOT_2, "SRANK SHOT 2" },
    { BBItem_SRANK_SHOT_3, "SRANK SHOT 3" },
    { BBItem_SRANK_SHOT_4, "SRANK SHOT 4" },
    { BBItem_SRANK_SHOT_5, "SRANK SHOT 5" },
    { BBItem_SRANK_SHOT_6, "SRANK SHOT 6" },
    { BBItem_SRANK_SHOT_7, "SRANK SHOT 7" },
    { BBItem_SRANK_SHOT_8, "SRANK SHOT 8" },
    { BBItem_SRANK_SHOT_9, "SRANK SHOT 9" },
    { BBItem_SRANK_SHOT_10, "SRANK SHOT 10" },
    { BBItem_SRANK_SHOT_11, "SRANK SHOT 11" },
    { BBItem_SRANK_SHOT_12, "SRANK SHOT 12" },
    { BBItem_SRANK_SHOT_13, "SRANK SHOT 13" },
    { BBItem_SRANK_SHOT_14, "SRANK SHOT 14" },
    { BBItem_SRANK_SHOT_15, "SRANK SHOT 15" },
    { BBItem_SRANK_SHOT_16, "SRANK SHOT 16" },
    { BBItem_SRANK_CANE_0, "SRANK CANE 0" },
    { BBItem_SRANK_CANE_1, "SRANK CANE 1" },
    { BBItem_SRANK_CANE_2, "SRANK CANE 2" },
    { BBItem_SRANK_CANE_3, "SRANK CANE 3" },
    { BBItem_SRANK_CANE_4, "SRANK CANE 4" },
    { BBItem_SRANK_CANE_5, "SRANK CANE 5" },
    { BBItem_SRANK_CANE_6, "SRANK CANE 6" },
    { BBItem_SRANK_CANE_7, "SRANK CANE 7" },
    { BBItem_SRANK_CANE_8, "SRANK CANE 8" },
    { BBItem_SRANK_CANE_9, "SRANK CANE 9" },
    { BBItem_SRANK_CANE_10, "SRANK CANE 10" },
    { BBItem_SRANK_CANE_11, "SRANK CANE 11" },
    { BBItem_SRANK_CANE_12, "SRANK CANE 12" },
    { BBItem_SRANK_CANE_13, "SRANK CANE 13" },
    { BBItem_SRANK_CANE_14, "SRANK CANE 14" },
    { BBItem_SRANK_CANE_15, "SRANK CANE 15" },
    { BBItem_SRANK_CANE_16, "SRANK CANE 16" },
    { BBItem_SRANK_ROD_0, "SRANK ROD 0" },
    { BBItem_SRANK_ROD_1, "SRANK ROD 1" },
    { BBItem_SRANK_ROD_2, "SRANK ROD 2" },
    { BBItem_SRANK_ROD_3, "SRANK ROD 3" },
    { BBItem_SRANK_ROD_4, "SRANK ROD 4" },
    { BBItem_SRANK_ROD_5, "SRANK ROD 5" },
    { BBItem_SRANK_ROD_6, "SRANK ROD 6" },
    { BBItem_SRANK_ROD_7, "SRANK ROD 7" },
    { BBItem_SRANK_ROD_8, "SRANK ROD 8" },
    { BBItem_SRANK_ROD_9, "SRANK ROD 9" },
    { BBItem_SRANK_ROD_10, "SRANK ROD 10" },
    { BBItem_SRANK_ROD_11, "SRANK ROD 11" },
    { BBItem_SRANK_ROD_12, "SRANK ROD 12" },
    { BBItem_SRANK_ROD_13, "SRANK ROD 13" },
    { BBItem_SRANK_ROD_14, "SRANK ROD 14" },
    { BBItem_SRANK_ROD_15, "SRANK ROD 15" },
    { BBItem_SRANK_ROD_16, "SRANK ROD 16" },
    { BBItem_SRANK_WAND_0, "SRANK WAND 0" },
    { BBItem_SRANK_WAND_1, "SRANK WAND 1" },
    { BBItem_SRANK_WAND_2, "SRANK WAND 2" },
    { BBItem_SRANK_WAND_3, "SRANK WAND 3" },
    { BBItem_SRANK_WAND_4, "SRANK WAND 4" },
    { BBItem_SRANK_WAND_5, "SRANK WAND 5" },
    { BBItem_SRANK_WAND_6, "SRANK WAND 6" },
    { BBItem_SRANK_WAND_7, "SRANK WAND 7" },
    { BBItem_SRANK_WAND_8, "SRANK WAND 8" },
    { BBItem_SRANK_WAND_9, "SRANK WAND 9" },
    { BBItem_SRANK_WAND_10, "SRANK WAND 10" },
    { BBItem_SRANK_WAND_11, "SRANK WAND 11" },
    { BBItem_SRANK_WAND_12, "SRANK WAND 12" },
    { BBItem_SRANK_WAND_13, "SRANK WAND 13" },
    { BBItem_SRANK_WAND_14, "SRANK WAND 14" },
    { BBItem_SRANK_WAND_15, "SRANK WAND 15" },
    { BBItem_SRANK_WAND_16, "SRANK WAND 16" },
    { BBItem_SRANK_TWIN_0, "SRANK TWIN 0" },
    { BBItem_SRANK_TWIN_1, "SRANK TWIN 1" },
    { BBItem_SRANK_TWIN_2, "SRANK TWIN 2" },
    { BBItem_SRANK_TWIN_3, "SRANK TWIN 3" },
    { BBItem_SRANK_TWIN_4, "SRANK TWIN 4" },
    { BBItem_SRANK_TWIN_5, "SRANK TWIN 5" },
    { BBItem_SRANK_TWIN_6, "SRANK TWIN 6" },
    { BBItem_SRANK_TWIN_7, "SRANK TWIN 7" },
    { BBItem_SRANK_TWIN_8, "SRANK TWIN 8" },
    { BBItem_SRANK_TWIN_9, "SRANK TWIN 9" },
    { BBItem_SRANK_TWIN_10, "SRANK TWIN 10" },
    { BBItem_SRANK_TWIN_11, "SRANK TWIN 11" },
    { BBItem_SRANK_TWIN_12, "SRANK TWIN 12" },
    { BBItem_SRANK_TWIN_13, "SRANK TWIN 13" },
    { BBItem_SRANK_TWIN_14, "SRANK TWIN 14" },
    { BBItem_SRANK_TWIN_15, "SRANK TWIN 15" },
    { BBItem_SRANK_TWIN_16, "SRANK TWIN 16" },
    { BBItem_SRANK_CLAW_0, "SRANK CLAW 0" },
    { BBItem_SRANK_CLAW_1, "SRANK CLAW 1" },
    { BBItem_SRANK_CLAW_2, "SRANK CLAW 2" },
    { BBItem_SRANK_CLAW_3, "SRANK CLAW 3" },
    { BBItem_SRANK_CLAW_4, "SRANK CLAW 4" },
    { BBItem_SRANK_CLAW_5, "SRANK CLAW 5" },
    { BBItem_SRANK_CLAW_6, "SRANK CLAW 6" },
    { BBItem_SRANK_CLAW_7, "SRANK CLAW 7" },
    { BBItem_SRANK_CLAW_8, "SRANK CLAW 8" },
    { BBItem_SRANK_CLAW_9, "SRANK CLAW 9" },
    { BBItem_SRANK_CLAW_10, "SRANK CLAW 10" },
    { BBItem_SRANK_CLAW_11, "SRANK CLAW 11" },
    { BBItem_SRANK_CLAW_12, "SRANK CLAW 12" },
    { BBItem_SRANK_CLAW_13, "SRANK CLAW 13" },
    { BBItem_SRANK_CLAW_14, "SRANK CLAW 14" },
    { BBItem_SRANK_CLAW_15, "SRANK CLAW 15" },
    { BBItem_SRANK_CLAW_16, "SRANK CLAW 16" },
    { BBItem_SRANK_BAZOOKA_0, "SRANK BAZOOKA 0" },
    { BBItem_SRANK_BAZOOKA_1, "SRANK BAZOOKA 1" },
    { BBItem_SRANK_BAZOOKA_2, "SRANK BAZOOKA 2" },
    { BBItem_SRANK_BAZOOKA_3, "SRANK BAZOOKA 3" },
    { BBItem_SRANK_BAZOOKA_4, "SRANK BAZOOKA 4" },
    { BBItem_SRANK_BAZOOKA_5, "SRANK BAZOOKA 5" },
    { BBItem_SRANK_BAZOOKA_6, "SRANK BAZOOKA 6" },
    { BBItem_SRANK_BAZOOKA_7, "SRANK BAZOOKA 7" },
    { BBItem_SRANK_BAZOOKA_8, "SRANK BAZOOKA 8" },
    { BBItem_SRANK_BAZOOKA_9, "SRANK BAZOOKA 9" },
    { BBItem_SRANK_BAZOOKA_10, "SRANK BAZOOKA 10" },
    { BBItem_SRANK_BAZOOKA_11, "SRANK BAZOOKA 11" },
    { BBItem_SRANK_BAZOOKA_12, "SRANK BAZOOKA 12" },
    { BBItem_SRANK_BAZOOKA_13, "SRANK BAZOOKA 13" },
    { BBItem_SRANK_BAZOOKA_14, "SRANK BAZOOKA 14" },
    { BBItem_SRANK_BAZOOKA_15, "SRANK BAZOOKA 15" },
    { BBItem_SRANK_BAZOOKA_16, "SRANK BAZOOKA 16" },
    { BBItem_SRANK_NEEDLE_0, "SRANK NEEDLE 0" },
    { BBItem_SRANK_NEEDLE_1, "SRANK NEEDLE 1" },
    { BBItem_SRANK_NEEDLE_2, "SRANK NEEDLE 2" },
    { BBItem_SRANK_NEEDLE_3, "SRANK NEEDLE 3" },
    { BBItem_SRANK_NEEDLE_4, "SRANK NEEDLE 4" },
    { BBItem_SRANK_NEEDLE_5, "SRANK NEEDLE 5" },
    { BBItem_SRANK_NEEDLE_6, "SRANK NEEDLE 6" },
    { BBItem_SRANK_NEEDLE_7, "SRANK NEEDLE 7" },
    { BBItem_SRANK_NEEDLE_8, "SRANK NEEDLE 8" },
    { BBItem_SRANK_NEEDLE_9, "SRANK NEEDLE 9" },
    { BBItem_SRANK_NEEDLE_10, "SRANK NEEDLE 10" },
    { BBItem_SRANK_NEEDLE_11, "SRANK NEEDLE 11" },
    { BBItem_SRANK_NEEDLE_12, "SRANK NEEDLE 12" },
    { BBItem_SRANK_NEEDLE_13, "SRANK NEEDLE 13" },
    { BBItem_SRANK_NEEDLE_14, "SRANK NEEDLE 14" },
    { BBItem_SRANK_NEEDLE_15, "SRANK NEEDLE 15" },
    { BBItem_SRANK_NEEDLE_16, "SRANK NEEDLE 16" },
    { BBItem_SRANK_SCYTHE_0, "SRANK SCYTHE 0" },
    { BBItem_SRANK_SCYTHE_1, "SRANK SCYTHE 1" },
    { BBItem_SRANK_SCYTHE_2, "SRANK SCYTHE 2" },
    { BBItem_SRANK_SCYTHE_3, "SRANK SCYTHE 3" },
    { BBItem_SRANK_SCYTHE_4, "SRANK SCYTHE 4" },
    { BBItem_SRANK_SCYTHE_5, "SRANK SCYTHE 5" },
    { BBItem_SRANK_SCYTHE_6, "SRANK SCYTHE 6" },
    { BBItem_SRANK_SCYTHE_7, "SRANK SCYTHE 7" },
    { BBItem_SRANK_SCYTHE_8, "SRANK SCYTHE 8" },
    { BBItem_SRANK_SCYTHE_9, "SRANK SCYTHE 9" },
    { BBItem_SRANK_SCYTHE_10, "SRANK SCYTHE 10" },
    { BBItem_SRANK_SCYTHE_11, "SRANK SCYTHE 11" },
    { BBItem_SRANK_SCYTHE_12, "SRANK SCYTHE 12" },
    { BBItem_SRANK_SCYTHE_13, "SRANK SCYTHE 13" },
    { BBItem_SRANK_SCYTHE_14, "SRANK SCYTHE 14" },
    { BBItem_SRANK_SCYTHE_15, "SRANK SCYTHE 15" },
    { BBItem_SRANK_SCYTHE_16, "SRANK SCYTHE 16" },
    { BBItem_SRANK_HAMMER_0, "SRANK HAMMER 0" },
    { BBItem_SRANK_HAMMER_1, "SRANK HAMMER 1" },
    { BBItem_SRANK_HAMMER_2, "SRANK HAMMER 2" },
    { BBItem_SRANK_HAMMER_3, "SRANK HAMMER 3" },
    { BBItem_SRANK_HAMMER_4, "SRANK HAMMER 4" },
    { BBItem_SRANK_HAMMER_5, "SRANK HAMMER 5" },
    { BBItem_SRANK_HAMMER_6, "SRANK HAMMER 6" },
    { BBItem_SRANK_HAMMER_7, "SRANK HAMMER 7" },
    { BBItem_SRANK_HAMMER_8, "SRANK HAMMER 8" },
    { BBItem_SRANK_HAMMER_9, "SRANK HAMMER 9" },
    { BBItem_SRANK_HAMMER_10, "SRANK HAMMER 10" },
    { BBItem_SRANK_HAMMER_11, "SRANK HAMMER 11" },
    { BBItem_SRANK_HAMMER_12, "SRANK HAMMER 12" },
    { BBItem_SRANK_HAMMER_13, "SRANK HAMMER 13" },
    { BBItem_SRANK_HAMMER_14, "SRANK HAMMER 14" },
    { BBItem_SRANK_HAMMER_15, "SRANK HAMMER 15" },
    { BBItem_SRANK_HAMMER_16, "SRANK HAMMER 16" },
    { BBItem_SRANK_MOON_0, "SRANK MOON 0" },
    { BBItem_SRANK_MOON_1, "SRANK MOON 1" },
    { BBItem_SRANK_MOON_2, "SRANK MOON 2" },
    { BBItem_SRANK_MOON_3, "SRANK MOON 3" },
    { BBItem_SRANK_MOON_4, "SRANK MOON 4" },
    { BBItem_SRANK_MOON_5, "SRANK MOON 5" },
    { BBItem_SRANK_MOON_6, "SRANK MOON 6" },
    { BBItem_SRANK_MOON_7, "SRANK MOON 7" },
    { BBItem_SRANK_MOON_8, "SRANK MOON 8" },
    { BBItem_SRANK_MOON_9, "SRANK MOON 9" },
    { BBItem_SRANK_MOON_10, "SRANK MOON 10" },
    { BBItem_SRANK_MOON_11, "SRANK MOON 11" },
    { BBItem_SRANK_MOON_12, "SRANK MOON 12" },
    { BBItem_SRANK_MOON_13, "SRANK MOON 13" },
    { BBItem_SRANK_MOON_14, "SRANK MOON 14" },
    { BBItem_SRANK_MOON_15, "SRANK MOON 15" },
    { BBItem_SRANK_MOON_16, "SRANK MOON 16" },
    { BBItem_SRANK_PSYCHOGUN_0, "SRANK PSYCHOGUN 0" },
    { BBItem_SRANK_PSYCHOGUN_1, "SRANK PSYCHOGUN 1" },
    { BBItem_SRANK_PSYCHOGUN_2, "SRANK PSYCHOGUN 2" },
    { BBItem_SRANK_PSYCHOGUN_3, "SRANK PSYCHOGUN 3" },
    { BBItem_SRANK_PSYCHOGUN_4, "SRANK PSYCHOGUN 4" },
    { BBItem_SRANK_PSYCHOGUN_5, "SRANK PSYCHOGUN 5" },
    { BBItem_SRANK_PSYCHOGUN_6, "SRANK PSYCHOGUN 6" },
    { BBItem_SRANK_PSYCHOGUN_7, "SRANK PSYCHOGUN 7" },
    { BBItem_SRANK_PSYCHOGUN_8, "SRANK PSYCHOGUN 8" },
    { BBItem_SRANK_PSYCHOGUN_9, "SRANK PSYCHOGUN 9" },
    { BBItem_SRANK_PSYCHOGUN_10, "SRANK PSYCHOGUN 10" },
    { BBItem_SRANK_PSYCHOGUN_11, "SRANK PSYCHOGUN 11" },
    { BBItem_SRANK_PSYCHOGUN_12, "SRANK PSYCHOGUN 12" },
    { BBItem_SRANK_PSYCHOGUN_13, "SRANK PSYCHOGUN 13" },
    { BBItem_SRANK_PSYCHOGUN_14, "SRANK PSYCHOGUN 14" },
    { BBItem_SRANK_PSYCHOGUN_15, "SRANK PSYCHOGUN 15" },
    { BBItem_SRANK_PSYCHOGUN_16, "SRANK PSYCHOGUN 16" },
    { BBItem_SRANK_PUNCH_0, "SRANK PUNCH 0" },
    { BBItem_SRANK_PUNCH_1, "SRANK PUNCH 1" },
    { BBItem_SRANK_PUNCH_2, "SRANK PUNCH 2" },
    { BBItem_SRANK_PUNCH_3, "SRANK PUNCH 3" },
    { BBItem_SRANK_PUNCH_4, "SRANK PUNCH 4" },
    { BBItem_SRANK_PUNCH_5, "SRANK PUNCH 5" },
    { BBItem_SRANK_PUNCH_6, "SRANK PUNCH 6" },
    { BBItem_SRANK_PUNCH_7, "SRANK PUNCH 7" },
    { BBItem_SRANK_PUNCH_8, "SRANK PUNCH 8" },
    { BBItem_SRANK_PUNCH_9, "SRANK PUNCH 9" },
    { BBItem_SRANK_PUNCH_10, "SRANK PUNCH 10" },
    { BBItem_SRANK_PUNCH_11, "SRANK PUNCH 11" },
    { BBItem_SRANK_PUNCH_12, "SRANK PUNCH 12" },
    { BBItem_SRANK_PUNCH_13, "SRANK PUNCH 13" },
    { BBItem_SRANK_PUNCH_14, "SRANK PUNCH 14" },
    { BBItem_SRANK_PUNCH_15, "SRANK PUNCH 15" },
    { BBItem_SRANK_PUNCH_16, "SRANK PUNCH 16" },
    { BBItem_SRANK_WINDMILL_0, "SRANK WINDMILL 0" },
    { BBItem_SRANK_WINDMILL_1, "SRANK WINDMILL 1" },
    { BBItem_SRANK_WINDMILL_2, "SRANK WINDMILL 2" },
    { BBItem_SRANK_WINDMILL_3, "SRANK WINDMILL 3" },
    { BBItem_SRANK_WINDMILL_4, "SRANK WINDMILL 4" },
    { BBItem_SRANK_WINDMILL_5, "SRANK WINDMILL 5" },
    { BBItem_SRANK_WINDMILL_6, "SRANK WINDMILL 6" },
    { BBItem_SRANK_WINDMILL_7, "SRANK WINDMILL 7" },
    { BBItem_SRANK_WINDMILL_8, "SRANK WINDMILL 8" },
    { BBItem_SRANK_WINDMILL_9, "SRANK WINDMILL 9" },
    { BBItem_SRANK_WINDMILL_10, "SRANK WINDMILL 10" },
    { BBItem_SRANK_WINDMILL_11, "SRANK WINDMILL 11" },
    { BBItem_SRANK_WINDMILL_12, "SRANK WINDMILL 12" },
    { BBItem_SRANK_WINDMILL_13, "SRANK WINDMILL 13" },
    { BBItem_SRANK_WINDMILL_14, "SRANK WINDMILL 14" },
    { BBItem_SRANK_WINDMILL_15, "SRANK WINDMILL 15" },
    { BBItem_SRANK_WINDMILL_16, "SRANK WINDMILL 16" },
    { BBItem_SRANK_HARISEN_0, "SRANK HARISEN 0" },
    { BBItem_SRANK_HARISEN_1, "SRANK HARISEN 1" },
    { BBItem_SRANK_HARISEN_2, "SRANK HARISEN 2" },
    { BBItem_SRANK_HARISEN_3, "SRANK HARISEN 3" },
    { BBItem_SRANK_HARISEN_4, "SRANK HARISEN 4" },
    { BBItem_SRANK_HARISEN_5, "SRANK HARISEN 5" },
    { BBItem_SRANK_HARISEN_6, "SRANK HARISEN 6" },
    { BBItem_SRANK_HARISEN_7, "SRANK HARISEN 7" },
    { BBItem_SRANK_HARISEN_8, "SRANK HARISEN 8" },
    { BBItem_SRANK_HARISEN_9, "SRANK HARISEN 9" },
    { BBItem_SRANK_HARISEN_10, "SRANK HARISEN 10" },
    { BBItem_SRANK_HARISEN_11, "SRANK HARISEN 11" },
    { BBItem_SRANK_HARISEN_12, "SRANK HARISEN 12" },
    { BBItem_SRANK_HARISEN_13, "SRANK HARISEN 13" },
    { BBItem_SRANK_HARISEN_14, "SRANK HARISEN 14" },
    { BBItem_SRANK_HARISEN_15, "SRANK HARISEN 15" },
    { BBItem_SRANK_HARISEN_16, "SRANK HARISEN 16" },
    { BBItem_SRANK_KATANA_0, "SRANK KATANA 0" },
    { BBItem_SRANK_KATANA_1, "SRANK KATANA 1" },
    { BBItem_SRANK_KATANA_2, "SRANK KATANA 2" },
    { BBItem_SRANK_KATANA_3, "SRANK KATANA 3" },
    { BBItem_SRANK_KATANA_4, "SRANK KATANA 4" },
    { BBItem_SRANK_KATANA_5, "SRANK KATANA 5" },
    { BBItem_SRANK_KATANA_6, "SRANK KATANA 6" },
    { BBItem_SRANK_KATANA_7, "SRANK KATANA 7" },
    { BBItem_SRANK_KATANA_8, "SRANK KATANA 8" },
    { BBItem_SRANK_KATANA_9, "SRANK KATANA 9" },
    { BBItem_SRANK_KATANA_10, "SRANK KATANA 10" },
    { BBItem_SRANK_KATANA_11, "SRANK KATANA 11" },
    { BBItem_SRANK_KATANA_12, "SRANK KATANA 12" },
    { BBItem_SRANK_KATANA_13, "SRANK KATANA 13" },
    { BBItem_SRANK_KATANA_14, "SRANK KATANA 14" },
    { BBItem_SRANK_KATANA_15, "SRANK KATANA 15" },
    { BBItem_SRANK_KATANA_16, "SRANK KATANA 16" },
    { BBItem_SRANK_J_CUTTER_0, "SRANK J-CUTTER 0" },
    { BBItem_SRANK_J_CUTTER_1, "SRANK J-CUTTER 1" },
    { BBItem_SRANK_J_CUTTER_2, "SRANK J-CUTTER 2" },
    { BBItem_SRANK_J_CUTTER_3, "SRANK J-CUTTER 3" },
    { BBItem_SRANK_J_CUTTER_4, "SRANK J-CUTTER 4" },
    { BBItem_SRANK_J_CUTTER_5, "SRANK J-CUTTER 5" },
    { BBItem_SRANK_J_CUTTER_6, "SRANK J-CUTTER 6" },
    { BBItem_SRANK_J_CUTTER_7, "SRANK J-CUTTER 7" },
    { BBItem_SRANK_J_CUTTER_8, "SRANK J-CUTTER 8" },
    { BBItem_SRANK_J_CUTTER_9, "SRANK J-CUTTER 9" },
    { BBItem_SRANK_J_CUTTER_10, "SRANK J-CUTTER 10" },
    { BBItem_SRANK_J_CUTTER_11, "SRANK J-CUTTER 11" },
    { BBItem_SRANK_J_CUTTER_12, "SRANK J-CUTTER 12" },
    { BBItem_SRANK_J_CUTTER_13, "SRANK J-CUTTER 13" },
    { BBItem_SRANK_J_CUTTER_14, "SRANK J-CUTTER 14" },
    { BBItem_SRANK_J_CUTTER_15, "SRANK J-CUTTER 15" },
    { BBItem_SRANK_J_CUTTER_16, "SRANK J-CUTTER 16" },
    { BBItem_MUSASHI, "MUSASHI" },
    { BBItem_YAMATO, "YAMATO" },
    { BBItem_ASUKA, "ASUKA" },
    { BBItem_SANGE_and_YASHA, "SANGE & YASHA" },
    { BBItem_SANGE, "SANGE" },
    { BBItem_YASHA, "YASHA" },
    { BBItem_KAMUI, "KAMUI" },
    { BBItem_PHOTON_LAUNCHER, "PHOTON LAUNCHER" },
    { BBItem_GUILTY_LIGHT, "GUILTY LIGHT" },
    { BBItem_RED_SCORPIO, "RED SCORPIO" },
    { BBItem_PHONON_MASER, "PHONON MASER" },
    { BBItem_TALIS, "TALIS" },
    { BBItem_MAHU, "MAHU" },
    { BBItem_HITOGATA, "HITOGATA" },
    { BBItem_DANCING_HITOGATA, "DANCING HITOGATA" },
    { BBItem_KUNAI, "KUNAI" },
    { BBItem_NUG2000_BAZOOKA, "NUG2000-BAZOOKA" },
    { BBItem_S_BERILLS_HANDS_0, "S-BERILLS HANDS #0" },
    { BBItem_S_BERILLS_HANDS_1, "S-BERILLS HANDS #1" },
    { BBItem_FLOWENS_SWORD_3060, "FLOWENS SWORD 3060" },
    { BBItem_FLOWENS_SWORD_3064, "FLOWENS SWORD 3064" },
    { BBItem_FLOWENS_SWORD_3067, "FLOWENS SWORD 3067" },
    { BBItem_FLOWENS_SWORD_3073, "FLOWENS SWORD 3073" },
    { BBItem_FLOWENS_SWORD_3077, "FLOWENS SWORD 3077" },
    { BBItem_FLOWENS_SWORD_3082, "FLOWENS SWORD 3082" },
    { BBItem_FLOWENS_SWORD_3083, "FLOWENS SWORD 3083" },
    { BBItem_FLOWENS_SWORD_3084, "FLOWENS SWORD 3084" },
    { BBItem_FLOWENS_SWORD_3079, "FLOWENS SWORD 3079" },
    { BBItem_DBS_SABER_3062, "DBS SABER 3062" },
    { BBItem_DBS_SABER_3067, "DBS SABER 3067" },
    { BBItem_DBS_SABER_3069_2, "DBS SABER 3069" },
    { BBItem_DBS_SABER_3064, "DBS SABER 3064" },
    { BBItem_DBS_SABER_3069_4, "DBS SABER 3069" },
    { BBItem_DBS_SABER_3073, "DBS SABER 3073" },
    { BBItem_DBS_SABER_3070, "DBS SABER 3070" },
    { BBItem_DBS_SABER_3075, "DBS SABER 3075" },
    { BBItem_DBS_SABER_3077, "DBS SABER 3077" },
    { BBItem_GI_GUE_BAZOOKA, "GI GUE BAZOOKA" },
    { BBItem_GUARDIANNA, "GUARDIANNA" },
    { BBItem_VIRIDIA_CARD, "VIRIDIA CARD" },
    { BBItem_GREENILL_CARD, "GREENILL CARD" },
    { BBItem_SKYLY_CARD, "SKYLY CARD" },
    { BBItem_BLUEFULL_CARD, "BLUEFULL CARD" },
    { BBItem_PURPLENUM_CARD, "PURPLENUM CARD" },
    { BBItem_PINKAL_CARD, "PINKAL CARD" },
    { BBItem_REDRIA_CARD, "REDRIA CARD" },
    { BBItem_ORAN_CARD, "ORAN CARD" },
    { BBItem_YELLOWBOZE_CARD, "YELLOWBOZE CARD" },
    { BBItem_WHITILL_CARD, "WHITILL CARD" },
    { BBItem_MORNING_GLORY, "MORNING GLORY" },
    { BBItem_PARTISAN_of_LIGHTNING, "PARTISAN of LIGHTNING" },
    { BBItem_GAL_WIND, "GAL WIND" },
    { BBItem_ZANBA, "ZANBA" },
    { BBItem_RIKAS_CLAW, "RIKAS CLAW" },
    { BBItem_ANGEL_HARP, "ANGEL HARP" },
    { BBItem_DEMOLITION_COMET, "DEMOLITION COMET" },
    { BBItem_NEIS_CLAW_0, "NEIS CLAW" },
    { BBItem_RAINBOW_BATON, "RAINBOW BATON" },
    { BBItem_DARK_FLOW, "DARK FLOW" },
    { BBItem_DARK_METEOR, "DARK METEOR" },
    { BBItem_DARK_BRIDGE, "DARK BRIDGE" },
    { BBItem_G_ASSASSINS_SABERS, "G-ASSASSINS SABERS" },
    { BBItem_RAPPYS_FAN, "RAPPYS FAN" },
    { BBItem_BOOMAS_CLAW, "BOOMAS CLAW" },
    { BBItem_GOBOOMAS_CLAW, "GOBOOMAS CLAW" },
    { BBItem_GIGOBOOMAS_CLAW, "GIGOBOOMAS CLAW" },
    { BBItem_RUBY_BULLET, "RUBY BULLET" },
    { BBItem_AMORE_ROSE, "AMORE ROSE" },
    { BBItem_SRANK_SWORDS_0, "SRANK SWORDS 0" },
    { BBItem_SRANK_SWORDS_1, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_2, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_3, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_4, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_5, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_6, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_7, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_8, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_9, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0A, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0B, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0C, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0D, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0E, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_0F, "SRANK SWORDS" },
    { BBItem_SRANK_SWORDS_10, "SRANK SWORDS" },
    { BBItem_SRANK_LAUNCHER_0, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_1, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_2, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_3, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_4, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_5, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_6, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_7, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_8, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_9, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0A, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0B, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0C, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0D, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0E, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_0F, "SRANK LAUNCHER" },
    { BBItem_SRANK_LAUNCHER_10, "SRANK LAUNCHER" },
    { BBItem_SRANK_CARDS_0, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_1, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_2, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_3, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_4, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_5, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_6, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_7, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_8, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_9, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0A, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0B, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0C, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0D, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0E, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_0F, "SRANK CARDS" },
    { BBItem_SRANK_CARDS_10, "SRANK CARDS" },
    { BBItem_SRANK_KNUCKLE_0, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_1, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_2, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_3, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_4, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_5, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_6, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_7, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_8, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_9, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0A, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0B, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0C, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0D, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0E, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_0F, "SRANK KNUCKLE" },
    { BBItem_SRANK_KNUCKLE_10, "SRANK KNUCKLE" },
    { BBItem_SRANK_AXE_0, "SRANK AXE" },
    { BBItem_SRANK_AXE_1, "SRANK AXE" },
    { BBItem_SRANK_AXE_2, "SRANK AXE" },
    { BBItem_SRANK_AXE_3, "SRANK AXE" },
    { BBItem_SRANK_AXE_4, "SRANK AXE" },
    { BBItem_SRANK_AXE_5, "SRANK AXE" },
    { BBItem_SRANK_AXE_6, "SRANK AXE" },
    { BBItem_SRANK_AXE_7, "SRANK AXE" },
    { BBItem_SRANK_AXE_8, "SRANK AXE" },
    { BBItem_SRANK_AXE_9, "SRANK AXE" },
    { BBItem_SRANK_AXE_0A, "SRANK AXE" },
    { BBItem_SRANK_AXE_0B, "SRANK AXE" },
    { BBItem_SRANK_AXE_0C, "SRANK AXE" },
    { BBItem_SRANK_AXE_0D, "SRANK AXE" },
    { BBItem_SRANK_AXE_0E, "SRANK AXE" },
    { BBItem_SRANK_AXE_0F, "SRANK AXE" },
    { BBItem_SRANK_AXE_10, "SRANK AXE" },
    { BBItem_SLICER_OF_FANATIC, "SLICER OF FANATIC" },
    { BBItem_LAME_DARGENT, "LAME DARGENT" },
    { BBItem_EXCALIBUR, "EXCALIBUR" },
    { BBItem_RAGE_DE_FEU_0, "RAGE DE FEU" },
    { BBItem_RAGE_DE_FEU_1, "RAGE DE FEU" },
    { BBItem_RAGE_DE_FEU_2, "RAGE DE FEU" },
    { BBItem_RAGE_DE_FEU_3, "RAGE DE FEU" },
    { BBItem_DAISY_CHAIN, "DAISY CHAIN" },
    { BBItem_OPHELIE_SEIZE, "OPHELIE SEIZE" },
    { BBItem_MILLE_MARTEAUX, "MILLE MARTEAUX" },
    { BBItem_LE_COGNEUR, "LE COGNEUR" },
    { BBItem_COMMANDER_BLADE, "COMMANDER BLADE" },
    { BBItem_VIVIENNE, "VIVIENNE" },
    { BBItem_KUSANAGI, "KUSANAGI" },
    { BBItem_SACRED_DUSTER, "SACRED DUSTER" },
    { BBItem_GUREN, "GUREN" },
    { BBItem_SHOUREN, "SHOUREN" },
    { BBItem_JIZAI, "JIZAI" },
    { BBItem_FLAMBERGE, "FLAMBERGE" },
    { BBItem_YUNCHANG, "YUNCHANG" },
    { BBItem_SNAKE_SPIRE, "SNAKE SPIRE" },
    { BBItem_FLAPJACK_FLAPPER, "FLAPJACK FLAPPER" },
    { BBItem_GETSUGASAN, "GETSUGASAN" },
    { BBItem_MAGUWA, "MAGUWA" },
    { BBItem_HEAVEN_STRIKER, "HEAVEN STRIKER" },
    { BBItem_CANNON_ROUGE, "CANNON ROUGE" },
    { BBItem_METEOR_ROUGE, "METEOR ROUGE" },
    { BBItem_SOLFERINO, "SOLFERINO" },
    { BBItem_CLIO, "CLIO" },
    { BBItem_SIREN_GLASS_HAMMER, "SIREN GLASS HAMMER" },
    { BBItem_GLIDE_DIVINE, "GLIDE DIVINE" },
    { BBItem_SHICHISHITO, "SHICHISHITO" },
    { BBItem_MURASAME, "MURASAME" },
    { BBItem_DAYLIGHT_SCAR, "DAYLIGHT SCAR" },
    { BBItem_DECALOG, "DECALOG" },
    { BBItem_5TH_ANNIV_BLADE, "5TH ANNIV. BLADE" },
    { BBItem_TYRELLS_PARASOL, "TYRELLS PARASOL" },
    { BBItem_AKIKOS_CLEAVER, "AKIKOS CLEAVER" },
    { BBItem_TANEGASHIMA, "TANEGASHIMA" },
    { BBItem_TREE_CLIPPERS, "TREE CLIPPERS" },
    { BBItem_NICE_SHOT, "NICE SHOT" },
    { BBItem_UNKNOWN3, "UNKNOWN3" },
    { BBItem_UNKNOWN4, "UNKNOWN4" },
    { BBItem_ANO_BAZOOKA, "ANO BAZOOKA" },
    { BBItem_SYNTHESIZER, "SYNTHESIZER" },
    { BBItem_BAMBOO_SPEAR, "BAMBOO SPEAR" },
    { BBItem_KANEI_TSUHO, "KANEI TSUHO" },
    { BBItem_JITTE, "JITTE" },
    { BBItem_BUTTERFLY_NET, "BUTTERFLY NET" },
    { BBItem_SYRINGE, "SYRINGE" },
    { BBItem_BATTLEDORE, "BATTLEDORE" },
    { BBItem_RACKET, "RACKET" },
    { BBItem_HAMMER, "HAMMER" },
    { BBItem_GREAT_BOUQUET, "GREAT BOUQUET" },
    { BBItem_TypeSA_SABER, "TypeSA/SABER" },
    { BBItem_TypeSL_SABER, "TypeSL/SABER" },
    { BBItem_TypeSL_SLICER, "TypeSL/SLICER" },
    { BBItem_TypeSL_CLAW, "TypeSL/CLAW" },
    { BBItem_TypeSL_KATANA, "TypeSL/KATANA" },
    { BBItem_TypeJS_SABER, "TypeJS/SABER" },
    { BBItem_TypeJS_SLICER, "TypeJS/SLICER" },
    { BBItem_TypeJS_J_SWORD, "TypeJS/J-SWORD" },
    { BBItem_TypeSW_SWORD, "TypeSW/SWORD" },
    { BBItem_TypeSW_SLICER, "TypeSW/SLICER" },
    { BBItem_TypeSW_J_SWORD, "TypeSW/J-SWORD" },
    { BBItem_TypeRO_SWORD, "TypeRO/SWORD" },
    { BBItem_TypeRO_HALBERT, "TypeRO/HALBERT" },
    { BBItem_TypeRO_ROD, "TypeRO/ROD" },
    { BBItem_TypeBL_BLADE, "TypeBL/BLADE" },
    { BBItem_TypeKN_BLADE, "TypeKN/BLADE" },
    { BBItem_TypeKN_CLAW, "TypeKN/CLAW" },
    { BBItem_TypeHA_HALBERT, "TypeHA/HALBERT" },
    { BBItem_TypeHA_ROD, "TypeHA/ROD" },
    { BBItem_TypeDS_DSABER, "TypeDS/D.SABER" },
    { BBItem_TypeDS_ROD, "TypeDS/ROD" },
    { BBItem_TypeDS, "TypeDS" },
    { BBItem_TypeCL_CLAW, "TypeCL/CLAW" },
    { BBItem_TypeSS_SW, "TypeSS/SW" },
    { BBItem_TypeGU_HAND, "TypeGU/HAND" },
    { BBItem_TypeGU_MECHGUN, "TypeGU/MECHGUN" },
    { BBItem_TypeRI_RIFLE, "TypeRI/RIFLE" },
    { BBItem_TypeME_MECHGUN, "TypeME/MECHGUN" },
    { BBItem_TypeSH_SHOT, "TypeSH/SHOT" },
    { BBItem_TypeWA_WAND, "TypeWA/WAND" },
    { BBItem_UNK_WEAPON, "UNK WEAPON" },
    { BBItem_Frame, "����" },
    { BBItem_Armor, "װ��" },
    { BBItem_Psy_Armor, "����װ��" },
    { BBItem_Giga_Frame, "Giga Frame" },
    { BBItem_Soul_Frame, "Soul Frame" },
    { BBItem_Cross_Armor, "Cross Armor" },
    { BBItem_Solid_Frame, "Solid Frame" },
    { BBItem_Brave_Armor, "Brave Armor" },
    { BBItem_Hyper_Frame, "Hyper Frame" },
    { BBItem_Grand_Armor, "Grand Armor" },
    { BBItem_Shock_Frame, "Shock Frame" },
    { BBItem_Kings_Frame, "Kings Frame" },
    { BBItem_Dragon_Frame, "Dragon Frame" },
    { BBItem_Absorb_Armor, "Absorb Armor" },
    { BBItem_Protect_Frame, "Protect Frame" },
    { BBItem_General_Armor, "General Armor" },
    { BBItem_Perfect_Frame, "Perfect Frame" },
    { BBItem_Valiant_Frame, "Valiant Frame" },
    { BBItem_Imperial_Armor, "Imperial Armor" },
    { BBItem_Holiness_Armor, "Holiness Armor" },
    { BBItem_Guardian_Armor, "Guardian Armor" },
    { BBItem_Divinity_Armor, "Divinity Armor" },
    { BBItem_Ultimate_Frame, "Ultimate Frame" },
    { BBItem_Celestial_Armor, "Celestial Armor" },
    { BBItem_HUNTER_FIELD, "HUNTER FIELD" },
    { BBItem_RANGER_FIELD, "RANGER FIELD" },
    { BBItem_FORCE_FIELD, "FORCE FIELD" },
    { BBItem_REVIVAL_GARMENT, "REVIVAL GARMENT" },
    { BBItem_SPIRIT_GARMENT, "SPIRIT GARMENT" },
    { BBItem_STINK_FRAME, "STINK FRAME" },
    { BBItem_D_PARTS_ver1_01, "D-PARTS ver1.01" },
    { BBItem_D_PARTS_ver2_10, "D-PARTS ver2.10" },
    { BBItem_PARASITE_WEAR_De_Rol, "PARASITE WEAR:De Rol" },
    { BBItem_PARASITE_WEAR_Nelgal, "PARASITE WEAR:Nelgal" },
    { BBItem_PARASITE_WEAR_Vajulla, "PARASITE WEAR:Vajulla" },
    { BBItem_SENSE_PLATE, "SENSE PLATE" },
    { BBItem_GRAVITON_PLATE, "GRAVITON PLATE" },
    { BBItem_ATTRIBUTE_PLATE, "ATTRIBUTE PLATE" },
    { BBItem_FLOWENS_FRAME, "FLOWENS FRAME" },
    { BBItem_CUSTOM_FRAME_ver_OO, "CUSTOM FRAME ver.OO" },
    { BBItem_DBS_ARMOR, "DBS ARMOR" },
    { BBItem_GUARD_WAVE, "GUARD WAVE" },
    { BBItem_DF_FIELD, "DF FIELD" },
    { BBItem_LUMINOUS_FIELD, "LUMINOUS FIELD" },
    { BBItem_CHU_CHU_FEVER, "CHU CHU FEVER" },
    { BBItem_LOVE_HEART, "LOVE HEART" },
    { BBItem_FLAME_GARMENT, "FLAME GARMENT" },
    { BBItem_VIRUS_ARMOR_Lafuteria, "VIRUS ARMOR:Lafuteria" },
    { BBItem_BRIGHTNESS_CIRCLE, "BRIGHTNESS CIRCLE" },
    { BBItem_AURA_FIELD, "AURA FIELD" },
    { BBItem_ELECTRO_FRAME, "ELECTRO FRAME" },
    { BBItem_SACRED_CLOTH, "SACRED CLOTH" },
    { BBItem_SMOKING_PLATE, "SMOKING PLATE" },
    { BBItem_STAR_CUIRASS, "STAR CUIRASS" },
    { BBItem_BLACK_HOUND_CUIRASS, "BLACK HOUND CUIRASS" },
    { BBItem_MORNING_PRAYER, "MORNING PRAYER" },
    { BBItem_BLACK_ODOSHI_DOMARU, "BLACK ODOSHI DOMARU" },
    { BBItem_RED_ODOSHI_DOMARU, "RED ODOSHI DOMARU" },
    { BBItem_BLACK_ODOSHI_RED_NIMAIDOU, "BLACK ODOSHI RED NIMAIDOU" },
    { BBItem_BLUE_ODOSHI_VIOLET_NIMAIDOU, "BLUE ODOSHI VIOLET NIMAIDOU" },
    { BBItem_DIRTY_LIFEJACKET, "DIRTY LIFEJACKET" },
    { BBItem_KROES_SWEATER, "KROES SWEATER" },
    { BBItem_WEDDING_DRESS, "WEDDING DRESS" },
    { BBItem_SONICTEAM_ARMOR, "SONICTEAM ARMOR" },
    { BBItem_RED_COAT, "RED COAT" },
    { BBItem_THIRTEEN, "THIRTEEN" },
    { BBItem_MOTHER_GARB, "MOTHER GARB" },
    { BBItem_MOTHER_GARB_PLUS, "MOTHER GARB+" },
    { BBItem_DRESS_PLATE, "DRESS PLATE" },
    { BBItem_SWEETHEART, "SWEETHEART" },
    { BBItem_IGNITION_CLOAK, "IGNITION CLOAK" },
    { BBItem_CONGEAL_CLOAK, "CONGEAL CLOAK" },
    { BBItem_TEMPEST_CLOAK, "TEMPEST CLOAK" },
    { BBItem_CURSED_CLOAK, "CURSED CLOAK" },
    { BBItem_SELECT_CLOAK, "SELECT CLOAK" },
    { BBItem_SPIRIT_CUIRASS, "SPIRIT CUIRASS" },
    { BBItem_REVIVAL_CURIASS, "REVIVAL CURIASS" },
    { BBItem_ALLIANCE_UNIFORM, "ALLIANCE UNIFORM" },
    { BBItem_OFFICER_UNIFORM, "OFFICER UNIFORM" },
    { BBItem_COMMANDER_UNIFORM, "COMMANDER UNIFORM" },
    { BBItem_CRIMSON_COAT, "CRIMSON COAT" },
    { BBItem_INFANTRY_GEAR, "INFANTRY GEAR" },
    { BBItem_LIEUTENANT_GEAR, "LIEUTENANT GEAR" },
    { BBItem_INFANTRY_MANTLE, "INFANTRY MANTLE" },
    { BBItem_LIEUTENANT_MANTLE, "LIEUTENANT MANTLE" },
    { BBItem_UNION_FIELD, "UNION FIELD" },
    { BBItem_SAMURAI_ARMOR, "SAMURAI ARMOR" },
    { BBItem_STEALTH_SUIT, "STEALTH SUIT" },
    { BBItem_UNK_ARMOR, "UNK ARMOR" },
    { BBItem_Barrier_0, "��" },
    { BBItem_Shield, "Shield" },
    { BBItem_Core_Shield, "Core Shield" },
    { BBItem_Giga_Shield, "Giga Shield" },
    { BBItem_Soul_Barrier, "Soul Barrier" },
    { BBItem_Hard_Shield, "Hard Shield" },
    { BBItem_Brave_Barrier, "Brave Barrier" },
    { BBItem_Solid_Shield, "Solid Shield" },
    { BBItem_Flame_Barrier, "Flame Barrier" },
    { BBItem_Plasma_Barrier, "Plasma Barrier" },
    { BBItem_Freeze_Barrier, "Freeze Barrier" },
    { BBItem_Psychic_Barrier, "Psychic Barrier" },
    { BBItem_General_Shield, "General Shield" },
    { BBItem_Protect_Barrier, "Protect Barrier" },
    { BBItem_Glorious_Shield, "Glorious Shield" },
    { BBItem_Imperial_Barrier, "Imperial Barrier" },
    { BBItem_Guardian_Shield, "Guardian Shield" },
    { BBItem_Divinity_Barrier, "Divinity Barrier" },
    { BBItem_Ultimate_Shield, "Ultimate Shield" },
    { BBItem_Spiritual_Shield, "Spiritual Shield" },
    { BBItem_Celestial_Shield, "Celestial Shield" },
    { BBItem_INVISIBLE_GUARD, "INVISIBLE GUARD" },
    { BBItem_SACRED_GUARD, "SACRED GUARD" },
    { BBItem_S_PARTS_ver1_16, "S-PARTS ver1.16" },
    { BBItem_S_PARTS_ver2_01, "S-PARTS ver2.01" },
    { BBItem_LIGHT_RELIEF, "LIGHT RELIEF" },
    { BBItem_SHIELD_OF_DELSABER, "SHIELD OF DELSABER" },
    { BBItem_FORCE_WALL, "FORCE WALL" },
    { BBItem_RANGER_WALL, "RANGER WALL" },
    { BBItem_HUNTER_WALL, "HUNTER WALL" },
    { BBItem_ATTRIBUTE_WALL, "ATTRIBUTE WALL" },
    { BBItem_SECRET_GEAR, "SECRET GEAR" },
    { BBItem_COMBAT_GEAR, "COMBAT GEAR" },
    { BBItem_PROTO_REGENE_GEAR, "PROTO REGENE GEAR" },
    { BBItem_REGENERATE_GEAR, "REGENERATE GEAR" },
    { BBItem_REGENE_GEAR_ADV, "REGENE GEAR ADV." },
    { BBItem_FLOWENS_SHIELD, "FLOWENS SHIELD" },
    { BBItem_CUSTOM_BARRIER_ver_OO, "CUSTOM BARRIER ver.OO" },
    { BBItem_DBS_SHIELD, "DBS SHIELD" },
    { BBItem_RED_RING, "RED RING" },
    { BBItem_TRIPOLIC_SHIELD, "TRIPOLIC SHIELD" },
    { BBItem_STANDSTILL_SHIELD, "STANDSTILL SHIELD" },
    { BBItem_SAFETY_HEART, "SAFETY HEART" },
    { BBItem_KASAMI_BRACER, "KASAMI BRACER" },
    { BBItem_GODS_SHIELD_SUZAKU, "GODS SHIELD SUZAKU" },
    { BBItem_GODS_SHIELD_GENBU, "GODS SHIELD GENBU" },
    { BBItem_GODS_SHIELD_BYAKKO, "GODS SHIELD BYAKKO" },
    { BBItem_GODS_SHIELD_SEIRYU, "GODS SHIELD SEIRYU" },
    { BBItem_HANTERS_SHELL, "HUNTERS SHELL" },
    { BBItem_RIKOS_GLASSES, "RIKOS GLASSES" },
    { BBItem_RIKOS_EARRING, "RIKOS EARRING" },
    { BBItem_BLUE_RING_33, "��ɫ����_33" },
    { BBItem_Barrier_34, "��" },
    { BBItem_SECURE_FEET, "ǧ����" },
    { BBItem_Barrier_36, "��" },
    { BBItem_Barrier_37, "��" },
    { BBItem_Barrier_38, "��" },
    { BBItem_Barrier_39, "��" },
    { BBItem_RESTA_MERGE, "ʥȪ������" },
    { BBItem_ANTI_MERGE, "״̬������" },
    { BBItem_SHIFTA_MERGE, "ǿ��������" },
    { BBItem_DEBAND_MERGE, "ǿ��������" },
    { BBItem_FOIE_MERGE, "����������" },
    { BBItem_GIFOIE_MERGE, "��ǽ������" },
    { BBItem_RAFOIE_MERGE, "����������" },
    { BBItem_RED_MERGE, "��ϵ������" },
    { BBItem_BARTA_MERGE, "����������" },
    { BBItem_GIBARTA_MERGE, "����������" },
    { BBItem_RABARTA_MERGE, "����������" },
    { BBItem_BLUE_MERGE, "��ϵ������" },
    { BBItem_ZONDE_MERGE, "����������" },
    { BBItem_GIZONDE_MERGE, "Ⱥ��������" },
    { BBItem_RAZONDE_MERGE, "�ױ�������" },
    { BBItem_YELLOW_MERGE, "��ϵ������" },
    { BBItem_RECOVERY_BARRIER, "�ظ�ϵǿ����" },
    { BBItem_ASSIST__BARRIER, "����ϵǿ����" },
    { BBItem_RED_BARRIER, "��ϵǿ����" },
    { BBItem_BLUE_BARRIER, "��ϵǿ����" },
    { BBItem_YELLOW_BARRIER, "��ϵǿ����" },
    { BBItem_WEAPONS_GOLD_SHIELD, "WEAPONS ���" },
    { BBItem_BLACK_GEAR, "��ɫ���ܶ�" },
    { BBItem_WORKS_GUARD, "WORKS ��" },
    { BBItem_RAGOL_RING, "���Ű¶�����" },
    { BBItem_BLUE_RING_53, "��ɫ����_53" },
    { BBItem_BLUE_RING_54, "��ɫ����_54" },
    { BBItem_BLUE_RING_55, "��ɫ����_55" },
    { BBItem_BLUE_RING_56, "��ɫ����_56" },
    { BBItem_BLUE_RING_57, "��ɫ����_57" },
    { BBItem_BLUE_RING_58, "��ɫ����_58" },
    { BBItem_BLUE_RING_59, "��ɫ����_59" },
    { BBItem_BLUE_RING_5A, "��ɫ����_5A" },
    { BBItem_GREEN_RING_5B, "��ɫ����_5B" },
    { BBItem_GREEN_RING_5C, "��ɫ����_5C" },
    { BBItem_GREEN_RING_5D, "��ɫ����_5D" },
    { BBItem_GREEN_RING_5E, "��ɫ����_5E" },
    { BBItem_GREEN_RING_5F, "��ɫ����_5F" },
    { BBItem_GREEN_RING_60, "��ɫ����_60" },
    { BBItem_GREEN_RING_61, "��ɫ����_61" },
    { BBItem_GREEN_RING_62, "��ɫ����_62" },
    { BBItem_YELLOW_RING_63, "��ɫ����_63" },
    { BBItem_YELLOW_RING_64, "��ɫ����_64" },
    { BBItem_YELLOW_RING_65, "��ɫ����_65" },
    { BBItem_YELLOW_RING_66, "��ɫ����_66" },
    { BBItem_YELLOW_RING_67, "��ɫ����_67" },
    { BBItem_YELLOW_RING_68, "��ɫ����_68" },
    { BBItem_YELLOW_RING_69, "��ɫ����_69" },
    { BBItem_YELLOW_RING_6A, "��ɫ����_6A" },
    { BBItem_PURPLE_RING_6B, "��ɫ����_6B" },
    { BBItem_PURPLE_RING_6C, "��ɫ����_6C" },
    { BBItem_PURPLE_RING_6D, "��ɫ����_6D" },
    { BBItem_PURPLE_RING_6E, "��ɫ����_6E" },
    { BBItem_PURPLE_RING_6F, "��ɫ����_6F" },
    { BBItem_PURPLE_RING_70, "��ɫ����_70" },
    { BBItem_PURPLE_RING_71, "��ɫ����_71" },
    { BBItem_PURPLE_RING_72, "��ɫ����_72" },
    { BBItem_ANTI_DARK_RING, "�ֿ���ɫ����" },
    { BBItem_WHITE_RING_74, "��ɫ����_74" },
    { BBItem_WHITE_RING_75, "��ɫ����_75" },
    { BBItem_WHITE_RING_76, "��ɫ����_76" },
    { BBItem_WHITE_RING_77, "��ɫ����_77" },
    { BBItem_WHITE_RING_78, "��ɫ����_78" },
    { BBItem_WHITE_RING_79, "��ɫ����_79" },
    { BBItem_WHITE_RING_7A, "��ɫ����_7A" },
    { BBItem_ANTI_LIGHT_RING, "�ֿ���������" },
    { BBItem_BLACK_RING_7C, "��ɫ����_7C" },
    { BBItem_BLACK_RING_7D, "��ɫ����_7D" },
    { BBItem_BLACK_RING_7E, "��ɫ����_7E" },
    { BBItem_BLACK_RING_7F, "��ɫ����_7F" },
    { BBItem_BLACK_RING_80, "��ɫ����_80" },
    { BBItem_BLACK_RING_81, "��ɫ����_81" },
    { BBItem_BLACK_RING_82, "��ɫ����_82" },
    { BBItem_WEAPONS_SILVER_SHIELD, "WEAPONS ����" },
    { BBItem_WEAPONS_COPPER_SHIELD, "WEAPONS ͭ��" },
    { BBItem_GRATIA, "���Ŷ�" },
    { BBItem_TRIPOLIC_REFLECTOR, "��ַ����" },
    { BBItem_STRIKER_PLUS, "���ǿ����" },
    { BBItem_REGENERATE_GEAR_BP, "�������ܶ� B.P." },
    { BBItem_RUPIKA, "¶�ſ�" },
    { BBItem_YATA_MIRROR, "���뾵" },
    { BBItem_BUNNY_EARS, "�ö�����" },
    { BBItem_CAT_EARS, "è������" },
    { BBItem_THREE_SEALS, "��ӡ֮��" },
    { BBItem_GODS_SHIELD_KOURYU, "����ܡ�������" },
    { BBItem_DF_SHIELD, "DF ֮��" },
    { BBItem_FROM_THE_DEPTHS, "��Ԩ����" },
    { BBItem_DE_ROL_LE_SHIELD, "�ϡ��������֮��" },
    { BBItem_HONEYCOMB_REFLECTOR, "���ѷ����" },
    { BBItem_EPSIGUARD, "������¡֮��" },
    { BBItem_ANGEL_RING, "��ʹԲ��" },
    { BBItem_UNION_GUARD_95, "�����_95" },
    { BBItem_UNION_GUARD_96, "�����_96" },
    { BBItem_UNION_GUARD_97, "�����_97" },
    { BBItem_UNION_GUARD_98, "�����_98" },
    { BBItem_STINK_SHIELD, "�ȳ���" },
    { BBItem_BLACK_GAUNTLETS, "��ɫ����" },
    { BBItem_GENPEI_9B, "Դƽ_9B" },
    { BBItem_GENPEI_9C, "Դƽ_9C" },
    { BBItem_GENPEI_9D, "Դƽ_9D" },
    { BBItem_GENPEI_9E, "Դƽ_9E" },
    { BBItem_GENPEI_9F, "Դƽ_9F" },
    { BBItem_GENPEI_A0, "Դƽ_A0" },
    { BBItem_GENPEI_A1, "Դƽ_A1" },
    { BBItem_GENPEI_A2, "Դƽ_A2" },
    { BBItem_GENPEI_A3, "Դƽ_A3" },
    { BBItem_GENPEI_A4, "Դƽ_A4" },
    { BBItem_UNK_SHIELD, "δ֪����" },
    { BBItem_Mag, "���" },
    { BBItem_Varuna, "��¥��" },
    { BBItem_Mitra, "�ܶ���" },
    { BBItem_Surya, "����Ү" },
    { BBItem_Vayu, "����" },
    { BBItem_Varaha, "������" },
    { BBItem_Kama, "��Ħ" },
    { BBItem_Ushasu, "��ɯ˹" },
    { BBItem_Apsaras, "����" },
    { BBItem_Kumara, "�Ħ��" },
    { BBItem_Kaitabha, "Ǭ����" },
    { BBItem_Tapas, "����˹" },
    { BBItem_Bhirava, "������" },
    { BBItem_Kalki, "�ȶ���" },
    { BBItem_Rudra, "¥����" },
    { BBItem_Marutah, "Ħ¦��" },
    { BBItem_Yaksa, "ҹ��" },
    { BBItem_Sita, "Ϥ��" },
    { BBItem_Garuda, "��¦��" },
    { BBItem_Nandin, "����" },
    { BBItem_Ashvinau, "��ϣ��" },
    { BBItem_Ribhava, "����" },
    { BBItem_Soma, "��Ħ" },
    { BBItem_Ila, "����" },
    { BBItem_Durga, "�Ŷ���" },
    { BBItem_Vritra, "��������" },
    { BBItem_Namuci, "��Ĳ��" },
    { BBItem_Sumba, "��ķ��" },
    { BBItem_Naga, "����" },
    { BBItem_Pitri, "������" },
    { BBItem_Kabanda, "�Ȱ��" },
    { BBItem_Ravana, "�޷���" },
    { BBItem_Marica, "Ħ��֧��" },
    { BBItem_Soniti, "��������" },
    { BBItem_Preta, "Ѧ���" },
    { BBItem_Andhaka, "������" },
    { BBItem_Bana, "����" },
    { BBItem_Naraka, "������" },
    { BBItem_Madhu, "���" },
    { BBItem_Churel, "������" },
    { BBItem_ROBOCHAO, "��е��ŷ" },
    { BBItem_OPA_OPA, "ŷ��ŷ��" },
    { BBItem_PIAN, "Ƥ��" },
    { BBItem_CHAO, "��ŷ" },
    { BBItem_CHU_CHU, "��" },
    { BBItem_KAPU_KAPU, "���տ���" },
    { BBItem_ANGELS_WING, "��ʹ֮��" },
    { BBItem_DEVILS_WING, "��ħ֮��" },
    { BBItem_ELENOR, "����ŵ��" },
    { BBItem_MARK3, "MARK3" },
    { BBItem_MASTER_SYSTEM, "MASTER SYSTEM" },
    { BBItem_GENESIS, "GENESIS" },
    { BBItem_SEGA_SATURN, "SEGA SATURN" },
    { BBItem_DREAMCAST, "DREAMCAST" },
    { BBItem_HAMBURGER, "������" },
    { BBItem_PANZERS_TAIL, "��β" },
    { BBItem_DAVILS_TAIL, "��ħ֮β" },
    { BBItem_Deva, "����" },
    { BBItem_Rati, "����" },
    { BBItem_Savitri, "ɯά��" },
    { BBItem_Rukmin, "¶����" },
    { BBItem_Pushan, "����" },
    { BBItem_Diwari, "������" },
    { BBItem_Sato, "���" },
    { BBItem_Bhima, "����" },
    { BBItem_Nidra, "�����" },
    { BBItem_Geung_si, "��ʬ" },
    { BBItem_Unknow1, "δ֪���1" },
    { BBItem_Tellusis, "������˹" },
    { BBItem_Striker_Unit, "�������" },
    { BBItem_Pioneer, "������" },
    { BBItem_Puyo, "����" },
    { BBItem_Moro, "Ī��" },
    { BBItem_Yahoo, "Yahoo!" },
    { BBItem_Rappy, "����" },
    { BBItem_Gael_Giel, "�����飦������" },
    { BBItem_Agastya, "��������" },
    { BBItem_Cell_of_MAG_0503_0, "���ϸ�� 0503" },
    { BBItem_Cell_of_MAG_0504_0, "���ϸ�� 0504" },
    { BBItem_Cell_of_MAG_0505_0, "���ϸ�� 0505" },
    { BBItem_Cell_of_MAG_0506_0, "���ϸ�� 0506" },
    { BBItem_Cell_of_MAG_0507_0, "���ϸ�� 0507" },
    { BBItem_UNK_MAG, "UNK MAG" },
    { BBItem_Knight_Power, "Knight/Power" },
    { BBItem_General_Power, "General/Power" },
    { BBItem_Ogre_Power, "Ogre/Power" },
    { BBItem_God_Power, "God/Power" },
    { BBItem_Priest_Mind, "Priest/Mind" },
    { BBItem_General_Mind, "General/Mind" },
    { BBItem_Angel_Mind, "Angel/Mind" },
    { BBItem_God_Mind, "God/Mind" },
    { BBItem_Marksman_Arm, "Marksman/Arm" },
    { BBItem_General_Arm, "General/Arm" },
    { BBItem_Elf_Arm, "Elf/Arm" },
    { BBItem_God_Arm, "God/Arm" },
    { BBItem_Thief_Legs, "Thief/Legs" },
    { BBItem_General_Legs, "General/Legs" },
    { BBItem_Elf_Legs, "Elf/Legs" },
    { BBItem_God_Legs, "God/Legs" },
    { BBItem_Digger_HP, "Digger/HP" },
    { BBItem_General_HP, "General/HP" },
    { BBItem_Dragon_HP, "Dragon/HP" },
    { BBItem_God_HP, "God/HP" },
    { BBItem_Magician_TP, "Magician/TP" },
    { BBItem_General_TP, "General/TP" },
    { BBItem_Angel_TP, "Angel/TP" },
    { BBItem_God_TP, "God/TP" },
    { BBItem_Warrior_Body, "Warrior/Body" },
    { BBItem_General_Body, "General/Body" },
    { BBItem_Metal_Body, "Metal/Body" },
    { BBItem_God_Body, "God/Body" },
    { BBItem_Angel_Luck, "Angel/Luck" },
    { BBItem_God_Luck, "God/Luck" },
    { BBItem_Master_Ability, "Master/Ability" },
    { BBItem_Hero_Ability, "Hero/Ability" },
    { BBItem_God_Ability, "God/Ability" },
    { BBItem_Resist_Fire, "Resist/Fire" },
    { BBItem_Resist_Flame, "Resist/Flame" },
    { BBItem_Resist_Burning, "Resist/Burning" },
    { BBItem_Resist_Cold, "Resist/Cold" },
    { BBItem_Resist_Freeze, "Resist/Freeze" },
    { BBItem_Resist_Blizzard, "Resist/Blizzard" },
    { BBItem_Resist_Shock, "Resist/Shock" },
    { BBItem_Resist_Thunder, "Resist/Thunder" },
    { BBItem_Resist_Storm, "Resist/Storm" },
    { BBItem_Resist_Light, "Resist/Light" },
    { BBItem_Resist_Saint, "Resist/Saint" },
    { BBItem_Resist_Holy, "Resist/Holy" },
    { BBItem_Resist_Dark, "Resist/Dark" },
    { BBItem_Resist_Evil, "Resist/Evil" },
    { BBItem_Resist_Devil, "Resist/Devil" },
    { BBItem_All_Resist, "All/Resist" },
    { BBItem_Super_Resist, "Super/Resist" },
    { BBItem_Perfect_Resist, "Perfect/Resist" },
    { BBItem_HP_Restorate, "HP/Restorate" },
    { BBItem_HP_Generate, "HP/Generate" },
    { BBItem_HP_Revival, "HP/Revival" },
    { BBItem_TP_Restorate, "TP/Restorate" },
    { BBItem_TP_Generate, "TP/Generate" },
    { BBItem_TP_Revival, "TP/Revival" },
    { BBItem_PB_Amplifier, "PB/Amplifier" },
    { BBItem_PB_Generate, "PB/Generate" },
    { BBItem_PB_Create, "PB/Create" },
    { BBItem_Wizard_Technique, "Wizard/Technique" },
    { BBItem_Devil_Technique, "Devil/Technique" },
    { BBItem_God_Technique, "God/Technique" },
    { BBItem_General_Battle, "General/Battle" },
    { BBItem_Devil_Battle, "Devil/Battle" },
    { BBItem_God_Battle, "God/Battle" },
    { BBItem_Cure_Poison, "Cure/Poison" },
    { BBItem_Cure_Paralysis, "Cure/Paralysis" },
    { BBItem_Cure_Slow, "Cure/Slow" },
    { BBItem_Cure_Confuse, "Cure/Confuse" },
    { BBItem_Cure_Freeze, "Cure/Freeze" },
    { BBItem_Cure_Shock, "Cure/Shock" },
    { BBItem_YASAKANI_MAGATAMA, "YASAKANI MAGATAMA" },
    { BBItem_V101, "V101" },
    { BBItem_V501, "V501" },
    { BBItem_V502, "V502" },
    { BBItem_V801, "V801" },
    { BBItem_LIMITER, "LIMITER" },
    { BBItem_ADEPT, "ADEPT" },
    { BBItem_Sl_JORDSf_WJ_LORE, "Sl-JORDSf-WJ LORE" },
    { BBItem_PROOF_OF_SWORD_SAINT, "PROOF OF SWORD-SAINT" },
    { BBItem_SMARTLINK, "SMARTLINK" },
    { BBItem_DIVINE_PROTECTION, "DIVINE PROTECTION" },
    { BBItem_Heavenly_Battle, "���ü�/ս��" },
    { BBItem_Heavenly_Power, "���ü�/����" },
    { BBItem_Heavenly_Mind, "���ü�/����" },
    { BBItem_Heavenly_Arms, "���ü�/����" },
    { BBItem_Heavenly_Legs, "���ü�/�ر�" },
    { BBItem_Heavenly_Body, "���ü�/����" },
    { BBItem_Heavenly_Luck, "���ü�/��" },
    { BBItem_Heavenly_Ability, "���ü�/ȫ����" },
    { BBItem_Centurion_Ability, "���˳�/ȫ����" },
    { BBItem_Friend_Ring, "�����ָ" },
    { BBItem_Heavenly_HP, "���ü�/HP" },
    { BBItem_Heavenly_TP, "���ü�/TP" },
    { BBItem_Heavenly_Resist, "���ü�/����" },
    { BBItem_Heavenly_Technique, "���ü�/ħ��" },
    { BBItem_HP_Ressurection, "HP/����" },
    { BBItem_TP_Ressurection, "TP/����" },
    { BBItem_PB_Increase, "PB/����" },
    { BBItem_UNK_UNIT, "UNK UNIT" },
    { BBItem_Monomate, "СHP�ظ�Һ" },
    { BBItem_Dimate, "��HP�ظ�Һ" },
    { BBItem_Trimate, "��HP�ظ�Һ" },
    { BBItem_Monofluid, "СTP�ظ�Һ" },
    { BBItem_Difluid, "��TP�ظ�Һ" },
    { BBItem_Trifluid, "��TP�ظ�Һ" },
    { BBItem_Disk_Lv01, "����:Lv.1" },
    { BBItem_Disk_Lv02, "����:Lv.2" },
    { BBItem_Disk_Lv03, "����:Lv.3" },
    { BBItem_Disk_Lv04, "����:Lv.4" },
    { BBItem_Disk_Lv05, "����:Lv.5" },
    { BBItem_Disk_Lv06, "����:Lv.6" },
    { BBItem_Disk_Lv07, "����:Lv.7" },
    { BBItem_Disk_Lv08, "����:Lv.8" },
    { BBItem_Disk_Lv09, "����:Lv.9" },
    { BBItem_Disk_Lv10, "����:Lv.10" },
    { BBItem_Disk_Lv11, "����:Lv.11" },
    { BBItem_Disk_Lv12, "����:Lv.12" },
    { BBItem_Disk_Lv13, "����:Lv.13" },
    { BBItem_Disk_Lv14, "����:Lv.14" },
    { BBItem_Disk_Lv15, "����:Lv.15" },
    { BBItem_Disk_Lv16, "����:Lv.16" },
    { BBItem_Disk_Lv17, "����:Lv.17" },
    { BBItem_Disk_Lv18, "����:Lv.18" },
    { BBItem_Disk_Lv19, "����:Lv.19" },
    { BBItem_Disk_Lv20, "����:Lv.20" },
    { BBItem_Disk_Lv21, "����:Lv.21" },
    { BBItem_Disk_Lv22, "����:Lv.22" },
    { BBItem_Disk_Lv23, "����:Lv.23" },
    { BBItem_Disk_Lv24, "����:Lv.24" },
    { BBItem_Disk_Lv25, "����:Lv.25" },
    { BBItem_Disk_Lv26, "����:Lv.26" },
    { BBItem_Disk_Lv27, "����:Lv.27" },
    { BBItem_Disk_Lv28, "����:Lv.28" },
    { BBItem_Disk_Lv29, "����:Lv.29" },
    { BBItem_Disk_Lv30, "����:Lv.30" },
    { BBItem_Sol_Atomizer, "��֮��" },
    { BBItem_Moon_Atomizer, "��֮��" },
    { BBItem_Star_Atomizer, "��֮��" },
    { BBItem_Antidote, "�ⶾ��" },
    { BBItem_Antiparalysis, "�⾷��" },
    { BBItem_Telepipe, "������" },
    { BBItem_Trap_Vision, "����̽����" },
    { BBItem_Scape_Doll, "������ż" },
    { BBItem_Monogrinder, "С��ĥʯ" },
    { BBItem_Digrinder, "�д�ĥʯ" },
    { BBItem_Trigrinder, "���ĥʯ" },
    { BBItem_Power_Material, "������ҩ" },
    { BBItem_Mind_Material, "������ҩ" },
    { BBItem_Evade_Material, "�ر���ҩ" },
    { BBItem_HP_Material, "HP ҩ" },
    { BBItem_TP_Material, "TP ҩ" },
    { BBItem_Def_Material, "������ҩ" },
    { BBItem_Luck_Material, "��֮ҩ" },
    { BBItem_Hit_Material, "������ҩ" },
    { BBItem_Cell_of_MAG_502, "���ϸ�� 502" },
    { BBItem_Cell_of_MAG_213, "���ϸ�� 213" },
    { BBItem_Parts_of_RoboChao, "��е��ŷ�Ĳ���" },
    { BBItem_Heart_of_Opa_Opa, "ŷ��ŷ��֮��" },
    { BBItem_Heart_of_Pian, "Ƥ��֮��" },
    { BBItem_Heart_of_Chao, "��ŷ֮��" },
    { BBItem_Sorcerers_Right_Arm, "���編ʦ������" },
    { BBItem_S_beats_Arms, "�����̵�˫��" },
    { BBItem_P_arms_Arms, "����ֵ�˫��" },
    { BBItem_Delsabers_Right_Arm, "��ħ������" },
    { BBItem_Bringers_Right_Arm, "������ʿ������" },
    { BBItem_Delsabers_Left_Arm, "��ħ������" },
    { BBItem_S_reds_Arm, "����̵�˫��" },
    { BBItem_Dragons_Claw_7, "��֮צ" },
    { BBItem_Hildebears_Head, "�񱩾�Գ��ͷ" },
    { BBItem_Hildeblues_Head, "�񱩰�Գ��ͷ" },
    { BBItem_Parts_of_Baranz, "���ױ��ݵ����" },
    { BBItem_Belras_Right_Arm, "�����������" },
    { BBItem_Gi_Gues_body, "Gi Gue's body" },
    { BBItem_Sinow_Berills_Aras, "Sinow Berill's Aras" },
    { BBItem_Grass_Assassins_Aras, "Grass Assassin's Aras" },
    { BBItem_Boomas_Right_Ara, "Booma's Right Ara" },
    { BBItem_Goboomas_Right_Arm, "Gobooma's Right Arm" },
    { BBItem_Gigoboomas_Right_Arm, "Gigobooma's Right Arm" },
    { BBItem_Gal_Gryphons_Wing, "Gal Gryphon's Wing" },
    { BBItem_Rappys_Ming, "Rappy's Ming" },
    { BBItem_Cladding_of_Epsilon, "Cladding of Epsilon" },
    { BBItem_De_Rol_Le_Shell, "De Rol Le Shell" },
    { BBItem_Berill_Photon, "Berill Photon" },
    { BBItem_Parasitic_gene_Flow, "Parasitic gene 'Flow'" },
    { BBItem_Magic_Stone_Iritista, "Magic Stone 'Iritista'" },
    { BBItem_Blue_black_stone, "Blue-black stone" },
    { BBItem_Syncesta, "Syncesta" },
    { BBItem_Magic_Water, "Magic Water" },
    { BBItem_Parasitic_cell_Type_D, "Parasitic cell Type D" },
    { BBItem_magic_rock_Heart_Key, "magic rock 'Heart Key'" },
    { BBItem_magic_rock_Moola, "magic rock 'Moola'" },
    { BBItem_Star_Amplifier, "Star Amplifier" },
    { BBItem_Book_of_HITOGATA, "Book of HITOGATA" },
    { BBItem_Heart_of_Chu_Chu, "Heart of Chu Chu" },
    { BBItem_Parts_of_EGG_BLASTER, "Parts of EGG BLASTER" },
    { BBItem_Heart_of_Angel, "Heart of Angel" },
    { BBItem_Heart_of_Devil, "Heart of Devil" },
    { BBItem_Kit_of_Hamburger, "Kit of Hamburger" },
    { BBItem_Panthers_Spirit, "Panther's Spirit" },
    { BBItem_Kit_of_MARK3, "Kit of MARK3" },
    { BBItem_Kit_of_MASTER_SYSTEM, "Kit of MASTER SYSTEM" },
    { BBItem_Kit_of_GENESIS, "Kit of GENESIS" },
    { BBItem_Kit_of_SEGA_SATURN, "Kit of SEGA SATURN" },
    { BBItem_Kit_of_DREAMCAST, "Kit of DREAMCAST" },
    { BBItem_Amplifier_of_Resta, "Amplifier of Resta" },
    { BBItem_Amplifier_of_Anti, "Amplifier of Anti" },
    { BBItem_Amplifier_of_Shift, "Amplifier of Shift*" },
    { BBItem_Amplifier_of_Deband, "Amplifier of Deband" },
    { BBItem_Amplifier_of_Foie, "Amplifier of Foie" },
    { BBItem_Amplifier_of_Gifoie, "Amplifier of Gifoie" },
    { BBItem_Amplifier_of_Rafoie, "Amplifier of Rafoie" },
    { BBItem_Amplifier_of_Barta, "Amplifier of Barta" },
    { BBItem_Amplifier_of_Gibarta, "Amplifier of Gibarta" },
    { BBItem_Amplifier_of_Rabarta, "Amplifier of Rabarta" },
    { BBItem_Amplifier_of_Zonde, "Amplifier of Zonde" },
    { BBItem_Amplifier_of_Gizonde, "Amplifier of Gizonde" },
    { BBItem_Amplifier_of_Razonde, "Amplifier of Razonde" },
    { BBItem_Amplifier_of_Red, "Amplifier of Red" },
    { BBItem_Amplifier_of_Blue, "Amplifier of Blue" },
    { BBItem_Amplifier_of_Yellow, "Amplifier of Yellow" },
    { BBItem_Heart_of_KAPU_KAPU, "Heart of KAPU KAPU" },
    { BBItem_Photon_Booster, "���Ӽ�����" },
    { BBItem_AddSlot, "��չ���" },
    { BBItem_Photon_Drop, "����΢��" },
    { BBItem_Photon_Sphere, "���ӽᾧ" },
    { BBItem_Photon_Crystal, "����ˮ��" },
    { BBItem_Secret_Ticket, "���ܵ�ȯ" },
    { BBItem_Photon_Ticket, "���ӵ��" },
    { BBItem_Book_of__KATANA1, "Book of  KATANA1" },
    { BBItem_Book_of__KATANA2, "Book of  KATANA2" },
    { BBItem_Book_of__KATANA3, "Book of  KATANA3" },
    { BBItem_Weapons_Bronze_Badge, "Weapons ͭ��ѫ��" },
    { BBItem_Weapons_Silver_Badge_1, "Weapons ����ѫ��_1" },
    { BBItem_Weapons_Gold_Badge_2, "Weapons ����ѫ��_2" },
    { BBItem_Weapons_Crystal_Badge_3, "Weapons ˮ��ѫ��_3" },
    { BBItem_Weapons_Steel_Badge_4, "Weapons ����ѫ��_4" },
    { BBItem_Weapons_Aluminum_Badge_5, "Weapons ����ѫ��_5" },
    { BBItem_Weapons_Leather_Badge_6, "Weapons Ƥ��ѫ��_6" },
    { BBItem_Weapons_Bone_Badge_7, "Weapons ��ͷѫ��_7" },
    { BBItem_Letter_of_appreciation, "Letter of appreciation" },
    { BBItem_Item_Ticket, "Item Ticket" },
    { BBItem_Valentines_Chocolate, "Valentine's Chocolate" },
    { BBItem_New_Years_Card, "����ؿ�" },
    { BBItem_Christmas_Card, "ʥ���ںؿ�" },
    { BBItem_Birthday_Card, "���պؿ�" },
    { BBItem_Proof_of_Sonic_Team, "�����С�ӵ�֤��" },
    { BBItem_Special_Event_Ticket, "�����볡ȯ" },
    { BBItem_Flower_Bouquet_10, "����_10" },
    { BBItem_Cake_11, "����_11" },
    { BBItem_Accessories, "Accessories" },
    { BBItem_MrNakas_Business_Card, "Mr.Naka's Business Card" },
    { BBItem_Present, "����" },
    { BBItem_Chocolate, "�ɿ���" },
    { BBItem_Candy, "�ǹ�" },
    { BBItem_Cake_2, "����_2" },
    { BBItem_Weapons_Silver_Badge_3, "Weapons ����ѫ��_3" },
    { BBItem_Weapons_Gold_Badge_4, "Weapons ����ѫ��_4" },
    { BBItem_Weapons_Crystal_Badge_5, "Weapons ˮ��ѫ��_5" },
    { BBItem_Weapons_Steel_Badge_6, "Weapons ����ѫ��_6" },
    { BBItem_Weapons_Aluminum_Badge_7, "Weapons ����ѫ��_7" },
    { BBItem_Weapons_Leather_Badge_8, "Weapons Ƥ��ѫ��_8" },
    { BBItem_Weapons_Bone_Badge_9, "Weapons ��ͷѫ��_9" },
    { BBItem_Bouquet, "����" },
    { BBItem_Decoction, "����ҩ" },
    { BBItem_Christmas_Present, "ʥ��������" },
    { BBItem_Easter_Egg, "����ڲʵ�" },
    { BBItem_Jack_0_Lantern, "�Ϲϵ�" },
    { BBItem_DISK_Vol1, "DISK Vol.1 '��������'" },
    { BBItem_DISK_Vol2, "DISK Vol.2 '����'" },
    { BBItem_DISK_Vol3, "DISK Vol.3 'ȼ�յ������'" },
    { BBItem_DISK_Vol4, "DISK Vol.4 '��������'" },
    { BBItem_DISK_Vol5, "DISK Vol.5 'Live & Learn'" },
    { BBItem_DISK_Vol6, "DISK Vol.6 '��ҹ'" },
    { BBItem_DISK_Vol7, "DISK Vol.7 '���������������ٰ�.��'" },
    { BBItem_DISK_Vol8, "DISK Vol.8 '������'" },
    { BBItem_DISK_Vol9, "DISK Vol.9 'Strange Blue'" },
    { BBItem_DISK_Vol10, "DISK Vol.10 'Reunion System'" },
    { BBItem_DISK_Vol11, "DISK Vol.11 '�۷�֮��'" },
    { BBItem_DISK_Vol12, "DISK Vol.12 '����ɴ���ս��'" },
    { BBItem_Hunters_Report_0, "���˱�����_0" },
    { BBItem_Hunters_Report_1, "���˱�����_1" },
    { BBItem_Hunters_Report_2, "���˱�����_2" },
    { BBItem_Hunters_Report_3, "���˱�����_3" },
    { BBItem_Hunters_Report_4, "���˱�����_4" },
    { BBItem_Tablet, "��λ" },
    { BBItem_UNKNOWN2, "UNKNOWN2" },
    { BBItem_Dragon_Scale, "����" },
    { BBItem_Heaven_Striker_Coat, "���ó����ͿĤ" },
    { BBItem_Pioneer_Parts, "�����ߵĲ���" },
    { BBItem_Amities_Memo, "������ı���¼" },
    { BBItem_Heart_of_Morolian, "Ħ�����˵���" },
    { BBItem_Rappys_Beak, "���������" },
    { BBItem_YahooIs_engine, "YahooI's ����" },
    { BBItem_D_Photon_Core, "D�����Ӻ���" },
    { BBItem_Liberta_Kit, "�������װ��" },
    { BBItem_Cell_of_MAG_0503_0B, "���ϸ�� 0503" },
    { BBItem_Cell_of_MAG_0504_0C, "���ϸ�� 0504" },
    { BBItem_Cell_of_MAG_0505_0D, "���ϸ�� 0505" },
    { BBItem_Cell_of_MAG_0506_0E, "���ϸ�� 0506" },
    { BBItem_Cell_of_MAG_0507_0F, "���ϸ�� 0507" },
    { BBItem_Team_Points_500, "������� 500" },
    { BBItem_Team_Points_1000, "������� 1000" },
    { BBItem_Team_Points_5000, "������� 5000" },
    { BBItem_Team_Points_10000, "������� 10000" },
    { BBItem_UNK_TOOL, "δ֪��Ʒ" },
    { BBItem_Meseta, "������" },
    { BBItem_NoSuchItem, "��Ʒ������" }
};

const char* bbitem_get_name_by_code(bbitem_code_t code, int version) {
    bbitem_map_t* cur = &bbitem_list_en[0];
    //TODO δ��ɶ�������Ʒ����
    (void)version;

    //TODO δ��ɶ�������Ʒ����
    int32_t languageCheck = 1;
    if (languageCheck) {
        cur = &bbitem_list_cn[0];
    }

    /* Take care of mags so that we'll match them properly... */
    if ((code & 0xFF) == 0x02) {
        code &= 0xFFFF;
    }

    /* Look through the list for the one we want */
    while (cur->code != Item_NoSuchItem) {
        if (cur->code == code) {
            return cur->name;
        }

        ++cur;
    }

    ERR_LOG("��ƷID������ %06X", cur->code);

    /* No item found... */
    return NULL;
}

/* ��ȡ��Ʒ���� */
const char* item_get_name(item_t* item, int version) {
    uint32_t code = item->data_b[0] | (item->data_b[1] << 8) |
        (item->data_b[2] << 16);

    /* ��ȡ�����Ʒ�����Ա� */
    switch (item->data_b[0]) {
    case ITEM_TYPE_WEAPON:  /* ���� */
        if (item->data_b[5]) {
            code = (item->data_b[5] << 8);
        }
        break;

    case ITEM_TYPE_GUARD:  /* װ�� */
        if (item->data_b[1] != 0x03 && item->data_b[3]) {
            code = code | (item->data_b[3] << 16);
        }
        //printf("����1: %02X ����3: %02X\n", item->item_data1[1], item->item_data1[3]);
        break;

    case ITEM_TYPE_MAG:  /* ��� */
        if (item->data_b[1] == 0x00 && item->data_b[2] >= 0xC9) {
            code = 0x02 | (((item->data_b[2] - 0xC9) + 0x2C) << 8);
        }
        break;

    case ITEM_TYPE_TOOL: /* ��Ʒ */
        if (code == 0x060D03 && item->data_b[3]) {
            code = 0x000E03 | ((item->data_b[3] - 1) << 16);
        }
        break;

    case ITEM_TYPE_MESETA: /* ������ */
        break;

    default:
        ERR_LOG("δ�ҵ���Ʒ����");
        break;
    }

    if (version == CLIENT_VERSION_BB)
        return bbitem_get_name_by_code((bbitem_code_t)code, version);
    else
        return item_get_name_by_code((item_code_t)code, version);
}

/* ��ӡ��Ʒ���� */
void print_item_data(item_t* item, int version) {
    ITEM_LOG("////////////////////////////////////////////////////////////");
    ITEM_LOG("��Ʒ:(ID %d / %08X) %s",
        item->item_id, item->item_id, item_get_name(item, version));
    ITEM_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
        item->data_b[0], item->data_b[1], item->data_b[2], item->data_b[3],
        item->data_b[4], item->data_b[5], item->data_b[6], item->data_b[7],
        item->data_b[8], item->data_b[9], item->data_b[10], item->data_b[11],
        item->data2_b[0], item->data2_b[1], item->data2_b[2], item->data2_b[3]);
}

/* ��ӡ��Ʒ���� */
void print_iitem_data(iitem_t* iitem, int item_index, int version) {
    ITEM_LOG("��Ʒ:(ID %d / %08X) %s",
        iitem->data.item_id, iitem->data.item_id, item_get_name(&iitem->data, version));
    ITEM_LOG(""
        "��λ (%d) "
        "(%s) %04X "
        "���� %d "
        "(%s) Flags %08X",
        item_index, 
        ((iitem->present == 0x0001) ? "��ռ��λ" : "δռ��λ"), 
        iitem->present, 
        iitem->tech, 
        ((iitem->flags == 0x00000008) ? "��װ��" : "δװ��"),
        iitem->flags
    );
    ITEM_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
        iitem->data.data_b[0], iitem->data.data_b[1], iitem->data.data_b[2], iitem->data.data_b[3],
        iitem->data.data_b[4], iitem->data.data_b[5], iitem->data.data_b[6], iitem->data.data_b[7],
        iitem->data.data_b[8], iitem->data.data_b[9], iitem->data.data_b[10], iitem->data.data_b[11],
        iitem->data.data2_b[0], iitem->data.data2_b[1], iitem->data.data2_b[2], iitem->data.data2_b[3]);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

/* ��ʼ��������Ʒ�б����� */
void clear_lobby_item(lobby_t* l) {
    uint32_t ch = 0, item_count = 0;
    size_t len;

    while (ch < l->item_count) {
        // Combs the entire game inventory for items in use
        if (l->item_list[ch] != EMPTY_STRING) {
            if (ch > item_count)
                l->item_list[item_count] = l->item_list[ch];
            item_count++;
        }
        ch++;
    }

    len = (MAX_LOBBY_SAVED_ITEMS - item_count) * 4;

    if (item_count < MAX_LOBBY_SAVED_ITEMS)
        memset(&l->item_list[item_count], 0xFF, len);

    l->item_count = item_count;
}

/* �޸���ұ������� */
void fix_up_pl_iitem(lobby_t* l, ship_client_t* c) {
    uint32_t id;
    int i;

    if (c->version == CLIENT_VERSION_BB) {
        /* ���·�����������ұ���ID */
        id = 0x00010000 | (c->client_id << 21) | (l->item_player_id[c->client_id]);

        for (i = 0; i < c->bb_pl->inv.item_count; ++i, ++id) {
            c->bb_pl->inv.iitems[i].data.item_id = LE32(id);
        }

        --id;
        l->item_player_id[c->client_id] = id;
    }
    else {
        /* Fix up the inventory for their new lobby */
        id = 0x00010000 | (c->client_id << 21) | (l->item_player_id[c->client_id]);

        for (i = 0; i < c->item_count; ++i, ++id) {
            c->iitems[i].data.item_id = LE32(id);
        }

        --id;
        l->item_player_id[c->client_id] = id;
    }
}

/* ��ʼ����Ʒ���� */
void clear_item(item_t* item) {
    item->data_l[0] = 0;
    item->data_l[1] = 0;
    item->data_l[2] = 0;
    item->item_id = 0xFFFFFFFF;
    item->data2_l = 0;
}

/* ��ʼ��������Ʒ���� */
void clear_iitem(iitem_t* iitem) {
    iitem->present = 0x0000;
    iitem->tech = 0x0000;
    iitem->flags = 0x00000000;
    clear_item(&iitem->data);
}

/* ��ʼ��������Ʒ���� */
void clear_fitem(fitem_t* fitem) {
    clear_iitem(&fitem->inv_item);
    fitem->x = 0;
    fitem->z = 0;
    fitem->area = 0;
}

/* ����һ����Ʒ������������. �������ڵ������֮ǰ������д����Ļ�����.
��������Ŀ����û������Ʒ�Ŀռ�,�򷵻�NULL. */
iitem_t* lobby_add_new_item_locked(lobby_t* l, item_t* new_item) {
    lobby_item_t* item;

    /* �����Լ��... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    if (!(item = (lobby_item_t*)malloc(sizeof(lobby_item_t))))
        return NULL;

    memset(item, 0, sizeof(lobby_item_t));

    /* Copy the item data in. */
    item->d.data.data_l[0] = LE32(new_item->data_l[0]);
    item->d.data.data_l[1] = LE32(new_item->data_l[1]);
    item->d.data.data_l[2] = LE32(new_item->data_l[2]);
    item->d.data.item_id = LE32(l->item_next_lobby_id);
    item->d.data.data2_l = LE32(new_item->data2_l);

    /* Increment the item ID, add it to the queue, and return the new item */
    ++l->item_next_lobby_id;
    TAILQ_INSERT_HEAD(&l->item_queue, item, qentry);
    return &item->d;
}

iitem_t* lobby_add_item_locked(lobby_t* l, iitem_t* it) {
    lobby_item_t* item;

    /* �����Լ��... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    item = (lobby_item_t*)malloc(sizeof(lobby_item_t));

    if (!item)
        return NULL;

    memset(item, 0, sizeof(lobby_item_t));

    /* Copy the item data in. */
    memcpy(&item->d, it, sizeof(iitem_t));

    /* Add it to the queue, and return the new item */
    TAILQ_INSERT_HEAD(&l->item_queue, item, qentry);
    return &item->d;
}

int lobby_remove_item_locked(lobby_t* l, uint32_t item_id, iitem_t* rv) {
    lobby_item_t* i, * tmp;

    if (l->version != CLIENT_VERSION_BB)
        return -1;

    memset(rv, 0, sizeof(iitem_t));
    rv->data.data_l[0] = LE32(Item_NoSuchItem);

    i = TAILQ_FIRST(&l->item_queue);
    while (i) {
        tmp = TAILQ_NEXT(i, qentry);

        if (i->d.data.item_id == item_id) {
            memcpy(rv, &i->d, sizeof(iitem_t));
            TAILQ_REMOVE(&l->item_queue, i, qentry);
            free_safe(i);
            return 0;
        }

        i = tmp;
    }

    return 1;
}

//�ͷŷ�����Ʒ����ڴ�,���ڷ�����Ʒ�������������µ���Ʒ�洢λ��
uint32_t free_lobby_item(lobby_t* l) {
    uint32_t i, rv, old_item_id;

    rv = old_item_id = EMPTY_STRING;

    // �����ƷID�ڵ�ǰ����Ϊ0,��ֱ�ӷ���
    if ((l->item_count < MAX_LOBBY_SAVED_ITEMS) && (l->item_id_to_lobby_item[l->item_count].inv_item.data.item_id == 0))
        return l->item_count;

    // ɨ��gameItem�������Ƿ��п��õ���Ʒ�� 
    for (i = 0; i < MAX_LOBBY_SAVED_ITEMS; i++) {
        if (l->item_id_to_lobby_item[i].inv_item.data.item_id == 0) {
            rv = i;
            break;
        }
    }

    if (rv != EMPTY_STRING)
        return rv;

    // ����ڴ治�㣡��ʱ��ɾ����Ϸ����ɵĵ�����Ʒ��
    for (i = 0; i < MAX_LOBBY_SAVED_ITEMS; i++) {
        if ((l->item_id_to_lobby_item[i].inv_item.data.item_id < old_item_id) &&
            (l->item_id_to_lobby_item[i].inv_item.data.item_id >= 0x810000)) {
            rv = i;
            old_item_id = l->item_id_to_lobby_item[i].inv_item.data.item_id;
        }
    }

    if (rv != EMPTY_STRING) {
        l->item_id_to_lobby_item[rv].inv_item.data.item_id = 0; // Item deleted. ��Ʒɾ��
        return rv;
    }

    ERR_LOG("��ע��:���䱳������!!!!");

    return 0;
}

/* ������ƷID */
uint32_t generate_item_id(lobby_t* l, uint8_t client_id) {
    if (client_id < l->max_clients) {
        return l->item_player_id[client_id]++;
    }

    return l->item_next_lobby_id++;
}

uint32_t primary_identifier(item_t* i) {
    // The game treats any item starting with 04 as Meseta, and ignores the rest
    // of data1 (the value is in data2)
    switch (i->data_b[0])
    {
    case ITEM_TYPE_MESETA:
        return 0x00040000;

    case ITEM_TYPE_MAG:
        return 0x00020000 | (i->data_b[1] << 8); // Mag

    case ITEM_TYPE_TOOL:
        if(i->data_b[1] == ITEM_SUBTYPE_DISK)
            return 0x00030200; // Tech disk (data1[2] is level, so omit it)

    default:
        return (i->data_b[0] << 16) | (i->data_b[1] << 8) | i->data_b[2];
    }
}

size_t stack_size_for_item(item_t item) {
    if (item.data_b[0] == ITEM_TYPE_MESETA)
        return 999999;

    if (item.data_b[0] == ITEM_TYPE_TOOL)
        if ((item.data_b[1] < 9) && (item.data_b[1] != ITEM_SUBTYPE_DISK))
            return 10;
        else if (item.data_b[1] == ITEM_SUBTYPE_PHOTON)
            return 99;

    return 1;
}

// TODO: Eliminate duplication between this function and the parallel function
// in PlayerBank
int add_item(ship_client_t* c, iitem_t iitem) {
    uint32_t pid = primary_identifier(&iitem.data);

    // �ȽϷ��ľ���, ������ֻ������ disp_data, not in the inventory struct. If the
    // item is meseta, we have to modify disp instead.
    if (pid == 0x00040000) {
        c->bb_pl->character.disp.meseta += iitem.data.data2_l;
        if (c->bb_pl->character.disp.meseta > 999999) {
            c->bb_pl->character.disp.meseta = 999999;
        }
        return 0;
    }

    // ����ѵ���Ʒ
    size_t combine_max = stack_size_for_item(iitem.data);
    if (combine_max > 1) {
        //�����ҵĿ�����Ѿ���һ����ͬ����Ʒ,���ȡ��Ʒ���� 
        size_t y;
        for (y = 0; y < c->bb_pl->inv.item_count; y++) {
            if (primary_identifier(&c->bb_pl->inv.iitems[y].data) == primary_identifier(&iitem.data)) {
                break;
            }
        }

        // ����Ѿ����ִ���ͬ���Ͷѵ���Ʒ, �����������ͬ��Ʒ��λ
        if (y < c->bb_pl->inv.item_count) {
            c->bb_pl->inv.iitems[y].data.data_b[5] += iitem.data.data_b[5];
            if (c->bb_pl->inv.iitems[y].data.data_b[5] > combine_max) {
                c->bb_pl->inv.iitems[y].data.data_b[5] = (uint8_t)combine_max;
            }
            return 0;
        }
    }

    // ������ǵ�������, then it's not meseta and not a combine item, so it needs to
    // go into an empty inventory slot
    if (c->bb_pl->inv.item_count >= 30) {
        ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷�ʰȡ!",
            c->guildcard);
        return -1;
    }

    c->bb_pl->inv.iitems[c->bb_pl->inv.item_count] = iitem;
    c->bb_pl->inv.item_count++;

    return 0;
}












/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
size_t item_get_inv_item_slot(inventory_t inv, uint32_t item_id) {
    size_t x;

    for (x = 0; x < inv.item_count; x++) {
        if (inv.iitems[x].data.item_id == item_id) {
            return x;
        }
    }

    ERR_LOG("δ�ӱ������ҵ�����Ʒ");

    ship_client_t* c = (ship_client_t*)malloc(sizeof(ship_client_t));

    if (!c)
        return -1;

    c->version = 5;

    print_item_data(&inv.iitems->data, c->version);

    free_safe(c);

    return -1;
}

/* �Ƴ�������Ʒ���� */
int item_remove_from_inv(iitem_t *inv, int inv_count, uint32_t item_id,
                         uint32_t amt) {
    int i;
    uint32_t tmp;

    /* Look for the item in question */
    for(i = 0; i < inv_count; ++i) {
        if(inv[i].data.item_id == item_id) {
            break;
        }
    }

    /* Did we find it? If not, return error. */
    if(i == inv_count) {
        return -1;
    }

    /* Check if the item is stackable, since we may have to do some stuff
       differently... */
    if(item_is_stackable(LE32(inv[i].data.data_l[0])) && amt != EMPTY_STRING) {
        tmp = inv[i].data.data_b[5];

        if(amt < tmp) {
            tmp -= amt;
            inv[i].data.data_b[5] = tmp;
            return 0;
        }
    }

    /* Move the rest of the items down to take over the place that the item in
       question used to occupy. */
    memmove(inv + i, inv + i + 1, (inv_count - i - 1) * sizeof(iitem_t));
    return 1;
}

/* ����������Ʒ���� */
int item_add_to_inv(iitem_t *inv, int inv_count, iitem_t *it) {
    int i, rv = 1;

    /* Make sure there's space first. */
    if(inv_count >= MAX_PLAYER_INV_ITEMS) {
        return -1;
    }

    //DBG_LOG("��������ǰ %d", inv_count);

    /* Look for the item in question. If it exists, we're in trouble! */
    for(i = 0; i < inv_count; ++i) {
        if(inv[i].data.item_id == it->data.item_id) {
            rv = -1;
        }
    }

    /* Check if the item is stackable, since we may have to do some stuff
       differently... */
    if(item_is_stackable(LE32(it->data.data_l[0]))) {
        /* Look for anything that matches this item in the inventory. */
        for(i = 0; i < inv_count; ++i) {
            if(inv[i].data.data_l[0] == it->data.data_l[0]) {
                inv[i].data.data_b[5] += it->data.data_b[5];
                rv = 1;
            }
        }
    }

    /* Copy the new item in at the end. */
    inv[inv_count++] = *it;

    //DBG_LOG("���������� %d", inv_count);

    return rv;
}

/* ������Ʒ���� */
void cleanup_bb_bank(ship_client_t *c) {
    uint32_t item_id = 0x80010000 | (c->client_id << 21);
    uint32_t count = LE32(c->bb_pl->bank.item_count), i;

    for(i = 0; i < count; ++i) {
        c->bb_pl->bank.items[i].data.item_id = LE32(item_id);
        ++item_id;
    }

    /* Clear all the rest of them... */
    for(; i < MAX_PLAYER_ITEMS; ++i) {
        memset(&c->bb_pl->bank.items[i], 0, sizeof(bitem_t));
        c->bb_pl->bank.items[i].data.item_id = EMPTY_STRING;
    }
}

int item_deposit_to_bank(ship_client_t *c, bitem_t *it) {
    uint32_t i, count = LE32(c->bb_pl->bank.item_count);
    int amount;

    /* Make sure there's space first. */
    if(count == MAX_PLAYER_ITEMS) {
        return -1;
    }

    /* Check if the item is stackable, since we may have to do some stuff
       differently... */
    if(item_is_stackable(LE32(it->data.data_l[0]))) {
        /* Look for anything that matches this item in the inventory. */
        for(i = 0; i < count; ++i) {
            if(c->bb_pl->bank.items[i].data.data_l[0] == it->data.data_l[0]) {
                amount = c->bb_pl->bank.items[i].data.data_b[5] += it->data.data_b[5];
                c->bb_pl->bank.items[i].amount = LE16(amount);
                return 0;
            }
        }
    }

    /* Copy the new item in at the end. */
    c->bb_pl->bank.items[count] = *it;
    ++count;
    c->bb_pl->bank.item_count = count;

    return 1;
}

int item_take_from_bank(ship_client_t *c, uint32_t item_id, uint8_t amt,
                        bitem_t *rv) {
    uint32_t i, count = LE32(c->bb_pl->bank.item_count);
    bitem_t *it;

    /* Look for the item in question */
    for(i = 0; i < count; ++i) {
        if(c->bb_pl->bank.items[i].data.item_id == item_id) {
            break;
        }
    }

    /* Did we find it? If not, return error. */
    if(i == count) {
        return -1;
    }

    /* Grab the item in question, and copy the data to the return pointer. */
    it = &c->bb_pl->bank.items[i];
    *rv = *it;

    /* Check if the item is stackable, since we may have to do some stuff
       differently... */
    if(item_is_stackable(LE32(it->data.data_l[0]))) {
        if(amt < it->data.data_b[5]) {
            it->data.data_b[5] -= amt;
            it->amount = LE16(it->data.data_b[5]);

            /* Fix the amount on the returned value, and return. */
            rv->data.data_b[5] = amt;
            rv->amount = LE16(amt);

            return 0;
        }
        else if(amt > it->data.data_b[5]) {
            return -1;
        }
    }

    /* Move the rest of the items down to take over the place that the item in
       question used to occupy. */
    memmove(c->bb_pl->bank.items + i, c->bb_pl->bank.items + i + 1,
            (count - i - 1) * sizeof(bitem_t));
    --count;
    c->bb_pl->bank.item_count = LE32(count);

    return 1;
}

/* �ѵ���Ʒ��� */
int item_is_stackable(uint32_t code) {
    if((code & 0x000000FF) == 0x03) {
        code = (code >> 8) & 0xFF;
        if(code < 0x1A && code != 0x02) {
            return 1;
        }
        //if(code < 0x09 && code != 0x02) {
        //    return 1;
        //}
    }

    return 0;
}

//���װ���������item_equip_flags
int item_check_equip(uint8_t װ����ǩ, uint8_t �ͻ���װ����ǩ)
{
    int32_t eqOK = EQUIP_FLAGS_OK;
    uint32_t ch;

    for (ch = 0; ch < EQUIP_FLAGS_MAX; ch++)
    {
        if ((�ͻ���װ����ǩ & (1 << ch)) && (!(װ����ǩ & (1 << ch))))
        {
            eqOK = EQUIP_FLAGS_NONE;
            break;
        }
    }
    return eqOK;
}

/* ��Ʒ���װ����ǩ */
int item_check_equip_flags(ship_client_t* c, uint32_t item_id) {
    pmt_weapon_bb_t tmp_wp = { 0 };
    pmt_guard_bb_t tmp_guard = { 0 };
    uint32_t found_item = 0, found_slot = 0, j = 0, slot[4] = { 0 }, inv_count = 0;
    size_t i = 0;

    i = item_get_inv_item_slot(c->bb_pl->inv, item_id);
#ifdef DEBUG
    DBG_LOG("ʶ���λ %d ������ƷID %d ������ƷID %d", i, c->bb_pl->inv.iitems[i].data.item_id, item_id);
    print_item_data(&c->bb_pl->inv.iitems[i].data, c->version);
#endif // DEBUG

    if (c->bb_pl->inv.iitems[i].data.item_id == item_id) {
        found_item = 1;
        inv_count = c->bb_pl->inv.item_count;

        switch (c->bb_pl->inv.iitems[i].data.data_b[0])
        {
        case ITEM_TYPE_WEAPON:
            if (pmt_lookup_weapon_bb(c->bb_pl->inv.iitems[i].data.data_l[0], &tmp_wp)) {
                ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                    c->guildcard);
                return -1;
            }

            if (item_check_equip(tmp_wp.equip_flag, c->equip_flags)) {
                ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                    c->guildcard);
                return -2;
            }
            else {
                // �����ɫ���κ�����������װ��������ֹ�ѵ��� 
                for (j = 0; j < inv_count; j++)
                    if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_WEAPON) &&
                        (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {
                        c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                        //DBG_LOG("ж������");
                    }
                //DBG_LOG("����ʶ�� %02X", tmp_wp.equip_flag);
            }
            break;

        case ITEM_TYPE_GUARD:
            switch (c->bb_pl->inv.iitems[i].data.data_b[1]) {
            case ITEM_SUBTYPE_FRAME:
                if (pmt_lookup_guard_bb(c->bb_pl->inv.iitems[i].data.data_l[0], &tmp_guard)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                        c->guildcard);
                    return -3;
                }

                if (c->bb_pl->character.disp.level < tmp_guard.level_req) {
                    ERR_LOG("GC %" PRIu32 " �ȼ�����, ��Ӧ��װ������Ʒ����!",
                        c->guildcard);
                    return -4;
                }

                if (item_check_equip(tmp_guard.equip_flag, c->equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                        c->guildcard);
                    return -5;
                }
                else {
                    //DBG_LOG("װ��ʶ��");
                    // �Ƴ�����װ�׺Ͳ��
                    for (j = 0; j < inv_count; ++j) {
                        if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                            (c->bb_pl->inv.iitems[j].data.data_b[1] != ITEM_SUBTYPE_BARRIER) &&
                            (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {
                            //DBG_LOG("ж��װ��");
                            c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                            c->bb_pl->inv.iitems[j].data.data_b[4] = 0x00;
                        }
                    }
                    break;
                }
                break;

            case ITEM_SUBTYPE_BARRIER: // Check barrier equip requirements ��⻤��װ������
                if (pmt_lookup_guard_bb(c->bb_pl->inv.iitems[i].data.data_l[0], &tmp_guard)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                        c->guildcard);
                    return -3;
                }

                if (c->bb_pl->character.disp.level < tmp_guard.level_req) {
                    ERR_LOG("GC %" PRIu32 " �ȼ�����, ��Ӧ��װ������Ʒ����!",
                        c->guildcard);
                    return -4;
                }

                if (item_check_equip(tmp_guard.equip_flag, c->equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                        c->guildcard);
                    return -5;
                }else {
                    //DBG_LOG("����ʶ��");
                    // Remove any other barrier
                    for (j = 0; j < inv_count; ++j) {
                        if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                            (c->bb_pl->inv.iitems[j].data.data_b[1] == ITEM_SUBTYPE_BARRIER) &&
                            (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {
                            //DBG_LOG("ж�ػ���");
                            c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                            c->bb_pl->inv.iitems[j].data.data_b[4] = 0x00;
                        }
                    }
                }
                break;

            case ITEM_SUBTYPE_UNIT:// Assign unit a slot
                //DBG_LOG("���ʶ��");
                for (j = 0; j < 4; j++)
                    slot[j] = 0;

                for (j = 0; j < inv_count; j++) {
                    // Another loop ;(
                    if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                        (c->bb_pl->inv.iitems[j].data.data_b[1] == ITEM_SUBTYPE_UNIT)) {
                        //DBG_LOG("��� %d ʶ��", j);
                        if ((c->bb_pl->inv.iitems[j].flags & LE32(0x00000008)) &&
                            (c->bb_pl->inv.iitems[j].data.data_b[4] < 0x04)) {

                            slot[c->bb_pl->inv.iitems[j].data.data_b[4]] = 1;
                            //DBG_LOG("��� %d ж��", j);
                        }
                    }
                }

                for (j = 0; j < 4; j++) {
                    if (slot[j] == 0) {
                        found_slot = j + 1;
                        break;
                    }
                }

                if (found_slot && (c->mode > 0)) {
                    found_slot--;
                    c->bb_pl->inv.iitems[j].data.data_b[4] = (uint8_t)(found_slot);
                }
                else {
                    if (found_slot) {
                        found_slot--;
                        c->bb_pl->inv.iitems[j].data.data_b[4] = (uint8_t)(found_slot);
                    }
                    else {//ȱʧ TODO
                        c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                        ERR_LOG("GC %" PRIu32 " װ�������׵Ĳ����Ʒ����!",
                            c->guildcard);
                        return -1;
                    }
                }
                break;
            }
            break;

        case ITEM_TYPE_MAG:
            //DBG_LOG("���ʶ��");
            // Remove equipped mag
            for (j = 0; j < c->bb_pl->inv.item_count; j++)
                if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_MAG) &&
                    (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {

                    c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                    //DBG_LOG("ж�����");
                }
            break;
        }

        //DBG_LOG("���ж��, ����δʶ��ɹ�");

        /* TODO: Should really make sure we can equip it first... */
        c->bb_pl->inv.iitems[i].flags |= LE32(0x00000008);
    }

    return found_item;
}

/* ���ͻ��˱�ǿɴ���ְҵװ���ı�ǩ */
int item_class_tag_equip_flag(ship_client_t* c) {
    uint8_t c_class = c->bb_pl->character.dress_data.ch_class;

    c->equip_flags = 0;

    switch (c_class)
    {
    case CLASS_HUMAR:
        c->equip_flags |= EQUIP_FLAGS_HUNTER;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_HUNEWEARL:
        c->equip_flags |= EQUIP_FLAGS_HUNTER;
        c->equip_flags |= EQUIP_FLAGS_NEWMAN;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_HUCAST:
        c->equip_flags |= EQUIP_FLAGS_HUNTER;
        c->equip_flags |= EQUIP_FLAGS_DROID;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_HUCASEAL:
        c->equip_flags |= EQUIP_FLAGS_HUNTER;
        c->equip_flags |= EQUIP_FLAGS_DROID;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_RAMAR:
        c->equip_flags |= EQUIP_FLAGS_RANGER;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_RACAST:
        c->equip_flags |= EQUIP_FLAGS_RANGER;
        c->equip_flags |= EQUIP_FLAGS_DROID;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_RACASEAL:
        c->equip_flags |= EQUIP_FLAGS_RANGER;
        c->equip_flags |= EQUIP_FLAGS_DROID;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;
    case CLASS_RAMARL:
        c->equip_flags |= EQUIP_FLAGS_RANGER;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_FONEWM:
        c->equip_flags |= EQUIP_FLAGS_FORCE;
        c->equip_flags |= EQUIP_FLAGS_NEWMAN;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_FONEWEARL:
        c->equip_flags |= EQUIP_FLAGS_FORCE;
        c->equip_flags |= EQUIP_FLAGS_NEWMAN;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_FOMARL:
        c->equip_flags |= EQUIP_FLAGS_FORCE;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_FOMAR:
        c->equip_flags |= EQUIP_FLAGS_FORCE;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;
    }

    return 0;
}