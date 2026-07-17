#ifndef __QRENCODE_API_H__
#define __QRENCODE_API_H__
#include "qrencode.h"

unsigned int qren_encode_init(char *url, int bpp);
void qren_encode_show(unsigned int handle, const char* img_fd);
void qren_encode_close(unsigned int handle);


#endif

