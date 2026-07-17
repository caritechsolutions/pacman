#include "gui_core.h"
#include "gui_pool.h"
#include "common/gx_memblk_pool.h"

static GuiPool *gui_pool;
static GxMemblockPoolBucket *gui_block_map = NULL;

void GuiPool_Info(void)
{
	return (GxCore_MemblockPool_Info(gui_pool));
}

void GuiPool_SetSize(GuiPoolBlock *block_info)
{
	if(NULL == block_info) {
		return;
	}

	if(NULL == gui_block_map) {
		gui_block_map = GxCore_Mallocz(sizeof(GuiPoolBlock));
		if(NULL == gui_block_map) {
			return;
		}
		memcpy(gui_block_map, block_info, sizeof(GuiPoolBlock));
		gui_block_map->size_info_set = GxCore_Mallocz(gui_block_map->total_size * sizeof(int));
		memcpy(gui_block_map->size_info_set, block_info->size_info_set, gui_block_map->total_size * sizeof(int));
		gui_block_map->count_info_set = GxCore_Mallocz(gui_block_map->total_size * sizeof(int));
		memcpy(gui_block_map->count_info_set, block_info->count_info_set, gui_block_map->total_size * sizeof(int));
	}
}

void GuiPool_Init(size_t size)
{
	gui_pool = GxCore_MemblockPool_Init(size, gui_block_map);
}

void GuiPool_Destroy(void)
{
	return (GxCore_MemblockPool_Destroy(gui_pool));
}

void *GuiPool_Alloc(size_t size)
{
	return (GxCore_MemblockPool_Alloc(gui_pool, size));
}

void *GuiPool_Allocz(size_t size)
{
	return (GxCore_MemblockPool_Allocz(gui_pool, size));
}

void *GuiPool_Realloc(void *ptr, size_t size)
{
	return (GxCore_MemblockPool_Realloc(gui_pool, ptr, size));
}

char *GuiPool_Strdup(const char *s)
{
	return (GxCore_MemblockPool_Strdup(gui_pool, s));
}

void GuiPool_Free(void *ptr)
{
	return (GxCore_MemblockPool_Free(gui_pool, ptr));
}

