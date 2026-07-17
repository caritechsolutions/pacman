#ifndef __APP_CEC_CONFIG_H__
#define __APP_CEC_CONFIG_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include <gxtype.h>
#include <gxtype.h>
#include "app_cec_private.h"

extern bool app_cec_clear_last_received_feature_abort_info(void);
extern bool app_cec_set_last_received_feature_abort_info(P_CEC_FEATURE_ABORT_INFO p_cec_feature_abort_info);
extern bool app_cec_get_last_received_feature_abort_info(P_CEC_FEATURE_ABORT_INFO p_cec_feature_abort_info);
extern void app_cec_set_remote_passthrough_key(unsigned short remote_cec_key);
extern unsigned short app_cec_get_remote_passthrough_key(void);
extern void app_cec_set_system_audio_mode_status(bool b_flag);
extern bool app_cec_get_system_audio_mode_status(void);
extern void app_cec_set_system_audio_status(unsigned char status);
unsigned char app_cec_get_system_audio_status(void);
extern void app_cec_set_remote_control_passthrough_feature(bool b_flag);
extern bool app_cec_get_remote_control_passthrough_feature(void);
extern void app_cec_set_system_audio_control_feature(bool b_flag);
extern bool app_cec_get_system_audio_control_feature(void);
extern void app_cec_set_func_enable(bool b_flag);
extern bool app_cec_get_func_enable_flag(void);
extern void app_cec_set_osd_popup_enable(bool b_flag);
extern bool app_cec_get_osd_popup_enable_flag(void);
extern void app_cec_set_use_tv_remote_control_enable(bool b_flag);
extern bool app_cec_get_use_tv_remote_control_enable_flag(void);
extern void app_cec_set_tv_on_stb_activity_enable(bool b_flag);
extern bool app_cec_get_tv_on_stb_activity_enable_flag(void);
extern void app_cec_set_stb_standby_tv_ativity_enable(bool b_flag);
extern bool app_cec_get_stb_standby_tv_ativity_enable_flag(void);
extern void app_cec_set_source_active_state(E_CEC_SOURCE_STATE b_flag);
extern E_CEC_SOURCE_STATE app_cec_get_source_active_state(void);
extern void app_cec_set_device_standby_mode(E_CEC_STANDBY_MODE mode);
extern E_CEC_STANDBY_MODE app_cec_get_device_standby_mode(void);
extern bool app_cec_add_tuner_status_subscriber(E_CEC_LOGIC_ADDR subscriber);
extern bool app_cec_remove_tuner_status_subscriber(E_CEC_LOGIC_ADDR subscriber);
extern bool app_cec_get_tuner_status_single_subscriber_status(E_CEC_LOGIC_ADDR subscriber);
extern unsigned short app_cec_get_tuner_status_all_subscriber_status(void);
extern void app_cec_set_device_logical_address(E_CEC_LOGIC_ADDR logical_address);
extern E_CEC_LOGIC_ADDR app_cec_get_device_logical_address(void);
extern void app_cec_set_device_physical_address(unsigned short address);
extern unsigned short app_cec_get_device_physical_address(void);
extern E_CEC_MENU_STATE app_cec_get_menu_active_state(void);
extern void app_cec_set_menu_active_state(E_CEC_MENU_STATE menu_state);
extern void app_cec_update_bus_physical_address_info(E_CEC_LOGIC_ADDR logical_address, unsigned short physical_address);
extern unsigned short app_cec_get_bus_physical_address_info(E_CEC_LOGIC_ADDR logical_address);
extern bool app_cec_update_bus_active_source_info(E_CEC_LOGIC_ADDR logical_address, unsigned short physical_address);
extern void app_cec_set_channel_number_format(E_CHANNEL_NUM_FORAMT ch_num_fmt);
extern E_CHANNEL_NUM_FORAMT app_cec_get_channel_number_format(void);
extern void app_cec_set_service_id_method(E_SERVICE_ID_METHOD service_id_method);
extern E_SERVICE_ID_METHOD app_cec_get_service_id_method(void);
extern bool app_cec_check_retrieved_valid_physical_address(void);
extern void app_cec_set_device_vendor_id(unsigned int vendor_id);
extern unsigned int app_cec_get_device_vendor_id(void);
extern void app_cec_set_active_source_logical_address(E_CEC_LOGIC_ADDR logical_address);
extern E_CEC_LOGIC_ADDR app_cec_get_bus_active_source_logical_address(void);
#ifdef __cplusplus
 }
#endif
#endif
#endif
