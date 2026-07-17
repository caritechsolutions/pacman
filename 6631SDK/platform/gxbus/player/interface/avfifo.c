#include "avfifo.h"

static AvFIFO* AvFIFO_Init(unsigned char *buffer, unsigned int size)
{
	AvFIFO* fifo = NULL;

	fifo = av_mallocz(sizeof(AvFIFO));
	if (!fifo)
		return fifo;

	fifo->buffer = buffer;
	fifo->size = size;

	return fifo;
}

AvFIFO* AvFIFO_New(unsigned int size)
{
	unsigned char* buffer = NULL;
	AvFIFO* fifo = NULL;

	buffer = av_malloc(size);
	if (!buffer)
		return NULL;

	fifo = AvFIFO_Init(buffer, size);
	if (fifo == NULL) {
		av_free(buffer);
		return NULL;
	}

	GxCore_MutexCreate(&fifo->mutex);
	return fifo;
}

void AvFIFO_Free(AvFIFO *fifo)
{
	GxCore_MutexDelete(fifo->mutex);
	av_free(fifo->buffer);
	av_free(fifo);
}

unsigned int AvFIFO_Put(AvFIFO *fifo, unsigned char *buffer, unsigned int len)
{
	unsigned int l;

	GxCore_MutexLock(fifo->mutex);
	len = GXMIN(len, fifo->size - fifo->in + fifo->out);

	/* first put the data starting from fifo->in to buffer end*/
	l = GXMIN(len, fifo->size - (fifo->in %(fifo->size)));

	if(buffer) {
		memcpy(fifo->buffer + (fifo->in % (fifo->size)), buffer, l);
		/* then put the rest (if any) at the beginning of the buffer*/
		memcpy(fifo->buffer, buffer + l, len - l);
	}

	fifo->in += len;
	GxCore_MutexUnlock(fifo->mutex);
	return len;
}

unsigned int AvFIFO_Get(AvFIFO *fifo, unsigned char *buffer, unsigned int len)
{
	unsigned int l;
	GxCore_MutexLock(fifo->mutex);

	len = GXMIN(len, fifo->in - fifo->out);

	/* first get the data from fifo->out until the end of the buffer*/
	l = GXMIN(len, fifo->size - (fifo->out % (fifo->size)));

	if(buffer){
		memcpy(buffer, fifo->buffer + (fifo->out % (fifo->size)), l);
		/* then get the rest (if any) from the beginning of the buffer*/
		memcpy(buffer + l, fifo->buffer, len - l);
	}

	fifo->out += len;

	/*
	 *optimization: if the FIFO is empty, set the indices to 0
	 *so we don't wrap the next time
	*/
	if (fifo->in == fifo->out)
		fifo->in = fifo->out = 0;
	GxCore_MutexUnlock(fifo->mutex);
	return len;
}

unsigned int AvFIFO_Peek(AvFIFO *fifo, unsigned char *buffer, unsigned int len)
{
	unsigned int l;
	GxCore_MutexLock(fifo->mutex);

	len = GXMIN(len, fifo->in - fifo->out);

	/* first get the data from fifo->out until the end of the buffer*/
	l = GXMIN(len, fifo->size - (fifo->out % (fifo->size)));

	if(buffer){
		memcpy(buffer, fifo->buffer + (fifo->out % (fifo->size)), l);
		/* then get the rest (if any) from the beginning of the buffer*/
		memcpy(buffer + l, fifo->buffer, len - l);
	}

	GxCore_MutexUnlock(fifo->mutex);
	return len;
}

inline void AvFIFO_Reset(AvFIFO *fifo)
{
	GxCore_MutexLock(fifo->mutex);
	fifo->in = fifo->out = 0;
	GxCore_MutexUnlock(fifo->mutex);
}

inline unsigned int AvFIFO_Len(AvFIFO *fifo)
{
	unsigned int len;
	GxCore_MutexLock(fifo->mutex);
	len = fifo->in - fifo->out;
	GxCore_MutexUnlock(fifo->mutex);
	return len;
}

inline unsigned int AvFIFO_FreeLen(AvFIFO *fifo)
{
	return fifo->size - AvFIFO_Len(fifo);
}

