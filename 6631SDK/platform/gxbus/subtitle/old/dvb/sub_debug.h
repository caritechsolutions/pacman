#ifndef __SUB_DEBUG_H__
#define __SUB_DEBUG_H__

#include "gxcore.h"
#include "assert.h"

#define NO_SIMULATOR      0
#define DEMUX_SIMULATOR   1
#define RENDER_SIMULATOR  2

#define SIMULATOR           NO_SIMULATOR

//#define DEBUG_SUB
//#define SUBT_DEBUG          //Just open this for debug information without FILE/LINE
//#define SUBT_DEBUG_DETAIL

#define DEBUG_SUB_DECODE
#define DEBUG_SUB_DEMUX

#define DBG_TRACE(...)              do {\
                                            gxlogd("\n[%s:%d]: ", __FILE__, __LINE__);\
                                            gxlogd( __VA_ARGS__ );\
                                        } while (0)

#ifndef DEBUG_SUB                       

#ifndef SUBT_DEBUG
#define DBG_SUB(...)
#else
#define DBG_SUB(...)                    gxlogd( __VA_ARGS__ )
#endif
#define ASSERT(exp)
#define MALLOC(size)                    av_malloc(size)
#define CALLOC(n, size)                 av_malloc((n)*(size))
#define REALLOC(ptr, size)              av_realloc(ptr, size)
#define FREE(ptr)                       av_free(ptr)

#else

#define ASSERT(exp)                     assert(exp)
#define DBG_SUB(...)                    do {\
                                            gxlogd("\n[%s:%d]: ", __FILE__, __LINE__);\
                                            gxlogd( __VA_ARGS__ );\
                                        } while (0)
#define FREE(ptr)                       av_free(ptr)
void*   MALLOC                          (size_t size);
void*   CALLOC                          (size_t n, size_t size);
void*   REALLOC                         (void* ptr, size_t size);

#endif



#define TRACE(...)                      do {\
                                            gxlogd("\n[Subtitle] ");\
                                            gxlogd( __VA_ARGS__ );\
                                        } while (0)

#endif
