#include "compr.h"
#include "minifs_def.h"
#include "sys/types.h"
#include "linux/errno.h"

#if defined(__KERNEL__)
#  include <zlib.h>
#else
#  include "../zlib/include/zlib.h"
#endif

#ifndef zlib_deflateInit
#define zlib_deflateInit(x,y)  deflateInit(x,y)
#define zlib_deflate(x,y)      deflate(x,y)
#define zlib_deflateEnd(x)     deflateEnd(x)
#define zlib_inflateInit(x)    inflateInit(x)
#define zlib_inflateInit2(x,y) inflateInit2(x,y)
#define zlib_inflate(x,y)      inflate(x,y)
#define zlib_inflateEnd(x)     inflateEnd(x)
#endif

static z_stream inf_strm, def_strm;

/* Plan: call deflate() with avail_in == *sourcelen,
	avail_out = *dstlen - 12 and flush == Z_FINISH.
	If it doesn't manage to finish,	call it again with
	avail_in == 0 and avail_out set to the remaining 12
	bytes for it to clean up.
   Q: Is 12 bytes sufficient?
*/

#ifndef __KERNEL__
static pthread_mutex_t deflate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t inflate_mutex = PTHREAD_MUTEX_INITIALIZER;
#define mutex_lock pthread_mutex_lock
#define mutex_unlock pthread_mutex_unlock
#else
#define mutex_lock(x)
#define mutex_unlock(x)
//static int deflate_mutex;
//static int inflate_mutex;

int zlib_alloc_workspaces(void)
{
	def_strm.workspace = vmalloc(zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL));
	if (!def_strm.workspace) {
		printk(KERN_WARNING "Failed to allocate %d bytes for deflate workspace\n",
				zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL));
		return -ENOMEM;
	}

	inf_strm.workspace = vmalloc(zlib_inflate_workspacesize());
	if (!inf_strm.workspace) {
		printk(KERN_WARNING "Failed to allocate %d bytes for inflate workspace\n", zlib_inflate_workspacesize());
		vfree(def_strm.workspace);
		return -ENOMEM;
	}
	return 0;
}

void zlib_free_workspaces(void)
{
	vfree(def_strm.workspace);
	vfree(inf_strm.workspace);
}
#endif

static int zlib_compress(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen)
{
	int err;

	mutex_lock(&deflate_mutex);
	def_strm.next_in = (uint8_t*)source;
	def_strm.avail_in = (int)sourceLen;
#ifdef MAXSEG_64K
	/* Check for source > 64K on 16-bit machine: */
	if ((uLong)def_strm.avail_in != sourceLen) {
		err = Z_BUF_ERROR;
		goto out;
	}
#endif
	def_strm.next_out = dest;
	def_strm.avail_out = *destLen - STREAM_END_SPACE;
	if ((int)def_strm.avail_out != *destLen - STREAM_END_SPACE) {
		err = Z_BUF_ERROR;
		goto out;
	}

#ifndef __KERNEL__
	def_strm.zalloc = (alloc_func)0;
	def_strm.zfree = (free_func)0;
	def_strm.opaque = (voidpf)0;
#endif

	err = zlib_deflateInit(&def_strm, Z_BEST_COMPRESSION);
	if (err != Z_OK) goto out;

	err = zlib_deflate(&def_strm, Z_FINISH);
	if (err != Z_STREAM_END) {
		zlib_deflateEnd(&def_strm);
		err = Z_BUF_ERROR;
		goto out;
	}
	*destLen = def_strm.total_out;

	if (sourceLen != def_strm.total_in){
		err = Z_BUF_ERROR;
		goto out;
	}

	err = zlib_deflateEnd(&def_strm);
out:
	mutex_unlock(&deflate_mutex);
	return err;
}

static int zlib_uncompress(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen)
{
	int err;
	mutex_lock(&inflate_mutex);
	inf_strm.next_in = (uint8_t*)source; inf_strm.avail_in = (uInt)sourceLen;
	/* Check for source > 64K on 16-bit machine: */
	if ((uLong)inf_strm.avail_in != sourceLen) {
		err = Z_BUF_ERROR;
		goto out;
	}

	inf_strm.next_out = dest;
	inf_strm.avail_out = (uInt)*destLen;
	if ((uLong)inf_strm.avail_out != *destLen) {
		err = Z_BUF_ERROR;
		goto out;
	}

#ifndef __KERNEL__
	inf_strm.zalloc = (alloc_func)0;
	inf_strm.zfree = (free_func)0;
#endif
	err = zlib_inflateInit(&inf_strm);
	if (err != Z_OK) goto out;

	err = zlib_inflate(&inf_strm, Z_FINISH);
	if (err != Z_STREAM_END) {
		zlib_inflateEnd(&inf_strm);
		if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && inf_strm.avail_in == 0))
			err = Z_DATA_ERROR;
		goto out;
	}
	*destLen = inf_strm.total_out;

	err = zlib_inflateEnd(&inf_strm);
out:
	mutex_unlock(&inflate_mutex);
	return err;
}

static int minifs_zlib_init(void)
{
	int ret = -1;

	ret = zlib_alloc_workspaces();
	return ret;
}

static int minifs_zlib_free(void)
{
	int ret = 0;

	zlib_free_workspaces();
	return ret;
}

static int minifs_zlib_compress(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen)
{
	int ret = zlib_compress(dest, destLen, source, sourceLen);

	return ret == Z_OK ? 0 : -1;
}

static int minifs_zlib_uncompress(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen)
{
	int ret = zlib_uncompress(dest, destLen, source, sourceLen);

	return ret == Z_OK ? 0 : -1;
}

struct minifs_compressor minifs_zlib_comp = {
	.priority = 0,
	.name = "zlib",
	.init = minifs_zlib_init,
	.free = minifs_zlib_free,
	.compr = MINIFS_COMPR_ZLIB,
	.compress = minifs_zlib_compress,
	.uncompress = minifs_zlib_uncompress,
};

