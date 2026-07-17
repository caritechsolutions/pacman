#ifndef __MINIFS_RAMDISK_H__
#define __MINIFS_RAMDISK_H__

#include "minifs.h"

int spiflash_Init(minifs_device *dev);
#define MINIFS_DISK_INIT spiflash_Init

#endif
