#include "f_checksum.h"

//º∆À„–£—È¬Î
uint32_t psocn_crc32(void* data, uint32_t size)
{
	uint32_t offset, y, cs = 0xFFFFFFFF;
	for (offset = 0; offset < (uint32_t)size; offset++)
	{
		cs ^= *(uint8_t*)((uint32_t)data + offset);
		for (y = 0; y < 8; y++)
		{
			if (!(cs & 1)) cs = (cs >> 1) & 0x7FFFFFFF;
			else cs = ((cs >> 1) & 0x7FFFFFFF) ^ 0xEDB88320;
		}
	}
	return (cs ^ 0xFFFFFFFF);
}

long calculate_checksum(void* data, unsigned long size)
{
	long offset, y, cs = 0xFFFFFFFF;
	for (offset = 0; offset < (long)size; offset++)
	{
		cs ^= *(unsigned char*)((long)data + offset);
		for (y = 0; y < 8; y++)
		{
			if (!(cs & 1)) cs = (cs >> 1) & 0x7FFFFFFF;
			else cs = ((cs >> 1) & 0x7FFFFFFF) ^ 0xEDB88320;
		}
	}
	return (cs ^ 0xFFFFFFFF);
}