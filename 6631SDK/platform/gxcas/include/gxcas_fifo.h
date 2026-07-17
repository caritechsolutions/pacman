#ifndef __GXCAS_FIFO_H__
#define __GXCAS_FIFO_H__

#include "gxtype.h"

struct gxcas_fifo {
	unsigned char *data;
	uint32_t  size;
	uint32_t  pread;
	uint32_t  pwrite;
	uint32_t  malloced;
};

int32_t GxCas_Fifo_Init     (struct gxcas_fifo *fifo, void *buf, uint32_t size);
uint32_t GxCas_Fifo_Put     (struct gxcas_fifo *fifo, void *buf, uint32_t len);
uint32_t GxCas_Fifo_Get     (struct gxcas_fifo *fifo, void *buf, uint32_t len);
uint32_t GxCas_Fifo_Peek    (struct gxcas_fifo *fifo, void *buf, uint32_t len);
uint32_t GxCas_Fifo_Len     (struct gxcas_fifo *fifo);
uint32_t GxCas_Fifo_IsFull  (struct gxcas_fifo *fifo);
uint32_t GxCas_Fifo_IsEmpty (struct gxcas_fifo *fifo);
uint32_t GxCas_Fifo_Freelen (struct gxcas_fifo *fifo);
void GxCas_Fifo_Free  (struct gxcas_fifo *fifo);
void GxCas_Fifo_Reset (struct gxcas_fifo *fifo);
void GxCas_Fifo_Flush (struct gxcas_fifo *fifo);

#endif

