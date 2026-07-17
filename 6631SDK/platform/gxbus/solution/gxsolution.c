#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "gxsolution.h"
#include "gui_core.h"

static GxSolutionServiceConfigPara _config;

status_t GxSolutionServiceConfig(GxSolutionServiceConfigPara *config)
{
	if(NULL == config) return GXCORE_ERROR;

	_config.MsgInit    = config->MsgInit;
	_config.MsgCleanup = config->MsgCleanup;

	return GXCORE_SUCCESS;
}

status_t GxSolutionServiceInit(handle_t self, int priority_offset)
{
	handle_t sch;

	if( _config.MsgInit ) {
		_config.MsgInit(self);
	}

	/*
	 * 监听搜索消息
	 */
	GxBus_MessageListen(self, GXMSG_SEARCH_NEW_PROG_GET);
	GxBus_MessageListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
	GxBus_MessageListen(self, GXMSG_SEARCH_STATUS_REPLY);
	GxBus_MessageListen(self, GXMSG_SEARCH_STOP_OK);
	GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_REPLY);
	GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_STOP);
	GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_FINISH);
	/*
	 * 监听信号监测消息
	 */
	GxBus_MessageListen(self, GXMSG_FRONTEND_LOCKED);
	GxBus_MessageListen(self, GXMSG_FRONTEND_UNLOCKED);
	/*
	 * 监听预约消息
	 */
	GxBus_MessageListen(self, GXMSG_BOOK_TRIGGER);
	GxBus_MessageListen(self, GXMSG_BOOK_FINISH);
	/*
	 * 监听SI表消息
	 */
	GxBus_MessageListen(self, GXMSG_SI_SECTION_OK);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_OK);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_TIME_OUT);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_RELEASE_OK);
	/*
	 * 监听TDT时间同步消息
	 */
	GxBus_MessageListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
	/*
	 * 监听播放消息
	 */
	GxBus_MessageListen(self, GXMSG_PLAYER_STATUS_REPORT);
	GxBus_MessageListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
	GxBus_MessageListen(self, GXMSG_PLAYER_SPEED_REPORT);
	GxBus_MessageListen(self, GXMSG_PLAYER_RESOLUTION_REPORT);
	/*
	 * 监听USB插拔、升级消息
	 */
	GxBus_MessageListen(self, GXMSG_UPDATE_STATUS);
	GxBus_MessageListen(self, GXMSG_HOTPLUG_IN);
	GxBus_MessageListen(self, GXMSG_HOTPLUG_OUT);
	GxBus_MessageListen(self, GXMSG_HOTPLUG_CLEAN);

	/*
	 *  监听表版本和升级信息变化
	 */
	GxBus_MessageListen(self, GXMSG_SI_CONSOLE_CHANGE);

	/*
	 *  新添加，多GUI回传消息
	 */
	GxBus_MessageListen(self, GXMSG_GUI_ACKNOWLEDGE_MESSAGE);

	sch = GxBus_SchedulerCreate("SolutionMsgScheduler", 0, 1024 * 32, GXOS_DEFAULT_PRIORITY-1+priority_offset);
	GxBus_ServiceLink(self, sch);
	sch = GxBus_SchedulerCreate("SolutionConsoleScheduler", 1, 1024 * 32, GXOS_DEFAULT_PRIORITY-1+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxSolutionServiceDestroy(handle_t self)
{
	if( _config.MsgCleanup ) {
		_config.MsgCleanup(self);
	}

	GxBus_MessageUnListen(self, GXMSG_SEARCH_NEW_PROG_GET);
	GxBus_MessageUnListen(self, GXMSG_SEARCH_SAT_TP_REPLY);
	GxBus_MessageUnListen(self, GXMSG_SEARCH_STATUS_REPLY);
	GxBus_MessageUnListen(self, GXMSG_SEARCH_STOP_OK);
	GxBus_MessageUnListen(self, GXMSG_FRONTEND_LOCKED);
	GxBus_MessageUnListen(self, GXMSG_FRONTEND_UNLOCKED);
	GxBus_MessageUnListen(self, GXMSG_CA_ON_EVENT);
	GxBus_MessageUnListen(self, GXMSG_BOOK_TRIGGER);
	GxBus_MessageUnListen(self, GXMSG_BOOK_FINISH);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_OK);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_TIME_OUT);
	GxBus_MessageUnListen(self, GXMSG_EXTRA_SYNC_TIME_OK);
	GxBus_MessageUnListen(self, GXMSG_PLAYER_STATUS_REPORT);
	GxBus_MessageUnListen(self, GXMSG_PLAYER_AVCODEC_REPORT);
	GxBus_MessageUnListen(self, GXMSG_PLAYER_SPEED_REPORT);
	GxBus_MessageUnListen(self, GXMSG_UPDATE_STATUS);
	GxBus_MessageUnListen(self, GXMSG_PLAYER_RESOLUTION_REPORT);
	GxBus_MessageUnListen(self, GXMSG_UPDATE_STATUS);
	GxBus_MessageUnListen(self, GXMSG_HOTPLUG_IN);
	GxBus_MessageUnListen(self, GXMSG_HOTPLUG_OUT);
	GxBus_MessageUnListen(self, GXMSG_HOTPLUG_CLEAN);

	GxBus_ServiceUnlink(self);
	return;
}

GxMsgStatus GxSolutionServiceRecvMsg(handle_t self, GxMessage* Msg)
{
	GUI_Event event = {0};

	event.msg.type = GUI_SERVICE_MSG;
	event.msg.service_msg = (void*)Msg;

	GUI_CurrentLink(NULL);
	GUI_ExecEvent(&event);

	return GXMSG_OK;
}

void GxSolutionServiceConsole(handle_t self)
{
	GUI_CurrentLink(NULL);
	GUI_Exec();
}

GxServiceClass solution_service = {
	"gui_view service",
	GxSolutionServiceInit,
	GxSolutionServiceDestroy,
	GxSolutionServiceRecvMsg,
	GxSolutionServiceConsole,
	.priority_offset = 0,
};

