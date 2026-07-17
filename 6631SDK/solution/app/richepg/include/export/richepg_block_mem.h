#ifndef __RICHEPG_BLOCK_MEM_H__
#define __RICHEPG_BLOCK_MEM_H__

#include "gxtype.h"
#include "common/gxmalloc.h"
#include "richepg_pool_mem.h"

struct mem_list_head {
    struct mem_list_head *next, *prev;
};

typedef struct {
    struct mem_list_head list;          // 链表节点
    size_t size;                        // 内存大小
    unsigned char *ptr;                 // 首部地址
    unsigned char *cur;                 // 当前地址
} BlockMemData;

typedef struct {
    int mem_id;                         // 所属类别ID
    size_t def_size;                    // 默认分配块大小
    size_t align_byte;                  // 几字节对齐要求
    size_t max_size;                    // 限制的最大内存
    int fast_alloc;                     // (0只查找当前节点 / 1查找所有节点)分配不到就重新分配
} BlockMemMgrAttr;

typedef struct {
    struct mem_list_head list;          // 链表节点
    struct mem_list_head head;          // BlockMemData挂载节点
    BlockMemData *cur_node;             // 当前使用的内存块
    BlockMemMgrAttr attr;
    size_t used_size;                   // 当前已申请的内存大小
} BlockMemMgr;

typedef struct {
    struct mem_list_head head;          // BlockMemMgr挂载节点
} BlockMem;

/* 从mgr管理的内存块中分配一块size大小的内存 */
void *block_mem_mgr_alloc_data(size_t size, BlockMemMgr *mgr);
/* 从mgr管理的内存块中分配一块内存并将ptr的数据copy进去 */
void *block_mem_mgr_dup_data(void *ptr, size_t size, BlockMemMgr *mgr);
/* 从mgr管理的内存块中分配一块内存并将str字符串dup进去 */
void *block_mem_mgr_dup_str(const char *str, BlockMemMgr *mgr);
/* 将mgr管理的全部内存块节点realloc到已使用的大小 */
void block_mem_mgr_adjust_size(BlockMemMgr *mgr);
/* 计算mgr管理的全部内存块节点的内存使用的大小 */
size_t block_mem_mgr_tell_size(BlockMemMgr *mgr);
/* 释放mgr管理的全部内存块节点 */
void block_mem_mgr_free_data(BlockMemMgr *mgr);
/* 初始化一个内存块管理节点，需要调用者先设置好mgr的attr属性 */
int block_mem_mgr_init(BlockMemMgr *mgr);
/* 分配并初始化内存块管理节点, 并挂载到mem上，使用完成后需要调用block_mem_mgr_free释放*/
BlockMemMgr *block_mem_mgr_alloc(BlockMemMgrAttr *attr, BlockMem *mem);
/* 释放mgr管理的全部内存块节点并释放mgr */
void block_mem_mgr_free(BlockMemMgr *mgr);
/* 从mgr管理中分配一块size大小的新内存块 */
void *block_mem_mgr_alloc_independent_data(size_t size, BlockMemMgr *mgr);
/* 释放mgr管理的内存块中首地址是ptr的内存块 */
void block_mem_mgr_free_independent_data(void *ptr, BlockMemMgr *mgr);
/* 从mgr管理中分配一块size大小的新内存块并拷贝ptr对应内存块的内容到新内存块 */
void *block_mem_mgr_realloc_independent_data(void *ptr, size_t size, BlockMemMgr *mgr);

/* 通过mem_id找到挂载在mem上的内存块管理节点 */
BlockMemMgr *block_mem_mgr_get_byid(int mem_id, BlockMem *mem);
/* 通过mem_id先找到挂载在mem上的内存块管理节点，然后再调用对应的函数 */
void *block_mem_mgr_alloc_data_byid(size_t size, int mem_id, BlockMem *mem);
void *block_mem_mgr_dup_data_byid(void *ptr, size_t size, int mem_id, BlockMem *mem);
void *block_mem_mgr_dup_str_byid(const char *str, int mem_id, BlockMem *mem);
void block_mem_mgr_adjust_size_byid(int mem_id, BlockMem *mem);
size_t block_mem_mgr_tell_size_byid(int mem_id, BlockMem *mem);
void block_mem_mgr_free_data_byid(int mem_id, BlockMem *mem);
void block_mem_mgr_free_byid(int mem_id, BlockMem *mem);

/* 将挂载在mem上的mgr管理的所有内存块节点realloc到已使用的大小 */
void block_mem_adjust_size(BlockMem *mem);
/* 计算挂载在mem上的mgr管理的所有内存块节点的内存使用的大小 */
size_t block_mem_tell_size(BlockMem *mem);
/* 释放挂载在mem的mgr管理的所有内存块节点（不含内存块管理节点） */
void block_mem_free_data(BlockMem *mem);
/* 初始化内存块管理入口 */
int block_mem_init(BlockMem *mem);
/* 释放挂载在mem上的mgr管理的所有内存块节点和所有内存块管理节点 */
void block_mem_free(BlockMem *mem);

/* 初始化用于存储内存块管理节点和内存块节点的内存池 */
int block_mem_pool_init(pool_mem_num_t mgr_num, pool_mem_num_t data_num);
/* 释放用于存储内存块管理节点和内存块节点的内存池 */
int block_mem_pool_release(void);

#endif

