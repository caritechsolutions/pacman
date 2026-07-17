#ifndef __APP_CEC_MESSAGE_TX_H__
#define __APP_CEC_MESSAGE_TX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include "app_cec_private.h"

int tx_cec_msg_image_view_on(void);
int tx_cec_msg_text_view_on(void);
int tx_cec_msg_active_source(void);
int tx_cec_msg_request_active_source(void);
int tx_cec_msg_cec_version(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_get_cec_version(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_give_physical_address(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_report_physical_address(void);
int tx_cec_msg_get_menu_language(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_set_menu_language(E_CEC_LOGIC_ADDR dest_address, P_CEC_MENU_LANGUAGE p_menu_lang);
int tx_cec_msg_give_tuner_device_status(E_CEC_LOGIC_ADDR dest_address, P_CEC_STATUS_REQUEST status_request);
int tx_cec_msg_tuner_device_status(unsigned char dest_address, P_CEC_TUNER_DEVICE_INFO tuner_device_info);
int tx_cec_msg_tuner_step_increment(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_tuner_step_decrement(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_give_device_vendor_id(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_device_vendor_id(void);
int tx_cec_msg_set_osd_string(E_CEC_LOGIC_ADDR dest_address, P_CEC_OSD_STRING osd_string);
int tx_cec_msg_give_osd_name(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_set_osd_name(E_CEC_LOGIC_ADDR dest_address, P_CEC_OSD_NAME osd_name);
int tx_cec_msg_menu_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_MENU_STATE menu_state);
int tx_cec_msg_give_device_power_status(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_report_power_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_MSG_POW_STAUS pwr_state);
int tx_cec_msg_feature_abort(E_CEC_LOGIC_ADDR dest_address, E_CEC_OPCODE feature_opcode,
    E_CEC_FEATURE_ABORT_REASON abort_reason);
int tx_cec_msg_abort(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_user_control_pressed(E_CEC_LOGIC_ADDR dest_address, E_CEC_KEY_CODE ui_command);
int tx_cec_msg_user_control_released(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_give_audio_status(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_give_system_audio_mode_status(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_report_audio_status(E_CEC_LOGIC_ADDR dest_address, unsigned char audio_status);
int tx_cec_msg_request_short_audio_descriptor(E_CEC_LOGIC_ADDR dest_address,
    E_CEC_AUDIO_FORMAT_CODE *p_afc_code, unsigned char afc_num);
int tx_cec_msg_report_short_audio_descriptor(E_CEC_LOGIC_ADDR dest_address, unsigned char *p_short_audio_desc, unsigned char short_audio_desc_num);
int tx_cec_msg_system_audio_mode_request(E_CEC_LOGIC_ADDR dest_address, bool b_sac_mode_on);
int tx_cec_msg_system_audio_mode_status(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_inactive_source(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_system_standby(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_polling_message(E_CEC_LOGIC_ADDR dest_address);
int tx_cec_msg_set_system_audio_mode(E_CEC_LOGIC_ADDR dest_address, bool b_on_off);
int tx_cec_msg_record_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_RECORD_STATUS_INFO record_status);
int tx_cec_msg_deck_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_DECK_STATUS_INFO deck_status);
int tx_cec_msg_timer_status(E_CEC_LOGIC_ADDR dest_address, P_CEC_TIMER_STATUS_DATA timer_status_data);
int tx_cec_msg_timer_clear_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_TIMER_CLEAR_STATUS timer_clear_status);
#ifdef __cplusplus
 }
#endif
#endif
#endif
