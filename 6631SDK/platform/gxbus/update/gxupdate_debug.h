#ifndef __GXUPDATE_DEBUG_H__
#define __GXUPDATE_DEBUG_H__

#include "gxcore.h"
#include "assert.h"


#define E_INPUT                     (-2)
#define E_FAILURE                   (-1)
#define E_OK                        (0)
#define E_INVALID_HANLDE            (0)



//#define GXUPDATE_DEBUG


#define DBG_TRACE(...)              do {\
                                            gxlogd("\n[%s:%d]: ", __FILE__, __LINE__);\
                                            gxlogd( __VA_ARGS__ );\
                                        } while (0)

#ifndef GXUPDATE_DEBUG
#define ASSERT(exp)
#define DBG_UPDATE(...)
#define MALLOC(size)                    GxCore_Malloc(size)
#define CALLOC(n, size)                 GxCore_Calloc(n, size)
#define REALLOC(ptr, size)              GxCore_Realloc(ptr, size)
#define FREE(ptr)                       GxCore_Free(ptr)
#else
#define ASSERT(exp)                     assert(exp)
#define DBG_UPDATE(...)                    do {\
                                            gxlogd("\n[%s:%d]: ", __FILE__, __LINE__);\
                                            gxlogd( __VA_ARGS__ );\
                                        } while (0)
#define FREE(ptr)                       GxCore_Free(ptr)



void*   MALLOC                          (size_t size);
void*   CALLOC                          (size_t n, size_t size);
void*   REALLOC                         (void* ptr, size_t size);

#endif



#define TRACE(...)                      do {\
                                            gxlogd("\n[update] ");\
                                            gxlogd( __VA_ARGS__ );\
                                        } while (0)

#endif

