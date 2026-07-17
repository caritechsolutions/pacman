#include "gx_demux.h"

#ifdef CONFIG_MEM_POOL

static inline void demux_pool_sata(GxDemuxerPool* pool)
{
	int i;

	gxlogd("_____________________________________________________________________________\n");
	for (i = 0; i < MEMPOOL_NUM; i++)
	{
		gxlogd("%2dk: num=%-2d, max_need=%-4d, alloc=%-4d,av_free=%-4d\n",
				pool->size[i]/1024,
				pool->count[i], pool->data_pool[i]->max_need_count,
				pool->data_pool[i]->alloc_count, pool->data_pool[i]->free_count
			  );
	}
	gxlogd("_____________________________________________________________________________\n");
}

void GxDemuxPool_Init(GxDemuxerPool* pool, char* buffer, size_t size)
{
	int i;
	char* pos = buffer;
	char* packet_pool_buffer = NULL;
	char* data_pool_buffer[MEMPOOL_NUM];
	int block_size[MEMPOOL_NUM] = {512, 1*1024, 2*1024, 4*1024};
	int block_count[MEMPOOL_NUM] = {48, 36, 32, 32};

	memset(pool, 0, sizeof(GxDemuxerPool));
	memcpy(&pool->size, block_size, sizeof(pool->size));
	memcpy(&pool->count, block_count, sizeof(pool->count));

	memset(data_pool_buffer, 0, sizeof(data_pool_buffer));
	if(buffer && size){
		for(i = 0; i < MEMPOOL_NUM; i++){
			if(size >= (pool->size[i] + 8) * pool->count[i]){
				data_pool_buffer[i] = pos;
				pos += (pool->size[i] + 8) * pool->count[i];
				size -= (pool->size[i] + 8) * pool->count[i];
			}
			else
				break;
		}
		if(size >= (sizeof(GxDemuxPacket) + 8) * MAX_PACKS){
			packet_pool_buffer = pos;
			pos += (sizeof(GxDemuxPacket) + 8) * MAX_PACKS;
			size -= (sizeof(GxDemuxPacket) + 8) * MAX_PACKS;
		}
	}

	pool->packet_pool = gx_mempool_create(sizeof(GxDemuxPacket), MAX_PACKS, packet_pool_buffer);
	for (i = 0; i < MEMPOOL_NUM; i++)
		pool->data_pool[i] = gx_mempool_create(pool->size[i], pool->count[i], data_pool_buffer[i]);

	pool->init = 1;
}

void GxDemuxPool_Destroy(GxDemuxerPool* pool)
{
	int i = 0;

	if(pool && pool->init == 1){
		demux_pool_sata(pool);

		for (i = 0; i < MEMPOOL_NUM; i++)
			gx_mempool_destory(pool->data_pool[i]);

		gx_mempool_destory(pool->packet_pool);
	}
}

void GxDemuxPool_PacketAllocBuffer(GxDemuxerPool* pool, GxDemuxPacket* dp)
{
	int i = MEMPOOL_NUM;

	if(pool->init == 1){
		for (i=0; i<MEMPOOL_NUM; i++) {
			if (pool->size[i] >= dp->pool_size && pool->data_pool[i] != NULL) {
				dp->pool   = pool->data_pool[i];
				dp->buffer = gx_mempool_alloc(pool->data_pool[i], dp->pool_size);
				break;
			}
		}
	}

	if (i == MEMPOOL_NUM) {
		dp->pool = NULL;
		dp->buffer = av_malloc(dp->pool_size + 4);
		if(!dp->buffer){
			dp->len = 0;
			return;
		}
		dp->buffer[dp->pool_size + 0] = 0xa0;
		dp->buffer[dp->pool_size + 1] = 0xa0;
		dp->buffer[dp->pool_size + 2] = 0xa0;
		dp->buffer[dp->pool_size + 3] = 0xa1;
	}

	if (dp->buffer == NULL)
		dp->len = 0;

}

#else

void GxDemuxPool_PacketAllocBuffer(GxDemuxPacket* dp)
{
	dp->buffer = (unsigned char* )av_malloc(len);
	if (dp->buffer == NULL)
		dp->len = 0;
}

#endif

GxDemuxPacket *GxDemuxPool_PacketNew(GxDemuxerPool* pool)
{
	GxDemuxPacket *dp;

#ifdef CONFIG_MEM_POOL
	dp = (GxDemuxPacket*) gx_mempool_alloc(pool->packet_pool, sizeof(GxDemuxPacket));
#else
	dp = (GxDemuxPacket*) av_malloc(sizeof(GxDemuxPacket));
#endif
	if (dp)
		memset(dp, 0, sizeof(GxDemuxPacket));
	else
		gxlogd("[Demux]: packet alloc = 0x%x\n",(int)dp);

	return dp;
}

void GxDemuxPool_PacketFree(GxDemuxerPool* pool, GxDemuxPacket* dp)
{
	if (dp) {
#ifdef CONFIG_MEM_POOL
		if(dp->buffer){
			if (dp->pool)
				gx_mempool_free(dp->pool, dp->buffer, dp->pool_size);
			else
				av_free(dp->buffer);
		}
		gx_mempool_free(pool->packet_pool, dp, sizeof(GxDemuxPacket));
#else
		if(dp->buffer)
			av_free(dp->buffer);
		av_free(dp);
#endif
	}
}

