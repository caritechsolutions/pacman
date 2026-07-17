/*******************************************************************************
FileName          : $FILENAME
Author            : zhuzhiguo
Version           : 1.0
Date              : 2008.08.28
Description       :
Function List     :
 *********************************************************************************
History: Scroll to the end of this file.
 *********************************************************************************/

#ifndef _MINIFS_DEF_H_
#define _MINIFS_DEF_H_

#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>		/*mode */
#include <stdlib.h>
#include <unistd.h>		/*SEEK_CUR */
#include <string.h>
#include <linux/list.h>


#define MINIFS_MAX_CHUNK_ID          0x000FFFFF
#define MINIFS_UNUSED_OBJECT_ID      0x0003FFFF
#define MINIFS_ALLOCATION_NOBJECTS   10
#define MINIFS_ALLOCATION_NTNODES    10
#define MINIFS_MAX_NAME_LENGTH       32
#define MINIFS_MAX_ALIAS_LENGTH      159


#define MINIFS_NTNODES_LEVEL0        16
#define MINIFS_TNODES_LEVEL0_BITS    4
#define MINIFS_TNODES_LEVEL0_MASK    0xf

#define MINIFS_NTNODES_INTERNAL      (MINIFS_NTNODES_LEVEL0 / 2)
#define MINIFS_TNODES_INTERNAL_BITS  (MINIFS_TNODES_LEVEL0_BITS - 1)
#define MINIFS_TNODES_INTERNAL_MASK  0x7
#define MINIFS_TNODES_MAX_LEVEL      6

#define MINIFS_HW_PAGE_SIZE          512
#define MINIFS_BYTES_PER_CHUNK       (MINIFS_HW_PAGE_SIZE - sizeof(minifs_PageTags))

#define MINIFS_COMPR_BLOCK_SIZE      8192
#define MINIFS_COMPR_BLOCK_COUNT     1024
#define MINIFS_FILE_MAX_SIZE         MINIFS_COMPR_BLOCK_SIZE * MINIFS_COMPR_BLOCK_COUNT // 8M


#define MINIFS_OK   1
#define MINIFS_FAIL 0

#define PATH_MAX    255
#define FLASH_BLOCK_SIZE (64 * 1024)

#ifdef __KERNEL__
static inline void *YMALLOC(size_t size) {
	void *x;
	x = kmalloc(size, GFP_KERNEL);
	if (x == NULL)
		printk("minifs YMALLOC err: size %d.\n", size);

	return x;
}
static inline void YFREE(void **x) {
	if (x && *x) {
		kfree(*x);
		*x = NULL;
	}
}
#  define YPRINTF(fmt, args...)  printk(fmt, ##args)
static inline char *YSTRDUP(const char *str) {
        char *x;
        x = kstrdup(str, GFP_KERNEL);
		if (x == NULL)
			printk("minifs YSTRDUP err\n");
        return x;
}
#else
#  define YMALLOC(x)             malloc(x);
#  define YFREE(x)               free(x)
#  define YPRINTF(fmt, args...)  printf(fmt, ##args)
#  define YSTRDUP(str)           strdup(str)
#endif

#define T(p...)                do{ YPRINTF(p);} while(0)

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

/*---------------------- STRUCTURE/UNION DATA TYPES --------------------------*/
// Spare structure
typedef struct {
	uint32_t  chunkId     : 20;
	uint32_t  serialNumber:  2;
	uint32_t  byteCount   : 10;
	uint32_t  objectId    : 18;
	uint32_t  ecc         : 12;
	uint32_t  unusedStuff :  2;

	uint8_t   pageStatus;     // set to 0 to delete the page
	uint8_t   blockStatus;
} minifs_PageTags; // 10 byte


typedef struct minifs_device minifs_device;

typedef enum {
	MINIFS_BLOCK_STATE_UNKNOWN    = 0,
	MINIFS_BLOCK_STATE_SCANNING   = 1,  // 正在扫描块 Used while the block is being scanned.
	                                    // NB Don't erase blocks while they're being scanned

	MINIFS_BLOCK_STATE_EMPTY      = 2,  // 空块 This block is empty

	MINIFS_BLOCK_STATE_ALLOCATING = 3,  // 部分页已经被分配 This block is partially allocated.
	                                    // This is the one currently being used for page
	                                    // allocation. Should never be more than one of these

	MINIFS_BLOCK_STATE_FULL       = 4,  // 所有页都已经被分配 All the pages in this block have been allocated.
	                                    // At least one page holds valid data.

	MINIFS_BLOCK_STATE_DIRTY      = 5,  // 所有页都已分析却又删除，可以擦除该块
	                                    // All pages have been allocated and deleted. Erase me, reuse me.

	MINIFS_BLOCK_STATE_DEAD       = 6   // 该块已坏，不能再用 This block has failed and is not in use
} minifs_BlockState;

union minifs_Tnode_union {
	union minifs_Tnode_union *internal[MINIFS_NTNODES_INTERNAL];
	uint16_t level0[MINIFS_NTNODES_LEVEL0];
};

typedef union minifs_Tnode_union minifs_Tnode;

enum minifs_obj_type {
	MINIFS_OBJECT_TYPE_UNKNOWN,
	MINIFS_OBJECT_TYPE_FILE,
	MINIFS_OBJECT_TYPE_SYMLINK,
	MINIFS_OBJECT_TYPE_DIRECTORY,
	MINIFS_OBJECT_TYPE_HARDLINK,
	MINIFS_OBJECT_TYPE_SPECIAL
};

#define MINIFS_OBJECT_TYPE_MAX MINIFS_OBJECT_TYPE_SPECIAL

typedef struct {
	unsigned char flag_head;
	char  name[MINIFS_MAX_NAME_LENGTH + 1];

	// Thes following apply to directories, files, symlinks - not hard links
	uint32_t yst_mode;  // protection

	uint32_t yst_uid;   // user ID of owner
	uint32_t yst_gid;   // group ID of owner
	uint32_t yst_atime; // time of last access
	uint32_t yst_mtime; // time of last modification
	uint32_t yst_ctime; // time of last change

	uint32_t objectSize;
	uint32_t rawFileSize;
	enum minifs_obj_type type;
	uint32_t parent_obj_id;

	/* Equivalent object id applies to hard links only. */
	int equiv_id;
	/* Alias is for symlinks only. */
	char alias[MINIFS_MAX_ALIAS_LENGTH + 1];
	unsigned char flag_end;
} minifs_ObjectHeader;

typedef struct {
	int      chunkId;
	int      dirty;
	int      Bytes; // Only valid if the cache is dirty
	uint8_t  data[MINIFS_BYTES_PER_CHUNK];
} minifs_ChunkCache;

typedef struct minifs_ObjectStruct minifs_Object;

struct minifs_file_var {
	uint32_t fileSize;
	uint32_t scannedSize;
	int      topLevel;
	minifs_Tnode *topNode;
};

struct minifs_dir_var {
	struct list_head children; /* list of child links */
};

struct minifs_hardlink_var {
	minifs_Object *equiv_obj;
};

union minifs_obj_var {
	struct minifs_file_var     file_variant;
	struct minifs_dir_var      dir_variant;
	struct minifs_hardlink_var hardlink_variant;
};

struct minifs_ObjectStruct {
	struct list_head     siblings;
	uint8_t              deleted    : 1;   // This should only apply to unlinked files.
	uint8_t              softDeleted: 1;   // it has also been soft deleted
	uint8_t              fake       : 1;   // A fake object has no presence on NAND.
	uint8_t              unlinked   : 1;   // An unlinked file. The file should be in the unlinked pseudo directory.
	uint8_t              dirty      : 1;   // the object needs to be written to flash
	uint8_t              valid      : 1;   // When the file system is being loaded up, this
	                                       // object might be created before the data
	                                       // is available (ie. file data records appear before the header).
	uint8_t              serial;           // serial number of page in NAND. Store here so we don't have to
	                                       // read back the old one to update.
	uint16_t             sum;              // sum of the name to speed searching

	int                  chunkId;          // where it lives
	int                  dataChunks;
	uint32_t             objectId;         // the object id value
	uint32_t             inUse;

	minifs_ChunkCache    Cache;
	minifs_ObjectHeader  Header;
	minifs_device*       device;           // The device I'm on
	minifs_Object*       parent;
	union minifs_obj_var variant;
	void *handle;
};

typedef struct {
	int      softDeletions   : 12;  // number of soft deleted pages
	int      pagesInUse      : 12;  // number of pages in use
	uint32_t blockState      : 4;   // One of the above block states
	uint32_t needsRetiring   : 1;   // Data has failed on this block, need to get valid data off
	                                // and retire the block.
} minifs_BlockInfo;

typedef struct minifs_block {
	int               page_count;            // 块上有多少页
	int               start_page;            // 起始页
	// Block Info
	minifs_BlockInfo  blockInfo;             // 块信息
}minifs_Block;

struct minifs_device {
	int               StartAddr;             // flash上minifs分区起始地址，64KB对齐
	int               EndAddr;               // flash上minifs分区结束地址，64KB对齐

	int               StartBlock;            // 开始物理块编号
	int               EndBlock;              // 结束物理块编号
	int               block_count;           // 设备上物理块总
	int               page_count;            // 设备上页总数
	minifs_Block*     blocks;                // 设备上块列表
	int               DataBytesPerChunk;     // 对象上每块大小

	// Entry parameters set up way early. MiniFS sets up the rest.
	uint32_t          PagesMinBlock;         // 最小块上有多少页
	uint32_t          PagesMaxBlock;         // 最大块上有多少页
	int               ReservedBlocks;        // We want this tuneable so that we can reduce
	                                         // reserved blocks on NOR and RAM.

	int               FreePages;             // 空闲Page数量
	int               currentDirtyChecker;   // 当前脏页

	uint8_t*          pageBits;              // bitmap of pages in use
	int               pageBitmapStride;      // Number of bytes of pageBits per block.
	                                         // Must be consistent with nPagesPerBlock.

	uint16_t          pageGroupBits;         // 0 for devices <= 32MB. else log2(npages) - 16
	uint16_t          pageGroupSize;         // == 2^^pageGroupBits

	int               ErasedBlocks;
	int               allocationBlock;       // 当前被使用的块[Current block being allocated off]
	uint32_t          allocationPage;
	int               allocationBlockFinder; // Used to search for next allocation block

	char*             devname;
	char*             rootPath;
	size_t            size;
	void*             buffer;
	void*             self;
	minifs_Object*    root;
	void*             zdata;

	minifs_Tnode*     freeTnodes;         // 当前空闲结点链表

	struct list_head  scanTempList;
	int (*writePage)  (struct minifs_device *dev, int page, const uint8_t *data, minifs_PageTags *tags);
	int (*readPage)   (struct minifs_device *dev, int page, uint8_t *data,       minifs_PageTags *tags);
	int (*eraseBlock) (struct minifs_device *dev, int block);
	int (*Init)       (struct minifs_device *dev); // 建立块大小
	int (*Release)    (struct minifs_device *dev);
};


extern const char minifs_countBitsTable[256];

// device.c
#define minifs_CountBits(x) minifs_countBitsTable[x]

#define minifs_GetBlockInfo(dev, blk) \
	((blk)>= (dev)->StartBlock && (blk) <= (dev)->EndBlock) ? &(dev)->blocks[blk].blockInfo : NULL


int device_Init       (minifs_device *dev);
int device_WritePage  (struct minifs_device *dev, int page, const uint8_t *data, minifs_PageTags *tags);
int device_ReadPage   (struct minifs_device *dev, int page, uint8_t *data, minifs_PageTags *tags, int doErrorCorrection);
int device_EraseBlock (struct minifs_device *dev, int block);

#define device_BlockStartPage(dev, block)  ((dev)->blocks[(block)].start_page)
#define device_BlockPageCount(dev, block)  ((dev)->blocks[(block)].page_count)

int  device_StillSomePageBits(minifs_device *dev, int blk);
int  device_CheckPageBit     (minifs_device *dev, int blk, int page);
int  device_IsBlockBad       (minifs_device *dev, int blk);

#define minifs_BlockBits(dev, blk) \
	((blk) >= (dev)->StartBlock && (blk) <= (dev)->EndBlock ? \
	 (dev)->pageBits + ((dev)->pageBitmapStride * ((blk) - (dev)->StartBlock)): NULL)

// 将block上的page置1
#define device_SetPageBit(dev, blk, page) do { \
	uint8_t *blkBits = minifs_BlockBits((dev), (blk)); \
	blkBits[(page) / 8] |= (1<<((page) & 7)); \
} while (0)


#define device_ClearPageBits(dev, blk) do { \
	uint8_t *blkBits = minifs_BlockBits((dev), (blk)); \
	memset(blkBits, 0, (dev)->pageBitmapStride); \
} while(0)


// 将block上的page置0
#define device_ClearPageBit(dev, blk, page) do { \
	uint8_t *blkBits = minifs_BlockBits((dev), (blk)); \
	blkBits[(page) / 8] &=  ~ (1<<((page) & 7)); \
} while(0)

// node.c
minifs_Tnode *minifs_GetTnode    (minifs_device *dev);
void minifs_FreeTnode            (minifs_device *dev, minifs_Tnode *tn);
int  minifs_AllocatePage         (minifs_device *dev, int useReserve);
void minifs_DeletePage           (minifs_device *dev, int pageId, int markNAND);
int  minifs_FindBlock            (minifs_device *dev, int pageId);
void minifs_BlockBecameDirty     (minifs_device *dev, int blockNo);
void minifs_BadBlockRecycling    (minifs_device *dev, int blockNo);

// object.c
minifs_Object *minifs_FindObjectByName(minifs_device *dev, const char *name);
minifs_Object *minifs_FindObjectByIdFromRootTree(minifs_device *dev, int objectId);
minifs_Object *minifs_CreateNewObject(minifs_device *dev, minifs_Object *parent, int objectId,
		enum minifs_obj_type type, uint32_t uid, uint32_t gid, uint32_t mode);
minifs_Object *minifs_FindOrCreateObjectById(minifs_device *dev, minifs_Object *parent, int objectId,
		enum minifs_obj_type type, uint32_t uid, uint32_t gid, uint32_t mode);
minifs_Object *minifs_CreateNewObjectByName(minifs_device *dev, const char *name,
		enum minifs_obj_type type, uint32_t uid, uint32_t gid, uint32_t mode,
		minifs_Object *equiv_obj, const char *alias);

char *minifs_GetObjectPath        (minifs_Object *object, char *path, size_t size);
int  minifs_PutPageIntoObject     (minifs_Object *object, int chunk, int page, int inScan);
int  minifs_ReadChunkFromObject   (minifs_Object *object, int chunk, uint8_t *buffer);
int  minifs_WriteChunkToObject    (minifs_Object *object, int chunk, const uint8_t *buffer, int nBytes, int useReserve);
int  minifs_UpdateObjectHeader    (minifs_Object *object, const char *name, int force);
void minifs_FlushObjectPageCache  (minifs_Object *object);
void minifs_FreeObject            (minifs_Object *object);
int  minifs_DeleteObject          (minifs_Object *object);

int minifs_ShowPage               (minifs_Object *object);
int minifs_FlushObject            (minifs_Object *object);
int minifs_ReadDataFromObject     (minifs_Object *object, uint8_t * buffer, uint32_t offset, int nBytes);
int minifs_WriteDataToObject      (minifs_Object *object, const uint8_t * buffer, uint32_t offset, int nBytes);

#endif

