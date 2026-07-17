#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include "app_cec_private.h"
#include "app_cec_message_tx.h"
#include "app_cec_message_rx.h"
#include "app_cec_common_str.h"
#include "app_cec_link.h"
#include "app_cec_config.h"
#include "app_cec_message.h"
#include "app_cec_message_api.h"
#include "av/hal/gxav_hal_vout.h"
#include "module/app_log.h"

extern int app_cec_module_init(void);
static E_CEC_MSG_ERRNO cec_opcode_abort(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_FEATURE_ABORT:
            t_result = rx_cec_msg_feature_abort(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_ABORT:
            t_result = rx_cec_msg_abort(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_view_on(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_IMAGE_VIEW_ON:
            t_result = rx_cec_msg_image_view_on(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_TEXT_VIEW_ON:
            t_result = rx_cec_msg_text_view_on(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_source(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_ACTIVE_SOURCE:
            t_result = rx_cec_msg_active_source(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REQUEST_ACTIVE_SOURCE:
            t_result = rx_cec_msg_request_active_source(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SET_STREAM_PATH:
            t_result = rx_cec_msg_set_stream_path(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_INACTIVE_SOURCE:
            t_result = rx_cec_msg_inactive_source(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_ROUTING_CHANGE:
            t_result = rx_cec_msg_routing_change(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_ROUTING_INFORMATION:
            t_result = rx_cec_msg_routing_information(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
          return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_system_standby(unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;
    t_result = rx_cec_msg_system_standby(source_address, dest_address, param_ptr, param_length);
    if (CEC_MSG_OK != t_result)
    {
          return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_record(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;
	switch (opcode)
    {
        case OPCODE_RECORD_OFF:
			 t_result = rx_cec_msg_record_off(source_address, dest_address, param_ptr, param_length);
			break;
        case OPCODE_RECORD_ON:
			if ((param_ptr[0] != OWN_SOURCE)&&(param_ptr[0] != DIGITAL_SERVICE))
			{
			    break;
			}
			t_result = rx_cec_msg_record_on(source_address, dest_address, param_ptr, param_length);
			break;
		case OPCODE_RECORD_STATUS:
        case OPCODE_RECORD_TV_SCREEN:
			// 12 - 2, Pass Criteria: The DUT ignores the message.
			break;
		 default:
            t_result = CEC_MSG_OK;
			app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
    }
    return t_result;
}

static void cec_opcode_timer(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_CLEAR_ANALOG_TIMER:
            t_result = rx_cec_msg_clear_analogue_timer(source_address, dest_address, param_ptr, param_length);
            if (CEC_MSG_OK != t_result)
            {
                return ;
            }
            app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
        case OPCODE_CLEAR_DIGITAL_TIMER:
            t_result = rx_cec_msg_clear_digital_timer(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_CLEAR_EXTERNAL_TIMER:
            t_result = rx_cec_msg_clear_external_timer(source_address, dest_address, param_ptr, param_length);
            if (CEC_MSG_OK != t_result)
            {
                return ;
            }
            app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
        case OPCODE_SET_ANALOG_TIMER:
            t_result = rx_cec_msg_set_analogue_timer(source_address, dest_address, param_ptr, param_length);
            if (CEC_MSG_OK != t_result)
            {
                return ;
            }
            app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
        case OPCODE_SET_DIGITAL_TIMER:
            t_result = rx_cec_msg_set_digital_timer(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SET_EXTERNAL_TIMER:
            t_result = rx_cec_msg_set_external_timer(source_address, dest_address, param_ptr, param_length);
            if (CEC_MSG_OK != t_result)
            {
                return ;
            }
            app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
        case OPCODE_SET_TIMER_PROGRAM_TITLE:
            t_result = rx_cec_msg_set_timer_program_title(source_address, dest_address, param_ptr, param_length);
            if (CEC_MSG_OK != t_result)
            {
                return ;
            }
            app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
        case OPCODE_TIMER_CLEARED_STATUS:
            t_result =  rx_cec_msg_timer_cleared_status(source_address, dest_address, param_ptr, param_length);
            if (CEC_MSG_OK != t_result)
            {
                return ;
            }
            app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
        case OPCODE_TIMER_STATUS:
            t_result = rx_cec_msg_timer_status(source_address, dest_address, param_ptr, param_length);
            if (CEC_MSG_OK != t_result)
            {
                return ;
            }
            app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
        default:
            break;
    }
}

static E_CEC_MSG_ERRNO cec_opcode_system_info(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_GET_CEC_VERSION:
            t_result = rx_cec_msg_get_cec_version(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GIVE_PHYSICAL_ADDR:
            t_result = rx_cec_msg_give_physical_address(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GET_MENU_LANGUAGE:
            t_result = rx_cec_msg_get_menu_language(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_CEC_VERSION:
            t_result = rx_cec_msg_cec_version(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REPORT_PHYSICAL_ADDR:
            t_result = rx_cec_msg_report_physical_address(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SET_MENU_LANGUAGE:
            t_result = rx_cec_msg_set_menu_language(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static void cec_opcode_deck_control(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_DECK_CONTROL:
            t_result = rx_cec_msg_deck_control(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_DECK_STATUS:
            t_result = rx_cec_msg_deck_status(source_address, dest_address, param_ptr, param_length);
            if (CEC_MSG_OK != t_result)
            {
                return;
            }
            app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
            break;
        case OPCODE_GIVE_DECK_STATUS:
            t_result = rx_cec_msg_give_deck_status(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_PLAY:
            t_result = rx_cec_msg_play(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            break;
    }
}

static E_CEC_MSG_ERRNO cec_opcode_tuner_control(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch(opcode)
    {
        case OPCODE_GIVE_TUNER_DEVICE_STATUS:
            t_result = rx_cec_msg_give_tuner_device_status(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SELECT_DIGITAL_SERVICE:
            t_result = rx_cec_msg_select_digital_service(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SELECT_ANALOG_SERVICE:
            t_result = rx_cec_msg_select_analog_service(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_TUNER_STEP_DECREMENT:
            t_result = rx_cec_msg_tuner_step_decrement(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_TUNER_STEP_INCREMENT:
            t_result = rx_cec_msg_tuner_step_increment(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_TUNER_DEVICE_STATUS:
            t_result = rx_cec_msg_tuner_device_status(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_vendor(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_DEVICE_VENDOR_ID:
            t_result = rx_cec_msg_device_vendor_id(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GIVE_DEVICE_VENDOR_ID:
            t_result = rx_cec_msg_give_device_vendor_id(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_VENDOR_COMMAND:
            t_result = rx_cec_msg_vendor_command(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_VENDOR_COMMAND_WITH_ID:
            t_result = rx_cec_msg_vendor_command_with_id(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_VENDOR_REOMTE_BUTTON_DOWN:
            t_result = rx_cec_msg_vendor_remote_button_down(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_VENDOR_REOMTE_BUTTON_UP:
            t_result = rx_cec_msg_vendor_remote_button_up(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_osd_display(unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    t_result = rx_cec_msg_set_osd_string(source_address, dest_address, param_ptr, param_length);
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_osd_transfer(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_GIVE_OSD_NAME:
            t_result = rx_cec_msg_give_osd_name(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SET_OSD_NAME:
            t_result = rx_cec_msg_set_osd_name(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_device_menu_control(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_MENU_REQUEST:
            t_result = rx_cec_msg_menu_request(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_MENU_STATUS:
            t_result = rx_cec_msg_menu_status(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_rpc(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_USER_CTRL_PRESSED:
            t_result = rx_cec_msg_user_control_pressed(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_USER_CTRL_RELEASED:
            t_result = rx_cec_msg_user_control_released(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_power_status(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_GIVE_POWER_STATUS:
            t_result = rx_cec_msg_give_power_status(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REPORT_POWER_STATUS:
            t_result = rx_cec_msg_report_power_status(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static E_CEC_MSG_ERRNO cec_opcode_system_audio_control(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO t_result = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_GIVE_AUDIO_STATUS:
            t_result = rx_cec_msg_give_audio_status(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
            t_result = rx_cec_msg_give_system_audio_mode_status(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REPORT_AUDIO_STATUS:
            t_result = rx_cec_msg_report_audio_status(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR:
            t_result = rx_cec_msg_report_short_audio_descriptor(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR:
            t_result = rx_cec_msg_request_short_audio_descriptor(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SET_SYSTEM_AUDIO_MODE:
            t_result = rx_cec_msg_set_system_audio_mode(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
            t_result = rx_cec_msg_system_audio_mode_request(source_address, dest_address, param_ptr, param_length);
            break;

        case OPCODE_SYSTEM_AUDIO_MODE_STATUS:
            t_result = rx_cec_msg_system_audio_mode_status(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            t_result = CEC_MSG_OK;
            break;
    }
    if (CEC_MSG_OK != t_result)
    {
        return t_result;
    }
    return t_result;
}

static void cec_opcode_audio_rate_control(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO ret = CEC_MSG_UNKNOWN;

    ret = rx_cec_msg_set_audio_rate(source_address, dest_address, param_ptr, param_length);
    if (ret != CEC_MSG_OK)
    {
        return;
    }
    app_cec_msg_feature_abort(source_address, opcode, REASON_REFUSED);
}

static void cec_opcode_arcc_audio_return_channel_control(unsigned char opcode, unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO ret = CEC_MSG_UNKNOWN;

    switch (opcode)
    {
        case OPCODE_INITIATE_ARC:
            ret = rx_cec_msg_initiate_arc(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REPORT_ARC_INITIATED:
            ret = rx_cec_msg_report_arc_initiated(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REPORT_ARC_TERMINATED:
            ret = rx_cec_msg_report_arc_terminated(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REQUEST_ARC_INITIATION:
            ret = rx_cec_msg_request_arc_initiation(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_REQUEST_ARC_TERMINATION:
            ret = rx_cec_msg_request_arc_termination(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_TERMINAE_ARC:
            ret = rx_cec_msg_terminate_arc(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            ret = CEC_MSG_OK;
            break;
    }
     if (ret != CEC_MSG_OK)
     {
             return;
     }
}

static void cec_opcode_cdc_message(unsigned char source_address, unsigned char dest_address, unsigned char *param_ptr, unsigned char param_length)
{
    E_CEC_MSG_ERRNO ret = CEC_MSG_UNKNOWN;

    ret = rx_cec_msg_cdc_message(source_address, dest_address, param_ptr, param_length);
    if (ret != CEC_MSG_OK)
    {
        return;
    }
}

void app_cec_message_proc(GxHdmiCecMessage *msg)
{
    unsigned char header = 0;
    unsigned char source_address = 0;
    unsigned char dest_address = 0;
    unsigned char opcode = 0;
    unsigned char param_length = 0;
    unsigned char *param_ptr = NULL;
    bool broadcast_message = false;
    E_CEC_MSG_ERRNO tresult = CEC_MSG_UNKNOWN;
	GxMessage *new_msg = NULL;
	AppMsg_CecUIControl *ui_msg_param = NULL;

    if (NULL == msg)
    {
        return;
    }
    header = msg->header;
    source_address = (header & 0xF0)>>4;
    dest_address = (header & 0x0F);
    broadcast_message = ((header & 0x0F) == 0x0F) ? true : false;
    opcode = msg->message[0];
    param_length = msg->message_len - 2;
    if (param_length)
    {
        param_ptr =&(msg->message[1]);
    }

    if ((!broadcast_message)
	  &&(opcode != OPCODE_SYSTEM_STANDBY) // 11.2.3 - 3, Pass Criteria: The DUT switches to standby.
	  &&(dest_address != app_cec_get_device_logical_address()))
    {
        if (0xf == app_cec_get_device_logical_address())
        {
            app_cec_set_func_enable(false);
	        app_cec_module_init();
        }
		else
		{
            app_log_error("Receive a None-Broadcast message and its destination is NOT us! ");
		}
        return;
    }

    if (1 == msg->message_len)
    {
        return;
    }
    switch (opcode)
    {
        case OPCODE_FEATURE_ABORT:
        case OPCODE_ABORT:
            tresult = cec_opcode_abort(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_IMAGE_VIEW_ON:
        case OPCODE_TEXT_VIEW_ON:
            tresult = cec_opcode_view_on(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_ACTIVE_SOURCE:
        case OPCODE_REQUEST_ACTIVE_SOURCE:
        case OPCODE_SET_STREAM_PATH:
        case OPCODE_INACTIVE_SOURCE:
        case OPCODE_ROUTING_CHANGE:
        case OPCODE_ROUTING_INFORMATION:
            tresult = cec_opcode_source(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SYSTEM_STANDBY:
            tresult = cec_opcode_system_standby(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_RECORD_OFF:
        case OPCODE_RECORD_ON:
        case OPCODE_RECORD_STATUS:
        case OPCODE_RECORD_TV_SCREEN:
            tresult = cec_opcode_record(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_CLEAR_ANALOG_TIMER:
        case OPCODE_CLEAR_DIGITAL_TIMER:
        case OPCODE_CLEAR_EXTERNAL_TIMER:
        case OPCODE_SET_ANALOG_TIMER:
        case OPCODE_SET_DIGITAL_TIMER:
        case OPCODE_SET_EXTERNAL_TIMER:
        case OPCODE_SET_TIMER_PROGRAM_TITLE:
        case OPCODE_TIMER_CLEARED_STATUS:
        case OPCODE_TIMER_STATUS:
            cec_opcode_timer(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GET_CEC_VERSION:
        case OPCODE_GIVE_PHYSICAL_ADDR:
        case OPCODE_GET_MENU_LANGUAGE:
        case OPCODE_CEC_VERSION:
        case OPCODE_REPORT_PHYSICAL_ADDR:
        case OPCODE_SET_MENU_LANGUAGE:
            tresult = cec_opcode_system_info(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_DECK_CONTROL:
        case OPCODE_DECK_STATUS:
        case OPCODE_GIVE_DECK_STATUS:
        case OPCODE_PLAY:
            cec_opcode_deck_control(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GIVE_TUNER_DEVICE_STATUS:
        case OPCODE_SELECT_DIGITAL_SERVICE:
        case OPCODE_SELECT_ANALOG_SERVICE:
        case OPCODE_TUNER_STEP_DECREMENT:
        case OPCODE_TUNER_STEP_INCREMENT:
        case OPCODE_TUNER_DEVICE_STATUS:
            tresult = cec_opcode_tuner_control(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_DEVICE_VENDOR_ID:
        case OPCODE_GIVE_DEVICE_VENDOR_ID:
        case OPCODE_VENDOR_COMMAND:
        case OPCODE_VENDOR_COMMAND_WITH_ID:
        case OPCODE_VENDOR_REOMTE_BUTTON_DOWN:
        case OPCODE_VENDOR_REOMTE_BUTTON_UP:
            tresult = cec_opcode_vendor(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SET_OSD_STRING:
            tresult = cec_opcode_osd_display(source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GIVE_OSD_NAME:
        case OPCODE_SET_OSD_NAME:
            tresult = cec_opcode_osd_transfer(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_MENU_REQUEST:
        case OPCODE_MENU_STATUS:
            tresult = cec_opcode_device_menu_control(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_USER_CTRL_PRESSED:
        case OPCODE_USER_CTRL_RELEASED:
            tresult = cec_opcode_rpc(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GIVE_POWER_STATUS:
        case OPCODE_REPORT_POWER_STATUS:
            tresult = cec_opcode_power_status(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_GIVE_AUDIO_STATUS:
        case OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
        case OPCODE_REPORT_AUDIO_STATUS:
        case OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR:
        case OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR:
        case OPCODE_SET_SYSTEM_AUDIO_MODE:
        case OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
        case OPCODE_SYSTEM_AUDIO_MODE_STATUS:
            tresult = cec_opcode_system_audio_control(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_SET_AUDIO_RATE:
            cec_opcode_audio_rate_control(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_INITIATE_ARC:
        case OPCODE_REPORT_ARC_INITIATED:
        case OPCODE_REPORT_ARC_TERMINATED:
        case OPCODE_REQUEST_ARC_INITIATION:
        case OPCODE_REQUEST_ARC_TERMINATION:
        case OPCODE_TERMINAE_ARC:
            cec_opcode_arcc_audio_return_channel_control(opcode, source_address, dest_address, param_ptr, param_length);
            break;
        case OPCODE_CDC_MESSAGE:
            cec_opcode_cdc_message(source_address, dest_address, param_ptr, param_length);
            break;
        default:
            app_log_error("[From(0x%X): <%s> | New CEC Command is Found, need to update CEC to support this command!\n", source_address, cec_opcode_str[opcode]);
            app_cec_msg_feature_abort(source_address,  opcode, REASON_UNRECOGNIZE_OPCODE);
            break;

    }
    if (app_cec_get_osd_popup_enable_flag())
    {
        switch (opcode)
        {
            case OPCODE_MENU_REQUEST:
            case OPCODE_MENU_STATUS:
            case OPCODE_ABORT:
            case OPCODE_USER_CTRL_PRESSED:
            case OPCODE_USER_CTRL_RELEASED:
			case OPCODE_RECORD_ON:
                break;
            default:
                if (tresult == CEC_MSG_OK)
                {
				    new_msg = GxBus_MessageNew(APPMSG_HDMI_CEC_UI_CONTROL);
				    ui_msg_param = (AppMsg_CecUIControl*)GxBus_GetMsgPropertyPtr(new_msg, AppMsg_CecUIControl);
				    if(ui_msg_param != NULL)
				    {
				        ui_msg_param->type = CEC_UI_SHOW_MESSAGE;
	                    ui_msg_param->opcode_handler.opcode = opcode;
						ui_msg_param->opcode_handler.param_length = param_length;
						ui_msg_param->opcode_handler.dest_address = dest_address;
						ui_msg_param->opcode_handler.source_address = source_address;
						memcpy(ui_msg_param->opcode_handler.param, param_ptr, param_length);
				        GxBus_MessageSend(new_msg);
				    }
                }
                break;
        }
    }
}

void app_cec_message_write(P_CEC_CMD cmd)
{
    void *ptr = NULL;
    if (NULL == cmd)
    {
        return;
    }
    ptr = (void*)cmd->params;
    switch (cmd->type)
    {
        case CEC_CMD_IMAGE_VIEW_ON:
            tx_cec_msg_image_view_on();
            break;
        case CEC_CMD_TEXT_VIEW_ON:
            tx_cec_msg_text_view_on();
            break;
        case CEC_CMD_ACTIVE_SOURCE:
            tx_cec_msg_active_source();
            break;
        case CEC_CMD_INACTIVE_SOURCE:
            tx_cec_msg_inactive_source(cmd->dest_addr);
            break;
        case CEC_CMD_REQUEST_ACTIVE_SOURCE:
            tx_cec_msg_request_active_source();
            break;
        case CEC_CMD_SYSTEM_STANDBY:
			app_log_error("\n");
            tx_cec_msg_system_standby(cmd->dest_addr);
            break;
        case CEC_CMD_GET_CEC_VERSION:
            tx_cec_msg_get_cec_version(cmd->dest_addr);
            break;
        case CEC_CMD_GIVE_PHYSICAL_ADDRESS:
            tx_cec_msg_give_physical_address(cmd->dest_addr);
            break;
        case CEC_CMD_GET_MENU_LANGUAGE:
            tx_cec_msg_get_menu_language(cmd->dest_addr);
            break;
        case CEC_CMD_SET_MENU_LANGUAGE:
            tx_cec_msg_set_menu_language(cmd->dest_addr, (P_CEC_MENU_LANGUAGE)cmd->params);
            break;
        case CEC_CMD_GIVE_DEVICE_VENDOR_ID:
            tx_cec_msg_give_device_vendor_id(cmd->dest_addr);
            break;
        case CEC_CMD_SET_OSD_STRING:
            tx_cec_msg_set_osd_string(CEC_LA_TV, (P_CEC_OSD_STRING)cmd->params);
            break;
        case CEC_CMD_GIVE_OSD_NAME:
            tx_cec_msg_give_osd_name(cmd->dest_addr);
            break;
        case CEC_CMD_MENU_STATUS:
            tx_cec_msg_menu_status(cmd->dest_addr, cmd->params[0]);
            break;
        case CEC_CMD_GIVE_DEVICE_POWER_STATUS:
            tx_cec_msg_give_device_power_status(cmd->dest_addr);
            break;
        case CEC_CMD_ABORT:
            tx_cec_msg_abort(cmd->dest_addr);
            break;
        case CEC_CMD_CEC_VERSION:
            tx_cec_msg_cec_version(cmd->dest_addr);
            break;
        case CEC_CMD_REPORT_PHYSICAL_ADDRESS:
            tx_cec_msg_report_physical_address();
            break;
        case CEC_CMD_POLLING_MESSAGE:
            tx_cec_msg_polling_message(cmd->dest_addr);
            break;
        case CEC_CMD_GIVE_TUNER_DEVICE_STATUS:
            tx_cec_msg_give_tuner_device_status(cmd->dest_addr, (P_CEC_STATUS_REQUEST)cmd->params);
            break;
        case CEC_CMD_TUNER_DEVICE_STATUS:
            tx_cec_msg_tuner_device_status(cmd->dest_addr, (P_CEC_TUNER_DEVICE_INFO)cmd->params);
            break;
        case CEC_CMD_TUNER_STEP_INCREMENT:
            tx_cec_msg_tuner_step_increment(cmd->dest_addr);
            break;
        case CEC_CMD_TUNER_STEP_DECREMENT:
            tx_cec_msg_tuner_step_decrement(cmd->dest_addr);
            break;
        case CEC_CMD_DEVICE_VENDOR_ID:
            tx_cec_msg_device_vendor_id();
            break;
        case CEC_CMD_SET_OSD_NAME:
            tx_cec_msg_set_osd_name(cmd->dest_addr, (P_CEC_OSD_NAME)cmd->params);
            break;
        case CEC_CMD_REPORT_POWER_STATUS:
            tx_cec_msg_report_power_status(cmd->dest_addr, cmd->params[0]);
            break;
        case CEC_CMD_FEATURE_ABORT:
            tx_cec_msg_feature_abort(cmd->dest_addr, ((P_CEC_FEATURE_ABORT)ptr)->opcode,
               ((P_CEC_FEATURE_ABORT)ptr)->reason);
            break;
        case CEC_CMD_SYSTEM_AUDIO_MODE_REQUEST:
            tx_cec_msg_system_audio_mode_request(cmd->dest_addr, cmd->params[0]);
            break;
        case CEC_CMD_GIVE_SYSTEM_AUDIO_MODE_STATUS:
            tx_cec_msg_give_system_audio_mode_status(cmd->dest_addr);
            break;
        case CEC_CMD_USER_CONTROL_PRESSED:
            tx_cec_msg_user_control_pressed(cmd->dest_addr, cmd->params[0]);
            break;
        case CEC_CMD_USER_CONTROL_RELEASED:
            tx_cec_msg_user_control_released(cmd->dest_addr);
            break;
        case CEC_CMD_SYSTEM_AUDIO_MODE_STATUS:
            tx_cec_msg_system_audio_mode_status(cmd->dest_addr);
            break;
        case CEC_CMD_REPORT_AUDIO_STATUS:
            tx_cec_msg_report_audio_status(cmd->dest_addr, cmd->params[0]);
            break;
        case CEC_CMD_SET_SYSTEM_AUDIO_MODE:
            tx_cec_msg_set_system_audio_mode(cmd->dest_addr, cmd->params[0]);
            break;
		case CEC_CMD_RECORD_STATUS:
			tx_cec_msg_record_status(cmd->dest_addr, cmd->params[0]);
			break;
		case CEC_CMD_DECK_STATUS:
			tx_cec_msg_deck_status(cmd->dest_addr, cmd->params[0]);
			break;
		case CEC_CMD_TIMER_STATUS:
			tx_cec_msg_timer_status(cmd->dest_addr, (P_CEC_TIMER_STATUS_DATA)cmd->params);
			break;
		case CEC_CMD_TIMER_CLEARED_STATUS:
			tx_cec_msg_timer_clear_status(cmd->dest_addr, cmd->params[0]);
			break;
        default:
            app_log_error("Unknown CEC_CMD_TYPE(0x%02X) from UI is Found, need to update LIB_CEC to support this command!\n", cmd->type);
            break;
    }
}
#endif
