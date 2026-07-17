#include "app.h"
#if HDMI_CEC_SUPPORT
#include "app_cec.h"
#include "app_cec_config.h"
#include "app_cec_private.h"
#include "app_cec_message.h"
#include "app_cec_operation.h"
#include "app_cec_message_rx.h"
#include "app_cec_message_tx.h"
#include <common/gxpm.h>
#include "av/hal/gxav_hal_hdmi.h"

#define CEC_MBF_SIZE (8)
#define PRINT(...) do { printf(__VA_ARGS__); } while(0)

static int g_hdmi_cec_device = 0;
static int g_hdmi_cec_module = 0;

// Test ID 9.4-2, Test ID 9.6-1, Test ID 9.7-2: need to create a ring buffer for receiving msg from driver
static volatile unsigned char cec_msg_buf_cnt = 0;
static volatile unsigned char cec_header_buf[CEC_MBF_SIZE] = {0};
static volatile unsigned char cec_msg_buf[CEC_MBF_SIZE][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

int app_hdmi_cec_msg_ring_buffer(GxHdmiCecMessage *msg)
{
    GxMessage *new_msg = NULL;
    GxHdmiCecMessage* param = NULL;

    if (NULL == msg)
    {
        return -1;
    }
    // Test ID 9.4-2, Test ID 9.6-1, Test ID 9.7-2: need to create a ring buffer for receiving msg from driver    
	cec_header_buf[cec_msg_buf_cnt] = msg->header;
	memcpy((unsigned char *)(cec_msg_buf[cec_msg_buf_cnt]), msg->message, (msg->message_len - 1));

    new_msg = GxBus_MessageNew(GXMSG_HDMI_CEC_READ_DONE);
    param = (GxHdmiCecMessage*)GxBus_GetMsgPropertyPtr(new_msg, GxHdmiCecMessage);
    if(param != NULL)
    {
        param->header = cec_header_buf[cec_msg_buf_cnt];
		param->message_len = msg->message_len;
		memcpy(param->message, (unsigned char *)cec_msg_buf[cec_msg_buf_cnt], (msg->message_len - 1));
        GxBus_MessageSend(new_msg);
    }
    cec_msg_buf_cnt++;
    cec_msg_buf_cnt = cec_msg_buf_cnt % CEC_MBF_SIZE;
	return 0;
}

int app_cec_send_command_message(E_CEC_AP_CMD_TYPE command_type, E_CEC_LOGIC_ADDR logical_address, unsigned char *param)
{
    CEC_CMD command = {0};
	GxMessage *new_msg = NULL;
    P_CEC_CMD cec_param = NULL;

    memset(&command, 0, sizeof(CEC_CMD));
    command.type = command_type;
    command.dest_addr = logical_address;
    if (app_cec_check_retrieved_valid_physical_address() == false)
    {
        app_log_error("CEC Doesn't have a valid physical address! (drop CEC_CMD)\n");
        return GXCORE_ERROR;
    }
    switch (command_type)
    {
        case CEC_CMD_IMAGE_VIEW_ON:
            break;
        case CEC_CMD_TEXT_VIEW_ON:
            break;
        case CEC_CMD_ACTIVE_SOURCE:
            break;
        case CEC_CMD_INACTIVE_SOURCE:
            break;
        case CEC_CMD_REQUEST_ACTIVE_SOURCE:
            break;
        case CEC_CMD_SYSTEM_STANDBY:
            break;
        case CEC_CMD_CEC_VERSION:
            break;
        case CEC_CMD_GET_CEC_VERSION:
            break;
        case CEC_CMD_GIVE_PHYSICAL_ADDRESS:
            break;
        case CEC_CMD_REPORT_PHYSICAL_ADDRESS:
            break;
        case CEC_CMD_GET_MENU_LANGUAGE:
            break;
        case CEC_CMD_SET_MENU_LANGUAGE:
            if (param)
            {
                memcpy(command.params, param, sizeof(CEC_MENU_LANGUAGE));
            }
            break;
        case CEC_CMD_POLLING_MESSAGE:
            break;
        case CEC_CMD_GIVE_TUNER_DEVICE_STATUS:
            if (param)
            {
                memcpy(command.params, param, sizeof(CEC_STATUS_REQUEST));
            }
            break;
        case CEC_CMD_TUNER_STEP_INCREMENT:
            break;
        case CEC_CMD_TUNER_STEP_DECREMENT:
            break;
        case CEC_CMD_TUNER_DEVICE_STATUS:
            if (param)
            {
                memcpy(command.params, param, sizeof(CEC_TUNER_DEVICE_INFO));
            }
            break;
        case CEC_CMD_GIVE_DEVICE_VENDOR_ID:
            break;
        case CEC_CMD_DEVICE_VENDOR_ID:
            break;
        case CEC_CMD_SET_OSD_STRING:
            if (param)
            {
                memcpy(command.params, param, sizeof(CEC_OSD_STRING));
            }
            break;
        case CEC_CMD_GIVE_OSD_NAME:
            break;
        case CEC_CMD_SET_OSD_NAME:
            if (param)
            {
                memcpy(command.params, param, sizeof(CEC_OSD_NAME));
            }
            break;
        case CEC_CMD_MENU_STATUS:
            if (param)
            {
                command.params[0] = param[0];
            }
            break;
        case CEC_CMD_GIVE_DEVICE_POWER_STATUS:
            break;
        case CEC_CMD_REPORT_POWER_STATUS:
            if (param)
            {
                command.params[0] = param[0];
            }
            break;
        case CEC_CMD_FEATURE_ABORT:
            if (param)
            {
                memcpy(command.params, param, sizeof(CEC_FEATURE_ABORT));
            }
            break;
        case CEC_CMD_ABORT:
            break;
        case CEC_CMD_SYSTEM_AUDIO_MODE_REQUEST:
            if (param)
            {
                command.params[0] = param[0];
            }
            break;
        case CEC_CMD_GIVE_SYSTEM_AUDIO_MODE_STATUS:
            break;
        case CEC_CMD_USER_CONTROL_PRESSED:
            if (param)
            {
                command.params[0] = param[0];
            }
            break;
        case CEC_CMD_USER_CONTROL_RELEASED:
            break;
        case CEC_CMD_SYSTEM_AUDIO_MODE_STATUS:
            break;
        case CEC_CMD_SET_SYSTEM_AUDIO_MODE:
            if (param)
            {
                command.params[0] = param[0];
            }
            break;
        case CEC_CMD_REPORT_AUDIO_STATUS:
            if (param)
            {
                command.params[0] = param[0];
            }
            break;
		case CEC_CMD_RECORD_STATUS:
			if (param)
            {
                command.params[0] = param[0];
            }
			break;
		case CEC_CMD_DECK_STATUS:
			if (param)
            {
                command.params[0] = param[0];
            }
			break;
		case CEC_CMD_TIMER_STATUS:
            if (param)
            {
                memcpy(command.params, param, sizeof(CEC_TIMER_STATUS_DATA));
            }
		case CEC_CMD_TIMER_CLEARED_STATUS:
			if (param)
            {
                command.params[0] = param[0];
            }
            break;
        default:
            app_log_error("CEC Command(0x%02X) Not Supported!\n", command_type);
            return GXCORE_ERROR;
    }
    new_msg = GxBus_MessageNew(GXMSG_HDMI_CEC_WRITE);
    cec_param = (P_CEC_CMD)GxBus_GetMsgPropertyPtr(new_msg, CEC_CMD);
    if(cec_param != NULL)
    {
        cec_param->type = command.type;
        cec_param->dest_addr = command.dest_addr;
		memcpy(cec_param->params, &command.params, sizeof(command.params));
        cec_param->params[0] = command.params[0];
        GxBus_MessageSend(new_msg);
    }
    return GXCORE_SUCCESS;
}

static unsigned short app_cec_get_physical_address(void)
{
	GxHdmiCecAddr cec_addr = {0};
	int i = 0;

	do{
		if (GXCORE_SUCCESS == GxVoutHAL_HdmiCecGetAddr(&cec_addr))
		{
		    app_log_debug("Acquired CEC Physical Addr(0x%X)\n", cec_addr.physical_addr);
            if ((cec_addr.logical_addr == CEC_LA_TUNER_1)
			  ||(cec_addr.logical_addr == CEC_LA_TUNER_2)
			  ||(cec_addr.logical_addr == CEC_LA_TUNER_3)
			  ||(cec_addr.logical_addr == CEC_LA_TUNER_4))
            {
			    return cec_addr.physical_addr;
            }
		}
		//app_log_min("Acquired CEC Physical Addr(0x%X)\n", cec_addr.physical_addr);
		//app_log_min("Acquired CEC logical_addr(0x%X) i = %d \n", cec_addr.logical_addr, i);
		i++;
		GxCore_ThreadDelay(200);
	}while((cec_addr.logical_addr != CEC_LA_TUNER_1)&&(i < 30));

	return INVALID_CEC_PHYSICAL_ADDRESS;
}

int app_cec_enter_standby_mode(void)
{
	app_cec_set_source_active_state(CEC_SOURCE_STATE_IDLE);
	tx_cec_msg_report_power_status(CEC_LA_TV, POW_STANDBY);
	/*
	* 11.2.3 - 1, Pass Criteria: The DUT shall broadcast a <Standby> message, before going into standby itself.
    */
	if (app_cec_get_stb_standby_tv_ativity_enable_flag())
	{
	    tx_cec_msg_system_standby(CEC_LA_BROADCAST);
	}
	tx_cec_msg_inactive_source(CEC_LA_TV);
	if (app_cec_get_system_audio_mode_status())
	{
		app_cec_set_system_audio_mode_status(false);
		tx_cec_msg_set_system_audio_mode(CEC_LA_BROADCAST, false);
	}
	return GXCORE_SUCCESS;
}

static int app_cec_set_vendor_ieee_oui(void)
{
	int ret = 0;
	unsigned long tick = 0;
	GxHdmiEdidInfo edid_info = {0};

	tick = GxCore_TickStart(2200);
	do{
		memset(&edid_info, 0, sizeof(GxHdmiEdidInfo));
		if (GXCORE_SUCCESS == GxVoutHAL_HdmiGetEdid(&edid_info))
		{
			if ((edid_info.data[8] != 0x00)||(edid_info.data[9] != 0x00))
			{
				break;
			}
		}
		GxCore_ThreadDelay(20);
	}while(!GxCore_TickEnd(tick)&&(edid_info.data[8] == 0x00)&&(edid_info.data[9] == 0x00));

	//PRINT("\n 0x%02x  0x%02x \n", edid_data.data[8], edid_data.data[9]);
	if ((edid_info.data[8] == 0x2e)&&(edid_info.data[9] == 0x83))
	{
		app_cec_set_device_vendor_id(IEEE_OUI_NEXT);
	}
	else if ((edid_info.data[8] == 0x41)&&(edid_info.data[9] == 0x0c))
	{
		app_cec_set_device_vendor_id(IEEE_OUI_PHILIPS);
	}
	else
	{
		app_cec_set_device_vendor_id(STB_VENDOR_ID);
	}
	GxCore_ThreadDelay(22);
	return ret;
}


int app_cec_module_init(void)
{
    unsigned short i = 0;
	int32_t cec_config_value = 0;

    app_cec_set_device_logical_address(CEC_LA_BROADCAST);
    app_cec_set_device_physical_address(INVALID_CEC_PHYSICAL_ADDRESS);
    app_cec_set_device_vendor_id(STB_VENDOR_ID);
    app_cec_set_service_id_method(SERVICE_BY_DIGITAL_ID);
    app_cec_set_channel_number_format(CHANNEL_NUM_1_PART);
    app_cec_set_source_active_state(CEC_SOURCE_STATE_IDLE);
    app_cec_set_menu_active_state(CEC_MENU_STATE_UNINITED);
    for (i = CEC_LA_TV; i < CEC_LA_BROADCAST; i++)
    {
        app_cec_update_bus_physical_address_info(i, INVALID_CEC_PHYSICAL_ADDRESS);
    }
	app_cec_set_vendor_ieee_oui();
    app_cec_set_remote_control_passthrough_feature(true);
    app_cec_set_system_audio_control_feature(true);
    app_cec_set_func_enable(true);
    app_cec_set_osd_popup_enable(true);

	GxBus_ConfigGetInt(CEC_USE_TV_REMOTE_CONTROL_KEY, &cec_config_value, CEC_USE_TV_REMOTE_CONTROL_VALUE);
	app_cec_set_use_tv_remote_control_enable(cec_config_value);
	GxBus_ConfigGetInt(CEC_TV_ON_STB_ACTIVITY_KEY, &cec_config_value, CEC_TV_ON_STB_ACTIVITY_VALUE);
    app_cec_set_tv_on_stb_activity_enable(cec_config_value);
	GxBus_ConfigGetInt(CEC_STB_STANDBY_TV_ACTIVITY_KEY, &cec_config_value, CEC_STB_STANDBY_TV_ACTIVITY_VALUE);
	app_cec_set_stb_standby_tv_ativity_enable(cec_config_value);
	GxBus_ConfigGetInt(CEC_SYSTEM_AUDIO_KEY, &cec_config_value, CEC_SYSTEM_AUDIO_VALUE);
	app_cec_set_system_audio_mode_status(cec_config_value);

    app_cec_set_device_standby_mode(CEC_LOCAL_STANDBY_MODE);
	app_cec_set_device_physical_address(app_cec_get_physical_address());
    if (app_cec_check_retrieved_valid_physical_address())
    {
        cec_act_logical_address_allocation();
    }
    return GXCORE_SUCCESS;
}


int app_cec_init(void)
{
	 int cec_value = 0;

	 app_cec_module_init();
	 GxBus_ConfigGetInt(CEC_CONF_KEY, &cec_value, CEC_CONFIG_VALUE);
	 if(cec_value == 0)
	 {
		 app_cec_enable(0);
	 }
	 return 0;
}

int app_cec_enable(int enable)
{
    GxMessage *new_msg = NULL;
    int msgid = GXMSG_HDMI_CEC_ON;

    if (enable)
    {
        msgid = GXMSG_HDMI_CEC_ON;
    }
    else
    {
        msgid = GXMSG_HDMI_CEC_OFF;
    }
    new_msg = GxBus_MessageNew(msgid);
    if (NULL == new_msg)
    {
        return 0;
    }
    GxBus_MessageSend(new_msg);
    return 0;
}

static int app_cec_tv_ieee_oui_process(void)
{
	unsigned int vendor_id = app_cec_get_device_vendor_id();

    switch (vendor_id)
	{
	    case IEEE_OUI_PHILIPS:
			app_cec_set_func_enable(true);
			cec_act_logical_address_allocation();
			break;
	    case IEEE_OUI_NEXT:
			app_cec_module_init();
			break;
		case IEEE_OUI_SONY:
		case IEEE_OUI_MEI:
		case IEEE_OUI_LG:
		case IEEE_OUI_SAMSUNG:
		case IEEE_OUI_MSTAR:
		default:
			app_cec_set_func_enable(true);
			break;
	}
    return 0;
}

int app_cec_hdmiplug_process(GxMessage* msg)
{
	int cec_value = 0;
	unsigned int vendor_id = 0;
	unsigned int vendor_id_temp = 0;
	unsigned short physical_address = 0;

    if (msg == NULL)
    {
        return -1;
    }

	GxBus_ConfigGetInt(CEC_CONF_KEY, &cec_value, CEC_CONFIG_VALUE);
	if((cec_value == 1)&&(msg->msg_id == GXMSG_HDMI_HOTPLUG_IN))
	{
	    vendor_id = app_cec_get_device_vendor_id();
		physical_address = app_cec_get_device_physical_address();
	    app_cec_set_vendor_ieee_oui();
		vendor_id_temp = app_cec_get_device_vendor_id();
		if (vendor_id != vendor_id_temp)
		{
		    vendor_id = vendor_id_temp;
		}

		switch (vendor_id)
		{
		    case IEEE_OUI_PHILIPS:
                app_cec_module_init();
				break;
		    case IEEE_OUI_NEXT:
				if (0 == physical_address)
				{
				    app_cec_set_func_enable(false);
					app_cec_module_init();
				}
				break;
			case IEEE_OUI_SONY:
			case IEEE_OUI_MEI:
			case IEEE_OUI_LG:
			case IEEE_OUI_SAMSUNG:
			case IEEE_OUI_MSTAR:
			default:
				app_cec_set_func_enable(true);
				break;
		}
	}
	return 0;
}

status_t AppCecInit(handle_t self, int priority_offset)
{
    handle_t    sch;

	g_hdmi_cec_device  = GxAVCreateDevice(0);
	g_hdmi_cec_module = GxAVOpenModule(g_hdmi_cec_device, GXAV_MOD_HDMI, 0);

    GxBus_MessageRegister(GXMSG_HDMI_CEC_ON, 0);
    GxBus_MessageRegister(GXMSG_HDMI_CEC_OFF,0);
	GxBus_MessageRegister(GXMSG_HDMI_CEC_WRITE, sizeof(CEC_CMD));
    GxBus_MessageRegister(GXMSG_HDMI_CEC_READ_DONE, sizeof(GxHdmiCecMessage));
    GxBus_MessageListen(self, GXMSG_HDMI_CEC_ON);
    GxBus_MessageListen(self, GXMSG_HDMI_CEC_OFF);
	GxBus_MessageListen(self, GXMSG_HDMI_CEC_WRITE);
    GxBus_MessageListen(self, GXMSG_HDMI_CEC_READ_DONE);
    sch = GxBus_SchedulerCreate("CecMsgScheduler", GXBUS_SCHED_MSG, 1024 * 32, GXOS_DEFAULT_PRIORITY + priority_offset);
    GxBus_ServiceLink(self, sch);
    sch = GxBus_SchedulerCreate("CecConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 32, GXOS_DEFAULT_PRIORITY + priority_offset);
    GxBus_ServiceLink(self, sch);
    return GXCORE_SUCCESS;
}

void AppCecDestroy(handle_t self)
{
    GxBus_MessageUnregister(GXMSG_HDMI_CEC_ON);
    GxBus_MessageUnregister(GXMSG_HDMI_CEC_OFF);
	GxBus_MessageUnregister(GXMSG_HDMI_CEC_WRITE);
	GxBus_MessageUnregister(GXMSG_HDMI_CEC_READ_DONE);
    GxBus_MessageUnListen(self, GXMSG_HDMI_CEC_ON);
    GxBus_MessageUnListen(self, GXMSG_HDMI_CEC_OFF);
	GxBus_MessageUnListen(self, GXMSG_HDMI_CEC_WRITE);
    GxBus_MessageUnListen(self, GXMSG_HDMI_CEC_READ_DONE);
    GxBus_ServiceUnlink(self);
	GxAVCloseModule(g_hdmi_cec_device, g_hdmi_cec_module);
	GxAVDestroyDevice(g_hdmi_cec_device);
    return;
}

GxMsgStatus AppCecServiceRecvMsg(handle_t self, GxMessage *Msg)
{
	P_CEC_CMD cec_cmd = NULL;
	GxHdmiCecMessage *cec_msg = NULL;
	GxMsgStatus status = GXMSG_OK;

    switch (Msg->msg_id)
    {
        case GXMSG_HDMI_CEC_ON:
			app_cec_tv_ieee_oui_process();
            break;
        case GXMSG_HDMI_CEC_OFF:
            app_cec_set_func_enable(false);
            break;
		case GXMSG_HDMI_CEC_WRITE:
            cec_cmd = GxBus_GetMsgPropertyPtr(Msg, CEC_CMD);
            if (cec_cmd)
            {
                app_cec_message_write(cec_cmd);
            }
            break;
		case GXMSG_HDMI_CEC_READ_DONE:
            cec_msg = GxBus_GetMsgPropertyPtr(Msg, GxHdmiCecMessage);
            if (cec_msg)
            {
                app_cec_message_proc(cec_msg);
            }
            break;
        default:
            break;
    }
    return status;
}

extern bool app_simple_lowpower_status_get(void);
extern int top_stbk_halt(uint32_t scan_code);

void AppCecServiceConsole(handle_t self)
{
    int ret = -1;
	unsigned cec_event = 0;
	int cec_config_value = 0;
	GxHdmiCecMessage cec_msg = {0};

    GxBus_ConfigGetInt(CEC_CONF_KEY, &cec_config_value, CEC_CONFIG_VALUE);
	if (cec_config_value == 1)
	{
	    ret = GxAVWaitEvents(g_hdmi_cec_device, g_hdmi_cec_module, EVENT_HDMI_CEC_EOM, 100000, &cec_event);
		if (ret >= 0)
		{
            do {
				memset(&cec_msg, 0, sizeof(GxHdmiCecMessage));
				if (GXCORE_SUCCESS == GxVoutHAL_HdmiCecRead(&cec_msg))
				{
					if(app_simple_lowpower_status_get()&&(cec_msg.message[0] == OPCODE_GIVE_PHYSICAL_ADDR))
					{
					    GxVoutHAL_HdmiSetCecEnable(0);
					    top_stbk_halt(OPCODE_SYSTEM_STANDBY);
					    break;
					}
					else if(app_simple_lowpower_status_get()&&(cec_msg.message[0] == OPCODE_GIVE_POWER_STATUS))
					{
					   tx_cec_msg_report_power_status(CEC_LA_TV, POW_STANDBY);
					   continue;
					}
					else if(app_simple_lowpower_status_get())
					{
					   continue;
					}
					app_hdmi_cec_msg_ring_buffer(&cec_msg);
				}
			}while(cec_msg.message_len > 0);
	    }
	}
	else
	{
		/* Resolve problem [#338466] */
	    GxCore_ThreadDelay(20);
	}
}

GxServiceClass app_cec_service =
{
    "hdmi cec service",
    AppCecInit,
    AppCecDestroy,
    AppCecServiceRecvMsg,
    AppCecServiceConsole,
    .priority_offset = 0,
};
#endif
