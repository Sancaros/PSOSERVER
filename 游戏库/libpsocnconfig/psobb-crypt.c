// This is mostly as the code is in the psobb-crypt.cpp file in newserv b5.
// The only changes made were to add support for big-endian machines, to fix the
// data types to standard C ones, and to put bbtable in the file rather than in
// a separate include file.
//
// This encryption code works with the official US PSOBB client.
//

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "encryption.h"
#include "psobb-crypt_table.h"

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#define LE32(x) (((x >> 24) & 0x00FF) | \
                 ((x >>  8) & 0xFF00) | \
                 ((x & 0xFF00) <<  8) | \
                 ((x & 0x00FF) << 24))
#else
#define LE32(x) x
#endif

void CRYPT_BB_Decrypt(CRYPT_SETUP* pcry, void* vdata, uint32_t length)
{
    uint8_t* data = (uint8_t*)vdata;
    uint32_t eax, ecx, edx, ebx, ebp, esi, edi;

    edx = 0;
    ecx = 0;
    eax = 0;
    while (edx < length)
    {
        ebx = *(uint32_t*)&data[edx];
        ebx = ebx ^ pcry->keys[5];
        ebp = ((pcry->keys[(ebx >> 0x18) + 0x12] + pcry->keys[((ebx >> 0x10) & 0xff) + 0x112])
            ^ pcry->keys[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->keys[(ebx & 0xff) + 0x312];
        ebp = ebp ^ pcry->keys[4];
        ebp ^= *(uint32_t*)&data[edx + 4];
        edi = ((pcry->keys[(ebp >> 0x18) + 0x12] + pcry->keys[((ebp >> 0x10) & 0xff) + 0x112])
            ^ pcry->keys[((ebp >> 0x8) & 0xff) + 0x212]) + pcry->keys[(ebp & 0xff) + 0x312];
        edi = edi ^ pcry->keys[3];
        ebx = ebx ^ edi;
        esi = ((pcry->keys[(ebx >> 0x18) + 0x12] + pcry->keys[((ebx >> 0x10) & 0xff) + 0x112])
            ^ pcry->keys[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->keys[(ebx & 0xff) + 0x312];
        ebp = ebp ^ esi ^ pcry->keys[2];
        edi = ((pcry->keys[(ebp >> 0x18) + 0x12] + pcry->keys[((ebp >> 0x10) & 0xff) + 0x112])
            ^ pcry->keys[((ebp >> 0x8) & 0xff) + 0x212]) + pcry->keys[(ebp & 0xff) + 0x312];
        edi = edi ^ pcry->keys[1];
        ebp = ebp ^ pcry->keys[0];
        ebx = ebx ^ edi;
        *(uint32_t*)&data[edx] = ebp;
        *(uint32_t*)&data[edx + 4] = ebx;
        edx = edx + 8;
    }
}

void CRYPT_BB_Encrypt(CRYPT_SETUP* pcry, void* vdata, uint32_t length)
{
    uint8_t* data = (uint8_t*)vdata;
    uint32_t eax, ecx, edx, ebx, ebp, esi, edi;

    edx = 0;
    ecx = 0;
    eax = 0;
    while (edx < length)
    {
        ebx = *(uint32_t*)&data[edx];
        ebx = ebx ^ pcry->keys[0];
        ebp = ((pcry->keys[(ebx >> 0x18) + 0x12] + pcry->keys[((ebx >> 0x10) & 0xff) + 0x112])
            ^ pcry->keys[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->keys[(ebx & 0xff) + 0x312];
        ebp = ebp ^ pcry->keys[1];
        ebp ^= *(uint32_t*)&data[edx + 4];
        edi = ((pcry->keys[(ebp >> 0x18) + 0x12] + pcry->keys[((ebp >> 0x10) & 0xff) + 0x112])
            ^ pcry->keys[((ebp >> 0x8) & 0xff) + 0x212]) + pcry->keys[(ebp & 0xff) + 0x312];
        edi = edi ^ pcry->keys[2];
        ebx = ebx ^ edi;
        esi = ((pcry->keys[(ebx >> 0x18) + 0x12] + pcry->keys[((ebx >> 0x10) & 0xff) + 0x112])
            ^ pcry->keys[((ebx >> 0x8) & 0xff) + 0x212]) + pcry->keys[(ebx & 0xff) + 0x312];
        ebp = ebp ^ esi ^ pcry->keys[3];
        edi = ((pcry->keys[(ebp >> 0x18) + 0x12] + pcry->keys[((ebp >> 0x10) & 0xff) + 0x112])
            ^ pcry->keys[((ebp >> 0x8) & 0xff) + 0x212]) + pcry->keys[(ebp & 0xff) + 0x312];
        edi = edi ^ pcry->keys[4];
        ebp = ebp ^ pcry->keys[5];
        ebx = ebx ^ edi;
        *(uint32_t*)&data[edx] = ebp;
        *(uint32_t*)&data[edx + 4] = ebx;
        edx = edx + 8;
    }
}

void L_CRYPT_BB_InitKey(void* vdata)
{
    uint8_t* data = (uint8_t*)vdata;
    uint32_t x;
    for (x = 0; x < 48; x += 3)
    {
        data[x] ^= 0x19;
        data[x + 1] ^= 0x16;
        data[x + 2] ^= 0x18;
    }
}

void CRYPT_BB_CreateKeys(CRYPT_SETUP* pcry, void* salt)
{
    uint32_t eax, ecx, edx, ebx, ebp, esi, edi, ou, x;
    uint8_t s[48];
    uint16_t* pcryp;
    uint16_t* bbtbl;
    uint16_t dx;

    pcry->bb_posn = 0;
    pcry->mangle = NULL;

    memcpy(s, salt, sizeof(s));
    L_CRYPT_BB_InitKey(s);

    bbtbl = (uint16_t*)&BB_Server_table[0];
    pcryp = (uint16_t*)&pcry->keys[0];

    eax = 0;
    ebx = 0;

    for (ecx = 0; ecx < 0x12; ecx++)
    {
        dx = bbtbl[eax++];
        dx = ((dx & 0xFF) << 8) + (dx >> 8);
        pcryp[ebx] = dx;
        dx = bbtbl[eax++];
        dx ^= pcryp[ebx++];
        pcryp[ebx++] = dx;
    }
    //sancaros 做了注释 不明所以
    /*
    pcry->tbl[0] = 0x243F6A88;
    pcry->tbl[1] = 0x85A308D3;
    pcry->tbl[2] = 0x13198A2E;
    pcry->tbl[3] = 0x03707344;
    pcry->tbl[4] = 0xA4093822;
    pcry->tbl[5] = 0x299F31D0;
    pcry->tbl[6] = 0x082EFA98;
    pcry->tbl[7] = 0xEC4E6C89;
    pcry->tbl[8] = 0x452821E6;
    pcry->tbl[9] = 0x38D01377;
    pcry->tbl[10] = 0xBE5466CF;
    pcry->tbl[11] = 0x34E90C6C;
    pcry->tbl[12] = 0xC0AC29B7;
    pcry->tbl[13] = 0xC97C50DD;
    pcry->tbl[14] = 0x3F84D5B5;
    pcry->tbl[15] = 0xB5470917;
    pcry->tbl[16] = 0x9216D5D9;
    pcry->tbl[17] = 0x8979FB1B;

    */

    memcpy(&pcry->keys[18], &BB_Server_table[18], 4096);

    ecx = 0;
    //total key[0] length is min 0x412
    ebx = 0;

    while (ebx < 0x12)
    {
        //in a loop 在一个循环中
        ebp = ((uint32_t)(s[ecx])) << 0x18;
        eax = ecx + 1;
        edx = eax - ((eax / 48) * 48);
        eax = (((uint32_t)(s[edx])) << 0x10) & 0xFF0000;
        ebp = (ebp | eax) & 0xffff00ff;
        eax = ecx + 2;
        edx = eax - ((eax / 48) * 48);
        eax = (((uint32_t)(s[edx])) << 0x8) & 0xFF00;
        ebp = (ebp | eax) & 0xffffff00;
        eax = ecx + 3;
        ecx = ecx + 4;
        edx = eax - ((eax / 48) * 48);
        eax = (uint32_t)(s[edx]);
        ebp = ebp | eax;
        eax = ecx;
        edx = eax - ((eax / 48) * 48);
        pcry->keys[ebx] = pcry->keys[ebx] ^ ebp;
        ecx = edx;
        ebx++;
    }

    ebp = 0;
    esi = 0;
    ecx = 0;
    edi = 0;
    ebx = 0;
    edx = 0x48;

    while (edi < edx)
    {
        esi = esi ^ pcry->keys[0];
        eax = esi >> 0x18;
        ebx = (esi >> 0x10) & 0xff;
        eax = pcry->keys[eax + 0x12] + pcry->keys[ebx + 0x112];
        ebx = (esi >> 8) & 0xFF;
        eax = eax ^ pcry->keys[ebx + 0x212];
        ebx = esi & 0xff;
        eax = eax + pcry->keys[ebx + 0x312];

        eax = eax ^ pcry->keys[1];
        ecx = ecx ^ eax;
        ebx = ecx >> 0x18;
        eax = (ecx >> 0x10) & 0xFF;
        ebx = pcry->keys[ebx + 0x12] + pcry->keys[eax + 0x112];
        eax = (ecx >> 8) & 0xff;
        ebx = ebx ^ pcry->keys[eax + 0x212];
        eax = ecx & 0xff;
        ebx = ebx + pcry->keys[eax + 0x312];

        for (x = 0; x <= 5; x++)
        {
            ebx = ebx ^ pcry->keys[(x * 2) + 2];
            esi = esi ^ ebx;
            ebx = esi >> 0x18;
            eax = (esi >> 0x10) & 0xFF;
            ebx = pcry->keys[ebx + 0x12] + pcry->keys[eax + 0x112];
            eax = (esi >> 8) & 0xff;
            ebx = ebx ^ pcry->keys[eax + 0x212];
            eax = esi & 0xff;
            ebx = ebx + pcry->keys[eax + 0x312];

            ebx = ebx ^ pcry->keys[(x * 2) + 3];
            ecx = ecx ^ ebx;
            ebx = ecx >> 0x18;
            eax = (ecx >> 0x10) & 0xFF;
            ebx = pcry->keys[ebx + 0x12] + pcry->keys[eax + 0x112];
            eax = (ecx >> 8) & 0xff;
            ebx = ebx ^ pcry->keys[eax + 0x212];
            eax = ecx & 0xff;
            ebx = ebx + pcry->keys[eax + 0x312];
        }

        ebx = ebx ^ pcry->keys[14];
        esi = esi ^ ebx;
        eax = esi >> 0x18;
        ebx = (esi >> 0x10) & 0xFF;
        eax = pcry->keys[eax + 0x12] + pcry->keys[ebx + 0x112];
        ebx = (esi >> 8) & 0xff;
        eax = eax ^ pcry->keys[ebx + 0x212];
        ebx = esi & 0xff;
        eax = eax + pcry->keys[ebx + 0x312];

        eax = eax ^ pcry->keys[15];
        eax = ecx ^ eax;
        ecx = eax >> 0x18;
        ebx = (eax >> 0x10) & 0xFF;
        ecx = pcry->keys[ecx + 0x12] + pcry->keys[ebx + 0x112];
        ebx = (eax >> 8) & 0xff;
        ecx = ecx ^ pcry->keys[ebx + 0x212];
        ebx = eax & 0xff;
        ecx = ecx + pcry->keys[ebx + 0x312];

        ecx = ecx ^ pcry->keys[16];
        ecx = ecx ^ esi;
        esi = pcry->keys[17];
        esi = esi ^ eax;
        pcry->keys[(edi / 4)] = esi;
        pcry->keys[(edi / 4) + 1] = ecx;
        edi = edi + 8;
    }


    eax = 0;
    edx = 0;
    ou = 0;
    while (ou < 0x1000)
    {
        edi = 0x48;
        edx = 0x448;

        while (edi < edx)
        {
            esi = esi ^ pcry->keys[0];
            eax = esi >> 0x18;
            ebx = (esi >> 0x10) & 0xff;
            eax = pcry->keys[eax + 0x12] + pcry->keys[ebx + 0x112];
            ebx = (esi >> 8) & 0xFF;
            eax = eax ^ pcry->keys[ebx + 0x212];
            ebx = esi & 0xff;
            eax = eax + pcry->keys[ebx + 0x312];

            eax = eax ^ pcry->keys[1];
            ecx = ecx ^ eax;
            ebx = ecx >> 0x18;
            eax = (ecx >> 0x10) & 0xFF;
            ebx = pcry->keys[ebx + 0x12] + pcry->keys[eax + 0x112];
            eax = (ecx >> 8) & 0xff;
            ebx = ebx ^ pcry->keys[eax + 0x212];
            eax = ecx & 0xff;
            ebx = ebx + pcry->keys[eax + 0x312];

            for (x = 0; x <= 5; x++)
            {
                ebx = ebx ^ pcry->keys[(x * 2) + 2];
                esi = esi ^ ebx;
                ebx = esi >> 0x18;
                eax = (esi >> 0x10) & 0xFF;
                ebx = pcry->keys[ebx + 0x12] + pcry->keys[eax + 0x112];
                eax = (esi >> 8) & 0xff;
                ebx = ebx ^ pcry->keys[eax + 0x212];
                eax = esi & 0xff;
                ebx = ebx + pcry->keys[eax + 0x312];

                ebx = ebx ^ pcry->keys[(x * 2) + 3];
                ecx = ecx ^ ebx;
                ebx = ecx >> 0x18;
                eax = (ecx >> 0x10) & 0xFF;
                ebx = pcry->keys[ebx + 0x12] + pcry->keys[eax + 0x112];
                eax = (ecx >> 8) & 0xff;
                ebx = ebx ^ pcry->keys[eax + 0x212];
                eax = ecx & 0xff;
                ebx = ebx + pcry->keys[eax + 0x312];
            }

            ebx = ebx ^ pcry->keys[14];
            esi = esi ^ ebx;
            eax = esi >> 0x18;
            ebx = (esi >> 0x10) & 0xFF;
            eax = pcry->keys[eax + 0x12] + pcry->keys[ebx + 0x112];
            ebx = (esi >> 8) & 0xff;
            eax = eax ^ pcry->keys[ebx + 0x212];
            ebx = esi & 0xff;
            eax = eax + pcry->keys[ebx + 0x312];

            eax = eax ^ pcry->keys[15];
            eax = ecx ^ eax;
            ecx = eax >> 0x18;
            ebx = (eax >> 0x10) & 0xFF;
            ecx = pcry->keys[ecx + 0x12] + pcry->keys[ebx + 0x112];
            ebx = (eax >> 8) & 0xff;
            ecx = ecx ^ pcry->keys[ebx + 0x212];
            ebx = eax & 0xff;
            ecx = ecx + pcry->keys[ebx + 0x312];

            ecx = ecx ^ pcry->keys[16];
            ecx = ecx ^ esi;
            esi = pcry->keys[17];
            esi = esi ^ eax;
            pcry->keys[(ou / 4) + (edi / 4)] = esi;
            pcry->keys[(ou / 4) + (edi / 4) + 1] = ecx;
            edi = edi + 8;
        }
        ou = ou + 0x400;
    }
}

void CRYPT_BB_DEBUG_PrintKeys(CRYPT_SETUP* cs, char* title)
{
    int x, y;
    printf("\n%s\n### ###+0000 ###+0001 ###+0002 ###+0003 ###+0004 ###+0005 ###+0006 ###+0007\n", title);
    for (x = 0; x < 131; x++)
    {
        printf("%03d", x * 8);
        for (y = 0; y < 8; y++) printf(" %08X", cs->keys[(x * 8) + y]);
        printf("\n");
    }
}

