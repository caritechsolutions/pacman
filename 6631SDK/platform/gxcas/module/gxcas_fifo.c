#include "gxcas_fifo.h"
#include "gxcore.h"

int32_t GxCas_Fifo_Init(struct gxcas_fifo *fifo, void *buffer, uint32_t size)
{
	if(size== 0 || fifo == NULL)
		return -1;

	memset(fifo, 0, sizeof(struct gxcas_fifo));

	fifo->size = size+1;
	if (buffer == NULL) {
		fifo->data = (unsigned char *)GxCore_Malloc(fifo->size);
		fifo->malloced = 1;
	}
	else
		fifo->data = buffer;
	if (NULL == fifo->data) {
		printf("alloc fifo->buffer NULL!\n");
		return -1;
	}

	memset(fifo->data, 0, fifo->size);

	return 0;
}

void GxCas_Fifo_Free(struct gxcas_fifo *fifo)
{
	if (fifo->malloced == 1)
		GxCore_Free(fifo->data);

	memset(fifo, 0, sizeof(struct gxcas_fifo));
}

uint32_t GxCas_Fifo_Put(struct gxcas_fifo *fifo, void *buf, uint32_t len)
{
	uint32_t todo, freelen;
	uint32_t split;

	freelen = GxCas_Fifo_Freelen(fifo);
	todo = len > freelen ? freelen : len;

	split = (fifo->pwrite + todo > fifo->size) ? fifo->size - fifo->pwrite : 0;

	if(split > 0) {
		memcpy(fifo->data+fifo->pwrite, buf, split);
		buf  += split;
		todo -= split;
		fifo->pwrite = 0;
	}
	memcpy(fifo->data + fifo->pwrite, buf, todo);
	fifo->pwrite = (fifo->pwrite + todo) % fifo->size;

	return (todo + split);
}

uint32_t GxCas_Fifo_Get(struct gxcas_fifo *fifo, void *buf, uint32_t len)
{
	uint32_t todo =len;
	uint32_t split;

	split = (fifo->pread + len > fifo->size) ? fifo->size - fifo->pread : 0;
	if (split > 0) {
		memcpy(buf, fifo->data+fifo->pread, split);
		buf  += split;
		todo -= split;
		fifo->pread = 0;
	}
	memcpy(buf, fifo->data+fifo->pread, todo);

	fifo->pread = (fifo->pread + todo) % fifo->size;

	return len;
}

uint32_t GxCas_Fifo_Peek(struct gxcas_fifo *fifo, void *buf, uint32_t len)
{
	unsigned int todo = len;
	unsigned int split;
	unsigned int pread;

	pread = fifo->pread;
	split = (pread + len > fifo->size) ? fifo->size - pread : 0;
	if (split > 0) {
		memcpy(buf, fifo->data+pread, split);
		buf  += split;
		todo -= split;
		pread = 0;
	}
	memcpy(buf, fifo->data+pread, todo);
	return len;
}

void GxCas_Fifo_Flush(struct gxcas_fifo *fifo)
{
	fifo->pread = fifo->pwrite;
}

void GxCas_Fifo_Reset(struct gxcas_fifo *fifo)
{
	fifo->pread = fifo->pwrite = 0;
}

uint32_t GxCas_Fifo_Len( struct gxcas_fifo *fifo)
{
	return (fifo->pwrite - fifo->pread + fifo->size) % fifo->size;
}

uint32_t GxCas_Fifo_IsEmpty(struct gxcas_fifo *fifo)
{
	return GxCas_Fifo_Len(fifo) == 0 ? 1 :0;
}

uint32_t GxCas_Fifo_IsFull(struct gxcas_fifo *fifo)
{
	return GxCas_Fifo_Len(fifo) == (fifo->size-1) ? 1 : 0;
}

uint32_t GxCas_Fifo_Freelen(struct gxcas_fifo *fifo)
{
	return fifo->size - GxCas_Fifo_Len(fifo) - 1;
}

