#ifndef _GX_PLAYER_COMMON_H_
#define _GX_PLAYER_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

#ifdef LINUX_OS
#include <time.h>
#include <netdb.h>
#include <signal.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/time.h>
#elif defined(ECOS_OS)
#include <sys/select.h>
#include <sys/time.h>
#elif defined(WIN32_OS)
#endif

#ifndef NO_OS
#include <sys/socket.h>
#endif
#include <sys/stat.h>

#include <gxcore.h>
#include <av/avapi.h>
#include <devapi/chip_info.h>
#include <gxsecure/gxsecure_api.h>
#include <common/gx_hw_malloc.h>
#include <common/memhole.h>

#include "gx_msg.h"
#include "gx_mem.h"
#include "gx_pvr.h"
#include "gx_avtick.h"
#include "gx_avdebug.h"
#include "gx_avflow.h"
#include "gx_config.h"
#include "gx_utils.h"
#include "gxavdev.h"
#include "gxplayer_url.h"
#include "player_config.h"
#include "libconfig.h"

#include "module/config/gxconfig.h"
#include "common/tlsf.h"

//#define ENABLE_AUTHOR

#ifdef ENABLE_AUTHOR
typedef struct gx_author {
	const char*             info;
	const char*             name;
	const char*             desc;
	const char*             author;
	const char*             comment;
}GxAuthor;

#define AUTHER GxAuthor author
#define DEF_AUTHOR(info_, name_, desc_, author_, commont_) \
	.author = {                     \
		.info = info_,       \
		.name = name_,       \
		.desc = desc_,       \
		.author = author_,   \
		.comment = commont_}
#else

#define AUTHER char author;
#define DEF_AUTHOR(info_, name_, desc_, author_, commont_) \
	.author = 0

#endif

#ifndef I386_LINUX
#define DELAY10MS (10)
#else
#define DELAY10MS (0.01)
#endif

#define GXMAX(a,b) ((a) > (b) ? (a) : (b))
#define GXMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

#define __weak		__attribute__((weak))

#define GX_PLAYER_ERROR     (-1)
#define GX_PLAYER_OK        (0)

#define BSWAP_32(x)         (((((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF))>>16) | ((((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF))<<16));

#ifndef UINT64_MAX
#define UINT64_MAX     18446744073709551614ULL
#endif

#ifndef UINT16_MAX
#define UINT16_MAX (65535U)
#endif

#ifndef INT64_MAX
#define INT64_MAX      9223372036854775807LL
#endif

#ifndef INT64_MIN
#define INT64_MIN      (-9223372036854775807LL-1)
#endif

#ifndef SIZE_MAX
#define SIZE_MAX      (4294967295U)
#endif

#define AV_LOG_QUIET    -8
#define AV_LOG_PANIC     0
#define AV_LOG_FATAL     8
#define AV_LOG_ERROR    16
#define AV_LOG_WARNING  24
#define AV_LOG_INFO     32
#define AV_LOG_VERBOSE  40
#define AV_LOG_DEBUG    48
#define AV_LOG_MAX_OFFSET (AV_LOG_DEBUG - AV_LOG_QUIET)

#ifdef GXDBG
#define av_log(avcl, level, ...) gxlogi(__VA_ARGS__ )
#define av_dlog(avcl, ...) gxlogi(__VA_ARGS__ )
#else
#define av_log(avcl, level, ...) ((void)0)
#define av_dlog(avcl, level, ...) ((void)0)
#endif


#define CHECK_RETURN_VALUE(ret);  \
	if(ret<0) {\
		gxloge("%s[%d]:%s###################error##########\n", __FILE__, __LINE__, __FUNCTION__);\
		return ret; \
	}

#define CHECK_NULL(p) {if(p == NULL) return GX_PLAYER_ERROR;}

#ifdef WORDS_BIGENDIAN
#define GX_FOURCC( a, b, c, d ) \
	( ((uint32_t)d) | ( ((uint32_t)c) << 8 ) \
	  | ( ((uint32_t)b) << 16 ) | ( ((uint32_t)a) << 24 ) )
#define GX_TWOCC( a, b ) \
	( (uint16_t)(b) | ( (uint16_t)(a) << 8 ) )

#else
#define GX_FOURCC( a, b, c, d ) \
	( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
	  | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )
#define GX_TWOCC( a, b ) \
	( (uint16_t)(a) | ( (uint16_t)(b) << 8 ) )
#endif

#define FOURCC2CHRA(fcc,psz_fourcc )  memcpy( psz_fourcc, &(fcc), 4 );

#define TIME_MINUS(e, s)\
	(abs((e.seconds*1000+e.microsecs/1000) - (s.seconds*1000+s.microsecs/1000)))

#if 0

#define PLAYER_RUNTIME(prompt, code) do {                \
	static GxTime start, stop;    \
	uint32_t ms1, ms2;                        \
	\
	GxCore_GetTickTime(&start);               \
	code;                                     \
	GxCore_GetTickTime(&stop);                \
	ms1 = stop.seconds * 1000 + stop.microsecs / 1000 - (start.seconds * 1000 + start.microsecs / 1000); \
	ms2 = stop.seconds * 1000 + stop.microsecs / 1000; \
	gxlogd("\033[031m%s:\033[036m %u \033[0mms, total\033[036m %u \033[0mms\n", prompt, ms1, ms2); \
} while(0)

#define PLAYER_SHOWTIME(prompt) do { \
	uint32_t ms;                                                \
	GxTime now_tick;                                            \
	GxCore_GetTickTime(&now_tick);                              \
	ms = now_tick.seconds * 1000 + now_tick.microsecs / 1000;   \
	gxlogd("\033[031m%s:\033[036m %u \033[0mms\n", prompt, ms); \
}while(0)
#else
#define PLAYER_RUNTIME(prompt, code) do {                \
	code;                                     \
} while(0)

#define PLAYER_SHOWTIME(prompt) do { \
}while(0)

#endif

#define closesocket(xx) do {                \
	close(xx);                             \
}while(0)

typedef struct {
	unsigned long long timestamp;
	unsigned long long pos;
}PlayerRecIndexEntry;

#define GX_SPEED_SLOW(s)   ((s) > 0 && (s) < 1000)
#define GX_SPEED_NORMAL(s) ((s) >= 1000 && (s) < 2000)
#define GX_SPEED_FAST(s)   ((s) == 2000)
#define GX_SPEED_JUMP(s)   ((s) >  2000 || (s) < 0)

typedef int  (*PLAYER_INTERRUPT_CBK)(void);

extern int GxPlayer_Encrypt(char* dst, char* src, size_t size, off_t pos);

extern int GxPlayer_Decrypt(char* dst, char* src, size_t size, off_t pos);

#endif

