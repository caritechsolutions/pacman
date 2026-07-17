#include <gxbus.h>
#include <gxmsg.h>
#include <service/gxfrontend.h>
#include "module/config/gxconfig.h"
#include "gxfrontend.h"

#define MAX_FRONTEND_NET_TIMEOUT_COUNT    (2)
extern int32_t GxFrontend_SetTpInternal(int tuner, GxFrontend_MonitorParams *params);
extern int GxFrontend_QueryMonitor(int tuner);
extern unsigned int GxFrontend_GetStrengthMonitor(int tuner);
extern unsigned int GxFrontend_GetQualityMonitor(int tuner ,GxFrontendSignalQuality* sq);
extern uint32_t g_check_count[FRONTEND_MAX];
extern void GxFrontend_SetVoltageMonitor(int32_t tuner, GxFrontend_MonitorParams *params);
extern void GxFrontend_Set22KMonitor(int32_t tuner, GxFrontend_MonitorParams *params);
extern int32_t GxFrontend_QueryFrontendStatusMonitor(int32_t tuner);
extern void GxFrontend_SetDiseqcMonitor(int32_t tuner, GxFrontend_MonitorParams *params);
extern void GxFrontend_ResetStatusMonitor(int32_t tuner);
extern int GxFrontend_GetGlobalParams(int32_t tuner, GxFrontend_MonitorParams *params);
extern void GxFrontend_SendUnicableMonitor(int32_t tuner, GxFrontend_MonitorParams *params);
extern fronted_set_status g_frontend_set[FRONTEND_MAX];
static handle_t gs_frontend_monitor = 0;

status_t GxFrontendServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;

	GxBus_MessageRegister(GXMSG_FRONTEND_STOP_MONITOR,sizeof(GxMsgProperty_FrontendMonitor));
	GxBus_MessageRegister(GXMSG_FRONTEND_START_MONITOR,sizeof(GxMsgProperty_FrontendMonitor));
	GxBus_MessageRegister(GXMSG_FRONTEND_SET_TP,sizeof(GxMsgProperty_FrontendSetTp));
	GxBus_MessageRegister(GXMSG_FRONTEND_SET_DISEQC,sizeof(GxMsgProperty_FrontendSetDiseqc));
	GxBus_MessageRegister(GXMSG_FRONTEND_LOCKED,sizeof(GxMsgProperty_FrontendLocked));
	GxBus_MessageRegister(GXMSG_FRONTEND_UNLOCKED,sizeof(GxMsgProperty_FrontendLocked));

	GxBus_MessageListen(self,GXMSG_FRONTEND_SET_TP);
	GxBus_MessageListen(self,GXMSG_FRONTEND_SET_DISEQC);
	GxBus_MessageListen(self,GXMSG_FRONTEND_STOP_MONITOR);
	GxBus_MessageListen(self,GXMSG_FRONTEND_START_MONITOR);

	GxCore_SemCreate(&gs_frontend_monitor,0);
	//GxFrontend_Init(FRONTEND_MAX);

	sch = GxBus_SchedulerCreate("FrontendMsgScheduler", GXBUS_SCHED_MSG, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset -1);
	GxBus_ServiceLink(self, sch);
	sch = GxBus_SchedulerCreate("FrontendConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;

}

void  GxFrontendServiceDestroy(handle_t self)
{
	GxBus_MessageUnListen(self,GXMSG_FRONTEND_SET_TP);
	GxBus_MessageUnListen(self,GXMSG_FRONTEND_SET_DISEQC);
	GxBus_MessageUnListen(self,GXMSG_FRONTEND_STOP_MONITOR);
	GxBus_MessageUnListen(self,GXMSG_FRONTEND_START_MONITOR);

	GxBus_MessageUnregister(GXMSG_FRONTEND_STOP_MONITOR);
	GxBus_MessageUnregister(GXMSG_FRONTEND_START_MONITOR);
	GxBus_MessageUnregister(GXMSG_FRONTEND_SET_TP);
	GxBus_MessageUnregister(GXMSG_FRONTEND_SET_DISEQC);
	GxBus_MessageUnregister(GXMSG_FRONTEND_LOCKED);
	GxBus_MessageUnregister(GXMSG_FRONTEND_UNLOCKED);

	GxBus_ServiceUnlink(self);
}

GxMsgStatus GxFrontendServiceRecvMsg(handle_t self, GxMessage * msg)
{
	GxMsgStatus ret = GXCORE_SUCCESS;

	if(msg == NULL)
		return GXCORE_ERROR;

	switch(msg->msg_id){
		case GXMSG_FRONTEND_SET_TP:{
				GxMsgProperty_FrontendSetTp *param = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_FrontendSetTp);
				if(param == NULL)
					return GXCORE_ERROR;
				GxFrontend_SetTp(param);
				break;
			}
        case GXMSG_FRONTEND_SET_DISEQC:{
				GxMsgProperty_FrontendSetDiseqc *param = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_FrontendSetDiseqc);
				if(param == NULL)
					return GXCORE_ERROR;
                GxFrontend_SetDiseqc(param);
                break;
            }
		case GXMSG_FRONTEND_STOP_MONITOR:{
				GxMsgProperty_FrontendMonitor *param = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_FrontendMonitor);
				if(param == NULL)
					return GXCORE_ERROR;
				GxFrontend_StopMonitor(*param);
				break;
			}
		case GXMSG_FRONTEND_START_MONITOR:{
				GxMsgProperty_FrontendMonitor *param = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_FrontendMonitor);
				if(param == NULL)
					return GXCORE_ERROR;
				GxFrontend_StartMonitor(*param);
				break;
			}

		default:
			ret = GXMSG_OK;
			break;
	}

	return ret;
}

void GxFrontendOccur(int32_t tuner)
{
	if(GxFrontend_QueryMonitor(tuner))
		GxCore_SemPost(gs_frontend_monitor);
}

int GxFrontendGetSemCount(int32_t tuner)
{
	int Count = 0;
	if(GxFrontend_QueryMonitor(tuner))
		GxCore_SemGetValue(gs_frontend_monitor,&Count);
	return Count;
}

#define GXBUS_FRONTEND_MONITOR_UNLOCK 0
#define GXBUS_FRONTEND_MONITOR_LOCK   1
static void _send_lock_status_msg(int32_t tuner, uint8_t lock_status)
{
	GxMessage *new_msg = NULL;
	GxMsgProperty_FrontendLocked *param = NULL;

	if(lock_status){
		new_msg = GxBus_MessageNew(GXMSG_FRONTEND_LOCKED);
		param = (GxMsgProperty_FrontendLocked*)GxBus_GetMsgPropertyPtr(new_msg,GxMsgProperty_FrontendLocked);
	}else{
		new_msg = GxBus_MessageNew(GXMSG_FRONTEND_UNLOCKED);
		param = (GxMsgProperty_FrontendUnlocked*)GxBus_GetMsgPropertyPtr(new_msg,GxMsgProperty_FrontendUnlocked);
	}
	if(param){
		*param = tuner;
		GxBus_MessageSend(new_msg);
	}
}

void  GxFrontendServiceConsole(handle_t self)
{
	int i = 0, ret = 0;
	static int32_t time = 0, count = 0;
	static uint32_t TimeCount[FRONTEND_MAX] = {1, 1, 1, 1};
	struct dvb_frontend_info info;
	uint32_t frontend_status = 0, demux_status = 0;
	uint32_t type = -1;
	static GxFrontend_MonitorParams params;

	if(time == 0){
		GxBus_ConfigGetInt(FRONTEND_CONFIG_WATCH_TIME,&time,1000);
		time = (time >= 20?time:20);
		memset(&params, 0, sizeof(GxFrontend_MonitorParams));
		for(i=0;i<FRONTEND_MAX;i++){//init params
			params.diseqc10[i].para.diseqc.type = DiSEQC10;
			params.diseqc11[i].para.diseqc.type = DiSEQC11;
			params.diseqc12[i].para.diseqc.type = DiSEQC12;
		}
	}
	if(count == 0){
		GxBus_ConfigGetInt(FRONTEND_CONFIG_WATCH_COUNT,&count,1);
		count = (count > 1?count:2);
	}
	ret = GxCore_SemTimedWait(gs_frontend_monitor, time);
	if(ret == 0)//sem occur
		memset(TimeCount, 0, sizeof(TimeCount));

	for(i=0;i<FRONTEND_MAX;i++){
		if(GxFrontend_QueryMonitor(i)){

			type = GxFrontendGetType(i);
			if(FRONTEND_DVB_NET == type){
				GxFrontend_ReadStatus(i, &frontend_status);
				if(frontend_status == FE_HAS_LOCK){
					if(TimeCount[i] > 0){
						TimeCount[i] = 0;
						gxlogd("[Frontend-%d]: Net Locked!...\n",i);
						_send_lock_status_msg(i, GXBUS_FRONTEND_MONITOR_LOCK);
					}
				}else{
					if(TimeCount[i] > count){
						TimeCount[i] = 0;
						gxlogd("[Frontend-%d]: Net Unlocked!...\n",i);
						_send_lock_status_msg(i, GXBUS_FRONTEND_MONITOR_UNLOCK);
						GxFrontend_ResetStatusMonitor(i);
						GxFrontend_GetGlobalParams(i, &params);
						GxFrontend_SetTpInternal(i, &params);
					}
					TimeCount[i]++;
				}
				continue;
			}
check:
			GxFrontend_GetGlobalParams(i, &params);
			GxFrontend_SendUnicableMonitor(i, &params);
			GxFrontend_SetVoltageMonitor(i, &params);
			GxFrontend_Set22KMonitor(i, &params);
			GxFrontend_SetDiseqcMonitor(i, &params);
			GxFrontend_SetTpInternal(i, &params);

			frontend_status = GxFrontend_QueryFrontendStatusMonitor(i);
			demux_status = GxFrontend_QueryStatus(i);
			if((demux_status == 1)&&(frontend_status == 1)){
				if(TimeCount[i] > 0){
					TimeCount[i] = 0;
					gxlogd("[Frontend-%d]: TS Locked!...\n",i);
					_send_lock_status_msg(i, GXBUS_FRONTEND_MONITOR_LOCK);
				}
			}else{
				if(TimeCount[i] > count){
					TimeCount[i] = 0;
					gxlogd("[Frontend-%d]: TS Unlock!...\n",i);
					_send_lock_status_msg(i, GXBUS_FRONTEND_MONITOR_UNLOCK);
					// monitor relock
					GxFrontend_GetDemodInfo(i,&info);
					if((info.caps &= FE_CAN_RECOVER)> 0){
						GxFrontend_ResetStatusMonitor(i);
						TimeCount[i]++;
						goto check;
					}
				}
				TimeCount[i]++;
			}
			GxFrontend_GetStrengthMonitor(i);
			GxFrontend_GetQualityMonitor(i,NULL);
		}
	}
}

GxServiceClass frontend_service = {
	"Frontend service",
	GxFrontendServiceInit,
	GxFrontendServiceDestroy,
	GxFrontendServiceRecvMsg,
	GxFrontendServiceConsole,
	.priority_offset = 0,
};

