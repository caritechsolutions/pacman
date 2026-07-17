
#include "app.h"
#include "app_send_msg.h"
#include "app_module.h"
#include "gx_apps.h"

#if NETWORK_SUPPORT
static int diff_time_network = 0;
int app_sync_mktime_network(void)
{
	gx_net_time_t *net_time = NULL;
	time_t nettime,mktime;
	int diff_time = 0;

	mktime = time(NULL);
	net_time = gx_net_get_time();
	if(net_time)
	{
		nettime = app_mktime(net_time->tm_year+1900, net_time->tm_mon+1, net_time->tm_mday, net_time->tm_hour, net_time->tm_min, net_time->tm_sec);
		free(net_time);
		net_time = NULL;

		diff_time = mktime - nettime;
		diff_time_network = diff_time;
		return 0;
	}
	return 1;
}

time_t app_get_networktime(time_t *timer)
{
	time_t nettime,mktime;

	mktime = time(NULL);
	nettime = mktime - diff_time_network;
	if(timer)
		*timer = nettime;

	return nettime;
}

uint64_t app_get_iptvtime(void)
{
	time_t nettime,mktime;

	mktime = time(NULL);
	nettime = mktime - diff_time_network;

	return nettime;
}

#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
static int get_net_time_status = 0;

static void start_get_net_time(void)
{
	get_net_time_status = 1;
}
#endif
static int sync_net_time_status = 0;

status_t AppNettimeServiceInit(handle_t self, int priority_offset)
{
	handle_t sch;

#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
	GxBus_MessageRegister(GXMSG_NETTIME_SYNC, 0);
	GxBus_MessageListen(self, GXMSG_NETTIME_SYNC);
#endif
	GxBus_MessageRegister(GXMSG_MKTIME_NET_SYNC, 0);

	GxBus_MessageListen(self, GXMSG_MKTIME_NET_SYNC);
	GxBus_MessageListen(self, GXMSG_NETWORK_STATE_REPORT);

	sch = GxBus_SchedulerCreate("NettimeMsgScheduler", GXBUS_SCHED_MSG, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);
	sch = GxBus_SchedulerCreate("NettimeConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 16, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void AppNettimeServiceDestroy(handle_t self)
{
#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
	GxBus_MessageUnListen(self, GXMSG_NETTIME_SYNC);
	GxBus_MessageUnregister(GXMSG_NETTIME_SYNC);
#endif
	GxBus_MessageUnListen(self, GXMSG_MKTIME_NET_SYNC);
	GxBus_MessageUnListen(self, GXMSG_NETWORK_STATE_REPORT);

	GxBus_MessageUnregister(GXMSG_MKTIME_NET_SYNC);

	GxBus_ServiceUnlink(self);

	return;
}

GxMsgStatus AppNettimeServiceRecvMsg(handle_t self, GxMessage *msg)
{
	GxMsgStatus ret = GXCORE_SUCCESS;
	if (msg == NULL)
		return GXCORE_ERROR;

	switch (msg->msg_id)
	{
#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
		case GXMSG_NETTIME_SYNC:
			start_get_net_time();
			break;
#endif
		case GXMSG_MKTIME_NET_SYNC:
			sync_net_time_status = 1;
			break;
		case GXMSG_NETWORK_STATE_REPORT:
		{
			GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
			if(dev_state->dev_state == IF_STATE_CONNECTED)
			{
				g_AppTime.start(&g_AppTime);
			}
		}
		break;

		default:
			break;
	}

	return ret;
}

static void AppNettimeServiceConsole(handle_t self)
{
#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
    gx_net_time_t *net_time = NULL;
#endif

    if(sync_net_time_status == 1)
    {
        if(app_sync_mktime_network())
        {
            GxCore_ThreadDelay(800);
        }
        else
        {
            sync_net_time_status = 0;
        }
    }
#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
    else if(get_net_time_status == 1)
    {
        net_time = gx_net_get_time();
        if(net_time)
        {
            time_t time = app_mktime(net_time->tm_year+1900, net_time->tm_mon+1, net_time->tm_mday, net_time->tm_hour, net_time->tm_min, net_time->tm_sec);
            g_AppTime.sync(&g_AppTime, time);
            get_net_time_status = 2;
            free(net_time);
            net_time = NULL;
        }
        else
        {
            GxCore_ThreadDelay(800);
        }
    }
#endif
    else
    {
        GxCore_ThreadDelay(800);
    }
}

GxServiceClass app_nettime_service =
{
	"app_nettime_service",
	AppNettimeServiceInit,
	AppNettimeServiceDestroy,
	AppNettimeServiceRecvMsg,
	AppNettimeServiceConsole,
};

#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
int app_net_time_sync_message(void)
{
	GxMessage* msg = GxBus_MessageNew(GXMSG_NETTIME_SYNC);
	if (msg == NULL)
	{
		return 1;
	}
	GxBus_MessageSend(msg);
	return 0;
}
#endif

int app_net_mktime_sync_message(void)
{
	GxMessage* msg = GxBus_MessageNew(GXMSG_MKTIME_NET_SYNC);
	if (msg == NULL)
	{
		return 1;
	}
	GxBus_MessageSend(msg);
	return 0;
}
#endif
