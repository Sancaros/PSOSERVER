/*
    梦幻之星中国 舰船服务器
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

/* 中途页面 用于区分GM指令的 */
#include "commands_gm.h"
#include "commands_pl_game.h"
#include "commands_pl_lobby.h"

typedef struct command {
    char trigger[10];
    int (*hnd)(ship_client_t *c, const char *params);
} command_t;

static command_t cmds[] = {
    /////////////////////////////////////////// 以下是GM指令
    { "gm"       , handle_gm            },
    { "debug"    , handle_gmdebug       },
    { "swarp"    , handle_shipwarp      },
    { "warp"     , handle_warp          },
    { "warpall"  , handle_warpall       },
    { "kill"     , handle_kill          },
    { "refresh"  , handle_refresh       },
    { "bcast"    , handle_bcast         },
    { "tmsg"     , handle_tmsg          },
    { "event"    , handle_event         },
    { "clinfo"   , handle_clinfo        },
    { "list"     , handle_list          },
    { "forgegc"  , handle_forgegc       },
    { "invuln"   , handle_invuln        },
    { "inftp"    , handle_inftp         },
    { "smite"    , handle_smite         },
    { "tp"       , handle_teleport      },
    { "showdcpc" , handle_showdcpc      },
    { "allowgc"  , handle_allowgc       },
    { "ws"       , handle_ws            },
    { "item"     , handle_item          },
    { "item1"    , handle_item1         },
    { "item2"    , handle_item2         },
    { "item3"    , handle_item3         },
    { "item4"    , handle_item4         },
    { "miitem"   , handle_miitem        },
    { "dbginv"   , handle_dbginv        },
    { "dbgbank"  , handle_dbgbank       },
    { "stfu"     , handle_stfu          },
    { "unstfu"   , handle_unstfu        },
    { "gameevent", handle_gameevent     },
    { "ban:d"    , handle_ban_d         },
    { "ban:w"    , handle_ban_w         },
    { "ban:m"    , handle_ban_m         },
    { "ban:p"    , handle_ban_p         },
    { "unban"    , handle_unban         },
    { "gbc"      , handle_gbc           },
    { "logout"   , handle_logout        },
    { "override" , handle_override      },
    { "maps"     , handle_maps          },
    { "exp"      , handle_exp           },
    { "level"    , handle_level         },
    { "sdrops"   , handle_sdrops        },
    { "trackinv" , handle_trackinv      },
    { "ep3music" , handle_ep3music      },
    { "dsdrops"  , handle_dsdrops       },
    { "lflags"   , handle_lflags        },
    { "cflags"   , handle_cflags        },
    { "t"        , handle_t             },    /* Short command = more precision. */
    { "quest"    , handle_quest         },
    { "teamlog"  , handle_teamlog       },
    { "eteamlog" , handle_eteamlog      },
    { "ib"       , handle_ib            },
    { "cheat"    , handle_cheat         },
    { "cmdc"     , handle_cmdcheck      },
    { "rsquest"  , handle_resetquest    },
    { "login"    , handle_login         },
    { "shutdown" , handle_shutdown      },
    /////////////////////////////////////////// 以下是玩家大厅指令
    { "pl"       , handle_player_menu   },
    { "save"     , handle_save          },
    { "restore"  , handle_restore       },
    { "bstat"    , handle_bstat         },
    { "legit"    , handle_legit         },
    { "restorebk", handle_restorebk     },
    { "enablebk" , handle_enablebk      },
    { "disablebk", handle_disablebk     },
    { "noevent"  , handle_noevent       },
    /////////////////////////////////////////// 以下是玩家房间指令
    { "minlvl"   , handle_min_level     },
    { "maxlvl"   , handle_max_level     },
    { "arrow"    , handle_arrow         },
    { "passwd"   , handle_passwd        },
    { "lname"    , handle_lname         },
    { "bug"      , handle_bug           },
    { "gban:d"   , handle_gban_d        },
    { "gban:w"   , handle_gban_w        },
    { "gban:m"   , handle_gban_m        },
    { "gban:p"   , handle_gban_p        },
    { "normal"   , handle_normal        },
    { "log"      , handle_log           },
    { "endlog"   , handle_endlog        },
    { "motd"     , handle_motd          },
    { "friendadd", handle_friendadd     },
    { "frienddel", handle_frienddel     },
    { "dconly"   , handle_dconly        },
    { "v1only"   , handle_v1only        },
    { "ll"       , handle_ll            },
    { "npc"      , handle_npc           },
    { "ignore"   , handle_ignore        },
    { "unignore" , handle_unignore      },
    { "quit"     , handle_quit          },
    { "cc"       , handle_cc            },
    { "qlang"    , handle_qlang         },
    { "friends"  , handle_friends       },
    { "ver"      , handle_ver           },
    { "restart"  , handle_restart       },
    { "search"   , handle_search        },
    { "showmaps" , handle_showmaps      },
    { "gcprotect", handle_gcprotect     },
    { "trackkill", handle_trackkill     },
    { "tlogin"   , handle_tlogin        },
    { "showpos"  , handle_showpos       },
    { "info"     , handle_info          },
    { "autolegit", handle_autolegit     },
    { "censor"   , handle_censor        },
    { "xblink"   , handle_xblink        },
    { "logme"    , handle_logme         },
    { "clean"    , handle_clean         },
    { "bank"     , handle_bank          },
    { "qr"       , handle_quick_return  },
    { "lb"       , handle_lobby         },
    { ""         , NULL                 }     /* End marker -- DO NOT DELETE */
};

static int command_call(ship_client_t *c, const char *txt, size_t len) {
    command_t *i = &cmds[0];
    char cmd[10] = { 0 }, params[512] = { 0 };
    const char *ch = txt + 3;           /* Skip the language code and '/'. */
    int clen = 0;

    /* Figure out what the command the user has requested is */
    while(*ch != ' ' && clen < 9 && *ch) {
        cmd[clen++] = *ch++;
    }

    cmd[clen] = '\0';

    /* Copy the params out for safety... */
    if (params) {
        if (!*ch) {
            memset(params, 0, len);
        }
        else {
            strcpy(params, ch + 1);
        }
    }else
        ERR_LOG("指令参数为空");

    /* Look through the list for the one we want */
    while (i->hnd) {
        /* If this is it, go ahead and handle it */
        if (!strcmp(cmd, i->trigger)) {
            return i->hnd(c, params);
        }

        i++;
    }

    /* Make sure a script isn't set up to respond to the user's command... */
    if (!script_execute(ScriptActionUnknownCommand, c, SCRIPT_ARG_PTR, c,
        SCRIPT_ARG_CSTRING, cmd, SCRIPT_ARG_CSTRING, params,
        SCRIPT_ARG_END)) {
        /* Send the user a message saying invalid command. */
        return send_txt(c, "%s", __(c, "\tE\tC7无效指令."));
    }

    return 0;
}

int command_parse(ship_client_t *c, dc_chat_pkt *pkt) {
    int len = LE16(pkt->hdr.dc.pkt_len), tlen = len - 12;
    size_t in, out;
    char *inptr;
    char *outptr;
    char buf[512] = { 0 };

    /* Convert the text to UTF-8. */
    in = tlen;
    out = tlen * 2;
    inptr = (char *)pkt->msg;
    outptr = buf;

    if(pkt->msg[0] == '\t' && pkt->msg[1] == 'J') {
        iconv(ic_sjis_to_utf8, &inptr, &in, &outptr, &out);
    }
    else {
        iconv(ic_8859_to_utf8, &inptr, &in, &outptr, &out);
    }

    /* Handle the command... */
    return command_call(c, buf, (tlen * 2) - out);
}

int wcommand_parse(ship_client_t *c, dc_chat_pkt *pkt) {
    int len = LE16(pkt->hdr.dc.pkt_len), tlen = len - 12;
    size_t in, out;
    char *inptr;
    char *outptr;
    char buf[512] = { 0 };

    /* Convert the text to UTF-8. */
    in = tlen;
    out = tlen * 2;
    inptr = (char *)pkt->msg;
    outptr = buf;
    iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &out);

    /* Handle the command... */
    return command_call(c, buf, (tlen * 2) - out);
}

int bbcommand_parse(ship_client_t *c, bb_chat_pkt *pkt) {
    int len = LE16(pkt->hdr.pkt_len), tlen = len - 16;
    size_t in, out;
    char *inptr;
    char *outptr;
    char buf[512] = { 0 };

    /* Convert the text to UTF-8. */
    in = tlen;
    out = tlen * 2;
    inptr = (char *)pkt->msg;
    outptr = buf;
    iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &out);

    /* Handle the command... */
    return command_call(c, buf, (tlen * 2) - out);
}
