#include "gxcore.h"
#include "sub_debug.h"
#include "gx_mem.h"

#ifdef DEBUG_SUB
static size_t subtitle_malloc_num   = 0;
static size_t subtitle_calloc_num   = 0;
static size_t subtitle_realloc_num  = 0;
static size_t subtitle_totoal_mem   = 0;

void* MALLOC(size_t size)
{
    subtitle_malloc_num++;
    subtitle_totoal_mem += size;
    return av_malloc(size);
}

void* CALLOC(size_t n, size_t size)
{
    subtitle_calloc_num++;
    subtitle_totoal_mem += size;
    return av_malloc(n * size);
}
void* REALLOC(void* ptr, size_t size)
{
    subtitle_realloc_num++;
    subtitle_totoal_mem += size;
    return av_realloc(ptr, size);
}

#endif
