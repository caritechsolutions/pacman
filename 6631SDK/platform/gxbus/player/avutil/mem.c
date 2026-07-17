/*
*  default memory allocator for libavutil
*  Copyright (c) 2002 Fabrice Bellard.
*
*  This file is part of FFmpeg.
*
*  FFmpeg is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  FFmpeg is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with FFmpeg; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

/**
*  @file mem.c
*  default memory allocator for libavutil.
*/

#include "common.h"
#include "gx_system.h"
#include "gx_mediafilter.h"


/**
 * Multiply two size_t values checking for overflow.
 * @return  0 if success, AVERROR(EINVAL) if overflow.
 */
static int av_size_mult(size_t a, size_t b, size_t *r)
{
    size_t t = a * b;
    /* Hack inspired from glibc: only try the division if nelem and elsize
     * are both greater than sqrt(SIZE_MAX). */
    if ((a | b) >= ((size_t)1 << (sizeof(size_t) * 4)) && a && t / a != b)
        return AVERROR(EINVAL);
    *r = t;
    return 0;
}

char *av_strndup(const char *s, size_t len)
{
    char *ret = NULL, *end;
	int error_status = PLAYER_ERROR_NO_ERROR;

    if (!s)
        return NULL;

    end = memchr(s, 0, len);
    if (end)
        len = end - s;

    ret = av_realloc(NULL, len + 1);
    if (!ret) {
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
        return NULL;
    }

    memcpy(ret, s, len);
    ret[len] = 0;
    return ret;
}

void* av_fast_realloc(void *ptr, unsigned int *size, unsigned int min_size)
{
	int error_status = PLAYER_ERROR_NO_ERROR;
	void *ret_ptr = NULL;
	unsigned int relloc_size;

	if (min_size <* size)
		return ptr;

	relloc_size = GXMAX(17*  min_size / 16 + 32, min_size);
	ret_ptr = av_realloc(ptr, relloc_size);
	if (ret_ptr) {
		*size = relloc_size;
	} else {
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}

	return ret_ptr;
}

void* av_fast_realloc2(void *ptr, unsigned int *num, unsigned int min_num,unsigned int unit_size)
{
	int error_status = PLAYER_ERROR_NO_ERROR;
	void *ret_ptr = NULL;
	unsigned int relloc_num;

	if (min_num <* num)
		return ptr;

	relloc_num = *num + 1024;
	ret_ptr = av_realloc(ptr,relloc_num*unit_size);
	if (ret_ptr) {
		*num = relloc_num;
	} else {
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}

	return ret_ptr;
}

int av_fast_malloc(void *ptr, int *size, size_t min_size, int zero_realloc)
{
	int error_status = PLAYER_ERROR_NO_ERROR;
	void **p = ptr;
	if (min_size < *size)
		return 0;
	min_size = GXMAX(17 * min_size / 16 + 32, min_size);
	av_free(*p);
	*p = zero_realloc ? av_mallocz(min_size) : av_malloc(min_size);
	if (!*p) {
		min_size = 0;
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}
	*size = min_size;
	return 1;
}

void *av_memdup(const void *p, size_t size)
{
	int error_status = PLAYER_ERROR_NO_ERROR;
    void *ptr = NULL;
    if (p) {
        ptr = av_malloc(size);
        if (ptr) {
            memcpy(ptr, p, size);
        } else {
			error_status = PLAYER_ERROR_MALLOC_FAILED;
			GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
        }
    }
    return ptr;
}


void av_fast_padded_malloc(void *ptr, int *size, size_t min_size)
{
	uint8_t **p = ptr;
	if (min_size > SIZE_MAX - FF_INPUT_BUFFER_PADDING_SIZE) {
		av_freep(p);
		*size = 0;
		return;
	}
	if (!av_fast_malloc(p, size, min_size + FF_INPUT_BUFFER_PADDING_SIZE, 1))
		memset(*p + min_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
}

void *av_realloc_f(void *ptr, size_t nelem, size_t elsize)
{
	int error_status = PLAYER_ERROR_NO_ERROR;
    size_t size;
    void *r;

    if (av_size_mult(elsize, nelem, &size)) {
        av_free(ptr);
        return NULL;
    }
    r = av_realloc(ptr, size);
    if (!r && size) {
        av_free(ptr);
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
    }
    return r;
}

int av_reallocp(void *ptr, size_t size)
{
    void **ptrptr = ptr;
    void *ret;
	int error_status = PLAYER_ERROR_NO_ERROR;

    if (!size) {
        av_freep(ptr);
        return 0;
    }
    ret = av_realloc(*ptrptr, size);

    if (!ret) {
        av_freep(ptr);
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
        return AVERROR(ENOMEM);
    }

    *ptrptr = ret;
    return 0;
}

void *av_realloc_array(void *ptr, size_t nmemb, size_t size)
{
	void *r;
	int error_status = PLAYER_ERROR_NO_ERROR;

    if (!size || nmemb >= INT_MAX / size)
        return NULL;
    r = av_realloc(ptr, nmemb * size);
	if (!r) {
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}
	return r;
}

int av_reallocp_array(void *ptr, size_t nmemb, size_t size)
{
    void **ptrptr = ptr;
	int error_status = PLAYER_ERROR_NO_ERROR;

    *ptrptr = av_realloc_f(*ptrptr, nmemb, size);
    if (!*ptrptr && nmemb && size) {
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
        return AVERROR(ENOMEM);
    }
    return 0;
}

void *av_mallocz_array(size_t nmemb, size_t size)
{
	void *r;
	int error_status = PLAYER_ERROR_NO_ERROR;

    if (!size || nmemb >= INT_MAX / size)
        return NULL;
    r = av_mallocz(nmemb * size);
	if (!r) {
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}
	return r;
}

void *av_malloc_array(size_t nmemb, size_t size)
{
	void *r;
	int error_status = PLAYER_ERROR_NO_ERROR;

    if (!size || nmemb >= INT_MAX / size)
        return NULL;
    r = av_malloc(nmemb * size);
	if (!r) {
		error_status = PLAYER_ERROR_MALLOC_FAILED;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}
	return r;
}

int is_system_large_memory(void)
{
	#define PLAYER_DEFAULT_SYSMEM_SIZE	(8*1024*1024)
	int sysMemSize = 0;
	GxPlayer_SystemGet(PSYS_PACKET_CACHE, &sysMemSize);
	if (sysMemSize >= PLAYER_DEFAULT_SYSMEM_SIZE)
		return 1;
	return 0;
}

