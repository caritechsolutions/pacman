#include "channel_common.h"
#include "module/app_frontend.h"
#include "app_epg.h"
#include "app_windows.h"
#include "app_channel_info.h"
#include "app_fuse_api.h"
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif
#if LCN_SUPPORT
#include "app_lcn.h"
#endif
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#define EPG_EVENT_TIME_PROGRESS_SUPPORT 0
#if (EPG_EVENT_TIME_PROGRESS_SUPPORT > 0)
#include "app_t2_system_setting.h"
#endif
/* Private variable------------------------------------------------------- */
static event_list* sp_ChanInfoTimer = NULL;
static event_list* sp_TimerSignal = NULL;
static GxEpgInfo *s_chinfo_epg_info = NULL;
static int s_timer_cnt = 0;
static GxBusPmDataProgType s_stream_bak = GXBUS_PM_PROG_ALL;
static void _channel_info_show(void);

/* widget macro -------------------------------------------------------- */
#define TEXT_CHANNEL_INFO_SAT_TP        "text_channel_info_sat_tp"
#define TEXT_CHANNEL_INFO_SMARTCARD     "text_channel_info_smartcard"
#define TEXT_SIGNAL_PARAMETERS          "text_signal_parameters"
#define TEXT_SIGNAL_TYPE                "text_signal_type"
#define TEXT_CHANNEL_INFO_NAME          "text_channel_info_name"
#define TEXT_CHANNEL_INFO_NAME_NUM      "text_channel_info_name_num"
#define TEXT_CHANNEL_INFO_PROVIDERNAME  "text_channel_info_providername"
#define TEXT_CHANNEL_INFO_AVFORMAT      "text_channel_info_avformat"
#define TEXT_CHANNEL_INFO_RESOLUTION    "text_channel_info_solution"
#define TEXT_CHANNEL_INFO_TIME          "text_channel_info_time"
#define TEXT_CHANNEL_INFO_DATA          "text_channel_info_date"
#define IMG_TV_RADIO                    "img_progbar_tv_radio"
#define TEXT_EPG_CUR                    "text_channel_info_current"
#define TEXT_EPG_NEXT                   "text_channel_info_next"
#define IMG_HD                          "img_progbar_hd"
#define AV_TYPE_STR_LEN                 16

extern GxEpgInfo *app_epg_info_mem_alloc(void);
extern int app_epg_cur_next_epg_info_str_get(GxMsgProperty_NodeByPosGet *node_prog,
        GxEpgInfo *epg_info, char **cur_str, char **next_str, int *cur_per);
extern int app_get_ca_system_pid_status(int mode,int *count);
extern int _ext_app_bypass_one_program_pvr_tms_state(void);

typedef struct CaSystem_s
{
	uint16_t m_wCasIdBigen;
	uint16_t m_wCasIdEnd;
	uint8_t m_chCasName[MAX_CAS_NAME];
}CaSystem_t;

CaSystem_t default_ca_system[MAX_CA_SYSTEM]= //TODO
{
    {CAS_ID_FREE,	CAS_ID_FREE,		"Free"},
    {0x0500,			0x05ff,			"Viaccess"},
    {0x0100,			0x01ff,			"SECA"},
    {0x0600,			0x06ff,			"Irdeto"},//IRDETO
    {0x1800,			0x18ff,			"Nagravision"},
    {0x001,			    0x001,			"Alphacrypt"},//?
    {0x1700,			0x17ff,			"Betacrypt"},
    {0x0b00,            0x0bff,		    "Conax"},
    {0x0d00,            0x0dff,         "Cryptoworks"},
    {0x0900,            0x09ff,			"NDS"},
    {0x0e00,            0x0eff,         "PowerVU"},
    {0x001,             0x001,          "Skycrypt"},//?
    {0x001,             0x001,          "Firecrpt"},//?
    {0x4a00,            0x4aff,			"Xcrypt"},
    {0x2600,            0x26ff,			"BISS"},
    {CAS_ID_UNKNOW,     CAS_ID_UNKNOW,	"Unknow"}
};

int app_channel_info_clear_epg_info(void)
{
    GUI_SetProperty(TEXT_EPG_CUR, "string", STR_ID_NO_INFO);
    GUI_SetProperty(TEXT_EPG_NEXT, "string", STR_ID_NO_INFO);
    return 0;
}

int app_channel_info_show_video_info(PlayerVideoTrack video, GxBusPmDataProgDefinition definition)
{
    char solution_str[15] = {0};
    sprintf(solution_str, "%d x %d", video.width, video.height);
    GUI_SetProperty("text_channel_info_solution", "string", solution_str);

    if((video.height > 576) && (video.width > 720))
    {
        GUI_SetProperty(IMG_HD, "img", "s_bar_hd");
    }
    else
    {
        if(definition != GXBUS_PM_PROG_HD)
        {
            GUI_SetProperty(IMG_HD, "img", "s_bar_hd_unfocus");
        }
    }
    return 0;
}

static int app_get_ca_system_by_id(uint16_t cas_id, CaSystem_t** pp_ca_system)//sub
{
	uint16_t index = 0;
	static uint16_t ret_value = 0;

	*pp_ca_system = (CaSystem_t*)&default_ca_system[MAX_CA_SYSTEM - 1];

	for( index = 0; index < MAX_CA_SYSTEM; index++)//MAX_CA_SYSTEM - 1 is unknow //zfz 20070828
	{

		 if(cas_id >= default_ca_system[index].m_wCasIdBigen &&
			cas_id <= default_ca_system[index].m_wCasIdEnd)
		 {
		 	*pp_ca_system = (CaSystem_t*)&default_ca_system[index];
			ret_value = index;
			return index;
		 }
	}


	return ret_value;
}

void app_channel_info_show_ca_system_name(uint16_t cas_id, char *buffer, int len)
{
	#define MAX_CAS_TOTAL	16
	#define SPACE           " "
	CaSystem_t *pCaNode;
	int count  = 1;
	int card_num  = 0;
	int flag = 0;
	uint16_t pmt_cas_id = 0;
	uint8_t index  = 0;

	memset(buffer, 0, len);
	app_get_ca_system_pid_status(0,&count);
	if((count>MAX_CAS_TOTAL)||(count<0))
	{
		count = MAX_CAS_TOTAL-1;
	}

	if(count == 0)
	{
		return;
	}

	for(index = 0; index < count; index++ )
	{
		pmt_cas_id = app_get_ca_system_pid_status(index,&count);
		card_num = app_get_ca_system_by_id(pmt_cas_id, &pCaNode);

		if((flag & (1 << card_num)) != 0)
		{
			continue;
		}
		flag |= (1 << card_num);

		if((strlen(buffer) + strlen((char*)pCaNode->m_chCasName) + 1) > len -1)
		{
			app_log_error("[%s]---error--len--\n",__FUNCTION__);
			break;
		}

       	sprintf(strlen(buffer)+buffer,"%s",pCaNode->m_chCasName);
		strcat(buffer,SPACE);
	}

}

char *app_channel_play_count_str_get(GxMsgProperty_NodeByPosGet *node_prog)
{
	static char buffer[8]={0};
	GxMsgProperty_NodeByPosGet node;

	if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}

#if TKGS_SUPPORT
	if(GX_TKGS_OFF != app_tkgs_get_operatemode())
	{
		AppProgExtInfo prog_ext_info = {{0}};
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node_prog->prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info))
			snprintf(buffer, sizeof(buffer), "%04d", prog_ext_info.tkgs.logicnum);
		else
			snprintf(buffer, sizeof(buffer), "%04d", g_AppPlayOps.normal_play.play_count+1);
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%04d", g_AppPlayOps.normal_play.play_count+1);
	}
#elif LCN_SUPPORT
    uint32_t pos =0;
    pos = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count)+1;
	if(app_lcn_config_get() == 1)
	{
		AppProgExtInfo prog_ext_info = {0};
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node_prog->prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info))
			snprintf(buffer, sizeof(buffer), "%03d", prog_ext_info.logicnum);
		else
			snprintf(buffer, sizeof(buffer), "%03d", pos);
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%03d", pos);
	}
#else
	snprintf(buffer, sizeof(buffer), "%04d", g_AppPlayOps.normal_play.play_count+1);
#endif

	return buffer;
}

static char *app_channel_play_group_str_get(GxMsgProperty_NodeByPosGet *node_prog)
{
#define LEN 64
	char *tmp_str = "";
	static char buffer[LEN];
    int use_len = 0;

#if DOUBLE_S2_SUPPORT
	tmp_str = (node_prog->prog_data.tuner == 1) ? "T2 - " : "T1 - ";
#endif
	if(GROUP_MODE_ALL == g_AppPlayOps.normal_play.view_info.group_mode)//all satellite
	{
		snprintf(buffer, sizeof(buffer), "%s%s", tmp_str, STR_ID_ALL);
	}
	else if(GROUP_MODE_SAT == g_AppPlayOps.normal_play.view_info.group_mode)//sat
	{
		GxMsgProperty_NodeByIdGet node_sat = {0};

		node_sat.node_type = NODE_SAT;
		node_sat.id = g_AppPlayOps.normal_play.view_info.sat_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);

		snprintf(buffer, sizeof(buffer), "%s%s", tmp_str, (char*)app_get_sat_name(&node_sat.sat_data));
	}
	else if(GROUP_MODE_FAV == g_AppPlayOps.normal_play.view_info.group_mode)//fav
	{
		GxMsgProperty_NodeByIdGet node_fav = {0};

		node_fav.node_type = NODE_FAV_GROUP;
		node_fav.id = g_AppPlayOps.normal_play.view_info.fav_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_fav);
		snprintf(buffer, sizeof(buffer), "%s%s", tmp_str, node_fav.fav_item.fav_name);
	}
    else if(GROUP_MODE_HD == g_AppPlayOps.normal_play.view_info.group_mode)
    {
        snprintf(buffer, sizeof(buffer), "%s%s", tmp_str, STR_ID_HD);
    }
    strcat(buffer, " - ");
    use_len = strlen(buffer);

	channel_prog_tp_info_str_get(&node_prog->prog_data, buffer + use_len, LEN - use_len);
	return buffer;
}

static bool app_check_h265_format_whether_10bit(AppProgVideoType video_type)
{
	if(video_type == APP_PM_PROG_H265)
	{
		extern bool app_get_avcodec_flag(void);
		PlayerProgInfo *info = app_player_prog_info_get();
		if((app_get_avcodec_flag() == true) && (GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info) == 0) && info->video.bpp == 10)
		{
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

char *app_channel_av_format_str_get(GxMsgProperty_NodeByPosGet *node_prog)
{
	AppProgAudioType a_type;
	AppProgVideoType v_type;
	static char av_type_str[AV_TYPE_STR_LEN];

	memset(av_type_str , 0 , AV_TYPE_STR_LEN);
	if(node_prog->prog_data.service_type == GXBUS_PM_PROG_TV)
	{
		v_type = channel_get_app_video_type(node_prog->prog_data.video_type);
		channel_get_video_type_str(av_type_str, v_type, AV_TYPE_STR_LEN);
		if(app_check_h265_format_whether_10bit(v_type) == true)
		{
			strcat(av_type_str, "-M10");
		}
	}
	else
	{
		a_type = channel_get_app_audio_type(node_prog->prog_data.cur_audio_type);
		channel_get_audio_type_str(av_type_str, a_type, AV_TYPE_STR_LEN);
	}

	return av_type_str;
}

char *app_channel_play_resolution_str_get(GxMsgProperty_NodeByPosGet *node_prog)
{
	GxMsgProperty_NodeByPosGet node;
	static char buffer[16] = {0};

	if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}

	memset(buffer, 0, sizeof(buffer));
	if(node_prog->prog_data.service_type == GXBUS_PM_PROG_TV)
	{
		if(g_AppFullArb.tip == FULL_STATE_DUMMY)
		{
			PlayerProgInfo *info = app_player_prog_info_get();
			if((GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info) == 0)
					&& (info->video.width  > 0 && info->video.width  <= 1920)
					&& (info->video.height > 0 && info->video.height <= 1088))
			{
				snprintf(buffer, sizeof(buffer), "%d x %d", info->video.width, info->video.height);
			}
		}
	}

	return buffer;
}


static void _channel_info_show_prog_logo(void)
{
	// tv/radio
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		GUI_SetProperty(IMG_TV_RADIO, "img", "s_bar_radio");
	}
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
	{
		GUI_SetProperty(IMG_TV_RADIO, "img", "s_bar_tv");
	}
}

static void _channel_info_show_prog_name(GxMsgProperty_NodeByPosGet *node_prog)
{
	GxMsgProperty_NodeByPosGet node = {0};

	if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}
	app_set_widget_string(TEXT_CHANNEL_INFO_NAME, (char *)node_prog->prog_data.prog_name);
}

static void _channel_info_show_prog_num(GxMsgProperty_NodeByPosGet *node_prog)
{
    GxMsgProperty_NodeByPosGet node = {0};
    char* num_str = NULL;

	if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}
	num_str = app_channel_play_count_str_get(node_prog);
    if(s_stream_bak != g_AppPlayOps.normal_play.view_info.stream_type)
    {
        s_stream_bak = g_AppPlayOps.normal_play.view_info.stream_type;
    }
    app_set_widget_string(TEXT_CHANNEL_INFO_NAME_NUM, num_str);
}

static void _channel_info_show_prog_sat_tp(GxMsgProperty_NodeByPosGet *node_prog)
{
    char *tmp_str = NULL;
    GxMsgProperty_NodeByPosGet node = {0};

    if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}
	tmp_str = app_channel_play_group_str_get(node_prog);
	app_set_widget_string("TEXT_CHANNEL_INFO_SAT_TP", tmp_str);

}

static void _channel_info_show_prog_format(GxMsgProperty_NodeByPosGet *node_prog)
{
    char *tmp_str = NULL;
    GxMsgProperty_NodeByPosGet node = {0};

    if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}
	tmp_str = app_channel_av_format_str_get(node_prog);
    app_set_widget_string(TEXT_CHANNEL_INFO_AVFORMAT, tmp_str);
}

static void _channel_info_show_prog_resolution(GxMsgProperty_NodeByPosGet *node_prog)
{
    char *tmp_str = NULL;
    GxMsgProperty_NodeByPosGet node = {0};

    if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}
    tmp_str = app_channel_play_resolution_str_get(node_prog);
    app_set_widget_string(TEXT_CHANNEL_INFO_RESOLUTION, tmp_str);
}

static void _channel_infor_show_event_progbar(int percent)
{
#define PROGBAR_EVENT   "progbar_channel_info_event_time"
	app_progbar_update_value(PROGBAR_EVENT, percent);
}

static void _channel_info_show_prog_provider_name(GxMsgProperty_NodeByPosGet *node_prog)
{
    GxMsgProperty_NodeByPosGet node = {0};

    if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}
	app_set_widget_string(TEXT_CHANNEL_INFO_PROVIDERNAME,(char*)node_prog->prog_data.u2.service_provider_name);
}

static void _channel_info_show_prog_time(void)
{
    char time_str[24] = {0};

    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str));
    app_set_widget_string(TEXT_CHANNEL_INFO_TIME, time_str+MIN_SEC_OFFSET);
}

static void _channel_info_show_prog_data(void)
{
    char time_str[24] = {0};

    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str));
    time_str[MIN_SEC_OFFSET] = '\0';
    app_set_widget_string(TEXT_CHANNEL_INFO_DATA, time_str);

}

#if HIGH_DYNAMIC_RANGE_SUPPORT

#define APP_HDR_TYPE_START_VALUE   (0)  // from gxbus  enum VideoDynamicRangeType VIDEO_DRT_SDR=0
#define APP_HDR_TYPE_TOTAL         (5)  // from gxbus  enum VideoDynamicRangeType
/*
- If the driver is updated, this content also needs to be updated. kk Label:expand
- also include hdr_type_str[APP_HDR_TYPE_TOTAL] = {" ", "HLG", "HDR10","HDR10_PLUS", "DOLBYVISION"}; in app_channel_info_show_format_info()
- APP_HDR_TYPE_START_VALUE  hdr_type_str[0]
- APP_HDR_TYPE_TOTAL
*/

static int __channel_video_get_hdr_type(void)
{
    int ret ;
    int hdr_type ;
    PlayerProgInfo *play_info = app_player_prog_info_get();
    ret = GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, play_info);
    if (GXCORE_SUCCESS == ret){
        hdr_type = play_info->video.dr.type ; //play_info->video.dr.dolby_flag
        //printf("call GxPlayer_MediaGetProgInfoByName, hdr = %d , %d \n",play_info->video.dr.type,play_info->video.dr.dolby_flag);
    }
    else{
        //printf("get_hdr_type GxPlayer_MediaGetProgInfoByName ret = %d \n",ret);
        hdr_type = APP_HDR_TYPE_START_VALUE ;
    }
    if ( hdr_type < APP_HDR_TYPE_START_VALUE || hdr_type >= APP_HDR_TYPE_TOTAL){
        //printf("check hdr type = %d \n",hdr_type);
        hdr_type = APP_HDR_TYPE_START_VALUE ;
    }

    //printf("Get HIGH DYNAMIC RANGE type = %d \n",hdr_type);
    return hdr_type ;
}
#endif

static void _channel_info_show_hdr_img(void)
{
    char* hdr_img_widget = "img_progbar_hdr" ;
#if HIGH_DYNAMIC_RANGE_SUPPORT
    extern int app_get_hdr_enable(void);
    if(app_get_hdr_enable())
    {
        char *hdr_image_name[2] = {"s_bar_hdr", "s_bar_hdr_unfocus"};
        int hdr_type = APP_HDR_TYPE_START_VALUE ;

        hdr_type = __channel_video_get_hdr_type() ;
        if (hdr_type > APP_HDR_TYPE_START_VALUE){
            GUI_SetProperty(hdr_img_widget, "img", hdr_image_name[0]);
        }
        else{
            GUI_SetProperty(hdr_img_widget, "img", hdr_image_name[1]);
        }
    }
    else
    {
        GUI_SetProperty(hdr_img_widget, "state", "hide");
    }
#else
    GUI_SetProperty(hdr_img_widget, "state", "hide");
#endif
}

static void _channel_info_show_flag_img(GxMsgProperty_NodeByPosGet *node_prog)
{
#define  IMG_TOTAL      (5)
    int i = 0;
    char* flag_img_widget[IMG_TOTAL] = {"img_progbar_ttx", "img_progbar_subt", "img_progbar_hd",
        "img_progbar_lock", "img_progbar_scramble"};
    char *image_flag_focus[IMG_TOTAL] = {"s_bar_ttx", "s_bar_subt", "s_bar_hd",
        "s_bar_lock", "s_bar_money"};
    char *image_flag_unfocus[IMG_TOTAL] = {"s_bar_ttx_unfocus", "s_bar_subt_unfocus", "s_bar_hd_unfocus",
        "s_bar_lock_unfocus", "s_bar_money_unfocus"};
    GxBusPmDataProgSwitch flag_type[IMG_TOTAL] = {0};

    flag_type[0] = node_prog->prog_data.ttx_flag;
    flag_type[1] = node_prog->prog_data.subt_flag | node_prog->prog_data.cc_flag;
    flag_type[2] = (GXBUS_PM_PROG_HD == node_prog->prog_data.definition) ? GXBUS_PM_PROG_BOOL_ENABLE : GXBUS_PM_PROG_BOOL_DISABLE;
    flag_type[3] = node_prog->prog_data.lock_flag;
    flag_type[4] = node_prog->prog_data.scramble_flag;

    for (i=0; i<IMG_TOTAL; i++)
    {
        if (flag_type[i] == GXBUS_PM_PROG_BOOL_ENABLE)
            GUI_SetProperty(flag_img_widget[i], "img", image_flag_focus[i]);
        else
            GUI_SetProperty(flag_img_widget[i], "img", image_flag_unfocus[i]);
    }
    if( flag_type[4] != GXBUS_PM_PROG_BOOL_ENABLE
            || (flag_type[3] == GXBUS_PM_PROG_BOOL_ENABLE && g_AppFullArb.tip == FULL_STATE_LOCKED))
    {
        app_set_widget_string(TEXT_CHANNEL_INFO_SMARTCARD,  NULL);
    }

    // hdr --add 20220629
    _channel_info_show_hdr_img();
}

static void _channel_info_show_epg(GxMsgProperty_NodeByPosGet *node_prog)
{
	int ret = 0;
	char *cur_epg_str = NULL;
	char *next_epg_str = NULL;
	int percent_time = 0;

	GxMsgProperty_NodeByPosGet node;
	if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}
	ret = app_epg_cur_next_epg_info_str_get(node_prog, s_chinfo_epg_info, &cur_epg_str, &next_epg_str, &percent_time);
	_channel_infor_show_event_progbar(percent_time);

	if (ret == GXCORE_SUCCESS)
	{
        app_set_widget_string(TEXT_EPG_CUR, cur_epg_str);
		app_set_widget_string(TEXT_EPG_NEXT, next_epg_str);
	}
}

static int timer_channel_info_timeout(void *userdata)
{
	if((g_AppFullArb.state.tv == STATE_OFF)
        || (GUI_CheckDialog(WND_CHANNEL_INFO_SIGNAL) == GXCORE_SUCCESS))
	{
        if(GUI_CheckDialog(WND_PVR_BAR) != GXCORE_SUCCESS)
        {
		    GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &s_timer_cnt, INFOBAR_TIMEOUT);
            _channel_info_show();
        }
	}
	else
	{
        s_timer_cnt--;
		if(s_timer_cnt == 0)
		{
			APP_TIMER_REMOVE(sp_ChanInfoTimer);
			GUI_EndDialog(WND_CHANNEL_INFO);
            GUI_SetProperty(WND_CHANNEL_INFO,"draw_now",NULL);
		}
		else
		{
            _channel_info_show();
		}
	}

    return 0;
}

static void timer_channel_info_timeout_reset(void)
{
	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &s_timer_cnt, INFOBAR_TIMEOUT);
	APP_TIMER_ADD(sp_ChanInfoTimer, timer_channel_info_timeout, 1000, TIMER_REPEAT);
}

static void _channel_info_show(void)
{
#define MAX_BUF_LEN     200
    char buffer[MAX_BUF_LEN];
    GxMsgProperty_NodeByPosGet node_prog = {0};

    node_prog.node_type = NODE_PROG;
    node_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

    //logo
    if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            && (s_stream_bak != GXBUS_PM_PROG_RADIO))
    {
        _channel_info_show_prog_logo();
    }

    if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
            && (s_stream_bak != GXBUS_PM_PROG_TV))
    {
        _channel_info_show_prog_logo();
    }
    //prog_name
    _channel_info_show_prog_name(&node_prog);
    //num
    _channel_info_show_prog_num(&node_prog);
    //sat_tp
    _channel_info_show_prog_sat_tp(&node_prog);
    //provider_name T2 only
    _channel_info_show_prog_provider_name(&node_prog);
    //ca system name
    app_channel_info_show_ca_system_name(node_prog.prog_data.cas_id, buffer, MAX_BUF_LEN); // cas info
    GUI_SetProperty(TEXT_CHANNEL_INFO_SMARTCARD, "string", buffer);
    //format
    _channel_info_show_prog_format(&node_prog);
    //resolution
    _channel_info_show_prog_resolution(&node_prog);
    //epg
    _channel_info_show_epg(&node_prog);
    // lock scramble hd subt & ttx
    _channel_info_show_flag_img(&node_prog);
    // time
    _channel_info_show_prog_time();
    //data
    _channel_info_show_prog_data();
    GUI_SetInterface("flush", NULL);
}

static int _up_or_down_normal(unsigned short key)
{
    g_AppFullArb.state.pause = STATE_OFF;
    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    if(STBK_UP == key)
    {
        g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_NEXT, 0);
    }
    else if(STBK_DOWN == key)
    {
        g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_PREV, 0);
    }
    return 0;
}

static int _key_up_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if (POP_VAL_OK == ret)
    {
        if(PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _up_or_down_normal(STBK_UP);
    }
    return 0;
}

static int _key_down_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if (POP_VAL_OK == ret)
    {
        if(PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _up_or_down_normal(STBK_DOWN);
    }
    return 0;
}

void app_channel_info_update(void)
{
    _channel_info_show();
    timer_channel_info_timeout_reset();
}

SIGNAL_HANDLER int app_channel_info_create(GuiWidget *widget, void *usrdata)
{
    s_stream_bak = GXBUS_PM_PROG_ALL;
	s_chinfo_epg_info = app_epg_info_mem_alloc();
    app_channel_info_update();

    if (GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
    {
        GUI_SetFocusWidget(WND_PASSWD);
    }

    if (GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
    {
        GUI_SetFocusWidget(WND_POP_BOOK);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_destroy(GuiWidget *widget, void *usrdata)
{
	APP_TIMER_REMOVE(sp_ChanInfoTimer);
	APP_FREE(s_chinfo_epg_info);
    s_stream_bak = GXBUS_PM_PROG_ALL;

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_keypress(GuiWidget *widget, void *usrdata)
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
                case STBK_CH_UP:
                case STBK_UP:
                    {
#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
                        if (g_AppChOps.get_list_total() != 0)
                        {
                            int prog_pos = g_AppChOps.get_next_pos(g_AppPlayOps.normal_play.play_count);
#if SAT2IP_SERVER_SUPPORT
                            if (app_sat2ip_switch_tip_get_by_pos(prog_pos) < 0)
                                break;
#endif
#if DVB2IP_SERVER_SUPPORT
                            if (app_dvb2ip_switch_tip_get_by_pos(prog_pos) < 0)
                                break;
#endif
                       }
#endif
                        if (  _ext_app_bypass_one_program_pvr_tms_state() )
                        {
                            break;
                        }
                        if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_key_up_stop_pvr_cb))
                        {
                            _up_or_down_normal(STBK_UP);
                        }
                        break;
                    }

                case STBK_CH_DOWN:
                case STBK_DOWN:
                    {
#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
                        if (g_AppChOps.get_list_total() != 0)
                        {
                            int prog_pos = g_AppChOps.get_prev_pos(g_AppPlayOps.normal_play.play_count);;
#if SAT2IP_SERVER_SUPPORT
                            if (app_sat2ip_switch_tip_get_by_pos(prog_pos) < 0)
                                break;
#endif
#if DVB2IP_SERVER_SUPPORT
                            if (app_dvb2ip_switch_tip_get_by_pos(prog_pos) < 0)
                                break;
#endif
                        }
#endif
                        if (  _ext_app_bypass_one_program_pvr_tms_state() )
                        {
                            break;
                        }
                        if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_key_down_stop_pvr_cb))
                        {
                            _up_or_down_normal(STBK_DOWN);
                        }
                        break;
                    }

                case STBK_EXIT:
                case VK_BOOK_TRIGGER:
                    {
                        if(g_AppFullArb.state.tv == STATE_ON)
                        {
                            GUI_EndDialog(WND_CHANNEL_INFO);
                        }
                    }
                    break;

                case STBK_INFO:
                    if(g_AppPvrOps.state == PVR_DUMMY)
                    {
                        g_AppFullArb.timer_stop();
                        app_clean_fullscreen_tip(true, false, false);
                        GUI_CreateDialog(WND_CHANNEL_INFO_SIGNAL);
                        GUI_SetProperty(WND_CHANNEL_INFO_SIGNAL, "draw_now", NULL);
                    }
                    else
                    {
						if (sp_ChanInfoTimer)
                        timer_stop(sp_ChanInfoTimer);
                        GUI_CreateDialog(WND_PVR_BAR);
                    }
                    break;

#if RECALL_LIST_SUPPORT
                case STBK_RECALL:
                    {
#if SAT2IP_SERVER_SUPPORT
                        if (app_sat2ip_switch_tip_get_force() < 0)
                            break;
#endif
#if DVB2IP_SERVER_SUPPORT
                        if (app_dvb2ip_switch_tip_get_force() < 0)
                            break;
#endif
                        if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_key_recall_stop_pvr_cb))
                        {
                            _recall_normal();
                        }
                    }
                    break;
#endif
#if WEBLET_SUPPORT
				case STBK_QR:
					GUI_SendEvent(WND_FULL_SCREEN, event);
					break;
#endif

                default:
                    {
                        if((g_AppFullArb.state.tv == STATE_ON)
                                || ((event->key.sym == STBK_OK)
                                    || (event->key.sym == STBK_SAT)
                                    || (event->key.sym == STBK_FAV)
                                   ))
                        {
                            GUI_EndDialog(WND_CHANNEL_INFO);
                        }
                        GUI_SendEvent(WND_FULL_SCREEN, event);
                    }
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_channel_info_got_focus(GuiWidget *widget, void *usrdata)
{
    timer_channel_info_timeout_reset();
    g_AppFullArb.timer_start();

    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_channel_info_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}

static char *app_dvbt2_module_str_get(fe_modulation_t modulation)
{
	switch(modulation)
	{
		case DVBT_QPSK:        return "DVBT, QPSK";
		case DVBT_16QAM:       return "DVBT, 16QAM";
		case DVBT_64QAM:       return "DVBT, 64QAM";
		case DVBT2_QPSK:       return "DVBT2, QPSK";
		case DVBT2_16QAM:      return "DVBT2, 16QAM";
		case DVBT2_64QAM:      return "DVBT2, 64QAM";
		case DVBT2_256QAM:     return "DVBT2, 256QAM";
		default:               return STR_ID_UNKNOW;
	}
	return STR_ID_UNKNOW;
}

static char *app_dvbs_module_str_get(fe_modulation_t modulation)
{
	switch(modulation)
	{
		case DVBS_QPSK_12:     return "QPSK 1/2";
		case DVBS_QPSK_23:     return "QPSK 2/3";
		case DVBS_QPSK_34:     return "QPSK 3/4";
		case DVBS_QPSK_56:     return "QPSK 5/6";
		case DVBS_QPSK_78:     return "QPSK 7/8";

		case DVBS2_QPSK_12:    return "QPSK 1/2";
		case DVBS2_QPSK_23:    return "QPSK 2/3";
		case DVBS2_QPSK_34:    return "QPSK 3/4";
		case DVBS2_QPSK_45:    return "QPSK 4/5";
		case DVBS2_QPSK_56:    return "QPSK 5/6";

		case DVBS2_QPSK_14:    return "QPSK 1/4";
		case DVBS2_QPSK_13:    return "QPSK 1/3";
		case DVBS2_QPSK_25:    return "QPSK 2/5";
		case DVBS2_QPSK_35:    return "QPSK 3/5";
		case DVBS2_QPSK_89:    return "QPSK 8/9";
		case DVBS2_QPSK_910:   return "QPSK 9/10";

		case DVBS2_8PSK_35:    return "8PSK 3/5";
		case DVBS2_8PSK_23:    return "8PSK 2/3";
		case DVBS2_8PSK_34:    return "8PSK 3/4";
		case DVBS2_8PSK_56:    return "8PSK 5/6";
		case DVBS2_8PSK_89:    return "8PSK 8/9";
		case DVBS2_8PSK_910:   return "8PSK 9/10";

		case DVBS2_16APSK_23:  return "16APSK 2/3";
		case DVBS2_16APSK_34:  return "16APSK 3/4";
		case DVBS2_16APSK_45:  return "16APSK 4/5";
		case DVBS2_16APSK_56:  return "16APSK 5/6";
		case DVBS2_16APSK_89:  return "16APSK 8/9";
		case DVBS2_16APSK_910: return "16APSK 9/10";

		case DVBS2_32APSK_34:  return "32APSK 3/4";
		case DVBS2_32APSK_45:  return "32APSK 4/5";
		case DVBS2_32APSK_56:  return "32APSK 5/6";
		case DVBS2_32APSK_89:  return "32APSK 8/9";
		case DVBS2_32APSK_910: return "32APSK 9/10";
		default:               return STR_ID_UNKNOW;
	}
	return STR_ID_UNKNOW;
}

char *app_prog_module_str_get(GxMsgProperty_NodeByPosGet *node_prog)
{
	AppFrontend_Info info;
	static char buffer[50] = {0};
	char *module_str = NULL;
	GxMsgProperty_NodeByIdGet node_tp = {0};
	GxMsgProperty_NodeByIdGet node_sat = {0};

	node_tp.node_type = NODE_TP;
	node_tp.id = node_prog->prog_data.tp_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

	node_sat.node_type = NODE_SAT;
	node_sat.id = node_tp.tp_data.sat_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);

	if(GXBUS_PM_SAT_S == node_sat.sat_data.type)
	{
		char *tmp_str = NULL;
		app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_INFO_GET, &info);
		tmp_str = app_dvbs_module_str_get(info.modulation);
		switch(info.type)
		{
			case FRONTEND_DVB_S:
				snprintf(buffer, sizeof(buffer), "%s, %s", tmp_str, STR_ID_DVBS);
				module_str = buffer;
				break;
			case FRONTEND_DVB_S2:
				snprintf(buffer, sizeof(buffer), "%s, %s", tmp_str, STR_ID_DVBS2);
				module_str = buffer;
				break;
			default:
				module_str = STR_ID_UNKNOW;
				break;
		}
	}
	else if(GXBUS_PM_SAT_T == node_sat.sat_data.type)
	{
		if(node_prog->prog_data.common_status == DVBT2_BASE || node_prog->prog_data.common_status == DVBT2_LITE)
			module_str = "DVB -T2";
		else
			module_str = "DVB -T";
	}
	else if(GXBUS_PM_SAT_DVBT2 == node_sat.sat_data.type)
	{
	    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_INFO_GET, &info);
		module_str = app_dvbt2_module_str_get(info.modulation);
	}
	else if(GXBUS_PM_SAT_C == node_sat.sat_data.type)
	{
		char *modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};

		if((node_tp.tp_data.tp_c.modulation >= DVBC_16QAM) && (node_tp.tp_data.tp_c.modulation <= DVBC_256QAM))
		{
			snprintf(buffer, sizeof(buffer), "%s, %s", modulation_str[node_tp.tp_data.tp_c.modulation-DVBC_16QAM], "DVB -C");
			module_str = buffer;
		}
	}
	else if(GXBUS_PM_SAT_DTMB == node_sat.sat_data.type)
	{
		if(GXBUS_PM_SAT_1501_DTMB == node_sat.sat_data.sat_dtmb.work_mode)
			module_str = "DTMB";
		else
			module_str = "DVB -C";
	}

	if (module_str == NULL)
		module_str = STR_ID_UNKNOW;

	return module_str;
}

static void app_channel_info_show_format_info(GxMsgProperty_NodeByPosGet *node_prog)
{
#define AV_TYPE_STR_LEN 16
#define AV_INFO_LEN  128
	AppProgAudioType a_type;
	AppProgVideoType v_type;
	char audio_type[AV_TYPE_STR_LEN] = {0};
	char video_type[AV_TYPE_STR_LEN] = {0};
	char buffer[AV_INFO_LEN] = {0};
#if HIGH_DYNAMIC_RANGE_SUPPORT
	int hdr_type = 0 ;
	char* hdr_type_str[APP_HDR_TYPE_TOTAL] = {" ", "HLG", "HDR10","HDR10_PLUS", "DOLBYVISION"};//0 is sdr , do not display
#endif

	if (!node_prog)
		return;
	/*MPEG1, MPEG2, AAC-LATM, AC3, AAC-ADTS, EAC3, LPCM, DTS, DOLBY-TRUEHD, AC3-PLUS, DTS-HD, DTS_MA, AC3-PLUS-SEC, DTS-HD-SEC*/
	a_type = channel_get_app_audio_type(node_prog->prog_data.cur_audio_type);
	channel_get_audio_type_str(audio_type, a_type, AV_TYPE_STR_LEN);
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
	{
		v_type = channel_get_app_video_type(node_prog->prog_data.video_type);
		channel_get_video_type_str(video_type, v_type, AV_TYPE_STR_LEN);
		if(app_check_h265_format_whether_10bit(v_type) == true)
		{
			strcat(video_type, "-M10");
		}
#if HIGH_DYNAMIC_RANGE_SUPPORT
		hdr_type = __channel_video_get_hdr_type() ;
#if (DOLBY_SUPPORT == 0)
		if ( hdr_type == (APP_HDR_TYPE_TOTAL -1) ) //DOLBYVISION need copyright
		{
			hdr_type = APP_HDR_TYPE_START_VALUE ;
		}
#endif
		snprintf(buffer, AV_INFO_LEN, "V-%s A-%s %s", video_type, audio_type,hdr_type_str[hdr_type]);
#else
		snprintf(buffer, AV_INFO_LEN, "V-%s A-%s", video_type, audio_type);
#endif

	}
	else
	{
		snprintf(buffer, AV_INFO_LEN, "A-%s", audio_type);
	}

	GUI_SetProperty("text_signal_format_value", "string", buffer);
}

static void app_channel_info_show_pid_info(GxMsgProperty_NodeByPosGet *node_prog)
{
#define PID_INFO_LEN  64
	char buffer[PID_INFO_LEN] = {0};
	if (node_prog->prog_data.service_type == GXBUS_PM_PROG_TV)
	{
		snprintf(buffer, PID_INFO_LEN, "V-%d A-%d PCR-%d", node_prog->prog_data.video_pid,
				node_prog->prog_data.cur_audio_pid, node_prog->prog_data.pcr_pid);
	}
	else
	{
		snprintf(buffer, PID_INFO_LEN, "A-%d PCR-%d",
				node_prog->prog_data.cur_audio_pid, node_prog->prog_data.pcr_pid);
	}

	GUI_SetProperty("text_signal_pid_value", "string", buffer);
}

static void _channel_info_signal_show_info(void)
{
	GxMsgProperty_NodeByPosGet node_prog = {0};
	char *tmp_str = NULL;

	node_prog.node_type = NODE_PROG;
	node_prog.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node_prog)))
	{
		tmp_str = app_prog_module_str_get(&node_prog);
		app_set_widget_string(TEXT_SIGNAL_PARAMETERS, (void*)tmp_str);
		app_channel_info_show_format_info(&node_prog);
		app_channel_info_show_pid_info(&node_prog);
	}
}

static int channel_info_signal_timer_exec(void *userdata)
{
#define TEXT_SIGNAL_STRENGTH_VALUE  "text_signal_strength_value"
#define TEXT_SIGNAL_QUALITY_VALUE   "text_signal_quality_value"
#define TEXT_SIGNAL_SNR_VALUE       "text_signal_snr_value"
#define PROGBAR_SIGNAL_SNR          "progbar_signal_snr"
#define PROGBAR_SIGNAL_STRENGTH     "progbar_signal_strength"
#define PROGBAR_SIGNAL_QUALITY      "progbar_signal_quality"
	uint32_t value = 0;
	char buffer[20] = {0};
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};

	// strength
	memset(buffer, 0, sizeof(buffer));
	app_ioctl( g_AppPlayOps.normal_play.tuner, FRONTEND_STRENGTH_GET, &value);
	siganl_bar.strength_bar = PROGBAR_SIGNAL_STRENGTH;
	siganl_val.strength_val = value;

	sprintf(buffer,"%d%%",value);
	GUI_SetProperty(TEXT_SIGNAL_STRENGTH_VALUE, "string", buffer);

	// progbar quality
	memset(buffer, 0, sizeof(buffer));
	app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_QUALITY_GET, &value);
	siganl_bar.quality_bar= PROGBAR_SIGNAL_QUALITY;
	siganl_val.quality_val = value;

	sprintf(buffer,"%d%%",value);
	GUI_SetProperty(TEXT_SIGNAL_QUALITY_VALUE, "string", buffer);
	app_signal_progbar_update(g_AppPlayOps.normal_play.tuner, &siganl_bar, &siganl_val);

	// error rate
	app_snr_progbar_update(g_AppPlayOps.normal_play.tuner, PROGBAR_SIGNAL_SNR, TEXT_SIGNAL_SNR_VALUE);
    app_update_pop_dlg(WND_POP_TIP);
	return 0;
}

SIGNAL_HANDLER int app_channel_info_signal_create(const char* widgetname, void *usrdata)
{
    _channel_info_signal_show_info();
	channel_info_signal_timer_exec(NULL);
	APP_TIMER_ADD(sp_TimerSignal, channel_info_signal_timer_exec, 100, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_signal_destroy(const char* widgetname, void *usrdata)
{
    APP_TIMER_REMOVE(sp_TimerSignal);
    app_panel_show_prog_num(0);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_signal_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(event->key.sym)
        {
            case STBK_INFO:
            case STBK_EXIT:
            case STBK_MENU:
            case VK_BOOK_TRIGGER:
                {
                    GUI_EndDialog(WND_CHANNEL_INFO_SIGNAL);
                    if(g_AppFullArb.state.tv == STATE_ON)
                        GUI_EndDialog(WND_CHANNEL_INFO);
                    break;
                }

            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_info_signal_got_focus(const char* widgetname, void *usrdata)
{
	if (sp_TimerSignal)
    reset_timer(sp_TimerSignal);
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_channel_info_signal_lost_focus(const char* widgetname, void *usrdata)
{
	if (sp_TimerSignal)
    timer_stop(sp_TimerSignal);
    return EVENT_TRANSFER_KEEPON;
}


/**** common channel "info / info_signal / list / epg" window APIS ****/

void app_channel_info_update_video_info(PlayerVideoTrack video, GxBusPmDataProgDefinition definition)
{
	app_fuse_common_channel_info_update_video_info(video,definition);
}

status_t app_channel_info_check_dialog(void)
{
	return app_fuse_common_channel_info_check_dialog();
}

int app_channel_info_compare_dialog(const char *wnd)
{
	return app_fuse_common_channel_info_compare_dialog(wnd);
}

void app_channel_info_create_dialog(void)
{
    app_fuse_common_channel_info_create_dialog();
}

void app_channel_info_end_dialog(void)
{
	app_fuse_common_channel_info_end_dialog();
}

status_t app_channel_info_signal_check_dialog(void)
{
	return app_fuse_common_channel_info_signal_check_dialog();
}

int app_channel_info_signal_compare_dialog(const char *wnd)
{
	return app_fuse_common_channel_info_signal_compare_dialog(wnd);
}

void app_channel_info_signal_end_dialog(void)
{
	app_fuse_common_channel_info_signal_end_dialog();
}

void app_channel_info_update_dialog(void)
{
	app_fuse_common_channel_info_update_dialog();
}

void app_channel_info_update_under_pop_dlg(void)
{
	app_fuse_common_channel_info_update_under_pop_dlg();
}

void app_channel_info_clear_epg_str(void)
{
	app_fuse_common_channel_info_clear_epg_str();
}

status_t app_channel_list_check_dialog(void)
{
	return app_fuse_common_channel_list_check_dialog();
}

int app_channel_list_compare_dialog(const char *wnd)
{
	return app_fuse_common_channel_list_compare_dialog(wnd);
}

status_t app_channel_epg_check_dialog(void)
{
	return app_fuse_common_epg_check_dialog();
}

int app_channel_epg_compare_dialog(const char *wnd)
{
	return app_fuse_common_epg_compare_dialog(wnd);
}

#if HIGH_DYNAMIC_RANGE_SUPPORT
int top_play_video_hdr_event_report(GxMessage* msg)
{
    GxMsgProperty_PlayerVideoDynamicRangeChanged *status;
    status = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerVideoDynamicRangeChanged);
    if (status == NULL ){
        //printf("video HDR event msg is null \n");
        return EVENT_TRANSFER_STOP ;
    }
    app_log_info("[app] HDR event msg: type=%d, flag=%d \n",status->dr.type,status->dr.dolby_flag);

    if((GUI_CheckDialog(WND_CHANNEL_INFO) == GXCORE_SUCCESS) && (GUI_CheckDialog(WND_CHANNEL_INFO_SIGNAL) == GXCORE_SUCCESS))
    {
        //printf("channel & signal wnd display, update hdr \n");
        if(strcmp(status->player, PLAYER_FOR_NORMAL) == 0)
        {
            _channel_info_signal_show_info();
        }
    }

    return EVENT_TRANSFER_STOP;
}
#endif
