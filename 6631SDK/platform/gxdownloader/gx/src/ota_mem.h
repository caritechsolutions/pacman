#ifndef __OTA_MEM_H__
#define __OTA_MEM_H__

unsigned char *download_memhole_realloc(unsigned char *mem, unsigned long old_size, unsigned long new_size);
unsigned char *download_memhole_malloc(unsigned long size);
void download_memhole_free(unsigned char *p);

#endif
