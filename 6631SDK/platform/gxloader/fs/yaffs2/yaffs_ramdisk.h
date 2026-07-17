#ifndef __YAFFS_RAMDISK_H__
#define __YAFFS_RAMDISK_H__


#include "yaffs_guts.h"
int yramdisk_EraseBlockInNAND(yaffs_Device *dev, int blockNumber);
int yramdisk_WriteChunkWithTagsToNAND(yaffs_Device *dev,int chunkInNAND,const __u8 *data, yaffs_ExtendedTags *tags);
int yramdisk_ReadChunkWithTagsFromNAND(yaffs_Device *dev,int chunkInNAND, __u8 *data, yaffs_ExtendedTags *tags);
int yramdisk_EraseBlockInNAND(yaffs_Device *dev, int blockNumber);
int yramdisk_InitialiseNAND(yaffs_Device *dev);
int yramdisk_MarkNANDBlockBad(yaffs_Device *dev,int blockNumber);
int yramdisk_QueryNANDBlock(yaffs_Device *dev, int blockNo, yaffs_BlockState *state, int *sequenceNumber);
#endif
