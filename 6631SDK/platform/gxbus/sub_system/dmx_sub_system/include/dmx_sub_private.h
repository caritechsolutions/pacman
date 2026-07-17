/** @defgroup pm_module PM Module*/
/*@{*/
#ifndef __DMX_SUB_PRIVATE_H__
#define __DMX_SUB_PRIVATE_H__

__BEGIN_DECLS

#include "sub_system/dmx_sub_system/dmx_sub_system.h"

/*错误值*/

/* Exported Types --------------------------------------------------------- */
//#ifndef __DEBUG
#define GX_DMX_SUB_ERR_DBUG
//#endif

#define DMX_SUB_PRINTF(...) gxlogd( __VA_ARGS__ )

#define DMX_SUB_ERR_PRINTF(msg)\
    do{\
        DMX_SUB_PRINTF("\n\n*****dmx sub system error*****\n");\
        DMX_SUB_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
        DMX_SUB_PRINTF("%s\n",msg);\
        DMX_SUB_PRINTF("~~~~~dmx sub system error~~~~~\n");\
    }while(0)

typedef  uint32_t slot_handle_t;

typedef struct
{
    uint32_t id;//驱动分配的filter的id，其实可以用来当数组下标用的
    dmx_handle_t dmx;
    slot_handle_t slot;
}GxSubsystemFilterCtr;

typedef struct
{
    uint32_t id;//驱动分配的slot的id，其实可以用来当数组下标用的
    dmx_handle_t dmx;
    uint32_t pid;
    //GxSubsystemFilterCtr filter[DMX_SUB_FILTER_NUM];
    GxSubsystemFilterCtr * filter;
    uint32_t count;
}GxSubsystemSlotCtr;

typedef struct
{
    uint8_t name[DMX_SUB_MAX_NAME];//用户给dmx取的名字
    uint32_t dmx_id;
    uint32_t ts;
    handle_t device;//通过驱动打开的设备句柄
    handle_t module;//通过驱动打开的模块句柄
    handle_t mutex;
   // GxSubsystemSlotCtr slot[DMX_SUB_SLOT_NUM];
    GxSubsystemSlotCtr * slot;
    uint32_t count;
} GxSubsystemDmxCtr;

/* Exported Functions ----------------------------------------------------- */
__END_DECLS

#endif
/*@}*/

