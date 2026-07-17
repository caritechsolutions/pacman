#include <stdio.h>
#include <stdlib.h>
#include "decompress.h"

static FILE *fp = NULL;
static int decompress_error = 0;

#if 0
static int compr_fill(void *buf, unsigned int len)
{
	int r = fread(buf, 1, len, fp);
	if (r < 0)
		printf("RAMDISK: error while reading compressed data");
	else if (r == 0)
		printf("RAMDISK: EOF while reading compressed data");
	return r;
}
#endif

#if 0
static int compr_flush(void *window, unsigned int outcnt)
{
	int written = fwrite(window, 1, outcnt, stdout);
	if (written != outcnt) {
		if (decompress_error == 0)
			printf( "RAMDISK: incomplete write (%d != %d)\n", written, outcnt);
		decompress_error = 1;
		return -1;
	}
	return outcnt;
}
#endif

#define SRC_SIZE 2048000
unsigned char in_buf[SRC_SIZE];
unsigned char out_buf[20480000] = {'\0', };
int main(int argc, char **argv)
{
	int len, res;

	fp = fopen(argv[1], "rb");
	len = fread(in_buf, 1, SRC_SIZE, fp);
	fclose(fp);

	res = decompress(in_buf, len, out_buf);

	if (res == 0)
		printf("decompress ok.\n");
	else
		printf("decompress error.\n");

	return 0;
}

