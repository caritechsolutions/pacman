#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include "app_cec_private.h"
#include "app_cec_message_tx.h"
#include "app_cec_message_rx.h"
#include "app_cec_common_str.h"
#include "app_cec_operation.h"
#include "app_cec_link.h"
#include "app_cec_config.h"
#include "app_cec_message_api.h"
#include "av/hal/gxav_hal_vout.h"
#include "module/app_log.h"

#define CEC_TUNER_ADDR_CNT    4
#define CEC_MAX_LEN 16

int cec_act_physical_address_allocation(void)
{
    unsigned short 	physical_address = 0;

	physical_address = app_cec_get_device_physical_address();
	app_log_debug("Phy Addr is %.1x.%.1x.%.1x.%.1x\n",
        (physical_address&0xF000) >> 12,(physical_address&0x0F00) >> 8, (physical_address&0x00F0) >> 4, physical_address&0x000F);
	if(physical_address == 0xFFFF)
	{
		return GXCORE_ERROR;
    }
    app_cec_set_device_physical_address(physical_address);
    app_log_debug("Acquired CEC Logical Addr(0x%X)\n", app_cec_get_device_logical_address());
    return GXCORE_SUCCESS;
}

int cec_act_logical_address_allocation(void)
{
    unsigned char	i = 0;
    unsigned char   tryindex = 0;
    unsigned char   tunerlogicaladdr[CEC_TUNER_ADDR_CNT] = {0};
    bool    bfound = false;
    GxHdmiCecAddr cec_addr = {0};

	tunerlogicaladdr[0] = CEC_LA_TUNER_1;
    tunerlogicaladdr[1] = CEC_LA_TUNER_2;
    tunerlogicaladdr[2] = CEC_LA_TUNER_3;
    tunerlogicaladdr[3] = CEC_LA_TUNER_4;

	if (GXCORE_SUCCESS == GxVoutHAL_HdmiCecGetAddr(&cec_addr))
	{
		app_log_min("Acquired CEC Logical Addr(0x%X)\n", cec_addr.logical_addr);
	    app_cec_set_device_logical_address(cec_addr.logical_addr);
	}

    switch (app_cec_get_device_logical_address())
    {
        case CEC_LA_TUNER_4:
            tryindex = 3;
            break;

        case CEC_LA_TUNER_3:
            tryindex = 2;
            break;

        case CEC_LA_TUNER_2:
            tryindex = 1;
            break;

        default:
        case CEC_LA_TUNER_1:
            tryindex = 0;
            break;
    }

	// Allocate the logical address. Ref. CEC 10.2.1 (Polling from last Logical Addr)
    for (i = 0; i < CEC_TUNER_ADDR_CNT; i++)
    {
        app_cec_set_device_logical_address(tunerlogicaladdr[((tryindex+i)%CEC_TUNER_ADDR_CNT)]);
        //  set  DRIVE logical address
       // api_set_logical_address(tunerlogicaladdr[((tryindex+i)%CEC_TUNER_ADDR_CNT)]);
        if (GXCORE_SUCCESS == app_cec_msg_polling_message(app_cec_get_device_logical_address()))
        {
            bfound = true;
            break;
        }
    }

	if (bfound)
	{
        cec_feature_one_touch_play();
        app_cec_msg_report_physical_address();
        app_cec_msg_device_vendor_id();
        app_cec_msg_give_system_audio_mode_status();
		return GXCORE_SUCCESS;
	}
	else
	{
        app_log_error("Allocate CEC Logical addr Failed!\n");
        app_cec_set_device_logical_address(CEC_LA_BROADCAST);
        return GXCORE_ERROR;
    }
}

int cec_feature_one_touch_play(void)
{
#if 1
    app_cec_msg_image_view_on(CEC_LA_TV);
#else
    app_cec_msg_text_view_on(CEC_LA_TV);
#endif
    app_cec_msg_active_source();
    app_cec_msg_menu_status(CEC_LA_TV, app_cec_link_get_current_menu_activate_status());
    return GXCORE_SUCCESS;
}

int cec_feature_system_standby_feature_mode(void)
{
    app_cec_set_source_active_state(CEC_SOURCE_STATE_IDLE);
    app_cec_msg_report_power_status(CEC_LA_TV, POW_STANDBY);
    app_cec_msg_system_standby(CEC_LA_BROADCAST);
    app_cec_msg_inactive_source(CEC_LA_TV);
    if (app_cec_get_system_audio_mode_status())
    {
        app_cec_set_system_audio_mode_status(FALSE);
        app_cec_msg_set_system_audio_mode(CEC_LA_BROADCAST, FALSE);
    }
    return GXCORE_SUCCESS;
}

int cec_feature_local_standby_mode(void)
{
    app_cec_set_source_active_state(CEC_SOURCE_STATE_IDLE);
    app_cec_msg_report_power_status(CEC_LA_TV, POW_STANDBY);
    app_cec_msg_inactive_source(CEC_LA_TV);
    if (app_cec_get_system_audio_mode_status())
    {
        app_cec_set_system_audio_mode_status(FALSE);
        app_cec_msg_set_system_audio_mode(CEC_LA_BROADCAST, FALSE);
    }
    return GXCORE_SUCCESS;
}
#endif
