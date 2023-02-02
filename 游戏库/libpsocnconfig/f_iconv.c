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

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <iconv.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "f_logs.h"
#include "debug.h"
#include "f_iconv.h"

int init_iconv(void) {
    ic_utf8_to_utf16 = iconv_open(UTF16LE, UTF8);

    if (ic_utf8_to_utf16 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-8 转换 UTF-16)");
        return -1;
    }

    ic_utf16_to_utf8 = iconv_open(UTF8, UTF16LE);

    if (ic_utf16_to_utf8 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-16 转换 UTF-8)");
        goto out1;
    }

    ic_8859_to_utf8 = iconv_open(UTF8, ISO8859);

    if (ic_8859_to_utf8 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (ISO-8859-1 转换 UTF-8)");
        goto out2;
    }

    ic_utf8_to_8859 = iconv_open(ISO8859, UTF8);

    if (ic_utf8_to_8859 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-8 转换 ISO-8859-1)");
        goto out3;
    }

    ic_sjis_to_utf8 = iconv_open(UTF8, SJIS);

    if (ic_sjis_to_utf8 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (SHIFT_JIS 转换 UTF-8)");
        goto out4;
    }

    ic_utf8_to_sjis = iconv_open(SJIS, UTF8);

    if (ic_utf8_to_sjis == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-8 转换 SHIFT_JIS)");
        goto out5;
    }

    ic_utf16_to_ascii = iconv_open(ASCII, UTF16LE);

    if (ic_utf16_to_ascii == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-16 转换 ASCII)");
        goto out6;
    }

    ic_utf16_to_sjis = iconv_open(SJIS, UTF16LE);

    if (ic_utf16_to_sjis == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-16 转换 SHIFT_JIS)");
        goto out7;
    }

    ic_utf16_to_8859 = iconv_open(ISO8859, UTF16LE);

    if (ic_utf16_to_8859 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-16 转换 ISO-8859-1)");
        goto out8;
    }

    ic_sjis_to_utf16 = iconv_open(UTF16LE, SJIS);

    if (ic_sjis_to_utf16 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (SHIFT_JIS 转换 UTF-16)");
        goto out9;
    }

    ic_8859_to_utf16 = iconv_open(UTF16LE, ISO8859);

    if (ic_8859_to_utf16 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (ISO-8859-1 转换 UTF-16)");
        goto out10;
    }

    ic_gb18030_to_utf16 = iconv_open(UTF16LE, GB18030);

    if (ic_gb18030_to_utf16 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (GB18030 转换 UTF-16)");
        goto out11;
    }

    ic_utf16_to_gb18030 = iconv_open(GB18030, UTF16LE);

    if (ic_utf16_to_gb18030 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-16 转换 GB18030)");
        goto out12;
    }

    ic_gb18030_to_utf8 = iconv_open(UTF8, GB18030);

    if (ic_gb18030_to_utf8 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (GB18030 转换 UTF-8)");
        goto out13;
    }

    ic_utf8_to_gb18030 = iconv_open(GB18030, UTF8);

    if (ic_utf8_to_gb18030 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-8 转换 GB18030)");
        goto out14;
    }

    ic_gbk_to_utf16 = iconv_open(UTF16LE, GBK);

    if (ic_gbk_to_utf16 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (GBK 转换 UTF-16)");
        goto out15;
    }

    ic_utf16_to_gbk = iconv_open(GBK, UTF16LE);

    if (ic_utf16_to_gbk == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-16 转换 GBK)");
        goto out16;
    }

    ic_sjis_to_gbk = iconv_open(GBK, SJIS);

    if (ic_sjis_to_gbk == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (SHIFT_JIS 转换 GBK)");
        goto out17;
    }

    ic_8859_to_gbk = iconv_open(GBK, ISO8859);

    if (ic_8859_to_gbk == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (ISO-8859-1 转换 GBK)");
        goto out18;
    }

    ic_gbk_to_sjis = iconv_open(SJIS, GBK);

    if (ic_gbk_to_sjis == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (GBK 转换 SHIFT_JIS)");
        goto out19;
    }

    ic_gbk_to_8859 = iconv_open(ISO8859, GBK);

    if (ic_gbk_to_8859 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (GBK 转换 ISO-8859-1)");
        goto out20;
    }

    ic_gbk_to_utf8 = iconv_open(UTF8, GBK);

    if (ic_gbk_to_utf8 == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (GBK 转换 UTF-8)");
        goto out21;
    }

    ic_gbk_to_ascii = iconv_open(ASCII, GBK);

    if (ic_gbk_to_ascii == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (GBK 转换 UTF-8)");
        goto out22;
    }

    ic_utf8_to_gbk = iconv_open(GBK, UTF8);

    if (ic_utf8_to_gbk == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文 (UTF-8 转换 SHIFT_JIS)");
        goto out23;
    }

    return 0;

out23:
    iconv_close(ic_gbk_to_ascii);
out22:
    iconv_close(ic_gbk_to_utf8);
out21:
    iconv_close(ic_gbk_to_8859);
out20:
    iconv_close(ic_gbk_to_sjis);
out19:
    iconv_close(ic_8859_to_gbk);
out18:
    iconv_close(ic_sjis_to_gbk);
out17:
    iconv_close(ic_utf16_to_gbk);
out16:
    iconv_close(ic_gbk_to_utf16);
out15:
    iconv_close(ic_utf8_to_gb18030);
out14:
    iconv_close(ic_gb18030_to_utf8);
out13:
    iconv_close(ic_utf16_to_gb18030);
out12:
    iconv_close(ic_gb18030_to_utf16);
out11:
    iconv_close(ic_8859_to_utf16);
out10:
    iconv_close(ic_sjis_to_utf16);
out9:
    iconv_close(ic_utf16_to_8859);
out8:
    iconv_close(ic_utf16_to_sjis);
out7:
    iconv_close(ic_utf16_to_ascii);
out6:
    iconv_close(ic_utf8_to_sjis);
out5:
    iconv_close(ic_sjis_to_utf8);
out4:
    iconv_close(ic_utf8_to_8859);
out3:
    iconv_close(ic_8859_to_utf8);
out2:
    iconv_close(ic_utf16_to_utf8);
out1:
    iconv_close(ic_utf8_to_utf16);
    return -1;
}

void cleanup_iconv(void) {
    iconv_close(ic_utf8_to_gbk);
    iconv_close(ic_gbk_to_ascii);
    iconv_close(ic_gbk_to_utf8);
    iconv_close(ic_gbk_to_8859);
    iconv_close(ic_gbk_to_sjis);
    iconv_close(ic_8859_to_gbk);
    iconv_close(ic_sjis_to_gbk);
    iconv_close(ic_utf16_to_gbk);
    iconv_close(ic_gbk_to_utf16);
    iconv_close(ic_utf16_to_gb18030);
    iconv_close(ic_gb18030_to_utf16);
    iconv_close(ic_8859_to_utf16);
    iconv_close(ic_sjis_to_utf16);
    iconv_close(ic_utf16_to_8859);
    iconv_close(ic_utf16_to_sjis);
    iconv_close(ic_utf16_to_ascii);
    iconv_close(ic_utf8_to_sjis);
    iconv_close(ic_sjis_to_utf8);
    iconv_close(ic_utf8_to_8859);
    iconv_close(ic_8859_to_utf8);
    iconv_close(ic_utf16_to_utf8);
    iconv_close(ic_utf8_to_utf16);
}

char* istrncpy(iconv_t ic, char* outs, const char* ins, int out_len) {
    size_t in, out;
    char* inptr;
    char* outptr;

    /* Clear the output buffer first */
    memset(outs, 0, out_len);

    in = strlen(ins);
    out = out_len;
    inptr = (char*)ins;
    outptr = outs;
    iconv(ic, &inptr, &in, &outptr, &out);

    return outptr;
}

size_t strlen16(const uint16_t* str) {
    size_t sz = 0;

    while (*str++) ++sz;
    return sz;
}

char* istrncpy16(iconv_t ic, char* outs, const uint16_t* ins, int out_len) {
    size_t in, out;
    char* inptr;
    char* outptr;

    /* Clear the output buffer first */
    memset(outs, 0, out_len);

    in = strlen16(ins) * 2;
    out = out_len;
    inptr = (char*)ins;
    outptr = outs;
    iconv(ic, &inptr, &in, &outptr, &out);

    return outptr;
}

uint16_t* strcpy16(uint16_t* d, const uint16_t* s) {
    uint16_t* rv = d;
    while ((*d++ = *s++));
    return rv;
}

uint16_t* strcat16(uint16_t* d, const uint16_t* s) {
    uint16_t* rv = d;

    /* Move to the end of the string */
    while (*d++);

    /* Tack on the new part */
    --d;
    while ((*d++ = *s++));
    return rv;
}

size_t strlen16_raw(const void* str) {
    size_t sz = 0;
    const uint8_t* str8 = (const uint8_t*)str;
    uint16_t tmp = *str8 | *(str8 + 1) << 8;

    while (tmp) {
        str8 += 2;
        ++sz;
        /* Note: We don't care about endianness here, as we're looking for 0.
           That said, we should always be dealing with little endian UTF-16. */
        tmp = *str8 | *(str8 + 1) << 8;
    }

    return sz;
}

char* istrncpy16_raw(iconv_t ic, char* outs, const void* ins,
    int out_len, int max_src) {
    size_t len = (size_t)max_src;

    if (max_src <= 0) {
        len = strlen16_raw(ins);
    }

    if (max_src > 0) {
        uint16_t src[4096];

        memcpy(src, ins, sizeof(uint16_t) * len);
        src[len] = 0;
        return istrncpy16(ic, outs, src, out_len);
    }
    else if (out_len > 0) {
        *outs = 0;
        return outs;
    }
    else {
        return NULL;
    }
}

void* strcpy16_raw(void* d, const void* s) {
    uint8_t* dst = (uint8_t*)d;
    const uint8_t* src = (const uint8_t*)s;

    while (*src || *(src + 1)) {
        *dst++ = *src++;
        *dst++ = *src++ << 8;
    }

    return d;
}

void* strcat16_raw(void* d, const void* s) {
    uint8_t* dst = (uint8_t*)d;
    const uint8_t* src = (const uint8_t*)s;

    /* Move to the end of the string */
    while (*dst || *(dst + 1)) dst += 2;

    /* Tack on the new part */
    while (*src || *(src + 1)) {
        *dst++ = *src++;
        *dst++ = *src++ << 8;
    }

    return d;
}
