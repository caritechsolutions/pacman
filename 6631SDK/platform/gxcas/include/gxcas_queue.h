#ifndef __GXCAS_QUEUE_H__
#define __GXCAS_QUEUE_H__

#include "gxtype.h"

int32_t GxCas_QueueInitEx(void);
int32_t GxCas_QueuePutEx(unsigned long type, void *data, uint32_t size, int32_t timeout);
int32_t GxCas_QueueGetEx(unsigned long *type, void **buf, int32_t timeout);
void GxCas_QueueReleaseEx(void);
int32_t GxCas_QueueInit(void);
int32_t GxCas_QueuePut(unsigned long type, void *data, uint32_t size, int32_t timeout);
int32_t GxCas_QueueGet(unsigned long *type, void **buf, int32_t timeout);
void GxCas_QueueRelease(void);

#endif

