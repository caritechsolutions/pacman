/*
 * =====================================================================================
 *
 *       Filename:  structpool.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/16/2009 09:36:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

static struct struct_pool {
	union {
		GxStream             stream;
		GxAudioOut           aout;
		GxDemuxer            demuxer;
		GxStreamAudioHeader  audio_header;
		GxStreamVideoHeader  video_header;
		GxStreamSubHeader    sub_header;
	}
}pool[32] = {0, };


static handle_t mpool_handle;

void spool_init(void)
{
	return av_malloc(pool, sizeof(pool), sizeof(struct struct_pool));
}

#define spool_av_malloc(type) (type)_spool_av_mallocz(sizeof(type)

void* _spool_av_mallocz(size_t size)
{
	void *buf = GxCore_MemPoolAllocZero(mpool_handle);

	if (buf == NULL)
		buf = av_malloc(size);

	return buf;
}

void spool_free(void *p)
{
	if (GxCore_MemPoolFree(mpool_handle, p) == GXCORE_ERROR) {
		av_free(p);
	}
}


