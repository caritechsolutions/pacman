#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxgui_view.h"
#include "gui_core.h"
#include "module/config/gxconfig.h"


//handle_t gui_mutex_id = 0;
static int schedule_flag = 0;

GuiViewAppCallback app_callback;

status_t GxGuiViewRegisterApp(GuiViewAppCallback* cb)
{
	if(NULL == cb) return GXCORE_ERROR;

	app_callback.app_msg_init = cb->app_msg_init;
	app_callback.app_msg_destroy = cb->app_msg_destroy;
	app_callback.app_init = cb->app_init;

	return GXCORE_SUCCESS;
}

static int first_run = 0;
status_t GxGuiViewServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;
	char key_value[10] = {0};

	//GxCore_MutexCreate(&gui_mutex_id);

	if(app_callback.app_msg_init)
	{
		app_callback.app_msg_init(self);
	}

	GxBus_MessageRegister(GXMSG_GUI_WARNING, sizeof(GxMsgProperty_GUI_Warning));
	GxBus_MessageRegister(GXMSG_GUI_RETURN_MESSAGE, sizeof(GxMsgProperty_GUI_Return_Message));
	GxBus_MessageRegister(GXMSG_GUI_TRANSFER_MESSAGE, sizeof(GxMsgProperty_GUI_Transfer_Message));
	GxBus_MessageRegister(GXMSG_GUI_ACKNOWLEDGE_MESSAGE, sizeof(GxMsgProperty_GUI_Acknowledge_Message));

	GxBus_MessageListen(self, GXMSG_GUI_WARNING);
	GxBus_MessageListen(self, GXMSG_GUI_ACKNOWLEDGE_MESSAGE);

	sch = GxBus_SchedulerCreate("GuiViewMsgScheduler", 0, 1024 * 32, GXOS_DEFAULT_PRIORITY-1+priority_offset);
	GxBus_ServiceLink(self, sch);
	sch = GxBus_SchedulerCreate("GuiViewConsoleScheduler", 1, 1024 * 32, GXOS_DEFAULT_PRIORITY-1+priority_offset);
	GxBus_ServiceLink(self, sch);

	GxBus_ConfigGet("prior_gui", key_value, 9, "false");
	if(0 == strcasecmp("true", key_value)) {
		if(0 == first_run) {
			if(app_callback.app_init)
			{
				RUNTIME("[APP INIT]", app_callback.app_init());
			}
			first_run = 1;
		}
	}

	return GXCORE_SUCCESS;
}

void GxGuiViewServiceDestroy(handle_t self)
{
	//GxCore_MutexDelete(gui_mutex_id);

	if(app_callback.app_msg_destroy)
	{
		app_callback.app_msg_destroy(self);
	}
	
	GxBus_MessageUnregister(GXMSG_GUI_WARNING);
	GxBus_MessageUnregister(GXMSG_GUI_RETURN_MESSAGE);
	GxBus_MessageUnregister(GXMSG_GUI_ACKNOWLEDGE_MESSAGE);
	GxBus_ServiceUnlink(self);
	return;
}

extern GuiCore gui;
static int _process_cb(void *userdata)
{
	MessagePara *para = NULL;
	GUI_Event *event = NULL;
	int ret = 0;
	GuiWidget *focus_widget = NULL;

#if 0
	/*Wait while schedule_flag is OK*/
	if(schedule_flag)
		return (0);
#endif

	para = (MessagePara *)userdata;
	if(NULL == para)
	{
		return (1);
	}

	event = para->event;
	if(NULL == event)
	{
		remove_timer(para->timer);
		return (1);
	}

	ret = widget_notify_top(NULL, (void *)event);
	if(EVENT_TRANSFER_KEEPON == ret)
	{
		focus_widget = gui.config.focus_widget;
		ret = widget_exec_event(focus_widget, (void *)event);
	}
	if((EVENT_TRANSFER_KEEPON == ret) && WIDGET_ISNOT_WINDOW(focus_widget))
	{
		ret = widget_notify_parent(focus_widget, (void *)event);
	}
	if((EVENT_TRANSFER_KEEPON == ret) && WIDGET_ISNOT_ROOT(focus_widget))
	{
		widget_notify_root(focus_widget, (void *)event);
	}

	GUI_FREE(event->msg.service_msg);
	GUI_FREE(event);
	remove_timer(para->timer);
	GUI_FREE(para);

	return (0);
}

static void _delay_process(GUI_Event *event)
{
	MessagePara *para = NULL;

	if(NULL == event)
		return;

	para = GUI_MALLOCZ(sizeof(MessagePara));
	if(NULL == para)
		return;

	para->event = GUI_MALLOCZ(sizeof(GUI_Event));
	if(NULL == para->event)
		return;
	memcpy(para->event, event, sizeof(GUI_Event));

	para->timer = create_timer(_process_cb, 100, (void *)para, TIMER_REPEAT);
	if(NULL == para->timer) {
		GUI_FREE(para->event);
		GUI_FREE(para);
		return;
	}
}

GxMsgStatus GxGuiViewServiceRecvMsg(handle_t self, GxMessage* Msg)
{
	GUI_Event event = {0};

	event.msg.type = GUI_SERVICE_MSG;
	event.msg.service_msg = (void *)Msg;

	if(schedule_flag)
	{
		event.msg.service_msg = GUI_MALLOCZ(sizeof(GxMessage) + Msg->size);
		if(NULL != event.msg.service_msg) {
			memcpy(event.msg.service_msg, Msg, sizeof(GxMessage) + Msg->size);
		} else {
			event.msg.service_msg = NULL;
		}
		_delay_process(&event);
		return (GXMSG_OK);
	}

	//GxCore_MutexLock(gui_mutex_id);
	GUI_CurrentLink(NULL);
	GUI_ExecEvent(&event);
	//GxCore_MutexUnlock(gui_mutex_id);

	return GXMSG_OK;
}

int GxGuiViewServiceRecvMsgStop(void)
{
	schedule_flag = 1;
	return (0);
}

int GxGuiViewServiceRecvMsgStart(void)
{
	schedule_flag  = 0 ;
	return (0);
}

void GxGuiViewServiceConsole(handle_t self)
{
	static int id_inited;

	if(0 == first_run)
	{
		if(app_callback.app_init)
		{
			RUNTIME("[APP INIT]", app_callback.app_init());
		}
		first_run = 1;
	}

	if(0 == id_inited) {
		GUI_CurrentLink(NULL);
		id_inited = 1;
	}

	//GxCore_MutexLock(gui_mutex_id);
	GUI_Exec();
	//GxCore_MutexUnlock(gui_mutex_id);
	//GxCore_ThreadDelay(60);
}

GxServiceClass gui_view_service = {
	"gui_view service",
	GxGuiViewServiceInit,
	GxGuiViewServiceDestroy,
	GxGuiViewServiceRecvMsg,
	GxGuiViewServiceConsole,
   .priority_offset = 0,
};

