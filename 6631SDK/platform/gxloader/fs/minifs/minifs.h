/*******************************************************************************
FileName          : $FILENAME
Author            : zhuzhiguo
Version           : 1.0
Date              : 2008.08.28
Description       :
Function List     :
 ********************************************************************************
History: Scroll to the end of this file.
 ********************************************************************************/

#ifndef _MINIFS_H_
#define _MINIFS_H_

#include "minifs_def.h"

struct block_info {
	uint32_t magic;
	uint32_t crc;
	uint16_t comprSize;
	uint8_t  sizeLow;
	uint8_t is_compress;
};

struct cache {
	struct block_info info;
	uint8_t data[MINIFS_COMPR_BLOCK_SIZE];
	char dirty;
};

typedef struct {
	minifs_Object* obj;         // the object
	uint32_t       blockCount;
	struct cache   *blocks[MINIFS_COMPR_BLOCK_COUNT];
	uint32_t       blockSize;
	uint32_t       blockTotalSize;

	int            dirty;
	int            ref;
}minifs_Handle;

typedef int (*Init)(minifs_device *dev);

typedef void* minifs_handle_file;
typedef void* minifs_handle_dev;

// mode

/* open with MINIFS_CREAT, if the file/dir is not exist, will create new */
#ifndef MINIFS_CREAT
#define MINIFS_CREAT     00000100
#endif

#endif

