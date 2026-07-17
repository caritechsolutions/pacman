#ifndef __GXFRONTEND_DEMUX_FIFO_H_
#define __GXFRONTEND_DEMUX_FIFO_H_
#include "gxtype.h"

void gxfrontend_initFifo(void);

int gxfrontend_createFifo(handle_t hFrontend, int dev, handle_t module);

int gxfrontend_destroyFifo(handle_t module);

int gxfrontend_writeFifo(void *hFrontend, char *buffer, uint32_t length);

#endif
