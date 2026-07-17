#ifndef __RICHEPG_POOL_MEM_H__
#define __RICHEPG_POOL_MEM_H__

#include "gxtype.h"
#include "common/gxmalloc.h"
#include "gxos/gxcore_os_core.h"

typedef unsigned short pool_mem_num_t;

typedef struct {
    int size;               // 每个内存单元的大小
    pool_mem_num_t num;     // 内存单元总数，不要等于pool_mem_num_t的最大值，目前最大值为65534
    pool_mem_num_t sel;     // 当前空闲的序号
    pool_mem_num_t *puse;   // 内存池状态数组
    void *begin;            // 内存池开始地址
    void *end;              // 内存池结束地址
    handle_t mtx;           // 互斥锁
    int redo;               // 内存池分配不到时: 0, 返回失败; 1, 继续malloc分配
} PoolMemMgr;

int pool_mem_mgr_init(PoolMemMgr *mgr, int size, pool_mem_num_t num, int redo);
int pool_mem_mgr_release(PoolMemMgr *mgr);
void* pool_mem_malloc(PoolMemMgr *mgr);
void* pool_mem_mallocz(PoolMemMgr *mgr);
void pool_mem_free(PoolMemMgr *mgr, void *ptr);

#endif

