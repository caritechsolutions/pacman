#ifndef __MY_FIFO_H__
#define __MY_FIFO_H__

typedef struct avfifo {
	unsigned char *buffer; /* the buffer holding the data */
	unsigned int  size;     /* the size of the allocated buffer */
	unsigned int  in;       /* data is added at offset (in % size) */
	unsigned int  out;      /* data is extracted from off. (out % size) */
	unsigned int  mutex;
} vsFIFO;

extern vsFIFO* vsFIFO_New(unsigned int size);
extern void vsFIFO_Free(vsFIFO *fifo);
extern unsigned int vsFIFO_Put(vsFIFO *fifo, unsigned char *buffer, unsigned int len);
extern unsigned int vsFIFO_Get(vsFIFO *fifo, unsigned char *buffer, unsigned int len);
extern unsigned int vsFIFO_Peek(vsFIFO *fifo, unsigned char *buffer, unsigned int len);
extern void vsFIFO_Reset(vsFIFO *fifo);
extern unsigned int vsFIFO_Len(vsFIFO *fifo);
extern unsigned int vsFIFO_FreeLen(vsFIFO *fifo);

#endif
