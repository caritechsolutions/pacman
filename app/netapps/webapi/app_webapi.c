#include "app.h"
#include "app_send_msg.h"
#include "app_module.h"
#include "app_pop.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include "app_webapi.h"

extern void app_webapi_volume_set(int value);
extern void app_webapi_set_remote_key(int key);
extern void app_webapi_set_net_play(int param);

static int _webapi_ui_control(AppMsg_WebapiUIControl *msg)
{
	int ret = 0;

	if(msg == NULL)
	{
		return -1;
	}

	switch(msg->type)
	{
		case WEBAPI_MSG_SET_VOLUME:
		{
			app_webapi_volume_set(msg->result);
			break;
		}

		case WEBAPI_MSG_REMOTE:
		{
			app_webapi_set_remote_key(msg->result);
			break;
		}

		case WEBAPI_MSG_NETPLAY:
		{
			app_webapi_set_net_play(msg->result);
			break;
		}

		default:
			break;
	}

	return ret;
}

int app_webapi_msg_proc(GxMessage *msg)
{
	AppMsg_WebapiUIControl *ui_msg;
	int ret = -1;
    int config = 0;

	if(msg == NULL)
	{
		return EVENT_TRANSFER_STOP;
	}

	switch(msg->msg_id)
	{
		case APPMSG_WEBAPI_UI_CONTROL:
		{
			ui_msg = GxBus_GetMsgPropertyPtr(msg, AppMsg_WebapiUIControl);
			ret = _webapi_ui_control(ui_msg);
			ui_msg->result = ret;
			break;
		}

		case GXMSG_NETWORK_STATE_REPORT:
		{
			GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
            if(dev_state->dev_state == IF_STATE_START)
            {
                GxBus_ConfigGetInt(WEBAPI_SWITCH, &config, WEBAPI_SWITCH_ON);
                if(config == 0)
                {
                    app_webapi_stop();
                }
            }
            else if(dev_state->dev_state == IF_STATE_CONNECTED)
			{
                GxBus_ConfigGetInt(WEBAPI_SWITCH, &config, WEBAPI_SWITCH_ON);
                if(config == 0) //ON
                {
                    char dev_name[SUPERCAST_SERVER_NAME_LEN] = {};
                    GxBus_ConfigGet(SUPERCAST_SERVER_NAME, dev_name, SUPERCAST_SERVER_NAME_LEN, SUPERCAST_SERVER_NAME_DEFAULT);
                    app_webapi_start(dev_name);
                }
            }
			break;
		}

		default:
			break;
    }

	return ret;
}

#endif
