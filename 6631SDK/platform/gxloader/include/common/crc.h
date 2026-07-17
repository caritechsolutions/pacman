#ifndef _GXLOADER_CRC_H
#define _GXLOADER_CRC_H

#include "sys/types.h"

uint32_t crc32 (uint32_t, const unsigned char *, uint);
uint32_t crc32_wd (uint32_t, const unsigned char *, uint, uint);
uint32_t crc32_no_comp (uint32_t, const unsigned char *, uint);

#endif /* _GXLOADER_CRC_H */
