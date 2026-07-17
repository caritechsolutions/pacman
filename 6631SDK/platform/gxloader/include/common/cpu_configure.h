/*
 * =====================================================================================
 *
 *       Filename:  cpu_configure.h
 *
 *    Description:  Cache MMU configuration
 *
 *        Version:  1.0
 *        Created:  01/25/2011 04:06:26 PM
 *       Revision:  V1.0.0.0
 *       Compiler:  gcc
 *
 *         Author:  Charle (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __CPU_CONFIGURE_H__
#define __CPU_CONFIGURE_H__

int configure_mmu_cache_close(void);
int configure_mmu_cache_open(void);
unsigned char configure_paging_table(void);
void gx_dcache_clean(void);

// CPSR flag
unsigned int gx_saveflag_clif(void);
unsigned int gx_saveflag_clf(void);
unsigned int gx_saveflag_cli(void);
unsigned int gx_getflag(void);
void gx_restoreflag(unsigned int cpsr_reg);

// Cache
// clean : write dirty buffer (D cache only)
// invalidate : invalidate the contents of cache (I & D cache)
// flush : clean + invalidate
void gx_get_cache_state(int *picache, int *pdcache, int *pwriteback);
void gx_enable_cache(int icache, int dcache, int writeback);
void gx_clean_cache_data(void);
void gx_clean_cache_data_region(unsigned int from, unsigned int to);
void gx_invalidate_cache_instruction(void);
void gx_invalidate_cache_instruction_region(unsigned int from, unsigned int to);
void gx_invalidate_cache_data(void);
void gx_invalidate_cache_data_region(unsigned int from, unsigned int to);

void gx_flush_cache_all(void);
void gx_flush_cache_data(void);
void gx_flush_cache_data_region(unsigned int from, unsigned int to);


#endif

