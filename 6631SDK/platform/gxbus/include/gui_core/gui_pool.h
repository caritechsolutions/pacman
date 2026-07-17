#ifndef __GUI_POOL_H__
#define __GUI_POOL_H__

#include "common/gx_memblk_pool.h"

#define GUIPOOL_SIZE       GX_MEMBLOCK_POOL_SIZE
#define GUIHW_POOL_SIZE    1 * 1024 * 1024

typedef GxMemBlockPool GuiPool;
typedef GxMemblockPoolBucket GuiPoolBlock;

void GuiPool_SetSize(GuiPoolBlock *block_info);
void GuiPool_Info(void);
void GuiPool_Init(size_t size);
void GuiPool_Destroy(void);
void *GuiPool_Alloc(size_t size);
void *GuiPool_Allocz(size_t size);
void *GuiPool_Realloc(void *ptr, size_t size);
char *GuiPool_Strdup(const char *s);
void GuiPool_Free(void *ptr);

#endif

