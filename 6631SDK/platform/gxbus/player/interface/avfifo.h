#ifndef __AV_FIFO_H__
#define __AV_FIFO_H__

#include "gx_common.h"

typedef struct avfifo {
	unsigned char *buffer; /* the buffer holding the data */
	unsigned int  size;     /* the size of the allocated buffer */
	unsigned int  in;       /* data is added at offset (in % size) */
	unsigned int  out;      /* data is extracted from off. (out % size) */
	int32_t       mutex;
} AvFIFO;

extern AvFIFO* AvFIFO_New(unsigned int size);
extern void AvFIFO_Free(AvFIFO *fifo);
extern unsigned int AvFIFO_Put(AvFIFO *fifo, unsigned char *buffer, unsigned int len);
extern unsigned int AvFIFO_Get(AvFIFO *fifo, unsigned char *buffer, unsigned int len);
extern unsigned int AvFIFO_Peek(AvFIFO *fifo, unsigned char *buffer, unsigned int len);
extern void AvFIFO_Reset(AvFIFO *fifo);
extern unsigned int AvFIFO_Len(AvFIFO *fifo);
extern unsigned int AvFIFO_FreeLen(AvFIFO *fifo);

#endif
