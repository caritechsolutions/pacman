#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "module/config/gxconfig.h"
#include "gxupdate.h"
#include "app_utility.h"
#include <gxcore.h>
#include "module/channel_edit.h"
#include "app_windows.h"
#include <zlib.h>
#include "module/tkgs/gxtkgs.h"
#include "app_tkgs_upgrade.h"
#include "app_channel_info.h"
#if TKGS_SUPPORT

#define TKGS_DEF_PID	8181

typedef enum{
	TKGS_UPGRADE_START = 0,
	TKGS_UPGRADE_SUSPEND,
	TKGS_UPGRADE_UPDATE,
	TKGS_UPGRADE_STOPPED
}APP_TKGS_UPGRADE_STATUS;

#define TKGSUPGRADE_DEBUG 0

static APP_TKGS_UPGRADE_MODE curUpgradeMode = TKGS_UPGRADE_MODE_MENU;
static APP_TKGS_UPGRADE_STATUS s_tkgs_upgrade_state = TKGS_UPGRADE_STOPPED;
uint8_t s_tkgs_force_stop = 0;
uint8_t s_tkgs_saving_data = 0;
static uint8_t solt_flag = 1;
static bool s_tkgs_upgrade_finish_pop_status = false;

extern void app_tkgs_upgrade_menu_exec(void);

bool app_tkgs_get_upgrade_finish_pop_status(void)
{
	return s_tkgs_upgrade_finish_pop_status;
}
void app_tkgs_set_upgrade_mode(APP_TKGS_UPGRADE_MODE mode)
{
	curUpgradeMode = mode;
}
APP_TKGS_UPGRADE_MODE app_tkgs_get_upgrade_mode(void)
{
	return curUpgradeMode;
}
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);

void _tkgs_upgrade_set_solt_flag(void)
{
    //GxMsgProperty_NodeNumGet sat_num = {0};
    //GxMsgProperty_NodeByPosGet node_sat = {0};
	//uint32_t pos;
	//GxBusPmViewInfo view_info;
	//GxBusPmViewInfo view_info_bak;
	//GxMsgProperty_NodeNumGet node_num_get = {0};

	solt_flag = 1;
	if((curUpgradeMode == TKGS_UPGRADE_MODE_BACKGROUND)
		&& (app_tkgs_get_operatemode() == GX_TKGS_OFF))
	{
		solt_flag = 0;
	}
#if 0//TKGS_EX_FUN
	if (app_tkgs_get_operatemode() != GX_TKGS_AUTOMATIC)
	{
		sat_num.node_type = NODE_SAT;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
		for(pos = 0; pos<sat_num.node_num; pos++)
		{
			node_sat.node_type = NODE_SAT;
			node_sat.pos = pos;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
			if(0 == strncasecmp((const char*)node_sat.sat_data.sat_s.sat_name,"turksat",7))
			{
				break;
			}
		}

		if (pos == sat_num.node_num)
		{
			app_log_error("==%s, %d, err\n", __FUNCTION__, __LINE__);
			return;
		}

		app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
		view_info_bak = view_info;	//bak
		view_info.stream_type = GXBUS_PM_PROG_TV;
		view_info.group_mode = GROUP_MODE_SAT;
		view_info.sat_id = node_sat.sat_data.id;
		view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
		view_info.gx_pm_prog_check = NULL;
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

		node_num_get.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));

		//app_log_debug("======sat tv num:%d\n", node_num_get.node_num);
		if(node_num_get.node_num != 0)
		{
			solt_flag = 0;
		}

		//restore
		view_info = view_info_bak;
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));
	}
#endif
}

void _tkgsupgradeprocess_upgrade_version_check(uint16_t pid)
{
	GxTkgsUpdateParm update_parm = {-1, TKGS_DEF_PID, GX_TKGS_UPDATE_ALL};

	//solt_flag = 1;
	if(curUpgradeMode == TKGS_UPGRADE_MODE_BACKGROUND)
	{
		update_parm.tp_id = 1;
		if ((pid>0)&& (pid<0x1fff))
		{
			update_parm.pid = pid;
		}
		if(app_tkgs_get_operatemode() == GX_TKGS_OFF)
		{
			update_parm.type = GX_TKGS_UPDATE_LOCATION_AND_BLOCKING;
			//solt_flag = 0;
		}
	}
	else if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
	{
		update_parm.tp_id = -1;
		if(app_tkgs_get_operatemode() == GX_TKGS_OFF)
		{
			update_parm.type = GX_TKGS_UPDATE_LOCATION_AND_BLOCKING;
			//solt_flag = 0;
		}
	}

	app_send_msg_exec(GXMSG_TKGS_START_VERSION_CHECK,&update_parm);
}

void _tkgsupgradeprocess_upgrade_start(void)
{
	//int tp_id = -1;
	GxTkgsUpdateParm update_parm = {-1, TKGS_DEF_PID, GX_TKGS_UPDATE_ALL};

	//solt_flag = 1;
	if(curUpgradeMode == TKGS_UPGRADE_MODE_BACKGROUND)
	{
		update_parm.tp_id = 1;
		if(app_tkgs_get_operatemode() == GX_TKGS_OFF)
		{
			update_parm.type = GX_TKGS_UPDATE_LOCATION_AND_BLOCKING;
			//solt_flag = 0;
		}
	}
	else if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
	{
		update_parm.tp_id = -1;
		if(app_tkgs_get_operatemode() == GX_TKGS_OFF)
		{
			update_parm.type = GX_TKGS_UPDATE_LOCATION_AND_BLOCKING;
			//solt_flag = 0;
		}
	}
	_tkgs_upgrade_set_solt_flag();
	//app_log_debug("function= %s,line=%d,GXMSG_TKGS_START_UPDATE=%d\n",__FUNCTION__,__LINE__,GXMSG_TKGS_START_UPDATE);
	app_send_msg_exec(GXMSG_TKGS_START_UPDATE,&update_parm);
}

void _tkgsupgradeprocess_upgrade_stop(void)
{
			 if(s_tkgs_saving_data == 1)
                   {
                   GUI_SetProperty("txt_tkgs_upgrade_process_info1", "string", STR_ID_TKGS_SAVE_WAIT);
	GUI_SetProperty("btn_tkgs_upgrade_process_cancel", "state", "hide");
	GUI_SetProperty("img_tkgs_upgrade_process_tip", "state", "hide");
				}
           else
		{
		s_tkgs_force_stop = 1;
	//app_log_debug("=====%s, %d, s_tkgs_force_stop :0x%x\n", __FUNCTION__, __LINE__,s_tkgs_force_stop);
	app_send_msg_exec(GXMSG_TKGS_STOP,NULL);
}
}


int app_tkgs_set_upgrade_stop(void)
{
	if(s_tkgs_upgrade_state != TKGS_UPGRADE_STOPPED)
	{
	if(app_tkgs_get_upgrade_mode() != TKGS_UPGRADE_MODE_STANDBY && s_tkgs_force_stop == 0)
		_tkgsupgradeprocess_upgrade_stop();
		return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_tkgs_upgrade_process_create(GuiWidget * widget, void *usrdata)
{
    //time_t timeout = 80000;

	GUI_SetProperty("text_tkgs_upgrade_process_tip", "string", STR_ID_TKGS_UPGRATE);
	GUI_SetProperty("txt_tkgs_upgrade_process_info1", "string", STR_ID_TKGS_CHECK_VERSION);
	GUI_SetProperty("btn_tkgs_upgrade_process_cancel", "state", "show");

    	s_tkgs_upgrade_state = TKGS_UPGRADE_START;

    	_tkgsupgradeprocess_upgrade_version_check(0);

    return EVENT_TRANSFER_STOP;
}

static int _tkgs_reset_reboot_cb(PopDlgRet ret)
{
    s_tkgs_upgrade_finish_pop_status = false;
    app_reboot();
    return 0;
}

SIGNAL_HANDLER int app_tkgs_upgrade_process_destroy(GuiWidget *widget, void *usrdata)
{
	s_tkgs_upgrade_state = TKGS_UPGRADE_STOPPED;
	return EVENT_TRANSFER_STOP;
}

/**
 * @brief ????????LCN??????????TURK SAT
 *?????LCN
 * @param
 * @Return
 *second
 */
status_t app_tkg_lcn_flush(void)
{
#define USER_DEF_INIT_LCN 2001
	uint16_t temp_lcn = USER_DEF_INIT_LCN;
	uint32_t i, j;
	uint32_t tpye;
	GxBusPmViewInfo view_info;
	GxMsgProperty_NodeNumGet node_num_get = {0};
	GxMsgProperty_NodeByPosGet ProgNode = {0};
	AppProgExtInfo prog_ext_info = {{0}};
	uint32_t *prog_id  = NULL;//[SYS_MAX_PROG];
	uint32_t prog_num = 0;

    prog_id = GxCore_Malloc(4*SYS_MAX_PROG);
    if(NULL == prog_id)
        return -1;

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

	for (tpye=GXBUS_PM_PROG_TV; tpye<GXBUS_PM_PROG_RADIO+1; tpye++)
	{
		view_info.stream_type = tpye;
		view_info.group_mode = GROUP_MODE_ALL;//all
		view_info.taxis_mode = TAXIS_MODE_NON;
		view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        GxBus_PmViewInfoMemSet(&view_info);

		node_num_get.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
		for (i=0; i<2; i++)//??????LCN?locatorid?????????
		{
			for (j=0; j<node_num_get.node_num; j++)//??????
			{
				ProgNode.node_type = NODE_PROG;
				ProgNode.pos = j;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
				if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(ProgNode.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
				{
					if ((i==0) && (prog_ext_info.tkgs.logicnum == TKGS_CONFLICTING_LCN/*0XFFFE*/))
					{
						prog_id[prog_num++] = ProgNode.prog_data.id;
					}
					else if ((i==1) && (prog_ext_info.tkgs.locator_id == TKGS_INVALID_LOCATOR/*0x8000*/))
					{
						prog_id[prog_num++] = ProgNode.prog_data.id;
					}
				}
				else
				{
					if (i==1)//???????locatorid?????????????
                                   	{
						prog_id[prog_num++] = ProgNode.prog_data.id;
					}
				}
			}
		}
	}

	for (i = 0; i < prog_num; i++)
	{
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(prog_id[i],sizeof(AppProgExtInfo),&prog_ext_info))
		{
			if (prog_ext_info.tkgs.logicnum != temp_lcn)
			{
				prog_ext_info.tkgs.logicnum = temp_lcn;
				GxBus_PmProgExtInfoAdd(prog_id[i],sizeof(AppProgExtInfo),&prog_ext_info);
			}
			prog_id[i] = -1;
			temp_lcn++;
		}
	}

	for (i = 0; i < prog_num; i++)//????????
	{
		if (prog_id[i] != -1)
		{
			prog_ext_info.tkgs.logicnum = temp_lcn++;
			prog_ext_info.tkgs.locator_id = TKGS_INVALID_LOCATOR;
			GxBus_PmProgExtInfoAdd(prog_id[i],sizeof(AppProgExtInfo),&prog_ext_info);
		}
	}

	GxBus_PmSync(GXBUS_PM_SYNC_PROG);
	GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);

    GxBus_PmViewInfoMemClear();
    if(prog_id)
    {
        GxCore_Free(prog_id);
    }
	return GXCORE_SUCCESS;
}

void app_tkgs_sort_channels_by_lcn(void)
{
	GxBusPmViewInfo view_info;
	GxMsgProperty_NodeNumGet node_num_get = {0};
	uint32_t i=0;
	sort_mode sort_mode = SORT_BY_LCN;
	uint32_t focus = 0;

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

	for (i=GXBUS_PM_PROG_TV; i<GXBUS_PM_PROG_RADIO+1; i++)
	{
		view_info.stream_type = i;
		view_info.group_mode = GROUP_MODE_ALL;//all
		view_info.taxis_mode = TAXIS_MODE_NON;//2014.09.23 add
		view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        GxBus_PmViewInfoMemSet(&view_info);

		node_num_get.node_type = NODE_PROG;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
		if(node_num_get.node_num != 0)
		{
			g_AppChOps.create(node_num_get.node_num);
			g_AppChOps.sort(sort_mode, &focus);
			g_AppChOps.save();
		}
	}

    GxBus_PmViewInfoMemClear();//recovery flash viewinfo
	return;
}

static int  _tkgs_upgrade_finish_pop_cb(PopDlgRet ret)
{

	if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY && g_AppPlayOps.normal_play.play_total != 0)
	{
        s_tkgs_upgrade_finish_pop_status = false;
		char* focus_Window = (char*)GUI_GetFocusWindow();
		if ((0 == strcasecmp(WND_FULL_SCREEN ,focus_Window))||(0 == app_channel_list_compare_dialog(focus_Window))||(0 == app_channel_epg_compare_dialog(focus_Window)))
		{
			if(0 == app_channel_epg_compare_dialog(focus_Window))
				GUI_EndDialog(focus_Window);
			app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
		}
	}
	if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
	{
        s_tkgs_upgrade_finish_pop_status = false;
	    app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_BACKGROUND);
	}
	if(curUpgradeMode == TKGS_UPGRADE_MODE_FACTORY && s_tkgs_upgrade_state == TKGS_UPGRADE_STOPPED)
	{
		PopDlg  pop;
	    memset(&pop, 0, sizeof(PopDlg));
	    pop.type = POP_TYPE_NO_BTN;
	    pop.mode = POP_MODE_UNBLOCK;
	    pop.format = POP_FORMAT_DLG;
	    pop.timeout_sec = 2;
	    pop.str  = STR_ID_REBOOT_NOW;
	    pop.exit_cb = _tkgs_reset_reboot_cb;
	    popdlg_create(&pop);
	}


    return 0;
}

static int  _tkgs_upgrade_save_ver_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        s_tkgs_upgrade_finish_pop_status = false;
        GUI_SetInterface("flush", NULL);//0125, add
        GUI_SetProperty("txt_tkgs_upgrade_process_info1", "string", STR_ID_TKGS_UPGRADE_WAIT);
        s_tkgs_upgrade_state = TKGS_UPGRADE_UPDATE;
        _tkgsupgradeprocess_upgrade_start();
    }
    else
    {
        s_tkgs_upgrade_finish_pop_status = false;
        s_tkgs_upgrade_state = TKGS_UPGRADE_SUSPEND;
        _tkgsupgradeprocess_upgrade_stop();
    }
    return 0;
}

static int  _tkgs_upgrade_update_ok_cb(PopDlgRet ret)
{
	int sel = 0;
    if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY && g_AppPlayOps.normal_play.play_total != 0)
    {
        s_tkgs_upgrade_finish_pop_status = false;
        char* focus_Window = (char*)GUI_GetFocusWindow();
        if ((0 == strcasecmp(WND_FULL_SCREEN ,focus_Window))||(0 == app_channel_list_compare_dialog(focus_Window))||(0 == app_channel_epg_compare_dialog(focus_Window)))
        {
            if(0 == app_channel_epg_compare_dialog(focus_Window))
                GUI_EndDialog(focus_Window);
            app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
        }
    }
    else if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
    {
        s_tkgs_upgrade_finish_pop_status = false;
	    app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_BACKGROUND);
    }
    else if(curUpgradeMode == TKGS_UPGRADE_MODE_FACTORY && s_tkgs_upgrade_state == TKGS_UPGRADE_STOPPED)
    {
		PopDlg  pop;
	    memset(&pop, 0, sizeof(PopDlg));
	    pop.type = POP_TYPE_NO_BTN;
	    pop.mode = POP_MODE_UNBLOCK;
	    pop.format = POP_FORMAT_DLG;
	    pop.timeout_sec = 2;
	    pop.str  = STR_ID_REBOOT_NOW;
	    pop.exit_cb = _tkgs_reset_reboot_cb;
	    popdlg_create(&pop);
        return 0;
    }
    g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
    s_tkgs_upgrade_state = TKGS_UPGRADE_STOPPED;
    if(curUpgradeMode == TKGS_UPGRADE_MODE_BACKGROUND)
    {
        //app_log_info("=====%s, %d, startPmt\n", __FUNCTION__, __LINE__);
        if(GUI_CheckDialog(WND_CHANNEL_LIST) == GXCORE_SUCCESS)
        {
            extern int app_channel_list_get_cur_list_sel(void);
            sel = app_channel_list_get_cur_list_sel();
            GUI_SetProperty("listview_channel_list", "update_all", NULL);
            GUI_SetProperty(WND_CHANNEL_LIST, "update_all", NULL);
            GUI_SetProperty("listview_channel_list", "select", &sel);
            GUI_SetProperty("listview_channel_list", "active", &sel);
        }
        else if(GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
        {
            extern void macro_ch_ed_stop_video(void);
            macro_ch_ed_stop_video();
            GUI_EndDialog(WND_CHANNEL_EDIT);
            GUI_CreateDialog(WND_CHANNEL_EDIT);
        }
        else if(GUI_CheckDialog(WND_EPG) == GXCORE_SUCCESS)
        {
            GUI_SetProperty("listview_epg_prog", "update_all", NULL);
            GUI_SetProperty(WND_EPG, "update_all", NULL);
            sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
            GUI_SetProperty("listview_epg_prog", "select", &sel);
            GUI_SetProperty("listview_epg_prog", "active", &sel);
        }

        g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
        g_AppPmt.start(&g_AppPmt);
    }
    else if(curUpgradeMode == TKGS_UPGRADE_MODE_MENU)
    {
        GUI_EndDialog(WND_SYSTEM_SETTING);
        app_tkgs_upgrade_menu_exec();
    }

    return 0;
}

int app_tkgs_upgrade_process_service(GxMessage* pMsg)
{
	int ret = EVENT_TRANSFER_KEEPON;
	//GUI_Event *event = NULL;
	GxTkgsUpdateMsg *pTkgsUpgradeMsg;

	if(pMsg->msg_id == GXMSG_TKGS_UPDATE_STATUS)
	{
		pTkgsUpgradeMsg = (GxTkgsUpdateMsg*)GxBus_GetMsgPropertyPtr(pMsg,
								GxTkgsUpdateMsg);
		//app_log_debug("status = %d\n",pTkgsUpgradeMsg->status);
		switch(pTkgsUpgradeMsg->status)
		{
			case GX_TKGS_FOUND_NEW_VER:
				//app_log_info("=======found new ver\n");
				if(curUpgradeMode != TKGS_UPGRADE_MODE_BACKGROUND)
				{
					GUI_SetProperty("txt_tkgs_upgrade_process_info1", "string", STR_ID_TKGS_UPGRADE_WAIT);
				}
				s_tkgs_upgrade_state = TKGS_UPGRADE_UPDATE;
				_tkgsupgradeprocess_upgrade_start();
				break;
			case GX_TKGS_FOUND_SAME_VER:
				{
				    //app_log_info("=======found same ver\n");
					if(curUpgradeMode == TKGS_UPGRADE_MODE_BACKGROUND)
					{
						s_tkgs_upgrade_state = TKGS_UPGRADE_SUSPEND;
						_tkgsupgradeprocess_upgrade_stop();
						break;
					}
					else if (curUpgradeMode == TKGS_UPGRADE_MODE_FACTORY)
					{
						s_tkgs_upgrade_state = TKGS_UPGRADE_UPDATE;
						_tkgsupgradeprocess_upgrade_start();
					}
					else if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
						{
	//app_log_info("=====%s, %d, _tkgsupgradeprocess_upgrade_stop3\n", __FUNCTION__, __LINE__);
							_tkgsupgradeprocess_upgrade_stop();
						s_tkgs_upgrade_state = TKGS_UPGRADE_STOPPED;
							app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_BACKGROUND);
					              if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS))
					               {
						          GUI_EndDialog(WND_TKGS_UPGRADE_PROCESS);
					                }
							{
					char* focus_Window = (char*)GUI_GetFocusWindow();
					if ((0 == strcasecmp(WND_FULL_SCREEN ,focus_Window))||(0 == app_channel_list_compare_dialog(focus_Window))||(0 == app_channel_epg_compare_dialog(focus_Window)))
						{
						if(0 == app_channel_epg_compare_dialog(focus_Window))
							GUI_EndDialog(focus_Window);
					     app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
						}
					}
					              ret = EVENT_TRANSFER_STOP;
						}
					else
					{
						PopDlg  pop;
						s_tkgs_upgrade_finish_pop_status = true;
						memset(&pop, 0, sizeof(PopDlg));
						pop.type = POP_TYPE_YES_NO;
						pop.mode = POP_MODE_UNBLOCK;//0125, add
						pop.format = POP_FORMAT_DLG;
						pop.str = STR_ID_TKGS_SAME_Version;
						s_tkgs_upgrade_state = TKGS_UPGRADE_SUSPEND;
						pop.show_time = FALSE;
						//pop.timeout_sec =  8;
						if ( APP_THEME_CLASSIC_BLUE || APP_THEME_CLASSIC_DARK ){
							pop.pos.x = APP_WND_TKGS_UPGD_POP_POS_X;
							pop.pos.y = APP_WND_TKGS_UPGD_POP_POS_Y;
						}
                        pop.exit_cb = _tkgs_upgrade_save_ver_cb;
						pop.default_ret = POP_VAL_CANCEL;
						popdlg_create(&pop);
					}
				}
				break;
			case GX_TKGS_UPDATE_OK:
				{
                    if((curUpgradeMode == TKGS_UPGRADE_MODE_BACKGROUND||curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
						&& (app_tkgs_get_operatemode() == GX_TKGS_OFF))
					{
					 if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
                                         app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_BACKGROUND);
						break;
					}

					PopDlg  pop;

					app_tkg_lcn_flush();
					if(1 == solt_flag)
					{
						app_tkgs_sort_channels_by_lcn();
					}

					if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS))
					{
						GUI_EndDialog(WND_TKGS_UPGRADE_PROCESS);
					}
					s_tkgs_upgrade_finish_pop_status = true;
					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_OK;
					pop.mode = POP_MODE_UNBLOCK;//0125 add
                    pop.format = POP_FORMAT_DLG;
					if( APP_THEME_CLASSIC_BLUE || APP_THEME_CLASSIC_DARK ){
						pop.pos.x = APP_WND_TKGS_UPGD_POP_POS_X;
						pop.pos.y = APP_WND_TKGS_UPGD_POP_POS_Y;
					}
					if(strlen(pTkgsUpgradeMsg->message) > 0)
						pop.str = pTkgsUpgradeMsg->message;
					else
						pop.str = STR_ID_UPDATE_SUCCESS;
					pop.show_time = FALSE;
					if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
					{
						pop.timeout_sec = 60;
					}
					else if(curUpgradeMode == TKGS_UPGRADE_MODE_FACTORY)
					{
						pop.timeout_sec = 2;
					}
                    else
					{
					    pop.timeout_sec = 30;
					}
                    pop.exit_cb = _tkgs_upgrade_update_ok_cb;
					popdlg_create(&pop);
                }
				break;

			case GX_TKGS_UPDATE_CANCEL:
			if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS))
			{
				GUI_EndDialog(WND_TKGS_UPGRADE_PROCESS);
			}
			/*
			if(curUpgradeMode == TKGS_UPGRADE_MODE_BACKGROUND)
			{
				g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
				g_AppPmt.start(&g_AppPmt);
			}
			*/
			if(curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
                      app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_BACKGROUND);
			s_tkgs_upgrade_state = TKGS_UPGRADE_STOPPED;
			break;

			case GX_TKGS_CHECK_VER_TIMEOUT:
			case GX_TKGS_UPDATE_TIMEOUT:

			case GX_TKGS_UPDATE_ERROR:
				s_tkgs_upgrade_state = TKGS_UPGRADE_STOPPED;
				if(curUpgradeMode == TKGS_UPGRADE_MODE_BACKGROUND)
				{
					g_AppPmt.ts_src = g_AppPlayOps.normal_play.ts_src;
					g_AppPmt.start(&g_AppPmt);
					//app_log_info("=====%s, %d, startPmt\n", __FUNCTION__, __LINE__);
					break;
				}
                            if((curUpgradeMode == TKGS_UPGRADE_MODE_STANDBY)
						&& (app_tkgs_get_operatemode() == GX_TKGS_OFF))
					{
                                         app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_BACKGROUND);
						break;
					}
				{
					PopDlg  pop;

					if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS))
					{
						GUI_EndDialog(WND_TKGS_UPGRADE_PROCESS);
					}
					s_tkgs_upgrade_finish_pop_status = true;
					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_OK;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.format = POP_FORMAT_DLG;
					if( APP_THEME_CLASSIC_BLUE || APP_THEME_CLASSIC_DARK ){
						pop.pos.x = APP_WND_TKGS_UPGD_POP_POS_X;
						pop.pos.y = APP_WND_TKGS_UPGD_POP_POS_Y;
					}
					pop.str = STR_ID_TKGS_UPGRADE_FAIL;
					pop.show_time = FALSE;
					pop.timeout_sec = 5;
					pop.exit_cb = _tkgs_upgrade_finish_pop_cb;
					popdlg_create(&pop);
				}
				break;
			default:
				break;
		}
	}

	return ret;
}

static int  _exit_tkgs_upgrade_pop_cb(PopDlgRet ret)
{
    if (POP_VAL_OK == ret)
    {
		_tkgsupgradeprocess_upgrade_stop();
    }
    else
    {
       	//do nothing
    }

	s_tkgs_upgrade_finish_pop_status = false;

    return 0;
}

SIGNAL_HANDLER int app_tkgs_upgrade_process_keypress(GuiWidget* widgetname, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
        case STBK_OK:
		  case STBK_EXIT:
		   	if(s_tkgs_upgrade_state == TKGS_UPGRADE_START||(s_tkgs_upgrade_state == TKGS_UPGRADE_UPDATE&&s_tkgs_saving_data ==0))
			{
				if(curUpgradeMode != TKGS_UPGRADE_MODE_FACTORY)
				{
#if 1
					PopDlg  pop;
                    s_tkgs_upgrade_finish_pop_status = true;
					memset(&pop, 0, sizeof(PopDlg));
					if( APP_THEME_CLASSIC_BLUE || APP_THEME_CLASSIC_DARK ){
						pop.pos.x = APP_WND_TKGS_UPGD_POP_POS_X;
						pop.pos.y = APP_WND_TKGS_UPGD_POP_POS_Y;
					}
					pop.type = POP_TYPE_YES_NO;
                	pop.format = POP_FORMAT_DLG;
                	pop.mode = POP_MODE_UNBLOCK;
                	pop.str = STR_ID_TKGS_ENSURE_EXIT;
                	pop.exit_cb = _exit_tkgs_upgrade_pop_cb;
					popdlg_create(&pop);
#else
					PopDlg  pop;
                    s_tkgs_upgrade_finish_pop_status = true;
					memset(&pop, 0, sizeof(PopDlg));
					pop.pos.x = APP_WND_TKGS_UPGD_POP_POS_X;
					pop.pos.y = APP_WND_TKGS_UPGD_POP_POS_Y;
					pop.type = POP_TYPE_YES_NO;
					pop.mode = POP_MODE_UNBLOCK;
					pop.format = POP_FORMAT_DLG;
					pop.str = STR_ID_TKGS_ENSURE_EXIT;
					app_log_debug("@@@@@@pop.mode:%d\n", pop.mode);
					if(popdlg_create(&pop) == POP_VAL_OK)
					{
						app_log_info("~~~~~~~~~~exit key ok\n");
						s_tkgs_upgrade_state = TKGS_UPGRADE_STOPPED;
						//GUI_EndDialog(WND_TKGS_UPGRADE_PROCESS);
						_tkgsupgradeprocess_upgrade_stop();
					}
					else
					{
						app_log_info("~~~~~~~~~~~~exit key cancel\n");
					}
#endif
				}
			}
			else if(s_tkgs_saving_data == 1)
                   {
                   GUI_SetProperty("txt_tkgs_upgrade_process_info1", "string", STR_ID_TKGS_SAVE_WAIT);
	GUI_SetProperty("btn_tkgs_upgrade_process_cancel", "state", "hide");
	GUI_SetProperty("img_tkgs_upgrade_process_tip", "state", "hide");

				}
		   	break;
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}


int app_tkgs_upgrade_process_menu_exec(void)
{
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS))
	{
		GUI_EndDialog(WND_TKGS_UPGRADE_PROCESS);
	}
	GUI_CreateDialog(WND_TKGS_UPGRADE_PROCESS);

	return 0;
}
#endif

