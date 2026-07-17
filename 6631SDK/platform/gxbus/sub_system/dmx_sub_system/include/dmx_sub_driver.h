/** @defgroup pm_module PM Module*/
/*@{*/
#ifndef __DMX_SUB_DRIVER_H__
#define __DMX_SUB_DRIVER_H__

__BEGIN_DECLS

#include "dmx_sub_private.h"
#include "sub_system/dmx_sub_system/dmx_sub_system.h"
/*¥ÌŒÛ÷µ*/

/* Exported Types --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

handle_t gx_dmx_sub_driver_open_device(uint32_t device_id);
handle_t gx_dmx_sub_driver_open_module(uint32_t device,uint32_t  module_id);
int32_t gx_dmx_sub_driver_config(handle_t device,handle_t demux,uint32_t ts);
int32_t gx_dmx_sub_driver_close_module(handle_t device,handle_t module);
int32_t gx_dmx_sub_driver_free_demux(int32_t dmx_id);
int32_t gx_dmx_sub_driver_close_device(handle_t device);
int32_t gx_dmx_sub_driver_alloc_filter(GxSubsystemDmxCtr* dmx,GxSubsystemSlotCtr* slot,GxSubsystemDmxAllocFilter* para);
int32_t gx_dmx_sub_driver_alloc_slot(GxSubsystemDmxCtr* dmx,GxSubsystemDmxAllocFilter* para);
int32_t gx_dmx_sub_driver_start_filter(GxSubsystemFilterCtr* filter);
int32_t gx_dmx_sub_driver_stop_filter(GxSubsystemFilterCtr* filter);
int32_t gx_dmx_sub_driver_release_filter(GxSubsystemFilterCtr* filter);
int32_t gx_dmx_sub_driver_release_slot(GxSubsystemSlotCtr* slot);
uint64_t gx_dmx_sub_driver_wait(GxSubsystemDmxCtr* dmx);
int32_t gx_dmx_sub_driver_read(GxSubsystemFilterCtr* filter,uint8_t* data, uint32_t buf_size);
int32_t gx_dmx_sub_driver_query_status(GxSubsystemDmxCtr* dmx);
int32_t gx_dmx_sub_driver_modify_filter(GxSubsystemFilterCtr* filter,GxSubsystemDmxAllocFilter* para);
__END_DECLS

#endif
/*@}*/

