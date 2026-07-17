#if 0
#ifdef MEMWATCH
#include <common/memwatch.h>
#else
#include <stdlib.h>
#endif
#include <stdio.h>
#include "gxavdev.h"
#include "service/gxsearch.h"
#include "gxcore.h"
#include "service/gxsi.h"
#include "module/frontend/gxfrontend_module.h"
#include "module/app_nim.h"
#include "module/app_ioctl.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include "module/config/gxconfig.h"
#include "app.h"
#include "gdi_core.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "gxtkgs.h"

#if TKGS_SUPPORT

void tkgs_test(void)
{
	GxTkgsPmtCheckParm pmt_check = {"abcliqg", 0};
	GxTkgsTpCheckParm	tp_check = {0, 334, 776, 1, 1};
	//int tp_id = -1;
	GxTkgsLocation location = {0, {0}, 2, {0}};
	GxTkgsOperatingMode operatingMode = 1;
	GxTkgsPreferredList preferred_list = {0, NULL};
	ubyte64 preferred_list_name = "HHHKK";
	GxTkgsBlockCheckParm block_check = {2, 0};
	int version = 54;

	app_log_flow("***********%s, %d\n", __FUNCTION__, __LINE__);
	app_send_msg_exec(GXMSG_TKGS_CHECK_BY_PMT, &pmt_check);
	app_send_msg_exec(GXMSG_TKGS_CHECK_BY_TP, &tp_check);
	//app_send_msg_exec(GXMSG_TKGS_START_VERSION_CHECK, &tp_id);
	//app_send_msg_exec(GXMSG_TKGS_START_UPDATE, &tp_id);
	app_send_msg_exec(GXMSG_TKGS_STOP, NULL);
	app_send_msg_exec(GXMSG_TKGS_SET_LOCATION, &location);
	app_send_msg_exec(GXMSG_TKGS_GET_LOCATION, &location);
	app_send_msg_exec(GXMSG_TKGS_SET_OPERATING_MODE, &operatingMode);
	app_send_msg_exec(GXMSG_TKGS_GET_OPERATING_MODE, &operatingMode);

	app_log_flow("=========%s, %d, num:%d, prefer:%p\n", __FUNCTION__, __LINE__, preferred_list.listNum, preferred_list.prefer);
	app_send_msg_exec(GXMSG_TKGS_GET_PREFERRED_LISTS, &preferred_list);
	app_log_flow("=========%s, %d, num:%d, prefer:%p\n", __FUNCTION__, __LINE__, preferred_list.listNum, preferred_list.prefer);
	app_send_msg_exec(GXMSG_TKGS_SET_PREFERRED_LIST, preferred_list_name);
	app_send_msg_exec(GXMSG_TKGS_GET_PREFERRED_LIST, preferred_list_name);
	app_send_msg_exec(GXMSG_TKGS_BLOCKING_CHECK, &block_check);
	app_send_msg_exec(GXMSG_TKGS_TEST_GET_VERNUM, &version);
	app_send_msg_exec(GXMSG_TKGS_TEST_RESET_VERNUM, NULL);
	app_send_msg_exec(GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST, NULL);
	app_send_msg_exec(GXMSG_TKGS_TEST_CLEAR_VISIBLE_LOCATIONS_LIST, NULL);
}

void tkgs_location_test(void)
{
	int i;
	GxTkgsLocation location = {0};
	GxTkgsLocation saved_location = {0};
	GxTkgsLocationParm locationTp[TKGS_MAX_VISIBLE_LOCATION+TKGS_MAX_INVISIBLE_LOCATION];
	uint16_t pid[TKGS_MAX_VISIBLE_LOCATION+TKGS_MAX_INVISIBLE_LOCATION];
	uint32_t num = 0;

	memset(&location, 0, sizeof(GxTkgsLocation));
	memset(&saved_location, 0, sizeof(GxTkgsLocation));

	location.visible_num = 2;
	location.visible_location[0].sat_id = 84;
	location.visible_location[0].search_demux_id = 0;
	//location.visible_location[0].search_diseqc = NULL;
	location.visible_location[0].frequency = 4147;
	location.visible_location[0].symbol_rate = 6150;
	location.visible_location[0].polar = 0;
	location.visible_location[0].pid = 8181;
	location.visible_location[1].sat_id = 84;
	location.visible_location[1].search_demux_id = 0;
	//location.visible_location[1].search_diseqc = NULL;
	location.visible_location[1].frequency = 4000;
	location.visible_location[1].symbol_rate = 27500;
	location.visible_location[1].polar = 0;
	location.visible_location[1].pid = 8181;

	location.invisible_num = 20;
	for (i=0; i<location.invisible_num; i++)
	{
		location.invisible_location[i].sat_id = 1;
		location.invisible_location[i].search_demux_id = 0;
		//location.invisible_location[i].search_diseqc = NULL;
		location.invisible_location[i].frequency = 3840+i*8;
		location.invisible_location[i].symbol_rate = 22500;
		location.invisible_location[i].polar = 1;
		location.invisible_location[i].pid = 8181;
	}

	app_send_msg_exec(GXMSG_TKGS_SET_LOCATION, &location);

	app_send_msg_exec(GXMSG_TKGS_GET_LOCATION, &saved_location);
	for (i=0; i<location.visible_num; i++)
	{
		if ((location.visible_location[i].sat_id != saved_location.visible_location[i].sat_id)
			|| (location.visible_location[i].search_demux_id != saved_location.visible_location[i].search_demux_id)
			//|| (location.visible_location[i].search_diseqc != saved_location.visible_location[i].search_diseqc)
			|| (location.visible_location[i].frequency != saved_location.visible_location[i].frequency)
			|| (location.visible_location[i].symbol_rate != saved_location.visible_location[i].symbol_rate)
			|| (location.visible_location[i].polar != saved_location.visible_location[i].polar)
			|| (location.visible_location[i].pid != saved_location.visible_location[i].pid))
		{
			app_log_debug("====i:%d, location:[%d, %d, %d, %d, %d, %d], saved location:[%d, %d, %d, %d, %d, %d]",
				i,
				location.visible_location[i].sat_id, location.visible_location[i].search_demux_id,
				//location.visible_location[i].search_diseqc, location.visible_location[i].frequency,
				location.visible_location[i].symbol_rate, location.visible_location[i].polar,
				location.visible_location[i].pid,
				saved_location.visible_location[i].sat_id, saved_location.visible_location[i].search_demux_id,
				//saved_location.visible_location[i].search_diseqc, saved_location.visible_location[i].frequency,
				saved_location.visible_location[i].symbol_rate, saved_location.visible_location[i].polar,
				saved_location.visible_location[i].pid);
		}
	}

	for (i=0; i<location.invisible_num; i++)
	{
		if ((location.invisible_location[i].sat_id != saved_location.invisible_location[i].sat_id)
			|| (location.invisible_location[i].search_demux_id != saved_location.invisible_location[i].search_demux_id)
			//|| (location.invisible_location[i].search_diseqc != saved_location.invisible_location[i].search_diseqc)
			|| (location.invisible_location[i].frequency != saved_location.invisible_location[i].frequency)
			|| (location.invisible_location[i].symbol_rate != saved_location.invisible_location[i].symbol_rate)
			|| (location.invisible_location[i].polar != saved_location.invisible_location[i].polar)
			|| (location.invisible_location[i].pid != saved_location.invisible_location[i].pid))
		{
			app_log_debug("====i:%d, location:[%d, %d, %d, %d, %d, %d], saved location:[%d, %d, %d, %d, %d, %d]",
				i,
				location.invisible_location[i].sat_id, location.invisible_location[i].search_demux_id,
				//location.invisible_location[i].search_diseqc, location.invisible_location[i].frequency,
				location.invisible_location[i].symbol_rate, location.invisible_location[i].polar,
				location.invisible_location[i].pid,
				saved_location.invisible_location[i].sat_id, saved_location.invisible_location[i].search_demux_id,
				//saved_location.invisible_location[i].search_diseqc, saved_location.invisible_location[i].frequency,
				saved_location.invisible_location[i].symbol_rate, saved_location.invisible_location[i].polar,
				saved_location.invisible_location[i].pid);
		}
	}

	app_log_info("====%s, %d, location set get test ok\n", __FUNCTION__, __LINE__);

	gx_TkgsSaveLocation(&location.invisible_location[1]);

	if (GXCORE_ERROR == gx_TkgsGetLocationInOrder(&num,locationTp,pid))
	{
		app_log_error("====%s, %d, gx_TkgsGetLocationInOrder err\n", __FUNCTION__, __LINE__);
	}

	app_log_info("====%s, %d, location num:%d\n", __FUNCTION__, __LINE__, num);
	for (i=0; i<num; i++)
	{
		app_log_info("@@location[%d]:{%d, %d, %d, %d, %d, %d}, pid:[%d]\n",
			i,
			locationTp[i].sat_id, locationTp[i].search_demux_id,
			locationTp[i].frequency, locationTp[i].symbol_rate, locationTp[i].polar, locationTp[i].pid, pid[i]);
	}
}

void tkgs_operating_mode_test(void)
{
	GxTkgsOperatingMode operatingMode = 1;
	GxTkgsOperatingMode saved_mode=-1;

	app_send_msg_exec(GXMSG_TKGS_GET_OPERATING_MODE, &saved_mode);
	app_log_flow("====%s, %d, mode:%d\n", __FUNCTION__, __LINE__, saved_mode);

	operatingMode = GX_TKGS_CUSTOMIZABLE;
	app_send_msg_exec(GXMSG_TKGS_SET_OPERATING_MODE, &operatingMode);
	app_send_msg_exec(GXMSG_TKGS_GET_OPERATING_MODE, &saved_mode);
	app_log_flow("====%s, %d, mode:%d\n", __FUNCTION__, __LINE__, saved_mode);

	operatingMode = GX_TKGS_OFF;
	app_send_msg_exec(GXMSG_TKGS_SET_OPERATING_MODE, &operatingMode);
	app_send_msg_exec(GXMSG_TKGS_GET_OPERATING_MODE, &saved_mode);
	app_log_flow("====%s, %d, mode:%d\n", __FUNCTION__, __LINE__, saved_mode);

	operatingMode = GX_TKGS_AUTOMATIC;
	app_send_msg_exec(GXMSG_TKGS_SET_OPERATING_MODE, &operatingMode);
	app_send_msg_exec(GXMSG_TKGS_GET_OPERATING_MODE, &saved_mode);
	app_log_flow("====%s, %d, mode:%d\n", __FUNCTION__, __LINE__, saved_mode);
}

void tkgs_start_version_check_test(void)
{
	GxTkgsUpdateParm update_parm = {-1, 0, GX_TKGS_UPDATE_ALL};
	//int tp_id = -1;
	GxMsgProperty_NodeByPosGet ProgNode;
	GxMsgProperty_NodeByIdGet tpNode;
	GxTkgsLocationParm location = {0};
	//GxTkgsOperatingMode operatingMode = GX_TKGS_CUSTOMIZABLE;

	//app_send_msg_exec(GXMSG_TKGS_SET_OPERATING_MODE, &operatingMode);
	//tkgs_location_test();
	ProgNode.node_type = NODE_PROG;
	ProgNode.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&ProgNode);

	tpNode.node_type = NODE_TP;
	tpNode.id = ProgNode.prog_data.tp_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tpNode);

	location.sat_id = tpNode.tp_data.sat_id;
	location.search_demux_id = 0;
	//location.search_diseqc = NULL;
	location.frequency = tpNode.tp_data.frequency;
	location.symbol_rate = tpNode.tp_data.tp_s.symbol_rate;
	location.polar = tpNode.tp_data.tp_s.polar;
	location.pid = 367;
	gx_TkgsSaveLocation(&location);

//	app_send_msg_exec(GXMSG_TKGS_START_VERSION_CHECK, &tp_id);
	app_send_msg_exec(GXMSG_TKGS_START_UPDATE, &update_parm);

}


void tkgs_prefered_list_print(GxTkgsPreferredList *list)
{
	int i;
	int j;

	if (list == NULL)
	{
		app_log_info("====have no prefered list\n");
	}

	app_log_flow("===prefered list num:%d\n", list->listNum);

	for (i=0; i<list->listNum; i++)
	{
		app_log_flow("=====prefered list[%d] name:%s, serviceNum :%d, servie is:\n", i, list->prefer[i].name, list->prefer[i].serviceNum);
		for(j=0; j<list->prefer[i].serviceNum; j++)
		{
			app_log_flow("[%d, %d, %d]", j, list->prefer[i].list[j].locator, list->prefer[i].list[j].lcn);
		}
		app_log_flow("\n");
	}
	app_log_flow("==end===\n");
}

void tkgs_get_prefered_list_test(void)
{
	GxTkgsPreferredList preferred_list = {0, NULL};

	GxBus_TkgsPreferredListGet(&preferred_list);
	tkgs_prefered_list_print(&preferred_list);
}
#endif
#endif
