#ifndef __APP_CEC_MESSAGE_RX_H__
#define __APP_CEC_MESSAGE_RX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"
#if HDMI_CEC_SUPPORT
void CEC_SWAP8(unsigned char *p);
E_CEC_MSG_ERRNO rx_cec_msg_image_view_on(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_text_view_on(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_active_source(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address,
    unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_request_active_source(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_stream_path(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_inactive_source(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_routing_change(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_routing_information(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_system_standby(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_get_cec_version(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_give_physical_address(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_get_menu_language(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_cec_version(E_CEC_LOGIC_ADDR source_address, \
    E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_report_physical_address(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_menu_language(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_give_tuner_device_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_select_digital_service(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_select_analog_service(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_tuner_step_decrement(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_tuner_step_increment(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_tuner_device_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_device_vendor_id(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_give_device_vendor_id(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_vendor_command(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_vendor_command_with_id(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_vendor_remote_button_down(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_vendor_remote_button_up(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);

E_CEC_MSG_ERRNO rx_cec_msg_set_osd_string(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_give_osd_name(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_osd_name(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_menu_request(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_menu_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_user_control_pressed(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_user_control_released(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_give_power_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_report_power_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_feature_abort(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_abort(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_give_audio_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_give_system_audio_mode_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_report_audio_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_report_short_audio_descriptor(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_request_short_audio_descriptor(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_system_audio_mode(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_system_audio_mode_request(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_system_audio_mode_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_record_off(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_record_on(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_record_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_record_tv_screen(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_clear_analogue_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_clear_digital_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_clear_external_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_analogue_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_digital_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_external_timer(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_timer_program_title(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_timer_cleared_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_timer_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_deck_control(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_deck_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_give_deck_status(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_play(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_set_audio_rate(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_initiate_arc(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_report_arc_initiated(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_report_arc_terminated(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_request_arc_initiation(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_request_arc_termination(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_terminate_arc(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
E_CEC_MSG_ERRNO rx_cec_msg_cdc_message(E_CEC_LOGIC_ADDR source_address, E_CEC_LOGIC_ADDR dest_address, unsigned char *param, unsigned short param_length);
#ifdef __cplusplus
 }
#endif
#endif
#endif
