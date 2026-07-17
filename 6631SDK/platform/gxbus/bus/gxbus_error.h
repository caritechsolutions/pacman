#ifndef __GXBUS_ERROR_H__
#define __GXBUS_ERROR_H__

#include "gxcore.h"
#include "common/config.h"

__BEGIN_DECLS

#ifdef __DEBUG
#define __GXBUS_DEBUG
#endif

#define GXBUS_TRACE( ... )                      gxlogd( __VA_ARGS__ )

#ifdef __GXBUS_DEBUG

#include <assert.h>

#define GXBUS_ERROR_INPUT                       (-1)
#define GXBUS_ERROR_NOT_ENOUGH_MEM              (-2)
#define GXBUS_ERROR_OS_CORE_FAULT               (-3)
#define GXBUS_ERROR_SERVICE_FAULT               (-4)
#define GXBUS_ERROR_MSG_THREAD                  (-5)
#define GXBUS_ERROR_MSG_QUEUE                   (-6)
#define GXBUS_ERROR_MSG_NOT_LISTEN              (-7)            
#define GXBUS_ERROR_BUS_NOT_INIT                (-8) 
#define GXBUS_ERROR_MSG_NOT_REGISTER            (-9)

#define GXBUS_ERROR_LIST                                                \
{                                                                       \
    { GXBUS_ERROR_INPUT,            "Input argument error!"},           \
    { GXBUS_ERROR_NOT_ENOUGH_MEM,   "Not enough memory error!"},        \
    { GXBUS_ERROR_OS_CORE_FAULT,    "Fault when calling CxCore API"},   \
    { GXBUS_ERROR_SERVICE_FAULT,    "Fault service!"},                  \
    { GXBUS_ERROR_MSG_THREAD,       "Fault when msg scheduler thread"}, \
    { GXBUS_ERROR_MSG_QUEUE,        "Fault when calling mqueue"},       \
    { GXBUS_ERROR_MSG_NOT_LISTEN,   "The message is not listening"},    \
    { GXBUS_ERROR_BUS_NOT_INIT,     "The bus do not have initialize"},  \
    { GXBUS_ERROR_MSG_NOT_REGISTER, "The message do not register"}      \
}

#define GXBUS_DbgPrint                          GXBUS_TRACE

#define GXBUS_ASSERT( expression )  assert( (expression) )

#define GXBUS_ERROR( error_code )                                       \
do {                                                                    \
    GXBUS_TRACE("\n\n----Error----\n");                                 \
    GXBUS_TRACE("%s:%s:%d\n", __FILE__,                                 \
                  __FUNCTION__, __LINE__ );                             \
    GxBus_Error( (error_code) );                                        \
    GXBUS_TRACE("----Error----\n\n");                                   \
    GXBUS_ASSERT( FALSE );                                              \
} while (0)

#define GXBUS_WARNING( msg )                                            \
do {                                                                    \
    GXBUS_TRACE("\n\n----Warning----\n");                               \
    GXBUS_TRACE("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );       \
    GXBUS_TRACE("%s\n", msg);                                           \
    GXBUS_TRACE("----Warning----\n\n");                                 \
} while( 0)




void GxBus_Error( int32_t ErrorCode );

#else

#define GXBUS_DbgPrint( ... )
#define GXBUS_WARNING( msg )
#define GXBUS_ASSERT( expression )
#define GXBUS_ERROR( error_code )
#define GXBUS_PRINT_VERSION()
#endif

#include "gxcore.h"

__END_DECLS

#endif
