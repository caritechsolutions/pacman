#ifndef __GUI_HW_POOL_H__
#define __GUI_HW_POOL_H__
#include "gui_private.h"
#include "common/gx_hw_malloc.h"

void gui_hw_pool_init(void* buf, int32_t addr, int32_t size);
void gui_hw_pool_malloc(GxHwMallocObj *p, GxHwMallocFlag flag);
void gui_hw_pool_free(GxHwMallocObj *p);
int32_t gui_hw_pool_check(GxHwMallocObj *p);

#endif /*__GUI_HW_POOL_H__*/

