/*
    梦幻之星中国 信息功能
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

#include "pso_message.h"

#define CLIENT_FLAG_IS_NTE          0x00010000

#define CLIENT_VERSION_DCV1     0
#define CLIENT_VERSION_DCV2     1
#define CLIENT_VERSION_PC       2
#define CLIENT_VERSION_GC       3
#define CLIENT_VERSION_EP3      4
#define CLIENT_VERSION_BB       5
#define CLIENT_VERSION_XBOX     6

/* Send a text message to the client (i.e, for stuff related to commands). */
//int send_msg(ship_client_t* c, uint16_t type, const char* fmt, ...) {
//    va_list args;
//    int rv = -1;
//
//    va_start(args, fmt);
//
//    /* Call the appropriate function. */
//    switch (c->version) {
//    case CLIENT_VERSION_DCV1:
//        /* The NTE will crash if it gets the standard version of this
//           packet, so fake it in the easiest way possible... */
//        if (c->flags & CLIENT_FLAG_IS_NTE) {
//            rv = send_dcnte_txt(c, fmt, args);
//            break;
//        }
//
//    case CLIENT_VERSION_DCV2:
//    case CLIENT_VERSION_PC:
//    case CLIENT_VERSION_GC:
//    case CLIENT_VERSION_EP3:
//    case CLIENT_VERSION_XBOX:
//        rv = send_dc_message(c, type, fmt, args);
//        break;
//
//    case CLIENT_VERSION_BB:
//        rv = send_bb_message(c, type, fmt, args);
//        break;
//    }
//
//    va_end(args);
//
//    return rv;
//}
