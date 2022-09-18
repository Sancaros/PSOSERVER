/*
    梦幻之星中国 星门服务器 数据包
    版权 (C) 2022 Sancaros

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

#ifndef SHIPGATE_PACKETS_LIST
#define SHIPGATE_PACKETS_LIST

/* Size of the shipgate login packet. */
#define SHIPGATE_LOGINV0_SIZE       64

/* The requisite message for the msg field of the shipgate_login_pkt. */
static const char shipgate_login_msg[] =
"梦幻之星中国 星门服务器 Copyright Sancaros";

/* Flags for the flags field of shipgate_hdr_t */
#define SHDR_RESPONSE       0x8000      /* Response to a request 32768 */
#define SHDR_FAILURE        0x4000      /* Failure to complete request 16384 */

/* Types for the pkt_type field of shipgate_hdr_t */
#define SHDR_TYPE_DC        0x0001      /* A decrypted Dreamcast game packet 1 */
#define SHDR_TYPE_BB        0x0002      /* A decrypted Blue Burst game packet 2 */
#define SHDR_TYPE_PC        0x0003      /* A decrypted PCv2 game packet 3 */
#define SHDR_TYPE_GC        0x0004      /* A decrypted Gamecube game packet 4 */
#define SHDR_TYPE_EP3       0x0005      /* A decrypted Episode 3 packet 5 */
#define SHDR_TYPE_XBOX      0x0006      /* A decrypted Xbox game packet 6 */
/* 0x0007 - 0x000F reserved */
#define SHDR_TYPE_LOGIN     0x0010      /* Shipgate hello packet 16 */
#define SHDR_TYPE_COUNT     0x0011      /* A Client/Game Count update 17 */
#define SHDR_TYPE_SSTATUS   0x0012      /* A Ship has come up or gone down 18 */
#define SHDR_TYPE_PING      0x0013      /* A Ping packet, enough said 19 */
#define SHDR_TYPE_CDATA     0x0014      /* Character data 20 */
#define SHDR_TYPE_CREQ      0x0015      /* Request saved character data 21 */
#define SHDR_TYPE_USRLOGIN  0x0016      /* User login request 22 */
#define SHDR_TYPE_GCBAN     0x0017      /* Guildcard ban 23 */
#define SHDR_TYPE_IPBAN     0x0018      /* IP ban 24 */
#define SHDR_TYPE_BLKLOGIN  0x0019      /* User logs into a block 25 */
#define SHDR_TYPE_BLKLOGOUT 0x001A      /* User logs off a block 26 */
#define SHDR_TYPE_FRLOGIN   0x001B      /* A user's friend logs onto a block 27 */
#define SHDR_TYPE_FRLOGOUT  0x001C      /* A user's friend logs off a block 28 */
#define SHDR_TYPE_ADDFRIEND 0x001D      /* Add a friend to a user's list 29 */
#define SHDR_TYPE_DELFRIEND 0x001E      /* Remove a friend from a user's list 30 */
#define SHDR_TYPE_LOBBYCHG  0x001F      /* A user changes lobbies 31 */
#define SHDR_TYPE_BCLIENTS  0x0020      /* A bulk transfer of client info 32 */
#define SHDR_TYPE_KICK      0x0021      /* A kick request 33 */
#define SHDR_TYPE_FRLIST    0x0022      /* Friend list request/reply 34 */
#define SHDR_TYPE_GLOBALMSG 0x0023      /* A Global message packet 35 */
#define SHDR_TYPE_USEROPT   0x0024      /* A user's options -- sent on login 36 */
#define SHDR_TYPE_LOGIN6    0x0025      /* A ship login (potentially IPv6) 37 */
#define SHDR_TYPE_BBOPTS    0x0026      /* A user's Blue Burst options 38 */
#define SHDR_TYPE_BBOPT_REQ 0x0027      /* Request Blue Burst options 39 */
#define SHDR_TYPE_CBKUP     0x0028      /* A character data backup packet 40 */
#define SHDR_TYPE_MKILL     0x0029      /* Monster kill update 41 */
#define SHDR_TYPE_TLOGIN    0x002A      /* Token-based login request 42 */
#define SHDR_TYPE_SCHUNK    0x002B      /* Script chunk 43 */
#define SHDR_TYPE_SDATA     0x002C      /* Script data 44 */
#define SHDR_TYPE_SSET      0x002D      /* Script set 45 */
#define SHDR_TYPE_QFLAG_SET 0x002E      /* Set quest flag 46 */
#define SHDR_TYPE_QFLAG_GET 0x002F      /* Read quest flag 47 */
#define SHDR_TYPE_SHIP_CTL  0x0030      /* Ship control packet 48 */
#define SHDR_TYPE_UBLOCKS   0x0031      /* User blocklist 49 */
#define SHDR_TYPE_UBL_ADD   0x0032      /* User blocklist add 50 */
#define SHDR_TYPE_8000      0x8000      /* 0x8000 51 */

/* Flags that can be set in the login packet */
#define LOGIN_FLAG_GMONLY   0x00000001  /* Only Global GMs are allowed */
#define LOGIN_FLAG_PROXY    0x00000002  /* Is a proxy -- exclude many pkts */
#define LOGIN_FLAG_NOV1     0x00000010  /* Do not allow DCv1 clients */
#define LOGIN_FLAG_NOV2     0x00000020  /* Do not allow DCv2 clients */
#define LOGIN_FLAG_NOPC     0x00000040  /* Do not allow PSOPC clients */
#define LOGIN_FLAG_NOEP12   0x00000080  /* Do not allow PSO Ep1&2 clients */
#define LOGIN_FLAG_NOEP3    0x00000100  /* Do not allow PSO Ep3 clients */
#define LOGIN_FLAG_NOBB     0x00000200  /* Do not allow PSOBB clients */
#define LOGIN_FLAG_NODCNTE  0x00000400  /* Do not allow DC NTE clients */
#define LOGIN_FLAG_NOXBOX   0x00000800  /* Do not allow Xbox clients */
#define LOGIN_FLAG_NOPCNTE  0x00001000  /* Do not allow PC NTE clients */
#define LOGIN_FLAG_LUA      0x00020000  /* Ship supports Lua scripting */
#define LOGIN_FLAG_32BIT    0x00040000  /* Ship is running on a 32-bit cpu */
#define LOGIN_FLAG_BE       0x00080000  /* Ship is big endian */
/* All other flags are reserved. */

/* "Version" values for the token login packet. */
#define TLOGIN_VER_NORMAL   0x00
#define TLOGIN_VER_XBOX     0x01

/* General error codes */
#define ERR_NO_ERROR            0x00000000
#define ERR_BAD_ERROR           0x80000001
#define ERR_REQ_LOGIN           0x80000002

/* Error codes in response to shipgate_login_reply_pkt */
#define ERR_LOGIN_BAD_KEY       0x00000001
#define ERR_LOGIN_BAD_PROTO     0x00000002
#define ERR_LOGIN_BAD_MENU      0x00000003  /* bad menu code (out of range) */
#define ERR_LOGIN_INVAL_MENU    0x00000004  /* menu code not allowed */

/* Error codes in response to game packets */
#define ERR_GAME_UNK_PACKET     0x00000001

/* Error codes in response to a character request */
#define ERR_CREQ_NO_DATA        0x00000001

/* Error codes in response to a client login */
#define ERR_USRLOGIN_NO_ACC     0x00000001
#define ERR_USRLOGIN_BAD_CRED   0x00000002
#define ERR_USRLOGIN_BAD_PRIVS  0x00000003

/* Error codes in response to a ban request */
#define ERR_BAN_NOT_GM          0x00000001
#define ERR_BAN_BAD_TYPE        0x00000002
#define ERR_BAN_PRIVILEGE       0x00000003

/* Error codes in response to a block login */
#define ERR_BLOGIN_INVAL_NAME   0x00000001
#define ERR_BLOGIN_ONLINE       0x00000002

/* Error codes in response to a ship control */
#define ERR_SCTL_UNKNOWN_CTL    0x00000001

/* Possible values for user options */
#define USER_OPT_QUEST_LANG     0x00000001
#define USER_OPT_ENABLE_BACKUP  0x00000002
#define USER_OPT_GC_PROTECT     0x00000003
#define USER_OPT_TRACK_KILLS    0x00000004
#define USER_OPT_LEGIT_ALWAYS   0x00000005
#define USER_OPT_WORD_CENSOR    0x00000006

/* Possible values for the fw_flags on a forwarded packet */
#define FW_FLAG_PREFER_IPV6     0x00000001  /* Prefer IPv6 on reply */
#define FW_FLAG_IS_PSOPC        0x00000002  /* Client is on PSOPC */

/* Possible values for versions in packets that need them. This list should be
   kept in-sync with those in clients.h from ship_server. */
#define CLIENT_VERSION_DCV1     0
#define CLIENT_VERSION_DCV2     1
#define CLIENT_VERSION_PC       2
#define CLIENT_VERSION_GC       3
#define CLIENT_VERSION_EP3      4
#define CLIENT_VERSION_BB       5
#define CLIENT_VERSION_XBOX     6

   /* Potentially ORed with any version codes, if needed/appropriate. */
#define CLIENT_QUESTING         0x20
#define CLIENT_CHALLENGE_MODE   0x40
#define CLIENT_BATTLE_MODE      0x80

/* Types for the script chunk packet. */
#define SCHUNK_TYPE_SCRIPT      0x01
#define SCHUNK_TYPE_MODULE      0x02
#define SCHUNK_CHECK            0x80

/* Error codes for schunk */
#define ERR_SCHUNK_NEED_SCRIPT  0x00000001

/* Error codes for quest flags */
#define ERR_QFLAG_NO_DATA       0x00000001
#define ERR_QFLAG_INVALID_FLAG  0x00000002

/* OR these into the flag_id for qflags to modify how the packets work... */
#define QFLAG_LONG_FLAG         0x80000000
#define QFLAG_DELETE_FLAG       0x40000000  /* Only valid on a set */

/* Ship control types. */
#define SCTL_TYPE_UNAME         0x00000001
#define SCTL_TYPE_VERSION       0x00000002
#define SCTL_TYPE_RESTART       0x00000003
#define SCTL_TYPE_SHUTDOWN      0x00000004

/* Things that can be blocked on the user blocklist */
#define BLOCKLIST_CHAT          0x00000001  /* Lobby chat and word select */
#define BLOCKLIST_SCHAT         0x00000002  /* Lobby symbol chat */
#define BLOCKLIST_MAIL          0x00000004  /* Simple mail */
#define BLOCKLIST_GSEARCH       0x00000008  /* Guild card search */
#define BLOCKLIST_FLIST         0x00000010  /* Friend list status */
#define BLOCKLIST_CSEARCH       0x00000020  /* Choice search */
#define BLOCKLIST_IGCHAT        0x00000040  /* Game chat and word select */
#define BLOCKLIST_IGSCHAT       0x00000080  /* Game symbol chat */

#endif /* !SHIPGATE_PACKETS_LIST */