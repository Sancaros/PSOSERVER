/*
    梦幻之星中国 文本字符串
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

#ifndef PSO_HAVE_TEXT
#define PSO_HAVE_TEXT

#include "pso_text.h"

uint8_t hexToByte(char* hs)
{
    uint32_t b;

    if (hs[0] < 58) b = (hs[0] - 48); else b = (hs[0] - 55);
    b *= 16;
    if (hs[1] < 58) b += (hs[1] - 48); else b += (hs[1] - 55);
    return (uint8_t)b;
}

int add_color_tag(uint16_t* text) {
    if (!text)
        return -1;

    while (*text != 0x0000)
    {
        if ((*text == 0x0009) || (*text == 0x000A))
            *text = 0x0020;
        text++;
    }

    return 0;
}

#endif /* !PSO_HAVE_TEXT */
