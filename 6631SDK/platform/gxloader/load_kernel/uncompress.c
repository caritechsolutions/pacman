#include "stdio.h"
#include "string.h"
#include "linux/errno.h"

#ifndef TEST
#include "config.h"
#include "util.h"
#endif

#include "zlib.h"

static z_stream stream;

//#define ZALLOC_ALIGNMENT	16
void *zalloc(void *, unsigned, unsigned);
void zfree(void *, void *, unsigned);

/* Returns length of decompressed data. */
int cramfs_uncompress_block (void *dst, void *src, int srclen)
{
	int err;

	zlib_inflateReset (&stream);

	stream.next_in = src;
	stream.avail_in = srclen;

	stream.next_out = dst;
	stream.avail_out = 4096 * 2;

	err = zlib_inflate (&stream, Z_FINISH);

	if (err != Z_STREAM_END)
		goto err;
	return stream.total_out;

err:
	/*printf ("Error %d while decompressing!\n", err); */
	/*printf ("%p(%d)->%p\n", src, srclen, dst); */
	return -1;
}

int cramfs_uncompress_init (void)
{
	int err;

	stream.next_in = 0;
	stream.avail_in = 0;

	stream.workspace = malloc(zlib_inflate_workspacesize());
	if (!stream.workspace) {
		printf("Error: zlib malloc workspace failed!\n");
		return -ENOMEM;
	}

	err = zlib_inflateInit (&stream);
	if (err != Z_OK) {
		printf ("Error: inflateInit2() returned %d\n", err);
		return -1;
	}

	return 0;
}

int cramfs_uncompress_exit (void)
{
	if (stream.workspace)
		free(stream.workspace);
	zlib_inflateEnd (&stream);
	return 0;
}

