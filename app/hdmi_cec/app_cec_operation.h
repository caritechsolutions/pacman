#ifndef __APP_CEC_OPERATION_H__
#define __APP_CEC_OPERATION_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"
#include <gxtype.h>

#if HDMI_CEC_SUPPORT
extern int cec_act_physical_address_allocation(void);
extern int cec_act_logical_address_allocation(void);
extern int cec_feature_one_touch_play(void);
extern int cec_feature_system_standby_feature_mode(void);
extern int cec_feature_local_standby_mode(void);
#ifdef __cplusplus
 }
#endif
#endif
#endif
