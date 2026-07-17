/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	cascam_ui_porting.c
* Author    :	zhangyg
* Project   :	CA Menu
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  2.0	   2018.11.07		 zhangyg			 Creation
*****************************************************************************/
#include "app.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_wnd_search.h"
#if CASCAM_SUPPORT
//#include "app_config.h"
#include "cascam_ui_porting.h"
#include "cascam_ui_common.h"
#include "cascam.h"
#include "module/hdmi/gxhdmi_module.h"
#include "full_screen.h"
#if (DEMOD_DVB_S > 0)
#include "app_wnd_search.h"
#endif
static uint8_t g_playback_type = CASCAM_PVR_TYPE_REC_PLAY;

//#define WND_MAIN_MENU		"wnd_main_menu"
//#define WND_EPG				"wnd_epg"
//#define WND_CHANNEL_EDIT	"wnd_channel_edit"
//#define WND_CHANNEL_INFO_SIGNAL	"win_channel_info_signal"
//#define WND_CHANNEL_INFO		"win_channel_info"
//#define WND_CHANNEL_LIST	"win_channel_list"

#define CASCAM_HOUR_TO_SEC         3600
#define CASCAM_HALF_HOUR_TO_SEC    1800

static GX_LIST_HEAD(program_tp_head);

typedef struct{
	struct gxlist_head list;
	uint32_t tp_id;
}ProgramTpNode;

static int32_t tp_add(ProgramTpNode* node)
{
	ProgramTpNode* pos = NULL, *n = NULL;

	gxlist_for_each_entry_safe(pos, n, &program_tp_head, list) {
		if (pos->tp_id == node->tp_id) {
			GxCore_Free(node);
			return -1;
		}
	}
	gxlist_add_tail(&node->list, &program_tp_head);

	return 0;
}

void cascam_ui_get_timezone(double* p_time_zone)
{
    int init_value = TIME_ZONE_VALUE;
    GxBus_ConfigGetInt(TIME_ZONE, &init_value, TIME_ZONE_VALUE);
    if(init_value < -12)
        init_value = -12;
    if(init_value > 12)
        init_value = 12;
	*p_time_zone = init_value;
}

void cascam_ui_get_timezome_detail(double *zone, int *half_zone, int *summer)
{
	int init_value = 0;
	cascam_ui_get_timezone(zone);

	GxBus_ConfigGetInt(TIME_HALF_ZONE, &init_value, TIME_HALF_ZONE_VALUE);
	if (half_zone)
		*half_zone = init_value;

	GxBus_ConfigGetInt(TIME_SUMMER, &init_value, TIME_SUMMER_VALUE);
	if (summer)
		*summer = init_value;
}

time_t cascam_ui_get_display_time_by_timezone(time_t src_sec)
{
	time_t ret = src_sec;
	double zone = 0;
	int half_zone = 0, summer = 0;
	cascam_ui_get_timezome_detail(&zone, &half_zone, &summer);

	ret += zone * CASCAM_HOUR_TO_SEC;
	ret += half_zone * CASCAM_HALF_HOUR_TO_SEC;
	ret += summer * CASCAM_HOUR_TO_SEC;
	return ret;
}

void cascam_ui_exit_to_fullscreen(void)
{
    GUI_EndDialog("after wnd_full_screen");
}

void cascam_ui_program_play_by_pos(uint32_t pos)
{
	g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, pos);
}

void cascam_ui_program_play(void)
{
	g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, g_AppPlayOps.current_play.play_count);
}

void cascam_ui_program_stop(void)
{
	g_AppPlayOps.program_stop();
}

extern void app_play_set_tp(GxMsgProperty_NodeByIdGet sat_get,
		GxMsgProperty_NodeByIdGet tp_get,
		GxMsgProperty_NodeByPosGet prog_get);
extern void app_play_set_diseqc(uint32_t sat_id, uint16_t tp_id, bool ForceFlag);
void cascam_ui_watch_limit_set_tp(void)
{
	GxMsgProperty_NodeByPosGet prog_get = {0};
	GxMsgProperty_NodeByIdGet tp_get = {0};
	GxMsgProperty_NodeByIdGet sat_get = {0};

	prog_get.node_type = NODE_PROG;
	prog_get.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_get));
	tp_get.node_type = NODE_TP;
	tp_get.id = prog_get.prog_data.tp_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);
	sat_get.node_type = NODE_SAT;
	sat_get.id = prog_get.prog_data.sat_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_get);
	if(sat_get.sat_data.type == GXBUS_PM_SAT_S)
		app_play_set_diseqc(prog_get.prog_data.sat_id,prog_get.prog_data.tp_id,true);
	app_play_set_tp(sat_get, tp_get, prog_get);

}

void cascam_ui_ippv_notify_before_create_dialog(void)
{
    if((1 == cascam_ui_check_window_exist(WND_CHANNEL_INFO, CASCAM_WINDOW_WIN)))
    {
        GUI_EndDialog(WND_CHANNEL_INFO);
    }
    if((1 == cascam_ui_check_window_exist(WND_CHANNEL_INFO_SIGNAL, CASCAM_WINDOW_WIN)))
    {
        GUI_EndDialog(WND_CHANNEL_INFO_SIGNAL);
    }
}

// for quick switch record ecmpid
void cascam_ui_cur_ecm_pid_notify(void* para)
{
	return;
	GxMsgProperty_NodeByPosGet prog_get = {0};
	CascamCurEcmPid* p;
	if(NULL == para)
	{
		printf("%s,%d, data is NULL!", __func__, __LINE__);
		return ;
	}
	p = (CascamCurEcmPid*)para;
	printf("[UI MSG] CUR ECM PID!service_id = %d, audio_ecmpid = %d, video_ecmpid = %d\n",
		p->service_id, p->audio_ecm_pid, p->video_ecm_pid);
	prog_get.node_type = NODE_PROG;
	prog_get.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_get));
	if(prog_get.prog_data.service_id == p->service_id){
		prog_get.prog_data.cur_audio_ecm_pid = p->audio_ecm_pid;
		prog_get.prog_data.ecm_pid_v = p->video_ecm_pid;
		app_send_msg_exec(GXMSG_PM_NODE_MODIFY, (void *)(&prog_get));
	}
}

uint8_t cascam_ui_check_cur_ecmpid(uint16_t ecm_pid)
{

	return 1;
}

//for fullscreen eg. finger hide
int32_t cascam_ui_check_notice_hide_by_video(void)
{
//for conax
	if((1 == cascam_ui_check_window_exist(WND_MAIN_MENU, CASCAM_WINDOW_WIN))
		&&((1 != cascam_ui_check_window_exist(WND_EPG, CASCAM_WINDOW_WIN))
			&& (1 != cascam_ui_check_window_exist(WND_CHANNEL_EDIT, CASCAM_WINDOW_WIN))
			&& (1 != cascam_ui_check_window_exist(WND_CHANNEL_LIST, CASCAM_WINDOW_WIN))))
		return 1;
	return 0;
}

int32_t cascam_ui_check_notice_hide_by_video_use_gui(void)
{
//for conax
	if((GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
		&&((GXCORE_SUCCESS != GUI_CheckDialog(WND_EPG))
			&& (GXCORE_SUCCESS != GUI_CheckDialog(WND_CHANNEL_EDIT))
			&& (GXCORE_SUCCESS != GUI_CheckDialog(WND_CHANNEL_LIST))))
		return 1;
	return 0;
}

//for fullscreen notice hide
int32_t cascam_ui_check_notice_hide(void)
{
	if(1 == cascam_ui_check_window_exist(WND_MAIN_MENU, CASCAM_WINDOW_WIN))
		return 1;
	return 0;
}
//for stop descramble
int32_t cascam_ui_stop_descramble_check(void)
{
	//if(1 == cascam_ui_check_window_exist(WND_MAIN_MENU, CASCAM_WINDOW_WIN))
		return 1;
	return 0;
}
//for ca pop msg create except fullscreen
int32_t cascam_ui_check_popmsg_create_postion(uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
{
	//channel edit
	//channel list
	//epg
	return 0;
}
//for ca osd rolling create except fullscreen
int32_t cascam_ui_check_osd_rolling_create_postion(uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
{
	//channel edit
	//channel list
	//epg
	return 0;
}

//for super finger hide  return 1  is hide
int32_t cascam_ui_check_super_finger_hide(void)
{
	//for conax
	if((1 == cascam_ui_check_window_exist(WND_MAIN_MENU, CASCAM_WINDOW_WIN))
		&&((1 != cascam_ui_check_window_exist(WND_EPG, CASCAM_WINDOW_WIN))
			&& (1 != cascam_ui_check_window_exist(WND_CHANNEL_EDIT, CASCAM_WINDOW_WIN))
			&& (1 != cascam_ui_check_window_exist(WND_CHANNEL_LIST, CASCAM_WINDOW_WIN))))
		return 1;
	return 0;
}
//for super osd hide  return 1  is hide
int32_t cascam_ui_check_super_osd_hide(void)
{
	return 0;
}


//for ca action
void cascam_ui_action_research(void)
{
	GxBusPmDataProg prog;
	int16_t i;
	int32_t prog_count;
	int32_t ret = 0;
	GxBusPmViewInfo sys;
	GxBusPmDataTP tp;
	SearchIdSet search_id;
	ProgramTpNode* pos = NULL, *n = NULL;


    GxBus_PmViewInfoGet(&sys);

    sys.stream_type = GXBUS_PM_PROG_ALL;
    sys.group_mode = GROUP_MODE_ALL;

    GxBus_PmViewInfoMemSet(&sys);
    prog_count = GxBus_PmProgNumGet();
	search_id.id_num = 0;

	for(i = 0; i < prog_count; i++)
	{
		GxBus_PmProgGetByPos(i, 1, &prog);
		GxBus_PmTpGetById(prog.tp_id, &tp);

		ProgramTpNode* pnode = GxCore_Mallocz(sizeof(ProgramTpNode));
		pnode->tp_id = tp.id;

		ret = tp_add(pnode);
		if( ret == 0)
			search_id.id_num++;
		else
			printf("same do not add\n");
	}

	search_id.id_array = (uint32_t *)GxCore_Calloc(search_id.id_num, sizeof(uint32_t));
	if(search_id.id_array != NULL)
	{
	    i = 0;
		gxlist_for_each_entry_safe(pos, n, &program_tp_head, list) {
			search_id.id_array[i] = pos->tp_id;
			i++;
		}

#if (DEMOD_DVB_S > 0)
		if(app_search_opt_dlg(SEARCH_TP, &search_id) == WND_OK)
		{
			GUI_SetFocusWidget("list_tp_list");
			app_search_start();
		}
#endif

		GxCore_Free(search_id.id_array);
		gxlist_for_each_entry_safe(pos, n, &program_tp_head, list) {
			gxlist_del(&pos->list);
			GxCore_Free(pos);
		}
	}
	return;

}

extern void app_sysinfo_menu_exec(void);
extern void app_system_reset(void);
void cascam_ui_action_show_system_info(void)
{
	app_sysinfo_menu_exec();
	return;
}

void cascam_ui_action_factory_reset(void)
{
	GUI_SetInterface("flush",NULL);
	app_system_reset();
	GxCore_ThreadDelay(2000);
	app_reboot();
	return;
}

void cascam_ui_action_reset(void)
{
	app_reboot();
	return;
}

typedef enum
{
	URI_OUTPUT_CVBS_CGMS = 0,			//cvbs cgms
	URI_OUTPUT_CVBS_MACROVISION = 1,	//cvbs macrovision
	URI_OUTPUT_HDMI_HDCP = 2,			//hdmi hdcp

	URI_OUTPUT_MAX

}CASCAM_URI_OUTPUT_e;

int cascam_ui_action_uri_ctrl(CASCAM_URI_OUTPUT_e output, uint8_t on_off)
{
#if HDMI_HDCP_SUPPORT
	static uint8_t hdcp_status_record = 1;//Default HDCP on.
#else
	static uint8_t hdcp_status_record = 0;
#endif

	//printf("[%s] output:%d, switch:%d\n", __func__, output, on_off);

	if(output >= URI_OUTPUT_MAX)
	{
		printf("[%s] params error.\n", __func__);
		return -1;
	}

	switch(output)
	{
		case URI_OUTPUT_CVBS_CGMS:
#if 0
			{
				handle_t dev = 0, mod_vout;
				GxVideoOutProperty_CGMSEnable cgms_enable;
				GxVideoOutProperty_CGMSConfig cgms_config;
				GxStatus ret  = -1;

				dev = GxAvdev_CreateDevice(0);
				if(dev <= 0){
					printf("[%s] dev create error.\n", __func__);
					return -1;
				}
				mod_vout = GxAvdev_OpenModule(dev, GXAV_MOD_VIDEO_OUT, 0);
				if(mod_vout <= 0){
					printf("[%s] vout mod open error.\n", __func__);
					if(dev) GxAvdev_DestroyDevice(0);
					return -1;
				}

				//refers to CEA-805-C and CONAX requirements.
				memset(&cgms_config, 0, sizeof(GxVideoOutProperty_CGMSConfig));
				cgms_config.CGMS_A_control = (on_off)? VOUT_CGMS_A_COPY_FORBIDDEN : VOUT_CGMS_A_COPY_PERMITTED;
				cgms_config.ASP_control = on_off;
				cgms_config.ASB_control = 0x0;
				ret = GxAVSetProperty(dev, mod_vout, GxVideoOutPropertyID_CGMSConfig, (void*)&cgms_config, sizeof(GxVideoOutProperty_CGMSConfig));
				if (GXCORE_SUCCESS != ret){
					printf("[%s], GxVideoOutPropertyID_CGMSConfig failed, ret=%d\n", __func__, ret);
					if(dev & mod_vout)GxAvdev_CloseModule(dev, mod_vout);
					if(dev) GxAvdev_DestroyDevice(0);
					return -1;
				}

				memset(&cgms_enable, 0, sizeof(GxVideoOutProperty_CGMSEnable));
				cgms_enable.enable = (on_off)? 1 : 0;
				ret = GxAVSetProperty(dev, mod_vout, GxVideoOutPropertyID_CGMSEnable, (void*)&cgms_enable, sizeof(GxVideoOutProperty_CGMSEnable));
				if (GXCORE_SUCCESS != ret){
					printf("[%s], GxVideoOutPropertyID_CGMSEnable failed, ret=%d\n", __func__, ret);
					if(dev & mod_vout)GxAvdev_CloseModule(dev, mod_vout);
					if(dev) GxAvdev_DestroyDevice(0);
					return -1;
				}
				else{
					if(dev & mod_vout)GxAvdev_CloseModule(dev, mod_vout);
					if(dev) GxAvdev_DestroyDevice(0);
					//printf("[%s], cvbs cgms: %s.\n", __func__, (on_off)? "on":"off");
				}
			}
#endif
			break;
		case URI_OUTPUT_CVBS_MACROVISION:
			printf("[%s] not support cvbs macrovision.\n", __func__);
			break;
		case URI_OUTPUT_HDMI_HDCP:
			{
				if(hdcp_status_record == on_off){
					//Do nothing.
				}
				else{
					int ret = 0;
					//int ret = GxHdmi_SetHdcpEnable(on_off);
					hdcp_status_record = on_off;
					printf("[%s], hdmi hdcp: %s, ret = %d\n", __func__, (on_off)? "on":"off", ret);
				}
				printf("every time[%s], hdmi hdcp: %s\n", __func__, (on_off)? "on":"off");
			}
			break;
		default:
			printf("[%s] unkown error.\n", __func__);
			return -1;
			break;
	}

	return 0;
}
void cascam_ui_action_request(uint16_t uri_value)
{
	uint8_t aps_copy_ctrl = 0;
	uint8_t emi_copy_ctrl = 0;
	uint8_t image_constraint = 0;
	uint8_t redistribution_ctrl = 0;
	uint8_t retention_limit = 0;
	uint8_t trick_play_ctrl = 0;
	uint8_t maturity_rating = 0;
	uint8_t analog_disable = 0;
#define _CONAX_URI_APS_COPY_CTRL(u16Val)       ((u16Val >> 14) & 0x3)
#define _CONAX_URI_EMI_COPY_CTRL(u16Val)       ((u16Val >> 12) & 0x3)
#define _CONAX_URI_IMAGE_CONSTRAINT(u16Val)    ((u16Val >> 11) & 0x1)
#define _CONAX_URI_REDISTRIBUTION_CTRL(u16Val) ((u16Val >> 10) & 0x1)
#define _CONAX_URI_RETENTION_LIMIT(u16Val)     ((u16Val >> 7) & 0x7)
#define _CONAX_URI_TRICK_PLAY_CTRL(u16Val)     ((u16Val >> 4) & 0x7)
#define _CONAX_URI_MATURITY_RATING(u16Val)     ((u16Val >> 1) & 0x7)
#define _CONAX_URI_ANALOG_DISABLE(u16Val)      ((u16Val >> 0) & 0x1)

	(void)retention_limit;
	(void)trick_play_ctrl;
	(void)maturity_rating;
	aps_copy_ctrl = _CONAX_URI_APS_COPY_CTRL(uri_value);
	emi_copy_ctrl = _CONAX_URI_EMI_COPY_CTRL(uri_value);
	image_constraint = _CONAX_URI_IMAGE_CONSTRAINT(uri_value);
	redistribution_ctrl = _CONAX_URI_REDISTRIBUTION_CTRL(uri_value);
	retention_limit = _CONAX_URI_RETENTION_LIMIT(uri_value);
	trick_play_ctrl = _CONAX_URI_TRICK_PLAY_CTRL(uri_value);
	maturity_rating = _CONAX_URI_MATURITY_RATING(uri_value);
	analog_disable = _CONAX_URI_ANALOG_DISABLE(uri_value);

	// HDCP is required
	if(emi_copy_ctrl != 0){
		cascam_ui_action_uri_ctrl(URI_OUTPUT_HDMI_HDCP, /*emi_copy_ctrl*/1);
	}
	else if(emi_copy_ctrl == 0 && redistribution_ctrl == 1){
		cascam_ui_action_uri_ctrl(URI_OUTPUT_HDMI_HDCP, 1);
	}
	else{
		cascam_ui_action_uri_ctrl(URI_OUTPUT_HDMI_HDCP, 0);
	}

	//CVBS downscale, not support
	if(image_constraint == 1){//image constraint required < 520K pixels (analog output)
		//Do nothing.
	}

	// CGMS is required
#if 0
	if(aps_copy_ctrl > 0){
		cascam_ui_action_uri_ctrl(URI_OUTPUT_CVBS_CGMS, /*aps_copy_ctrl*/1);
	}
	else{
		cascam_ui_action_uri_ctrl(URI_OUTPUT_CVBS_CGMS, 0);
	}

	if(analog_disable){//analog output disable
#else
	if(analog_disable || aps_copy_ctrl > 0){//analog output disable
#endif
		VideoOutPower cvbs_power = {GX_VIDEO_OUTPUT_RCA, 0};
		GxPlayer_SetVideoOutputPower2(cvbs_power);
	}else{
		VideoOutPower cvbs_power = {GX_VIDEO_OUTPUT_RCA, 1};
		GxPlayer_SetVideoOutputPower2(cvbs_power);
	}

	return;
}

void cascam_ui_get_oem_hardware_version(uint32_t *h_version)
{
	*h_version = 0xE1000011;
}

void cascam_ui_get_oem_software_version(uint32_t *s_version)
{
	*s_version = 0x00000011;
}

uint16_t cascam_ui_get_play_state(void)
{
	uint16_t ret = 0;

	ret = 1;
	return ret;
}

uint16_t cascam_ui_check_pvr_status(void)
{
	if(g_AppPvrOps.state == PVR_DUMMY)
		return 0;
	else
		return 1;
}

uint16_t cascam_ui_get_cur_serviceid(void)
{
	GxMsgProperty_NodeByPosGet prog_get = {0};

	prog_get.node_type = NODE_PROG;
	prog_get.pos = g_AppPlayOps.normal_play.play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_get));

	return prog_get.prog_data.service_id;
}

void cascam_ui_stop_pvr(void)
{
	app_pvr_stop();
}

void cascam_ui_get_app_password(char *buf)
{
	uint8_t i = 0;
	uint8_t length = 4;//default
	for(i = 0; i < length; i++){
		buf[i] = '0';
	}
}
//this function used for gui thread
int32_t cascam_ui_check_video_window(void)
{
	char *focus_win = NULL;
	focus_win = (char*)GUI_GetFocusWindow();
	if(0 == strcasecmp(focus_win, "wnd_full_screen")
			|| 0 == strcasecmp(focus_win, "wnd_channel_list")
			|| 0 == strcasecmp(focus_win, "wnd_epg")
			|| 0 == strcasecmp(focus_win, "wnd_channel_info")){
		return 1;
	}
	return 0;
}

int32_t cascam_ui_check_movie_view_window(void)
{
	if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
		return 1;
	return 0;
}

void cascam_ui_send_event_to_focus_win(GUI_Event *event)
{
	const char *parent = NULL;
	parent = GUI_GetFocusWidget();
	if(parent != NULL)
	{
		GUI_SendEvent(parent, event);
	}

}

time_t cascam_ui_get_tick_time(void)
{
	GxTime time = {0};
	GxCore_GetTickTime(&time);
	return time.seconds;
}

void cascam_ui_set_pvr_type(uint8_t type)
{
	g_playback_type = type;
}

uint8_t cascam_ui_get_playback_type(void)
{
	return g_playback_type;
}

int32_t cascam_ui_pause_resume_function(uint8_t pause)
{
	GxMsgProperty_PlayerPause player_pause;
	GxMsgProperty_PlayerResume player_resume;
	int freeze = 0;
	FullArb* arb = &g_AppFullArb;

	if(g_playback_type == CASCAM_PVR_TYPE_REC_PLAY)
	{
		//playback
		if(pause == 0)
		{
#ifdef CASCAM_CONAXCAS_SUPPORT
			app_conax_set_mat_pin_flag(1);
#endif
			GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
			app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
			player_resume.player = PMP_PLAYER_AV;
			app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
		}
		if(pause == 1)
		{
			GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
			app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
			player_pause.player = PMP_PLAYER_AV;
			app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));
		}
	}
	else if(g_playback_type == CASCAM_PVR_TYPE_TMS_PLAY)
	{
		//normal_play
		if(pause == 0)
		{
#ifdef CASCAM_CONAXCAS_SUPPORT
			app_conax_set_mat_pin_flag(1);
#endif
			if (arb->state.pause == STATE_ON)
			{
				GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
				app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
				player_resume.player = PLAYER_FOR_NORMAL;
				app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
				arb->state.pause = STATE_OFF;
			}
		}
		if(pause == 1)
		{
			if (arb->state.pause == STATE_OFF)
			{
				GxMsgProperty_PlayerTimeShift time_shift;

				freeze = 1;
				app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

				memset(&time_shift, 0, sizeof(GxMsgProperty_PlayerTimeShift));
				time_shift.enable = 0;
				time_shift.shift_dmxid = 1;
				app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));
				app_subt_pause();
				player_pause.player = PLAYER_FOR_NORMAL;
				app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));
				arb->state.pause = STATE_ON;
			}
		}
	}
	else if(g_playback_type == CASCAM_PVR_TYPE_RECORD)
	{
		//normal_play
		if(pause == 0)
		{
			GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
			app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
			player_resume.player = PLAYER_FOR_REC;
			app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
			arb->state.pause = STATE_OFF;
		}
		if(pause == 1)
		{
			GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
			app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
			player_pause.player = PLAYER_FOR_REC;
			app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));
			arb->state.pause = STATE_ON;
		}
	}
	return 0;
}

int32_t cascam_ui_playback_seek_function(int64_t start_time_ms)
{
	GxMessage *msg;
	GxMsgProperty_PlayerSeek *seek;
	msg = GxBus_MessageNew(GXMSG_PLAYER_SEEK);
	APP_CHECK_P(msg, 1);
	seek = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerSeek);
	APP_CHECK_P(seek, 1);
	seek->player = PMP_PLAYER_AV;
	seek->time = start_time_ms;
	seek->flag = SEEK_ORIGIN_SET;
//	seek->time = value;
//	seek->flag = SEEK_ORIGIN_PERCENT;

	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);

	return GXCORE_SUCCESS;
}

int32_t cascam_ui_playback_stop_function(void)
{
	play_movie_ctrol(PLAY_MOVIE_CTROL_STOP);
	return 0;
}

int32_t cascam_ui_show_video(void)
{
	GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
	return 0;
}

int32_t cascam_ui_hide_video(void)
{
	GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);
	return 0;
}
#endif
