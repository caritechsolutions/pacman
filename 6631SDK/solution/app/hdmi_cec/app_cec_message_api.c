#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include <string.h>
#include "app_cec.h"
#include "app_cec_link.h"
#include "app_cec_config.h"
#include "app_cec_private.h"
#include "app_cec_message_tx.h"
#include "app_cec_message_rx.h"
#include "app_cec_common_str.h"
#include "app_cec_message_api.h"

int app_cec_msg_image_view_on(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_IMAGE_VIEW_ON, dest_address, NULL);
}

int app_cec_msg_text_view_on(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_TEXT_VIEW_ON, dest_address, NULL);
}

int app_cec_msg_active_source(void)
{
    bool b_ret = false;
    app_cec_set_source_active_state(CEC_SOURCE_STATE_ACTIVE);
    b_ret = app_cec_update_bus_active_source_info(app_cec_get_device_logical_address(), app_cec_get_device_physical_address());
    if (!b_ret)
    {
        return GXCORE_ERROR;
    }
	// 11.2.1 - 1, Pass Criteria: then broadcasts an <Active Source> message
    return app_cec_send_command_message(CEC_CMD_ACTIVE_SOURCE, CEC_LA_BROADCAST, NULL); // CEC_LA_BROADCAST
}

int app_cec_msg_inactive_source(E_CEC_LOGIC_ADDR dest_address)
{
    app_cec_set_source_active_state(CEC_SOURCE_STATE_IDLE);
    return app_cec_send_command_message(CEC_CMD_INACTIVE_SOURCE, dest_address, NULL);
}

int app_cec_msg_request_active_source(void)
{
    return app_cec_send_command_message(CEC_CMD_REQUEST_ACTIVE_SOURCE, CEC_LA_BROADCAST, NULL);
}

int app_cec_msg_system_standby(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_SYSTEM_STANDBY, dest_address, NULL);
}

int app_cec_msg_cec_version(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_CEC_VERSION, dest_address, NULL);
}

int app_cec_msg_get_cec_version(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_GET_CEC_VERSION, dest_address, NULL);
}

int app_cec_msg_give_physical_address(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_GIVE_PHYSICAL_ADDRESS, dest_address, NULL);
}

int app_cec_msg_report_physical_address(void)
{
    return app_cec_send_command_message(CEC_CMD_REPORT_PHYSICAL_ADDRESS, CEC_LA_BROADCAST, NULL);
}

int app_cec_msg_get_menu_language(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_GET_MENU_LANGUAGE, dest_address, NULL);
}

int app_cec_msg_set_menu_language(E_CEC_LOGIC_ADDR dest_address)
{
    char *iso639_lang_str = NULL;
    CEC_MENU_LANGUAGE stmenulanguage;
    unsigned char t_len = sizeof(stmenulanguage.lang);

    iso639_lang_str = app_cec_convert_menu_lang_str_to_iso639_lang_str();
    memset(&stmenulanguage, 0x0, sizeof(CEC_MENU_LANGUAGE));
    if (!iso639_lang_str)
    {
        return GXCORE_ERROR;
    }
    memcpy(stmenulanguage.lang, iso639_lang_str, t_len);
    return app_cec_send_command_message(CEC_CMD_SET_MENU_LANGUAGE, dest_address, (unsigned char *)&stmenulanguage);
}

int app_cec_msg_polling_message(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_POLLING_MESSAGE, dest_address, NULL);
}

int app_cec_msg_give_tuner_device_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_TUNER_STATUS_REQUEST status_request)
{
    CEC_STATUS_REQUEST ststatusrequest;

    memset(&ststatusrequest, 0x0, sizeof(CEC_STATUS_REQUEST));
    ststatusrequest.request_type = status_request;
    return app_cec_send_command_message(CEC_CMD_GIVE_TUNER_DEVICE_STATUS, dest_address, (unsigned char *)&ststatusrequest);
}

int app_cec_msg_tuner_device_status(E_CEC_LOGIC_ADDR dest_address, CEC_TUNER_DEVICE_INFO *tuner_device_info)
{
    return app_cec_send_command_message(CEC_CMD_TUNER_DEVICE_STATUS, dest_address, (unsigned char *)tuner_device_info);
}

int app_cec_msg_tuner_step_increment(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_TUNER_STEP_INCREMENT, dest_address, NULL);
}

int app_cec_msg_tuner_step_decrement(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_TUNER_STEP_DECREMENT, dest_address, NULL);
}

int app_cec_msg_give_device_vendor_id(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_GIVE_DEVICE_VENDOR_ID, dest_address, NULL);
}

int app_cec_msg_device_vendor_id(void)
{
    return app_cec_send_command_message(CEC_CMD_DEVICE_VENDOR_ID, CEC_LA_BROADCAST, NULL);
}

int app_cec_msg_set_osd_string(E_CEC_LOGIC_ADDR dest_address, unsigned char *osd_string)
{
    unsigned char str_len = 0;
    CEC_OSD_STRING osd_display_featrue;

    memset(&osd_display_featrue, 0, sizeof(CEC_OSD_STRING));
    if (osd_string)
    {
        str_len = strlen((const char *)osd_string);
        if (CEC_OSD_STRING_LEN < str_len)
        {
            str_len = CEC_OSD_STRING_LEN;
        }
        strncpy((char *)osd_display_featrue.string,(const char *) osd_string, str_len);
        return app_cec_send_command_message(CEC_CMD_SET_OSD_STRING, dest_address, (unsigned char *)&osd_display_featrue);
    }
    return GXCORE_SUCCESS;
}

int app_cec_msg_give_osd_name(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_GIVE_OSD_NAME, dest_address, NULL);
}

int app_cec_msg_set_osd_name(E_CEC_LOGIC_ADDR dest_address, unsigned char *osd_name)
{
    unsigned char str_len = 0;
    CEC_OSD_NAME stosdname;

    memset(&stosdname, 0, sizeof(CEC_OSD_NAME));
    if (osd_name)
    {
        str_len = strlen((const char *)osd_name);
        if (CEC_OSD_NAME_LEN < str_len)
        {
            str_len = CEC_OSD_NAME_LEN;
        }
        strncpy((char *)stosdname.name, (const char *)osd_name, str_len);
        return app_cec_send_command_message(CEC_CMD_SET_OSD_NAME, dest_address, (unsigned char *)&stosdname);
    }
    return GXCORE_SUCCESS;
}

int app_cec_msg_menu_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_MENU_STATE menu_state)
{
    if (app_cec_get_source_active_state() == CEC_SOURCE_STATE_ACTIVE)
    {
        app_cec_set_menu_active_state(menu_state);
        return app_cec_send_command_message(CEC_CMD_MENU_STATUS, dest_address, (unsigned char *)&menu_state);
    }
    return GXCORE_SUCCESS;
}

int app_cec_msg_give_device_power_status(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_GIVE_DEVICE_POWER_STATUS, dest_address, NULL);
}

int app_cec_msg_report_power_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_MSG_POW_STAUS pwr_state)
{
    return app_cec_send_command_message(CEC_CMD_REPORT_POWER_STATUS, dest_address, (unsigned char *)&pwr_state);
}

int app_cec_msg_system_audio_mode_request(int bonoff)
{
    return app_cec_send_command_message(CEC_CMD_SYSTEM_AUDIO_MODE_REQUEST, CEC_LA_AUDIO_SYSTEM, (unsigned char *)&bonoff);
}

int app_cec_msg_set_system_audio_mode(E_CEC_LOGIC_ADDR dest_address, int bonoff)
{
    if (bonoff && (dest_address == CEC_LA_BROADCAST))
    {
        app_cec_set_system_audio_mode_status(true);
    }
    else
    {
        app_cec_set_system_audio_mode_status(false);
    }
    return app_cec_send_command_message(CEC_CMD_SET_SYSTEM_AUDIO_MODE, dest_address, (unsigned char *)&bonoff);
}

int app_cec_msg_give_system_audio_mode_status(void)
{
    return app_cec_send_command_message(CEC_CMD_GIVE_SYSTEM_AUDIO_MODE_STATUS, CEC_LA_AUDIO_SYSTEM, NULL);
}

int app_cec_msg_system_audio_mode_status(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_SYSTEM_AUDIO_MODE_STATUS, dest_address, NULL);
}

int app_cec_msg_report_audio_status(E_CEC_LOGIC_ADDR dest_address, unsigned char audio_status)
{
    return app_cec_send_command_message(CEC_CMD_REPORT_AUDIO_STATUS, dest_address, (unsigned char *)&audio_status);
}

int app_cec_msg_user_control_pressed(E_CEC_LOGIC_ADDR dest_address, E_CEC_KEY_CODE cec_key)
{
    return app_cec_send_command_message(CEC_CMD_USER_CONTROL_PRESSED, dest_address, (unsigned char *)&cec_key);
}

int app_cec_msg_user_control_released(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_USER_CONTROL_RELEASED, dest_address, NULL);
}

int app_cec_msg_feature_abort(E_CEC_LOGIC_ADDR dest_address, E_CEC_OPCODE feature_opcode, E_CEC_FEATURE_ABORT_REASON abort_reason)
{
    CEC_FEATURE_ABORT        stfeatureabort;

    memset(&stfeatureabort, 0x0, sizeof(CEC_FEATURE_ABORT));
    stfeatureabort.opcode        =    feature_opcode;
    stfeatureabort.reason        =    abort_reason;
    return app_cec_send_command_message(CEC_CMD_FEATURE_ABORT, dest_address, (unsigned char *)&stfeatureabort);
}

int app_cec_msg_abort(E_CEC_LOGIC_ADDR dest_address)
{
    return app_cec_send_command_message(CEC_CMD_ABORT, dest_address, NULL);
}

int app_cec_msg_record_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_RECORD_STATUS_INFO record_status)
{
    return app_cec_send_command_message(CEC_CMD_RECORD_STATUS, dest_address, (unsigned char *)&record_status);
}

int app_cec_msg_deck_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_DECK_STATUS_INFO deck_status)
{
    return app_cec_send_command_message(CEC_CMD_DECK_STATUS, dest_address, (unsigned char *)&deck_status);
}

int app_cec_msg_timer_status(E_CEC_LOGIC_ADDR dest_address, P_CEC_TIMER_STATUS_DATA timer_status_data)
{
    return app_cec_send_command_message(CEC_CMD_TIMER_STATUS, dest_address, (unsigned char *)&timer_status_data);
}

int app_cec_msg_timer_clear_status(E_CEC_LOGIC_ADDR dest_address, E_CEC_TIMER_CLEAR_STATUS timer_clear_status)
{
    return app_cec_send_command_message(CEC_CMD_TIMER_CLEARED_STATUS, dest_address, (unsigned char *)&timer_clear_status);
}


#endif
