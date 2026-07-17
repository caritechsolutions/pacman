#include "app.h"
#if CASCAM_SUPPORT
#include "cascam.h"
#include "cascam_ui_porting.h"
#include "cascam_ui_common.h"
#ifdef CASCAM_CDCAS_SUPPORT
#include "app_cdcas_config.h"
#endif
#ifdef CASCAM_ABVCAS_SUPPORT
#include "app_abvcas_config.h"
#endif
#ifdef CASCAM_DESAICAS_SUPPORT
#include "app_desaicas_config.h"
#endif
#ifdef CASCAM_CGCAS_SUPPORT
#include "app_cgcas_config.h"
#endif
#ifdef CASCAM_TELECASTCAS_SUPPORT
#include "app_telecastcas_config.h"
#endif
#ifdef CASCAM_CTICAS_SUPPORT
#include "app_cticas_config.h"
#endif
//#ifdef CASCAM_NAGRACAS_SUPPORT
//#include "app_nagracas_config.h"
//#endif

#if CASCAM_UI_MULTI_LAYER
#include "app_ca_timer.h"

#ifdef CASCAM_CDCAS_SUPPORT
extern cascam_ui_control cascam_ui_cdcas_ops;
#endif
#ifdef CASCAM_ABVCAS_SUPPORT
extern cascam_ui_control cascam_ui_abvcas_ops;
#endif

#ifdef CASCAM_TENGRUICAS_SUPPORT
extern cascam_ui_control cascam_ui_trcas_ops;
#endif

#ifdef CASCAM_DVTCAS_SUPPORT
extern cascam_ui_control cascam_ui_dvtcas_ops;
#endif

#ifdef CASCAM_DESAICAS_SUPPORT
extern cascam_ui_control cascam_ui_desaicas_ops;
#endif

#ifdef CASCAM_CONAXCAS_SUPPORT
extern cascam_ui_control cascam_ui_conaxcas_ops;
#endif

#ifdef CASCAM_CGCAS_SUPPORT
extern cascam_ui_control cascam_ui_cgcas_ops;
#endif
#ifdef CASCAM_GOSCAS_SUPPORT
extern cascam_ui_control cascam_ui_goscas_ops;
#endif

#ifdef CASCAM_IRDETOCAS_SUPPORT
extern cascam_ui_control cascam_ui_irdetocas_ops;
#endif

#ifdef CASCAM_VMXCAS_SUPPORT
extern cascam_ui_control cascam_ui_vmxcas_ops;
#endif

#ifdef CASCAM_TELECASTCAS_SUPPORT
extern cascam_ui_control cascam_ui_telecastcas_ops;
#endif

#ifdef CASCAM_CTICAS_SUPPORT
extern cascam_ui_control cascam_ui_cticas_ops;
#endif

#ifdef CASCAM_NAGRACAS_SUPPORT
extern cascam_ui_control cascam_ui_nagracas_ops;
#endif

static cascam_ui_control* ca_ops = NULL;
static void app_ca_timer_task(void* arg)
{
	while(1){
		app_ca_timer_exec();
		GxCore_ThreadDelay(10);
	}
}
#endif

void cascam_app_cas_init(void)
{
	//check which ca work
#ifdef CASCAM_CDCAS_SUPPORT
	ca_ops = &cascam_ui_cdcas_ops;
#endif
#ifdef CASCAM_ABVCAS_SUPPORT
	ca_ops = &cascam_ui_abvcas_ops;
#endif
#ifdef CASCAM_TENGRUICAS_SUPPORT
	ca_ops = &cascam_ui_trcas_ops;
#endif
#ifdef CASCAM_DVTCAS_SUPPORT
	ca_ops = &cascam_ui_dvtcas_ops;
#endif
#ifdef CASCAM_DESAICAS_SUPPORT
	ca_ops = &cascam_ui_desaicas_ops;
#endif
#ifdef CASCAM_CONAXCAS_SUPPORT
	ca_ops = &cascam_ui_conaxcas_ops;
#endif
#ifdef CASCAM_CGCAS_SUPPORT
	ca_ops = &cascam_ui_cgcas_ops;
#endif
#ifdef CASCAM_GOSCAS_SUPPORT
	ca_ops = &cascam_ui_goscas_ops;
#endif
#ifdef CASCAM_IRDETOCAS_SUPPORT
	ca_ops = &cascam_ui_irdetocas_ops;
#endif
#ifdef CASCAM_VMXCAS_SUPPORT
	ca_ops = &cascam_ui_vmxcas_ops;
#endif
#ifdef CASCAM_TELECASTCAS_SUPPORT
	ca_ops = &cascam_ui_telecastcas_ops;
#endif
#ifdef CASCAM_CTICAS_SUPPORT
	ca_ops = &cascam_ui_cticas_ops;
#endif
#ifdef CASCAM_NAGRACAS_SUPPORT
	ca_ops = &cascam_ui_nagracas_ops;
#endif
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_init))
		ca_ops->ca_init();

#if CASCAM_UI_MULTI_LAYER
	{
		int thread = 0;
		GxCore_ThreadCreate("app_ca_timer_thread", &thread, app_ca_timer_task, NULL, 60*1024, GXOS_DEFAULT_PRIORITY);
	}
#endif
}

void app_ca_menu_exec(void)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_menu_exec))
		ca_ops->ca_menu_exec();
}

void app_ca_standby(void)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_standby))
		ca_ops->ca_standby();
}

void app_ca_start_ca(void)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_start_ca))
		ca_ops->ca_start_ca();
}

void app_ca_stop_ca(void)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_stop_ca))
		ca_ops->ca_stop_ca();
}

void app_ca_stop_descramble(void)
{
	if(cascam_ui_stop_descramble_check()==0)
		return;
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_stop_descramble))
		ca_ops->ca_stop_descramble();
	cascam_ui_exit_msg_dialog_for_app();
}

void app_ca_switch_channel(CascamSwitchChannel switch_channel)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_switch_channel))
		ca_ops->ca_switch_channel(switch_channel);
	//cascam_ui_create_msg_dialog_for_app(0, 0, 0, 0);
}

void app_ca_quick_switch_channel(CascamQuickSwitchChannel quick_switch_channel)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_quick_switch_channel))
		ca_ops->ca_quick_switch_channel(quick_switch_channel);
	//cascam_ui_create_msg_dialog_for_app(0, 0, 0, 0);
}

void app_ca_switch_freq(void)
{
if((NULL != ca_ops)&&(NULL != ca_ops->ca_switch_freq))
	ca_ops->ca_switch_freq();

}

void app_ca_add_service(CascamServiceInfo service_info)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_add_service))
		ca_ops->ca_add_service(service_info);
}

void app_ca_delete_service(CascamServiceInfo service_info)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_delete_service))
		ca_ops->ca_delete_service(service_info);
}

void app_ca_destroy_finger(void)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_destroy_finger))
		ca_ops->ca_destroy_finger();
}

void app_ca_destroy_rolling_osd(void)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_destroy_rolling_osd))
		ca_ops->ca_destroy_rolling_osd();
}

void app_ca_rebuild_msg_dialog(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	cascam_ui_create_msg_dialog_for_app(x, y, w, h);

}

void app_ca_exit_msg_dialog(void)
{
	cascam_ui_exit_msg_dialog_for_app();

}

int32_t app_ca_check_pvr_forbid(void)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_check_pvr_forbid))
		return ca_ops->ca_check_pvr_forbid();
	return 0;
}

int32_t app_ca_pvr_control(uint32_t type, void* param)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_pvr_control))
		return ca_ops->ca_pvr_control(type, param);
	return 0;
}

int32_t app_ca_check_force_lock_status(void)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_check_force_lock_status))
		return ca_ops->ca_check_force_lock_status();
	return 0;
}

int32_t app_cascam_osd_send_message(uint32_t sub_id, void *para)
{
	GxMessage *msg = NULL;

	CascamOsdEvent *osd_event;
	msg = GxBus_MessageNew(GXMSG_CASCAM_EVENT);
	if(NULL == msg)
	{
	   return -1;
	}
	osd_event = GxBus_GetMsgPropertyPtr(msg, CascamOsdEvent);
	if(NULL == osd_event)
	{
	   return -1;
	}

	//printf("[%s] [%d] OSD EVENT Sub ID=%d\n",__FUNCTION__,__LINE__,sub_id);
	osd_event->msg_id = sub_id;
	osd_event->data = para;
	//GxBus_MessageSendWait(msg);
	GxBus_MessageSend(msg);
	//GxBus_MessageFree(msg);
	return 0;
}

int32_t app_cascam_osd_send_wait_message(uint32_t sub_id, void *para)
{
	GxMessage *msg = NULL;

	CascamOsdEvent *osd_event;
	msg = GxBus_MessageNew(GXMSG_CASCAM_EVENT);
	if(NULL == msg)
	{
	   return -1;
	}
	osd_event = GxBus_GetMsgPropertyPtr(msg, CascamOsdEvent);
	if(NULL == osd_event)
	{
	   return -1;
	}

	//printf("[%s] [%d] OSD EVENT Sub ID=%d\n",__FUNCTION__,__LINE__,sub_id);
	osd_event->msg_id = sub_id;
	osd_event->data = para;
	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);
	return 0;
}

int32_t app_ca_cascam_do_event(CascamOsdEvent* para)
{
	if((NULL != ca_ops)&&(NULL != ca_ops->ca_do_event))
		return ca_ops->ca_do_event(para);
	else{
		if(NULL != para->data)
			GxCore_Free(para->data);
	}
	return 0;
}

#if CASCAM_UI_MULTI_LAYER
int32_t app_ca_cascam_do_event_layer(uint32_t sub_id, void *para)
{
	CascamOsdEvent param;
	memset(&param, 0, sizeof(CascamOsdEvent));
	param.msg_id = sub_id;
	param.data = para;

	if((NULL != ca_ops)&&(NULL != ca_ops->ca_do_event_layer))
		return ca_ops->ca_do_event_layer(&param);
	else{
		if(NULL != para)
			GxCore_Free(para);
	}
	return 0;
}
#endif
#endif
