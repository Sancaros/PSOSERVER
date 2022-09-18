#pragma once
#include <inttypes.h>

/* Calculate a CRC32 checksum over a given block of data. */
uint32_t psocn_crc32(void* data, uint32_t size);
long calculate_checksum(void* data, unsigned long size);