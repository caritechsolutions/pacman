#ifndef __APP_CEC_MESSAGE_H__
#define __APP_CEC_MESSAGE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include <gxtype.h>
#include "app_cec_private.h"
#include "av/hal/gxav_hal_vout.h"

extern void app_cec_message_proc(GxHdmiCecMessage *msg);
extern void app_cec_message_write(P_CEC_CMD cmd);
#ifdef __cplusplus
 }
#endif
#endif
#endif
