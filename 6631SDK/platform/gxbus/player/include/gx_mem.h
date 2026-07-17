/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file mem.h
 * Memory handling functions.
 */

#ifndef AV_MEM_H
#define AV_MEM_H

#ifdef MEMWATCH
#include <common/memwatch.h>
#else
#include <stdlib.h>
#include <gxcore.h>
#endif

#ifdef __GNUC__
#define DECLARE_ALIGNED(n,t,v)       t v __attribute__ ((aligned (n)))
#else
#define DECLARE_ALIGNED(n,t,v)      __declspec(align(n)) t v
#endif

#ifndef MEMORY_DEBUG
/**
 * Memory allocation of size byte with alignment suitable for all
 * memory accesses (including vectors if available on the
 * CPU). av_malloc(0) must return a non NULL pointer.
 */
#ifdef AV_MEMORY_DEBUG

void *av_malloc_debug (const char *func, int line, unsigned int size);
void  av_free_debug   (const char *func, int line, void *ptr);
void *av_realloc_debug(const char *func, int line, void *ptr, unsigned int size);
void *av_mallocz_debug(const char *func, int line, unsigned int size);
void *av_calloc_debug (const char *func, int line, int n, size_t size);
char *av_strdup_debug (const char *func, int line, const char *s);
void av_freep_debug(const char *func, int line, void *ptr);

#define av_malloc(size)      av_malloc_debug(__func__, __LINE__, size)
#define av_mallocz(size)     av_mallocz_debug(__func__, __LINE__, size)
#define av_calloc(n,size)    av_calloc_debug(__func__, __LINE__, n, size)
#define av_realloc(ptr,size) av_realloc_debug(__func__, __LINE__, ptr, size)
#define av_free(ptr)         av_free_debug(__func__, __LINE__, ptr)
#define av_strdup(ptr)       av_strdup_debug(__func__, __LINE__, ptr)
#define av_freep(ptr)        av_freep_debug(__func__, __LINE__, ptr)

#else
void *av_malloc(unsigned int size);

/**
 * av_realloc semantics (same as glibc): if ptr is NULL and size > 0,
 * identical to GxCore_Malloc(size). If size is zero, it is identical to
 * GxCore_Free(ptr) and NULL is returned.
 */
void *av_realloc(void *ptr, unsigned int size);

/**
 * GxCore_Free memory which has been allocated with av_malloc(z)() or av_realloc().
 * NOTE: ptr = NULL is explicetly allowed
 * Note2: it is recommended that you use av_freep() instead
 */
void av_free(void *ptr);

void av_freep(void *ptr);

void *av_mallocz(unsigned int size);

void *av_calloc(int n, size_t size);

/**
 * Duplicates the string \p s.
 * @param s String to be duplicated.
 * @return Pointer to a newly allocated string containing a
 * copy of \p s or NULL if it cannot allocate it.
 */
char *av_strdup(const char *s);

/**
 * Frees memory and sets the pointer to NULL.
 * @param ptr pointer to the pointer which should be freed
 */

#endif
#else
#define av_malloc(size)        GxCore_Malloc(size)
#define av_mallocz(size)       GxCore_Mallocz(size)
#define av_realloc(ptr,size)   GxCore_Realloc(ptr,size)
#define av_free(ptr)           GxCore_Free(ptr)
#define av_strdup(ptr)         GxCore_Strdup(ptr)
#define av_calloc(n,size)      GxCore_Calloc(n,size)
#define av_freep(ptr)    do {         \
	void** xptr = (void **)ptr;       \
	GxCore_Free(*xptr);               \
	*xptr = NULL;                     \
} while(0)

#endif

void av_fast_padded_malloc(void *ptr, int *size, size_t min_size);
void *av_fast_realloc(void *ptr, unsigned int *size, unsigned int min_size);
void *av_fast_realloc2(void *ptr, unsigned int *num, unsigned int min_num,unsigned int unit_size);
int av_fast_malloc(void *ptr, int *size, size_t min_size, int zero_realloc);

void *av_realloc_f(void *ptr, size_t nelem, size_t elsize);
int av_reallocp(void *ptr, size_t size);
void *av_realloc_array(void *ptr, size_t nmemb, size_t size);
int av_reallocp_array(void *ptr, size_t nmemb, size_t size);

void av_memhole_init(void);
unsigned int   av_memhole_tsw_getsize(void);
unsigned char *av_memhole_malloc(unsigned long size, unsigned int flag);
void av_memhole_free(unsigned char *p, unsigned int flag);
void av_memhole_flush(void);
unsigned char* av_memhole_mmap(unsigned char *p);
unsigned char *av_memhole_malloc_user(unsigned long size);
void av_memhole_free_user(unsigned char *p);
unsigned char *av_memhole_malloc_kernel(unsigned long size);
void av_memhole_free_kernel(unsigned char *p);
unsigned int av_memhole_now_used(void);
unsigned int av_memhole_max_used(void);

void av_mmv_init(unsigned long start, unsigned long size);
unsigned char *av_mmv_malloc(unsigned long size, unsigned int flag);
int av_mmv_free(unsigned char *p, unsigned int flag);
void av_mmv_flush(void);

void av_mma_init(unsigned long start, unsigned long size);

unsigned char *av_mma_malloc(unsigned long size, unsigned int flag);
int av_mma_free(unsigned char *p, unsigned int flag);
void av_mma_flush(void);

void *av_memdup(const void *p, size_t size);
char *av_strndup(const char *s, size_t len);
void *av_mallocz_array(size_t nmemb, size_t size);
void *av_malloc_array(size_t nmemb, size_t size);
int av_fast_malloc(void *ptr, int *size, size_t min_size, int zero_realloc);
int is_system_large_memory(void);

#endif

