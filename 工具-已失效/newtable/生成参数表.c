/* random number functions */

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mtwist.h>

#pragma comment(lib, "libpsoconfig.lib")

extern void	mt_bestseed(void);
extern void	mt_seed(void);	/* Choose seed from random input. */
extern uint32_t	mt_lrand(void);	/* Generate 32-bit random value */

typedef struct st_pcrys
{
	uint32_t tbl[18];
} PCRYS;

void main()
{
	PCRYS pcrys = {0};
	PCRYS* pcry;
	unsigned value;
	FILE* fp;
	FILE* bp;
	unsigned ch;
	//sfmt_t sfmt;

	pcry = &pcrys;
	init_genrand((uint32_t)time(NULL));
	//mt_bestseed();
	//srand((uint32_t)time(NULL));
	//sfmt_init_gen_rand(&sfmt, (uint32_t)time(NULL));

	fp = fopen("BB_Server_table.h", "w");
	bp = fopen("BB_Server_table.bin", "wb");
	fprintf(fp, "\n\nstatic const uint32_t BB_Server_table[18+1024] =\n{\n");
	for (ch = 0;ch < 1024 + 18;ch++)
	{
		value = genrand_int32();
		//fprintf(fp, "0x%08x,\n", value, value);
		fprintf(fp, "0x%08x,\n", value);
		fwrite(&value, 1, 4, bp);
	}
	fprintf(fp, "};\n");
	fclose(fp);

}