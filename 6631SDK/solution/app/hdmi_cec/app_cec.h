#ifndef __APP_CEC_H__
#define __APP_CEC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include <gxtype.h>
#include "app_cec_private.h"

extern int app_cec_send_command_message(E_CEC_AP_CMD_TYPE command_type, E_CEC_LOGIC_ADDR logical_address, unsigned char* param);
extern int app_cec_init(void);
extern int app_cec_enable(int enable);
extern int app_cec_hdmiplug_process(GxMessage* msg);
#ifdef __cplusplus
 }
#endif
#endif
#endif
