#ifndef __APP_CEC_COMMON_STR__
#define __APP_CEC_COMMON_STR__

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"
#if HDMI_CEC_SUPPORT
extern char *cec_key_str[]; // cec_key string
extern char *cec_opcode_str[]; // cec message code string
extern char *cdc_opcode_str[]; // cdc message code string
extern char *cec_logical_addr_str[]; // cec logical addr string
extern char *cec_short_audio_desc_str[]; // cec short audio descrition string
#ifdef __cplusplus
 }
#endif
#endif
#endif
