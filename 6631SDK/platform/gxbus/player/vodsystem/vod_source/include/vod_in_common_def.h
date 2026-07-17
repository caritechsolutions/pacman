#ifndef _VOD_IN_COMMON_DEF_H_
#define _VOD_IN_COMMON_DEF_H_

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#if 0
#define GxVod_debug(...)     gxlogf ( __VA_ARGS__ )
#else
#define GxVod_debug(...)     ((void)0)
#endif

#ifndef DEFINED_U8
#define DEFINED_U8
typedef unsigned char  U8;
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
typedef unsigned short U16;
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned int   U32;
#endif

/* Common signed types */
#ifndef DEFINED_S8
#define DEFINED_S8
/*typedef signed char  S8;*/
typedef signed char  S8;
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
typedef signed short S16;
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
typedef signed int   S32;
#endif
#ifndef uint64_t
typedef struct uint64 {U32 up32; U32 low32;} U64;
#endif
typedef signed long S64;
#ifdef _WIN32

/*∑¿÷π≥ÂÕª*/
//#include <windows.h>
#include	<time.h>
typedef signed char BOOLEAN;

#else

#ifndef DEFINED_BOOLEAN
#define DEFINED_BOOLEAN
typedef signed char BOOLEAN;
#endif

#endif


#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif


#define xmalloc(memsize)	vod_porting_mem_malloc(0,memsize)
#define xfree(ptr)			vod_porting_mem_free(0,ptr)
#define strdup(str)			av_strdup(str)
#define strlcpy				av_strlcpy
#define strncasecmp			av_strncasecmp

#define MALLOC_STRUCTURE(a) ((a *)xmalloc(sizeof(a)))
#define CHECK_AND_FREE(a) if ((a) != NULL) { xfree((void *)(a)); (a) = NULL;}
#define NUM_ELEMENTS_IN_ARRAY(name) ((sizeof((name))) / (sizeof(*(name))))
#define ADV_SPACE(a) {while (isspace(*(a)) && (*(a) != '\0'))(a)++;}

#endif




