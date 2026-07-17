#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include "app_cec_private.h"
#include "app_cec_config.h"
#include "av/hal/gxav_hal_vout.h"
#include "module/app_log.h"

static CEC_CONFIG cec_config;

bool app_cec_clear_last_received_feature_abort_info(void)
{
    memset(&cec_config.cec_feature_abort_info, 0, sizeof(CEC_FEATURE_ABORT_INFO));
    return true;
}

bool app_cec_set_last_received_feature_abort_info(P_CEC_FEATURE_ABORT_INFO p_cec_feature_abort_info)
{
    if (p_cec_feature_abort_info)
    {
        memcpy(&cec_config.cec_feature_abort_info, p_cec_feature_abort_info, sizeof(CEC_FEATURE_ABORT_INFO));
        return true;
    }
    else
    {
        return false;
    }
}

bool app_cec_get_last_received_feature_abort_info(P_CEC_FEATURE_ABORT_INFO p_cec_feature_abort_info)
{
    CEC_FEATURE_ABORT_INFO temp_cec_feature_abort_info;

    memset(&temp_cec_feature_abort_info, 0, sizeof(CEC_FEATURE_ABORT_INFO));
    if (p_cec_feature_abort_info)
    {
        if (0 == memcmp(&cec_config.cec_feature_abort_info, &temp_cec_feature_abort_info, sizeof(CEC_FEATURE_ABORT_INFO)))
        {
            //last received cec feature abort info is NOT exist
            return false;
        }
        else
        {

            memcpy(p_cec_feature_abort_info, &cec_config.cec_feature_abort_info, sizeof(CEC_FEATURE_ABORT_INFO));
            return true;
        }
    }
    else
    {
        return false;
    }
}

void app_cec_set_remote_passthrough_key(unsigned short remote_cec_key)
{
    cec_config.remote_passthrough_key= remote_cec_key;
}

unsigned short app_cec_get_remote_passthrough_key(void)
{
    return cec_config.remote_passthrough_key;
}

void app_cec_set_system_audio_mode_status(bool bflag)
{
    cec_config.system_audio_mode_status = bflag;
}

bool app_cec_get_system_audio_mode_status(void)
{
    return cec_config.system_audio_mode_status;
}

void app_cec_set_system_audio_status(unsigned char status)
{
    cec_config.system_audio_status= status;
}

unsigned char app_cec_get_system_audio_status(void)
{
    return cec_config.system_audio_status;
}

void app_cec_set_remote_control_passthrough_feature(bool bflag)
{
    cec_config.cec_feature_rcp_enable = bflag;
}

bool app_cec_get_remote_control_passthrough_feature(void)
{
    return cec_config.cec_feature_rcp_enable;
}

void app_cec_set_system_audio_control_feature(bool bflag)
{
    cec_config.cec_feature_sac_enable = bflag;
}

bool app_cec_get_system_audio_control_feature(void)
{
    return cec_config.cec_feature_sac_enable;
}

void app_cec_set_func_enable(bool bflag)
{
    app_log_debug("%s\n", (bflag) ? "On" : "Off");
    cec_config.cec_func_enable = bflag;
    GxVoutHAL_HdmiSetCecEnable(bflag);
}

bool app_cec_get_func_enable_flag(void)
{
    return cec_config.cec_func_enable;
}

void app_cec_set_osd_popup_enable(bool bflag)
{
    cec_config.cec_osd_popup_enable = bflag;
}

bool app_cec_get_osd_popup_enable_flag(void)
{
    return cec_config.cec_osd_popup_enable;
}

void app_cec_set_use_tv_remote_control_enable(bool bflag)
{
    cec_config.cec_use_tv_remote_control_enable = bflag;
}

bool app_cec_get_use_tv_remote_control_enable_flag(void)
{
    return cec_config.cec_use_tv_remote_control_enable;
}

void app_cec_set_tv_on_stb_activity_enable(bool bflag)
{
    cec_config.cec_tv_on_stb_activity_enable = bflag;
}

bool app_cec_get_tv_on_stb_activity_enable_flag(void)
{
    return cec_config.cec_tv_on_stb_activity_enable;
}

void app_cec_set_stb_standby_tv_ativity_enable(bool bflag)
{
    cec_config.cec_stb_standby_tv_ativity_enable = bflag;
}

bool app_cec_get_stb_standby_tv_ativity_enable_flag(void)
{
    return cec_config.cec_stb_standby_tv_ativity_enable;
}

void app_cec_set_source_active_state(E_CEC_SOURCE_STATE bflag)
{
    cec_config.source_active_state = bflag;
}

E_CEC_SOURCE_STATE app_cec_get_source_active_state(void)
{
    return cec_config.source_active_state;
}

void app_cec_set_device_standby_mode(E_CEC_STANDBY_MODE mode)
{
    cec_config.cec_device_standby_mode= mode;
}

E_CEC_STANDBY_MODE app_cec_get_device_standby_mode(void)
{
    return cec_config.cec_device_standby_mode;
}

bool app_cec_add_tuner_status_subscriber(E_CEC_LOGIC_ADDR subscriber)
{
    cec_config.tuner_status_subscribers |= 1<<subscriber;
    return true;
}

bool app_cec_remove_tuner_status_subscriber(E_CEC_LOGIC_ADDR subscriber)
{
    cec_config.tuner_status_subscribers &= ~(1<<subscriber);
    return true;
}

bool app_cec_get_tuner_status_single_subscriber_status(E_CEC_LOGIC_ADDR subscriber)
{
    return (cec_config.tuner_status_subscribers&(1<<subscriber));
}

unsigned short app_cec_get_tuner_status_all_subscriber_status(void)
{
    return cec_config.tuner_status_subscribers;
}

void app_cec_set_device_logical_address(E_CEC_LOGIC_ADDR logical_address)
{
    cec_config.device_logical_address = logical_address;
}

E_CEC_LOGIC_ADDR app_cec_get_device_logical_address(void)
{
    return cec_config.device_logical_address;
}

void app_cec_set_device_physical_address(unsigned short address)
{
    cec_config.device_physical_address = address;
#if 0
    GxHdmiEdidInfo edid = {0};

    if (GXCORE_SUCCESS == GxVoutHAL_HdmiGetEdid(&edid))
    {
        cec_config.device_physical_address = address;
    }
    else
    {
        // PHILIP50PUF6061 will plug out HDMI when TV power off, STB can NOT get phy addr to process CEC msg, Bug #121234
        cec_config.device_physical_address = FAKE_CEC_PHYSICAL_ADDRESS;
    }
#endif
}

unsigned short app_cec_get_device_physical_address(void)
{
    return cec_config.device_physical_address;
}

E_CEC_MENU_STATE app_cec_get_menu_active_state(void)
{
    return cec_config.menu_active_state;
}

void app_cec_set_menu_active_state(E_CEC_MENU_STATE menu_state)
{
    cec_config.menu_active_state = menu_state;
}

void app_cec_update_bus_physical_address_info(E_CEC_LOGIC_ADDR logical_address, unsigned short physical_address)
{
    cec_config.bus_physical_address[logical_address] = physical_address;
}

unsigned short app_cec_get_bus_physical_address_info(E_CEC_LOGIC_ADDR logical_address)
{
    return cec_config.bus_physical_address[logical_address];
}

bool app_cec_update_bus_active_source_info(E_CEC_LOGIC_ADDR logical_address, unsigned short physical_address)
{
    cec_config.bus_active_source_logical_address = logical_address;
    cec_config.bus_physical_address[logical_address] = physical_address;
    return true;
}

void app_cec_set_channel_number_format(E_CHANNEL_NUM_FORAMT ch_num_fmt)
{
    cec_config.channel_num_format= ch_num_fmt;
}

E_CHANNEL_NUM_FORAMT app_cec_get_channel_number_format(void)
{
    return cec_config.channel_num_format;
}

void app_cec_set_service_id_method(E_SERVICE_ID_METHOD service_id_method)
{
    cec_config.service_id_method= service_id_method;
}

E_SERVICE_ID_METHOD app_cec_get_service_id_method(void)
{
    return cec_config.service_id_method;
}

bool app_cec_check_retrieved_valid_physical_address(void)
{
    return ((cec_config.device_physical_address != INVALID_CEC_PHYSICAL_ADDRESS) ? 1 : 0);
}

void app_cec_set_device_vendor_id(unsigned int vendor_id)
{
    cec_config.device_vendor_id = vendor_id&0xFFFFFF;
}

unsigned int app_cec_get_device_vendor_id(void)
{
    return cec_config.device_vendor_id;
}

void app_cec_set_active_source_logical_address(E_CEC_LOGIC_ADDR logical_address)
{
    cec_config.bus_active_source_logical_address = logical_address;
}

E_CEC_LOGIC_ADDR app_cec_get_bus_active_source_logical_address(void)
{
    return cec_config.bus_active_source_logical_address;
}
#endif