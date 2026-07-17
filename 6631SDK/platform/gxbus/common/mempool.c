#include "gx_mempool.h"

struct node {
	struct node* next;
};

gx_mempool *gx_mempool_create(size_t block_size, size_t block_number, char* buffer)
{
	block_size += 4;
	int i;
	int mem_size = block_size * block_number;
	gx_mempool* pool;
	struct node* chunk;

	if(buffer == NULL){
		pool = (gx_mempool*)GxCore_Mallocz(sizeof(gx_mempool) + mem_size);
		if(!pool)
			return NULL;

		pool->buffer = (char*)pool + sizeof(gx_mempool);
	}
	else{
		pool = (gx_mempool*)GxCore_Mallocz(sizeof(gx_mempool));
		if(!pool)
			return NULL;

		pool->buffer = buffer;
	}

	pool->block_number   = block_number;
	pool->block_size     = block_size;
	pool->buffer_end     = pool->buffer + mem_size;
	pool->max_need_count = 0;
	pool->alloc_count    = 0;
	pool->free_count     = block_number;

	chunk = (struct node* )pool->buffer;
	pool->chunk = chunk;

	for (i = 0; i < block_number - 1; i++) {
		chunk->next =(struct node*)((char*) chunk + block_size);
		chunk = chunk->next;
	}

	chunk->next = NULL;

	GxCore_MutexCreate(&pool->mutex);

	return pool;
}

void* gx_mempool_alloc(gx_mempool *pool, size_t size)
{
	char *mem = NULL;

	if (size <= 0)
		return NULL;

	if(pool){
		GxCore_MutexLock(pool->mutex);

		if (pool->chunk != NULL && size <= pool->block_size) {
			mem = (char*)pool->chunk;
			pool->chunk = pool->chunk->next;
			if(pool->free_count<=0){
				gxlogd("###free_count=%d###\n",pool->free_count);
				while(1);
			}
			pool->free_count--;
			if((pool->block_number - pool->free_count) > pool->max_malloc_count) {
				pool->max_malloc_count = (pool->block_number - pool->free_count);
			}
#if 0
			pool->alloc_count++;
			if (pool->alloc_count - pool->free_count > pool->max_need_count)
				pool->max_need_count = pool->alloc_count - pool->free_count;
#endif
			mem[size + 3] = 0xa0;
		}

		GxCore_MutexUnlock(pool->mutex);
	}

	if (mem == NULL) {
		mem = GxCore_Malloc(size + 4);
		if (mem == NULL) {
			gxlogd("\033[31m%s:%d malloc failed!\033[0m\n", __func__, __LINE__);
			return mem;
		}
		mem[size + 3] = 0xa1;
	}

	memset(mem, 0x0, size);
	if (mem) {
		mem[size + 0] = 0xa0;
		mem[size + 1] = 0xa0;
		mem[size + 2] = 0xa0;
	}

	return mem;
}

static int a = 1;
void gx_mempool_free(gx_mempool* pool, void *buffer, size_t size)
{
	struct node* chunk = (struct node *)buffer;
	char *mem = (char*)buffer;

	if (mem == NULL)
		return;

	if (mem[size + 0] != 0xa0 || mem[size + 1] != 0xa0 || mem[size + 2] != 0xa0 ) {
		gxlogd("[BUG]: %s: buffer %p overflow.\n", __func__, buffer);
		while (a) GxCore_ThreadDelay(1000);
	}

	if(pool){
		GxCore_MutexLock(pool->mutex);

		if ((char*)buffer >= pool->buffer && (char*)buffer < pool->buffer_end) {
			if (mem[size + 3] != 0xa0) {
				gxlogd("Not in pool\n");
				while (a)
					GxCore_ThreadDelay(1000);
			}
			chunk->next = pool->chunk;
			pool->chunk = chunk;

			pool->free_count++;
			if((pool->block_number - pool->free_count) > pool->max_malloc_count) {
				pool->max_malloc_count = (pool->block_number - pool->free_count);
			}
		}
		else{
			if (mem[size + 3] != 0xa1) {
				gxlogd("Not GxCore_Malloc memory\n");
				while (a)
					GxCore_ThreadDelay(1000);
			}
			GxCore_Free(buffer);
		}

		GxCore_MutexUnlock(pool->mutex);
		return;
	}

	if (mem[size + 3] != 0xa1) {
		gxlogd("Not GxCore_Malloc memory\n");
		while (a)
			GxCore_ThreadDelay(1000);
	}
	GxCore_Free(buffer);
}

void gx_mempool_destory(gx_mempool* pool)
{
	if(pool){
		GxCore_MutexDelete(pool->mutex);
		GxCore_Free(pool);
	}
}
