/*
    ÃÎ»ÃÖ®ÐÇÖÐ¹ú ½¢´¬·þÎñÆ÷
    °æÈ¨ (C) 2022, 2023 Sancaros

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
#include <stdint.h>
#include <iconv.h>

// ¼ÓÔØ iconv ×Ö·û´®×ª»»¿â
iconv_t ic_utf8_to_utf16;
iconv_t ic_utf16_to_utf8;
iconv_t ic_8859_to_utf8;//
iconv_t ic_utf8_to_8859;//
iconv_t ic_sjis_to_utf8;
iconv_t ic_utf8_to_sjis;
iconv_t ic_utf16_to_ascii;
iconv_t ic_8859_to_utf16;//
iconv_t ic_sjis_to_utf16;
iconv_t ic_utf16_to_8859;//
iconv_t ic_utf16_to_sjis;
iconv_t ic_gb18030_to_utf16;
iconv_t ic_utf16_to_gb18030;
iconv_t ic_gb18030_to_utf8;
iconv_t ic_utf8_to_gb18030;
iconv_t ic_gbk_to_utf16;
iconv_t ic_utf16_to_gbk;
iconv_t ic_sjis_to_gbk;
iconv_t ic_8859_to_gbk;
iconv_t ic_gbk_to_sjis;
iconv_t ic_gbk_to_8859;
iconv_t ic_gbk_to_utf8;
iconv_t ic_gbk_to_ascii;
iconv_t ic_utf8_to_gbk;

int init_iconv(void);

/* Ð¶ÔØ iconv ×Ö·û´®×ª»»¿â */
void cleanup_iconv(void);

char* istrncpy(iconv_t ic, char* outs, const char* ins, int out_len);
size_t strlen16(const uint16_t* str);
char* istrncpy16(iconv_t ic, char* outs, const uint16_t* ins, int out_len);
uint16_t* strcpy16(uint16_t* d, const uint16_t* s);
uint16_t* strcat16(uint16_t* d, const uint16_t* s);

size_t strlen16_raw(const void* ins);
char* istrncpy16_raw(iconv_t ic, char* outs, const void* ins, int out_len,
    int max_src);
void* strcpy16_raw(void* d, const void* s);
void* strcat16_raw(void* d, const void* s);

/* ×Ö·û´®¸ñÊ½×ª»» */
char* convert_enc(char* encFrom, char* encTo, const char* in);
