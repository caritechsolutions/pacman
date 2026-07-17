#ifndef CACHE2_H
#define CACHE2_H

#include "gx_stream.h"

extern void GxStream_CacheUninit(GxStream * s);
int GxStream_CacheControl(GxStream *stream, int cmd, void *arg);

#endif
