#include "fifo.h"
#include "gxcore.h"

static int FIFO_MIN(int a, int b)
{
	return((a) > (b) ? (b) : (a));
}

static vsFIFO* vsFIFO_Init(unsigned char *buffer, unsigned int size)
{
	vsFIFO* fifo = NULL;

	fifo = GxCore_Mallocz(sizeof(vsFIFO));
	if (!fifo)
		return fifo;

	fifo->buffer = buffer;
	fifo->size = size;

	return fifo;
}

vsFIFO* vsFIFO_New(unsigned int size)
{
	unsigned char* buffer = NULL;
	vsFIFO* fifo = NULL;

	buffer = GxCore_Malloc(size);
	if (!buffer)
		return NULL;

	fifo = vsFIFO_Init(buffer, size);
	if (fifo == NULL) {
		GxCore_Free(buffer);
		return NULL;
	}

	GxCore_MutexCreate((handle_t *)&fifo->mutex);
	return fifo;
}

void vsFIFO_Free(vsFIFO *fifo)
{
	GxCore_MutexDelete(fifo->mutex);
	GxCore_Free(fifo->buffer);
	GxCore_Free(fifo);
}

unsigned int vsFIFO_Put(vsFIFO *fifo, unsigned char *buffer, unsigned int len)
{
	unsigned int l;

	GxCore_MutexLock(fifo->mutex);
	len = FIFO_MIN(len, fifo->size - fifo->in + fifo->out);

	/* first put the data starting from fifo->in to buffer end*/
	l = FIFO_MIN(len, fifo->size - (fifo->in %(fifo->size)));

	if(buffer) {
		memcpy(fifo->buffer + (fifo->in % (fifo->size)), buffer, l);
		/* then put the rest (if any) at the beginning of the buffer*/
		memcpy(fifo->buffer, buffer + l, len - l);
	}

	fifo->in += len;
	GxCore_MutexUnlock(fifo->mutex);
	return len;
}

unsigned int vsFIFO_Get(vsFIFO *fifo, unsigned char *buffer, unsigned int len)
{
	unsigned int l;
	GxCore_MutexLock(fifo->mutex);

	len = FIFO_MIN(len, fifo->in - fifo->out);

	/* first get the data from fifo->out until the end of the buffer*/
	l = FIFO_MIN(len, fifo->size - (fifo->out % (fifo->size)));

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

unsigned int vsFIFO_Peek(vsFIFO *fifo, unsigned char *buffer, unsigned int len)
{
	unsigned int l;
	GxCore_MutexLock(fifo->mutex);

	len = FIFO_MIN(len, fifo->in - fifo->out);

	/* first get the data from fifo->out until the end of the buffer*/
	l = FIFO_MIN(len, fifo->size - (fifo->out % (fifo->size)));

	if(buffer){
		memcpy(buffer, fifo->buffer + (fifo->out % (fifo->size)), l);
		/* then get the rest (if any) from the beginning of the buffer*/
		memcpy(buffer + l, fifo->buffer, len - l);
	}

	GxCore_MutexUnlock(fifo->mutex);
	return len;
}

inline void vsFIFO_Reset(vsFIFO *fifo)
{
	GxCore_MutexLock(fifo->mutex);
	fifo->in = fifo->out = 0;
	GxCore_MutexUnlock(fifo->mutex);
}

inline unsigned int vsFIFO_Len(vsFIFO *fifo)
{
	unsigned int len;
	GxCore_MutexLock(fifo->mutex);
	len = fifo->in - fifo->out;
	GxCore_MutexUnlock(fifo->mutex);
	return len;
}

inline unsigned int vsFIFO_FreeLen(vsFIFO *fifo)
{
	return fifo->size - vsFIFO_Len(fifo);
}

