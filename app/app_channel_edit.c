/*
 * =====================================================================================
 *
 *       Filename:  app_channel_edit.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年07月04日 15时06分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "channel_common.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"
#include "app_channel_info.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
extern void app_tkgs_cantwork_popup(void);
extern int app_check_tkgs_control_db(void);
GxTkgsOperatingMode app_tkgs_get_operatemode(void);
static uint32_t selbackuppos=0xffffffff;
#endif

#define LISTVIEW_CH_ED      "listview_channel_edit"
#define LISTVIEW_POP_FAV    "listview_pop_fav"
#define GROUP_CH_ED         "cmb_channel_edit_group"
#define TXT_SAT_INFO        "txt_channel_edit_sat_info"
#define TXT_PROG_INFO       "txt_channel_edit_prog_info"
#define TXT_TP_INFO         "txt_channel_edit_tp_info"
#define TXT_PID_INFO        "txt_channel_edit_pid_info"

#if TKGS_SUPPORT || LCN_SUPPORT
#define SORT_MODE_TOTAL   (8)
#else
#define SORT_MODE_TOTAL   (7)
#endif

#define TIME_PRECISION  (30)


#if CVBS_SCALE_SUPPORT
extern int app_top_get_cvbs_mode_status(void);
#define EDIT_CVBS_OFFSET_X 10
#define EDIT_CVBS_OFFSET_Y 16
#define EDIT_CVBS_OFFSET_W 12
#endif

#define MAIN_MENU_BACKGROUND_IMG    WORK_PATH"theme/image/bg.jpg"
static char *main_menu_background_key_img = "main_menu_background_key";

void back_to_main_menu_update_background(void)
{
    if (GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS)
    {
        if( APP_THEME_CLASSIC_BLUE || APP_THEME_CLASSIC_PURPLE || APP_THEME_GRID_PURPLE ){
        return ;
        }
        else{
        GUI_SetProperty(WND_MAIN_MENU, "back_ground", "null");
        gal_add_key_path(main_menu_background_key_img, MAIN_MENU_BACKGROUND_IMG);
        app_set_window_back_ground(WND_MAIN_MENU,main_menu_background_key_img,true);
        }
    }
}

// special: can't use play program, it will pop up passwd.
// so record postion use the function in app_play_control
//extern uint32_t app_play_pos_set(int32_t pos);
extern bool app_check_av_running(void);
extern void app_set_avcodec_flag(bool state);
extern uint32_t app_play_id_set(uint32_t id);
extern uint32_t app_play_id_get(void);
static PlayerWindow video_wnd;
static AppIdOps* sp_GpIdOps = NULL;
static event_list *sp_ChEdTimer = NULL; // refresh time
static uint32_t s_ListSel = 0; // used for fav

static uint32_t s_VideoStart = 0;
static bool s_save_flag = false;
static bool s_favname_flag = false;


static uint32_t s_u32_unblock_temp_val1 = 0 ;
static uint32_t s_u32_unblock_temp_val2 = 0 ;
static uint32_t s_u32_unblock_temp_val3 = 0 ;
static uint32_t s_u32_unblock_temp_val4 = 0 ;

static void app_pop_set_temp_data(uint32_t data1,uint32_t data2,uint32_t data3,uint32_t data4)
{
	s_u32_unblock_temp_val1 = data1 ;
	s_u32_unblock_temp_val2 = data2 ;
	s_u32_unblock_temp_val3 = data3 ;
	s_u32_unblock_temp_val4 = data4 ;
}

static uint32_t app_pop_get_temp_data(uint32_t id)
{
	if (id == 1)
		return s_u32_unblock_temp_val1 ;
	else if (id == 2)
		return s_u32_unblock_temp_val2 ;
	else if (id == 3)
		return s_u32_unblock_temp_val3 ;
	else if (id == 4)
		return s_u32_unblock_temp_val4 ;

	return s_u32_unblock_temp_val1;
}

typedef enum
{
    CH_ED_MODE_NONE = 0,
    CH_ED_MODE_DEL,
#if TKGS_SUPPORT
    CH_ED_MODE_SWAP,
#else
    CH_ED_MODE_MOVE,
#endif
    CH_ED_MODE_SKIP,
    CH_ED_MODE_LOCK,
    CH_ED_MODE_FAV
}ch_ed_mode;
ch_ed_mode s_Old_Mode = CH_ED_MODE_NONE;
ch_ed_mode s_Mode = CH_ED_MODE_NONE;


static int channel_edit_cmb_group_build(void);
static int app_channel_edit_group_change(void);

static void app_channel_edit_end_dialog(void)
{
    GUI_EndDialog(WND_CHANNEL_EDIT);
}

static bool channel_edit_prog_find(int32_t prog_id, int32_t *prog_pos)
{
    int pos;
    GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_NodeNumGet nodeNum = {0};

    nodeNum.node_type = NODE_PROG;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&nodeNum)))
    {
        for(pos = 0; pos < nodeNum.node_num; pos++)
        {
            node.node_type = NODE_PROG;
            node.pos = pos;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
            {
                if(node.prog_data.id == prog_id)
                {
                    app_log_debug("[%s] Find program in target viewinfo in [Pos:%d/Total:%d]\n", __FILE__, node.pos, nodeNum.node_num);
                    if(prog_pos != NULL)
                    {
                        *prog_pos = node.pos;
                    }
                    return true;
                }
            }
        }
    }

    return false;
}
static void channel_edit_change_prog_state(void)
{
	GxBusPmViewInfo view_info;
	GxMsgProperty_NodeNumGet node_num_get = {0};
    uint32_t tv_num_count = 0;
	uint32_t radio_num_count = 0;

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	node_num_get.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	if(node_num_get.node_num == 0)
	{
		view_info.group_mode = GROUP_MODE_ALL;
		view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
		if( 0 == GxBus_PmProgNumGetByViewInfo(&view_info))
		{
			if(view_info.stream_type == GXBUS_PM_PROG_RADIO )
			{
				 view_info.stream_type = GXBUS_PM_PROG_TV;
				 g_AppFullArb.state.tv = STATE_ON;
			}
			else
			{
				app_all_channels_num_check(&tv_num_count, &radio_num_count);
				if((tv_num_count==0)&&(radio_num_count==0))
				{
					view_info.stream_type = GXBUS_PM_PROG_TV;
					g_AppFullArb.state.tv = STATE_ON;
				}
				else
				{
					view_info.stream_type = GXBUS_PM_PROG_RADIO;
					g_AppFullArb.state.tv = STATE_OFF;
				}
			}
		}

		g_AppPlayOps.play_list_create(&view_info);
	}

    return;
}

static void ch_ed_progname_release_cb(PopKeyboard *data)
{
    uint32_t sel = 0;
    GxMsgProperty_NodeByIdGet node;
    GxMsgProperty_NodeModify node_modify = {0};

    if(data->in_ret == POP_VAL_CANCEL)
    {
	  GUI_SetProperty(WND_CHANNEL_EDIT,"update",NULL);
          return;
    }

    if (data->out_name == NULL)
        return;

    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
    node.node_type = NODE_PROG;
    node.id = g_AppChOps.get_id(sel);
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    node_modify.node_type = NODE_PROG;
    node_modify.prog_data = node.prog_data;
    memcpy(node_modify.prog_data.prog_name, data->out_name, MAX_PROG_NAME-1);
    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
    GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
    GUI_SetProperty(TXT_PROG_INFO, "string", node_modify.prog_data.prog_name);
#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_update_prog_total();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_prog_num_update();
#endif
}

static bool _fav_special_name_check(char *out_name)
{
    if(NULL == out_name)
        return false;

    if(0 == strcmp(out_name, "DVB-C")
            || 0 == strcmp(out_name, "DVB-T")
            || 0 == strcmp(out_name, "DVB-DTMB")
            || 0 == strcmp(out_name, "DVB-T/T2")
            || 0 == strcmp(out_name, "DVB-ATSC-C")
            || 0 == strcmp(out_name, "DVB-ATSC-T")
            || 0 == strcmp(out_name, STR_ID_ALL)
            || 0 == strcmp(out_name, STR_ID_HD)
            )
        return true;

    return false;
}

static void ch_ed_favname_release_cb(PopKeyboard *data)
{
    uint32_t sel = 0;
    GxMsgProperty_NodeByPosGet node;
    GxMsgProperty_NodeModify node_modify = {0};

    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

	//invalid characters
	if(strchr(data->out_name,',') != NULL
			|| strchr(data->out_name,'[') != NULL
			|| strchr(data->out_name,']') != NULL
            || _fav_special_name_check(data->out_name) == true)
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
		pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_INVALID_SYMBOLS;
		pop.timeout_sec = 2;
		popdlg_create(&pop);

		return;
	}

	GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
#if 1
	GxMsgProperty_NodeNumGet num_node = {0};
	int i ;
	num_node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&num_node));
#if TKGS_SUPPORT
	if (num_node.node_num>9)
	{
		num_node.node_num -= 9;
	}
#endif
	for(i = 0; i<num_node.node_num; i++)
	{
		if(i != sel)
		{
			node.pos = i;
			node.node_type = NODE_FAV_GROUP;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
			if(strcmp((const char*)data->out_name, (const char*)node.fav_item.fav_name) == 0)
			{
				PopDlg  pop;
				memset(&pop, 0, sizeof(PopDlg));
				pop.type = POP_TYPE_NO_BTN;
                pop.mode = POP_MODE_UNBLOCK;
				pop.format = POP_FORMAT_DLG;
				pop.str = STR_ID_NAME_EXIST;
				pop.timeout_sec= 2;
				popdlg_create(&pop);
				return;
			}
		}
	}
#endif

	node.pos = sel;
    node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

    node_modify.node_type = NODE_FAV_GROUP;
    node_modify.fav_item = node.fav_item;
    strncpy((char*)node_modify.fav_item.fav_name,
            (const char*)data->out_name, MAX_FAV_GROUP_NAME-1);

    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
    GUI_SetProperty(LISTVIEW_POP_FAV, "update_row", &sel);

	s_favname_flag = true;
}

static void channel_edit_show_tip(char *red, char *green, char *yellow, char *mode, char *ok)
{
#define IMG_RED     "img_ch_edit_tip_red"
#define TXT_RED     "text_ch_edit_tip_red"
#define IMG_GREEN   "img_ch_edit_tip_green"
#define TXT_GREEN   "text_ch_edit_tip_green"
#define IMG_YELLOW   "img_ch_edit_tip_yellow"
#define TXT_YELLOW   "text_ch_edit_tip_yellow"
#define IMG_MODE	"img_channel_edit_select"
#define TXT_MODE	"text_channel_edit_select"
#define IMG_OK      "img_ch_edit_tip_ok"
#define TXT_OK      "text_ch_edit_tip_ok"


    if (red == NULL)
    {
        GUI_SetProperty(IMG_RED, "state", "hide");
        GUI_SetProperty(TXT_RED, "state", "hide");
    }
    else
    {
        // first line force update img
        GUI_SetProperty(IMG_RED, "img", "s_ts_red");
        GUI_SetProperty(IMG_RED, "state", "show");
        GUI_SetProperty(TXT_RED, "string", red);
        GUI_SetProperty(TXT_RED, "state", "show");
    }

    if (green == NULL)
    {
        GUI_SetProperty(IMG_GREEN, "state", "hide");
        GUI_SetProperty(TXT_GREEN, "state", "hide");
    }
    else
    {
        GUI_SetProperty(IMG_GREEN, "state", "show");
        GUI_SetProperty(TXT_GREEN, "string", green);
        GUI_SetProperty(TXT_GREEN, "state", "show");
    }

    if ((yellow == NULL) || (0 == strcmp(STR_ID_PID, yellow)))
    {
        GUI_SetProperty(IMG_YELLOW, "state", "hide");
        GUI_SetProperty(TXT_YELLOW, "state", "hide");
    }
    else
    {
        GUI_SetProperty(IMG_YELLOW, "state", "show");
        GUI_SetProperty(TXT_YELLOW, "string", yellow);
        GUI_SetProperty(TXT_YELLOW, "state", "show");
    }

	if(mode == NULL)
	{
		GUI_SetProperty(IMG_MODE, "state", "hide");
		GUI_SetProperty(TXT_MODE, "state", "hide");
	}
	else
	{
		GUI_SetProperty(IMG_MODE, "state", "show");
		GUI_SetProperty(TXT_MODE, "string", mode);
		GUI_SetProperty(TXT_MODE, "state", "show");
	}

		if(ok == NULL)
		{
			GUI_SetProperty(IMG_OK, "state", "hide");
			GUI_SetProperty(TXT_OK, "state", "hide");
		}
		else
		{
			GUI_SetProperty(IMG_OK, "state", "show");
			GUI_SetProperty(TXT_OK, "string", ok);
			GUI_SetProperty(TXT_OK, "state", "show");
		}
}

static void ch_ed_mode_change_tips(ch_ed_mode mode)
{
	if (mode == CH_ED_MODE_NONE)
	{
		if( APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE ){
			channel_edit_show_tip(NULL, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE, NULL);
		}
		else{
			channel_edit_show_tip(STR_ID_SORT, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE, NULL);
		}
	}

	else if (mode == CH_ED_MODE_DEL ||  mode == CH_ED_MODE_LOCK || mode == CH_ED_MODE_SKIP)
	{
		channel_edit_show_tip(STR_ID_ALL, NULL, NULL, NULL, STR_ID_SELECT);
	}
#if TKGS_SUPPORT
	else if(mode == CH_ED_MODE_SWAP)
	{
		channel_edit_show_tip(NULL, NULL, NULL, NULL, STR_ID_SELECT);
	}
#else
	else if(mode == CH_ED_MODE_MOVE)
	{
		channel_edit_show_tip(NULL, STR_ID_SELECT_GROUP_CHANNELS, NULL, NULL, STR_ID_SELECT);
	}
#endif
	else
	{
		channel_edit_show_tip(NULL, NULL, NULL, NULL, STR_ID_SELECT);
	}
}

void ch_ed_mode_change(ch_ed_mode mode)
{
#define IMG_DEL         "img_ch_ed_del"
#define IMG_MOVE        "img_ch_ed_move"
#define IMG_SKIP        "img_ch_ed_skip"
#define IMG_LOCK        "img_ch_ed_lock"
#define IMG_FAV         "img_ch_ed_fav"

#define IMG_DEL_N         "img_ch_ed_del1"
#define IMG_MOVE_N        "img_ch_ed_move1"
#define IMG_SKIP_N        "img_ch_ed_skip1"
#define IMG_LOCK_N        "img_ch_ed_lock1"
#define IMG_FAV_N         "img_ch_ed_fav1"

#define TXT_DEL         "txt_ch_ed_del"
#define TXT_MOVE        "txt_ch_ed_move"
#define TXT_SKIP        "txt_ch_ed_skip"
#define TXT_LOCK        "txt_ch_ed_lock"
#define TXT_FAV         "txt_ch_ed_fav"

	GUI_SetProperty("img_ch_edit_tip_ok", "state", "hide");
	GUI_SetProperty("text_ch_edit_tip_ok", "state", "hide");
	char *mode_img[5] = {IMG_DEL, IMG_MOVE, IMG_SKIP, IMG_LOCK, IMG_FAV};

	if( APP_THEME_CLASSIC_DARK || APP_THEME_SKY_BLUE || APP_THEME_CLASSIC_PURPLE || APP_THEME_GRID_PURPLE ){
	char *mode_img_n[5] = {IMG_DEL_N, IMG_MOVE_N, IMG_SKIP_N, IMG_LOCK_N, IMG_FAV_N};

	if(s_Old_Mode != CH_ED_MODE_NONE)
	{
		GUI_SetProperty(mode_img_n[s_Old_Mode-1], "state","hide");
		GUI_SetProperty(mode_img[s_Old_Mode-1], "state","show");
	}

	s_Mode = mode;
	s_Old_Mode = mode;

	if (s_Mode != CH_ED_MODE_NONE)
	{
		GUI_SetProperty(mode_img[s_Mode-1], "state","hide");
		GUI_SetProperty(mode_img_n[s_Mode-1], "state","show");
	}
	}else{

    char *mode_text[5] = {TXT_DEL, TXT_MOVE, TXT_SKIP, TXT_LOCK, TXT_FAV};
    char *mode_str[5] = {STR_ID_DELETE, STR_ID_MOVE, STR_ID_SKIP, STR_ID_LOCK, STR_ID_FAV};
    char mode_bmp[24] = {0};

	if(s_Old_Mode != CH_ED_MODE_NONE)
	{
		sprintf(mode_bmp, "s_channellist_%d", s_Old_Mode);
		GUI_SetProperty(mode_img[s_Old_Mode-1], "img", mode_bmp);
		GUI_SetProperty(mode_text[s_Old_Mode-1], "string", mode_str[s_Old_Mode-1]);
	}

	s_Mode = mode;
	s_Old_Mode = mode;

	if (s_Mode != CH_ED_MODE_NONE)
	{
		sprintf(mode_bmp, "s_channellist_%d_c", s_Mode);
		GUI_SetProperty(mode_img[s_Mode-1], "img", mode_bmp);
		GUI_SetProperty(mode_text[s_Mode-1], "string", mode_str[s_Mode-1]);
	}
	}

    ch_ed_mode_change_tips(s_Mode);
}

void macro_ch_ed_start_video(void)
{
#define VIDEO_TEXT "txt_channel_edit_for_pp"
    int x,y,w,h = 0;
    uint32_t sel = 0;
    uint32_t prog_id = 0;
    sscanf((widget_get_property(gui_get_widget(NULL,VIDEO_TEXT),"rect")),"[%d,%d,%d,%d]",&x,&y,&w,&h);

    // zoom out

#if CVBS_SCALE_SUPPORT
    if((app_top_get_cvbs_mode_status())
		&&(w > EDIT_CVBS_OFFSET_W)
		&&(x > EDIT_CVBS_OFFSET_X)
		&&(y > EDIT_CVBS_OFFSET_Y))
    {
        x = x - EDIT_CVBS_OFFSET_X;
        y = y - EDIT_CVBS_OFFSET_Y;
        w = w - EDIT_CVBS_OFFSET_W;
    }
#endif
#if (UI_MIRROR_SUPPORT >0)
    if(app_menu_mirror_get() == 1)
    {
        x= APP_THEME_XRES -(x+w);
    }
#endif
    video_wnd.x = x;
    video_wnd.y = y;
    video_wnd.width = w;
    video_wnd.height = h;
	app_log_info("video_wnd.x = %d,video_wnd.y=%d,video_wnd.w=%d,video_wnd.h =%d\n",x,y,w,h);
	GUI_SetInterface("flush", NULL);

    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
    s_VideoStart = 1;
    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
    if(GUI_CheckDialog(WND_MAIN_MENU)==GXCORE_SUCCESS)
    {
        g_AppPlayOps.normal_play.play_count =  g_AppChOps.real_pos(sel);
        app_play_current_prog(PLAY_MODE_POINT);
    }
    else
    {
        prog_id = g_AppChOps.get_id(sel);
        if(g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK || (prog_id != app_play_id_get()))
        {
            g_AppPlayOps.normal_play.play_count =  g_AppChOps.real_pos(sel);
            app_play_current_prog(PLAY_MODE_POINT);
        }
    }
    if ( g_AppFullArb.draw[EVENT_STATE] != NULL )
    {
        g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
    }
	GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
}

static void marco_ch_ed_start_full_video(void)
{
    // zoom out full screen
    PlayerWindow video_wnd = {0, 0, APP_THEME_XRES, APP_THEME_YRES};

    GUI_SetInterface("flush", NULL);
    app_subt_change();
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
    if(app_channel_info_check_dialog() != GXCORE_SUCCESS)
    {
        app_channel_info_create_dialog();
    }
	GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
}

void macro_ch_ed_stop_video(void)
{
    // zoom in
	PlayerWindow video_wnd = {0, 0, APP_THEME_XRES, APP_THEME_YRES};

    g_AppPlayOps.program_stop();
    g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);

    s_VideoStart = 0;
}

#define START_VIDEO()   macro_ch_ed_start_video()
#define STOP_VIDEO()    macro_ch_ed_stop_video()

static void macro_ch_ed_no_prog_after_save(void)
{
	GxBusPmViewInfo view_info_temp = {0};
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
	view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
	g_AppPlayOps.play_list_create(&view_info_temp);
	g_AppChOps.update_prog_num();
	app_channel_edit_end_dialog();
	GUI_SetInterface("flush",NULL);
	STOP_VIDEO();
	channel_edit_change_prog_state();
	if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
	{
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        if (g_AppChOps.get_list_total() != 0)
        {
            g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(g_AppPlayOps.normal_play.play_count);
            app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
            g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

            if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
                g_AppFullArb.tip = FULL_STATE_LOCKED;
            else
                g_AppFullArb.tip = FULL_STATE_DUMMY;
        }
	}
}
// groub build

void app_show_channel_pid(GxBusPmDataProg *prog_data)
{
	char buffer[40] = {0};
	if(prog_data->video_pid != 0)
		sprintf(buffer, "PID V:%d A:%d PCR:%d", prog_data->video_pid,
		prog_data->cur_audio_pid,prog_data->pcr_pid);
	else
		sprintf(buffer, "PID A:%d PCR:%d", prog_data->cur_audio_pid,prog_data->pcr_pid);

	GUI_SetProperty(TXT_PID_INFO, "string",buffer);
}

static int channel_edit_get_valid_group_id(void)
{
	status_t ret = GXCORE_SUCCESS;
	AppIdOps *cl_id = sp_GpIdOps;
	if(cl_id == NULL)
		return GXCORE_ERROR;


	ret = channel_get_valid_group_id(cl_id, NULL, NULL, NULL, VIEW_INFO_SKIP_CONTAIN);
	return ret;
}

//根据当前view_info找到现在是第几个组
static uint32_t channel_edit_get_cur_group_num(void)
{
	AppIdOps *cl_id = sp_GpIdOps;
	return channel_get_cur_group_num(cl_id);
}

static int channel_edit_cmb_group_build(void)
{
#define BUFFER_LENGTH_GROUP (24)

	if(channel_edit_get_valid_group_id() == GXCORE_ERROR)
	{
		app_log_error("----get group id err! %s:%d----\n", __func__, __LINE__);
		return GXCORE_ERROR;
	}

	AppIdOps *cl_id = sp_GpIdOps;
	char *buffer_all = NULL;
	if(GXCORE_ERROR == channel_cmb_group_content_dump(cl_id, &buffer_all))
	{
		app_log_error("----channel_cmb_group_content_dump error, %s:%d----\n",__func__,__LINE__);
		return GXCORE_ERROR;
	}

	GUI_SetProperty(GROUP_CH_ED,"content",buffer_all);
	GxCore_Free(buffer_all);

	int sel=0;
	sel = channel_edit_get_cur_group_num();
	GUI_SetProperty(GROUP_CH_ED, "select", &sel);

	return GXCORE_SUCCESS;
}

int _del_program_data_clean(void)
{
	sp_GpIdOps = new_id_ops();
	if (sp_GpIdOps == NULL)
	{
		app_log_error("----sp_GpIdOps NULL err! %s:%d----\n", __func__, __LINE__);
		return GXCORE_ERROR;
	}

	if(channel_edit_get_valid_group_id() == GXCORE_ERROR)
	{
		app_log_error("----get group id err! %s:%d----\n", __func__, __LINE__);
		return GXCORE_ERROR;
	}

	AppIdOps *id_ops = sp_GpIdOps;
	uint32_t id_total = 0;
	uint32_t *id_buffer = NULL;
	if(channel_id_total_and_buffer_dump(id_ops, &id_total, &id_buffer) == GXCORE_ERROR)
	{
		app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
		del_id_ops(sp_GpIdOps);
		sp_GpIdOps = NULL;
		return GXCORE_ERROR;
	}

	uint32_t i;
	GxMsgProperty_NodeByIdGet node = {0};
	GxMsgProperty_NodeModify node_modify = {0};
	for(i=1; i<id_total; i++)
	{
		if((id_buffer[i] & FAV_MASK) == 0)//sat
		{
			node.node_type = NODE_SAT;
			node.id = id_buffer[i];
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
			node.sat_data.cur_tv = 0;
			node.sat_data.cur_radio = 0;

			node_modify.node_type = NODE_SAT;
			node_modify.sat_data = node.sat_data;
			app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
		}
		else //fav
		{
			node.node_type = NODE_FAV_GROUP;
			node.id = id_buffer[i] & (~FAV_MASK);
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
			node.fav_item.cur_tv = 0;
			node.fav_item.cur_radio = 0;

			node_modify.node_type = NODE_FAV_GROUP;
			node_modify.fav_item = node.fav_item;
			app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
		}
	}

	del_id_ops(sp_GpIdOps);
	sp_GpIdOps = NULL;
	GxCore_Free(id_buffer);

	return GXCORE_SUCCESS;
}

// cmb group
static int app_channel_edit_group_change(void)
{
	uint32_t sel = 0;
	uint32_t old_id = 0;
	GxMsgProperty_NodeByPosGet node = {0};

	GUI_GetProperty(LISTVIEW_CH_ED, "active", &sel);
	old_id = g_AppChOps.get_id(sel);

	AppIdOps *cl_id = sp_GpIdOps;
	uint32_t id_total = 0;
	uint32_t *id_buffer = NULL;
	if(channel_id_total_and_buffer_dump(cl_id, &id_total, &id_buffer) == GXCORE_ERROR)
	{
		app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
		return GXCORE_ERROR;
	}

	GUI_GetProperty(GROUP_CH_ED, "select", &sel);
	if(id_buffer[sel] == ALL_SAT)//all satellite
	{//增加sattv后，会只有sattv卫星的节目，所以第1个不一定是ALL SAT. 兼容以前
		g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
	}
	else//sat or fav
	{
		if((id_buffer[sel] & FAV_MASK) == 0)//sat
		{
			g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_SAT;
			g_AppPlayOps.normal_play.view_info.sat_id = id_buffer[sel];
		}
        else if(id_buffer[sel] == HD_MASK)
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_HD;
        }
		else//fav
		{
			g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_FAV;
			g_AppPlayOps.normal_play.view_info.fav_id = (id_buffer[sel] & (~FAV_MASK));
		}
	}
    if ( app_fuse_spec_switch_work_edit_group_change() < 0 )
    {   // 左右切换时，以上接口完成某些卫星切换，需要从usb拷贝数据等操作。
        /* 如果切换失败，下面的代码还原，发生概率很少，上面的接口channel_id_total_and_buffer_dump到
         * 调用app_fuse_spec_switch_work_edit_group_change的时候拔掉u盘才发生
         */
        sel = 0 ;
        if( id_buffer[sel] == ALL_SAT )
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
        }
        else
        {
            int i = 0 ;
            for(i=0;i<id_total;i++)
            {
                if((id_buffer[i] & FAV_MASK) == 0)//sat
                {
                    sel = i ;
                    if (g_AppPlayOps.normal_play.view_info.sat_id != id_buffer[i])
                    {
                        break;
                    }
                }
            }
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_SAT;
            g_AppPlayOps.normal_play.view_info.sat_id = id_buffer[sel];
            app_log_min("recovery sel = %d, sat id =%d \n",sel,id_buffer[sel]);
        }
        GUI_SetProperty(GROUP_CH_ED, "select", &sel);
    }
    if ( app_fuse_spec_is_sattv_work_mode() )
    {   //sattv 模式不显示按钮
        channel_edit_show_tip(NULL,NULL,NULL,NULL,NULL);
    }

    g_AppPlayOps.normal_play.view_info.taxis_mode = TAXIS_MODE_NON;
	g_AppPlayOps.normal_play.view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
	if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info)))
	{
	    GxCore_Free(id_buffer);
	    return GXCORE_ERROR;
	}

	//update listview show
	GUI_SetProperty(LISTVIEW_CH_ED,"update_all",NULL);

    sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
	GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
    GUI_SetProperty(LISTVIEW_CH_ED,"select",&sel);

    GUI_SetInterface("flush", NULL);

    if (s_VideoStart != 0)
    {
        // special deal: when just in, only do ui, play will do in VIDEO_START
        node.node_type = NODE_PROG;
        node.pos = g_AppChOps.real_pos(sel);
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

        if(old_id != node.prog_data.id || g_AppFullArb.tip !=  FULL_STATE_DUMMY)
        {
            GUI_SetProperty("txt_channel_edit_signal", "state", "show");
            if(g_AppFullArb.state.tv  == STATE_ON)
            {
                 GUI_SetInterface("video_off", NULL);
            }
            GUI_SetProperty("txt_channel_edit_signal", "draw_now", NULL);
            g_AppPlayOps.program_stop();

            g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
			app_play_current_prog(PLAY_MODE_POINT);
		}
		else
		{
			app_play_id_set(old_id);
		}
    }

    GxCore_Free(id_buffer);
	return GXCORE_SUCCESS;
}

static void _channel_edit_list_change(void)
{
#define INFO_LEN	32
	uint32_t sel=0;
	uint32_t listview_sel = 0;
	static GxMsgProperty_NodeByIdGet ProgNode; //static GxMsgProperty_NodeByPosGet ProgNode;
	static GxMsgProperty_NodeByIdGet node;
	char buffer[INFO_LEN];
    char *tp_info = NULL;

	GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
	if(sel >= g_AppChOps.get_list_total())
    {
        sel = 0;
        GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
    }

    if (g_AppChOps.check(sel, MODE_FLAG_MOVING))
        return;

    //get prog node
    GUI_SetProperty(TXT_PROG_INFO, "string", " ");
    ProgNode.node_type = NODE_PROG;
    ProgNode.id = g_AppChOps.get_id(sel);//ProgNode.pos = sel;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &ProgNode);//app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
    GUI_SetProperty(TXT_PROG_INFO, "string", ProgNode.prog_data.prog_name);

    //get sat node
    node.node_type = NODE_SAT;
    node.id = ProgNode.prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

#if DOUBLE_S2_SUPPORT
    memcpy(buffer,(char*)app_get_sat_name(&node.sat_data),MAX_SAT_NAME);
    if(node.sat_data.tuner)//T2
        strcat(buffer," ,T2");
    else
        strcat(buffer," ,T1");
    GUI_SetProperty(TXT_SAT_INFO, "string", buffer);
#else
    char *sat_name = NULL;
    GUI_GetProperty(TXT_SAT_INFO, "string", &sat_name);
    if(NULL == sat_name || strcmp(sat_name, (char *)app_get_sat_name(&node.sat_data)))
    {
        memcpy(buffer,(char*)app_get_sat_name(&node.sat_data),MAX_SAT_NAME);
        GUI_SetProperty(TXT_SAT_INFO, "string", buffer);
    }
#endif
	//TP
	channel_prog_tp_info_str_get(&ProgNode.prog_data, buffer, INFO_LEN);
    GUI_GetProperty(TXT_TP_INFO, "string", &tp_info);
    if(NULL == tp_info || strcmp(tp_info, buffer))
        GUI_SetProperty(TXT_TP_INFO, "string", buffer);


    // PID
    app_show_channel_pid(&ProgNode.prog_data);
// 20141224
	listview_sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);/*after move, list in channel edit, is differ with play list*/
	GUI_SetProperty(LISTVIEW_CH_ED, "active", &listview_sel);
}


// signal channel eidt listview
SIGNAL_HANDLER int app_channel_edit_list_change(GuiWidget *widget, void *usrdata)
{
    _channel_edit_list_change();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_edit_list_create(GuiWidget *widget, void *usrdata)
{
    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_channel_edit_list_get_total(GuiWidget *widget, void *usrdata)
{
	return g_AppChOps.get_list_total();
}

SIGNAL_HANDLER int app_channel_edit_list_get_data(GuiWidget *widget, void *usrdata)
{
#define PROG_COUNT_LEN  (8)
#define PROG_NAME_LEN   (24)
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByIdGet node;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	node.node_type = NODE_PROG;
	node.id= g_AppChOps.get_id(item->sel);
	//get node
	if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
        return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
	item->image = NULL;
	item->string = app_fuse_common_get_pragram_num_str(item->sel);
	//col-1: scramble flag
	item = item->next;
	if (item == NULL) return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;

	if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
		item->string = "$";
	else
		item->string = NULL;

	// program name
	item = item->next;
	if (item == NULL) return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;
	item->string = (char*)node.prog_data.prog_name;

	//col-2: del
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_DEL))
	{
        item->image = "s_icon_del";
	}

	//col-3: move
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_MOVING)
	    || g_AppChOps.check(item->sel, MODE_FLAG_MOVE))
	{
        item->image = "s_icon_move";
	}

	//col-4: sikp
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;

	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_SKIP))
	{
        item->image = "s_icon_skip";
	}

	//col-5: lock
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;

	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_LOCK))
	{
        item->image = "s_icon_lock";
	}

	//col-6: fav
	item = item->next;
    if (item == NULL) return GXCORE_ERROR;

	item->x_offset = 0;
	item->string = NULL;
	item->image = NULL;
	if (g_AppChOps.check(item->sel, MODE_FLAG_FAV))
	{
        item->image = "s_icon_fav";
	}

	return EVENT_TRANSFER_STOP;
}


int ch_ed_lock_passwd_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result == PASSWD_OK)
	{
		ch_ed_mode_change(CH_ED_MODE_LOCK);
    }
    else if(ret->result == PASSWD_ERROR)
    {
		PasswdDlg passwd_dlg;
		memset(&passwd_dlg, 0, sizeof(PasswdDlg));
		passwd_dlg.passwd_exit_cb = ch_ed_lock_passwd_cb;
		passwd_dlg.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
		passwd_dlg.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
		passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg_create(&passwd_dlg);
    }
    else
    {}

	return 0;
}

static void _ch_ed_pop_sort_exit(PopDlgRet ret, uint32_t pos)
{
    uint32_t sel = 0;
	if(ret == POP_VAL_OK)
	{
		if(g_AppPlayOps.normal_play.rec == PLAY_KEY_UNLOCK && GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS)
		{
			g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
		}
		g_AppPlayOps.normal_play.play_count = pos;
        g_AppPlayOps.current_play= g_AppPlayOps.normal_play;

	}
	else
	{
		ch_ed_mode_change(CH_ED_MODE_NONE);
		g_AppChOps.create(g_AppPlayOps.normal_play.play_total);
	}
	sel = g_AppChOps.real_select(pos);
	GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
	GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
	GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);

}

static void _ch_ed_pop_pid_exit(PopDlgRet ret, uint32_t pid_v, uint32_t pid_a, uint32_t pid_pcr, uint32_t s_video_sel, uint32_t s_audio_sel)
{
	static int video_status = 0;

	if(ret == POP_VAL_OK)
	{
		uint32_t sel = 0;
        uint32_t pos = 0;
		GxMsgProperty_NodeByPosGet prog_node = {0}	;

		GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
	    pos = g_AppChOps.real_pos(sel);
		prog_node.node_type = NODE_PROG;
		prog_node.pos = pos;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node)))
		{
			GxMsgProperty_NodeModify prog_modify = {0};
			prog_modify.node_type= NODE_PROG;
			memcpy(&prog_modify.prog_data, &prog_node.prog_data, sizeof(GxBusPmDataProg));
			if(pid_v < 0x1fff)
			{
				prog_modify.prog_data.video_pid = pid_v;
			}
			if(pid_a < 0x1fff)
			{
				prog_modify.prog_data.cur_audio_pid = pid_a;
			}
			if(pid_pcr < 0x1fff)
			{
				prog_modify.prog_data.pcr_pid = pid_pcr;
			}
			prog_modify.prog_data.video_type = channel_get_bus_video_type((AppProgVideoType)s_video_sel);
			prog_modify.prog_data.cur_audio_type = channel_get_bus_audio_type((AppProgAudioType)s_audio_sel);
			app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &prog_modify);

			if(app_check_av_running() == true)
			{
				 g_AppFullArb.timer_stop();//20121108
				 video_status = 1;
			}
			else
			{
				if(video_status)
				{
					video_status = 0;
					 g_AppFullArb.timer_start();//20121108
				}
			}

            g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);//bug 56026
			g_AppPlayOps.program_play(PLAY_MODE_POINT, pos);
            sel = g_AppChOps.real_select(pos);
		    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
			GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
			app_show_channel_pid(&prog_modify.prog_data);

		}
	}
}
static int channel_edit_pid_modify_func(int key)
{
#define DEFAULT_ENTRY_KEY_LEN 4

	int key_code[DEFAULT_ENTRY_KEY_LEN] = {STBK_8, STBK_8, STBK_9, STBK_8};
	static int correct_code_num = 0;
	if((key_code[correct_code_num] == key))
	{
		if(++correct_code_num == DEFAULT_ENTRY_KEY_LEN)
		{
			correct_code_num = 0;

			// key code of pid_edit is 8898
			#if TKGS_SUPPORT
			if(app_check_tkgs_control_db() != 0)
			{
			}
			else
			#endif
			{
				PopDlgPos pop_pos = {APP_WND_CH_EDIT_POPDLG_POS_X, APP_WND_CH_EDIT_POPDLG_POS_Y};
				uint32_t cur_prog_id = 0;
				uint32_t sel=0;
                uint32_t pos=0;
				GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
				pos = g_AppChOps.real_pos(sel);
				channel_edit_show_tip(NULL, NULL, NULL, NULL, NULL);

				GxMsgProperty_NodeByPosGet ProgNode;
				ProgNode.node_type = NODE_PROG;
				ProgNode.pos = pos;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
				cur_prog_id = ProgNode.prog_data.id;
				app_ch_pid_pop(pop_pos, cur_prog_id, _ch_ed_pop_pid_exit);
			}
		}
	}
	else
	{
		correct_code_num = 0;
	}

	return 0;
}

static int channel_edit_change_tv_radio_mode(void)
{
    uint32_t sel = 0;
    unsigned int radio_num = 0;
    unsigned int tv_num = 0;
    unsigned int num_is_zero = 0 ;
    ch_ed_mode   temp_work_mode = CH_ED_MODE_NONE;

    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        radio_num = app_check_channel_num(GXBUS_PM_PROG_RADIO);
        if(radio_num > 0)
        {
            g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_RADIO;
            g_AppFullArb.state.tv = STATE_OFF;
            if ( g_AppFullArb.draw[EVENT_STATE] != NULL )
            {
                g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
            }
            ch_ed_mode_change(CH_ED_MODE_NONE);
            channel_edit_cmb_group_build();
            GUI_SetProperty(GROUP_CH_ED, "select", &sel);
            app_channel_edit_group_change();
            _channel_edit_list_change();
            /*when you switch to raido channel list, make sure spp layer is on top*/
            GUI_SetInterface("video_off", NULL);
            return 0 ;
        }
        else
        {
           num_is_zero = 1;
        }
    }
    else
    {
        tv_num = app_check_channel_num(GXBUS_PM_PROG_TV);
        if(tv_num > 0)
        {
            g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
            g_AppFullArb.state.tv = STATE_ON;
            g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
            ch_ed_mode_change(CH_ED_MODE_NONE);
            channel_edit_cmb_group_build();
            GUI_SetProperty(GROUP_CH_ED, "select", &sel);
            app_channel_edit_group_change();
            _channel_edit_list_change();
            return 0 ;
        }
        else
        {
            num_is_zero = 1;
        }
    }

    if( num_is_zero>0 )
    {
        temp_work_mode = s_Mode ;
        if ( (temp_work_mode == CH_ED_MODE_DEL) && (s_save_flag == true) )
        {
            STOP_VIDEO();
        }

        ch_ed_mode_change(CH_ED_MODE_NONE);
        channel_edit_cmb_group_build();
        GUI_SetProperty(GROUP_CH_ED, "select", &sel);
        app_channel_edit_group_change();
        _channel_edit_list_change();

        if ( (temp_work_mode == CH_ED_MODE_DEL) && (s_save_flag == true) )
        {
            START_VIDEO();
        }
    }
    return 0;
}
static int _channel_edit_tv_radio_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
    PasswdDlg passwd_dlg;
	if(ret->result ==  PASSWD_OK)
	{
        channel_edit_change_tv_radio_mode();
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_exit_cb = _channel_edit_tv_radio_passwd_dlg_cb;
        passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
        passwd_dlg.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }

	return 0;
}

static int channel_edit_create_tv_radio_menu(void)
{
    int init_value = 0;
    PasswdDlg passwd_dlg;
    GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
    if(init_value == 1)
    {
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_exit_cb = _channel_edit_tv_radio_passwd_dlg_cb;
        passwd_dlg.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
        passwd_dlg.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }
    else
    {
        channel_edit_change_tv_radio_mode();
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static int _ch_edit_pop_save_common1(PopDlgRet ret)
{
    PlayerStatusInfo player_status;

    memset(&player_status, 0, sizeof(player_status));
    GxPlayer_MediaGetStatus(PLAYER_FOR_NORMAL, &player_status);

    if(player_status.vcodec.state == AVCODEC_RUNNING) //radio
    {
        if(g_AppFullArb.state.tv == STATE_ON)
        {
            app_set_avcodec_flag(true);
        }
    }

    if(player_status.acodec.state == AVCODEC_RUNNING) //radio
    {
        if(g_AppFullArb.state.tv == STATE_OFF)
        {
            app_set_avcodec_flag(true);
        }
    }

    if((player_status.vcodec.state == AVCODEC_STOPPED) //tv
            || (player_status.vcodec.state == AVCODEC_READY))
    {
        if(g_AppFullArb.state.tv == STATE_ON)
        {
            app_set_avcodec_flag(false);
        }
    }

    if((player_status.acodec.state == AVCODEC_STOPPED) //radio
            ||(player_status.acodec.state == AVCODEC_READY))
    {
        if(g_AppFullArb.state.tv == STATE_OFF)
        {
            app_set_avcodec_flag(false);
        }
    }

    return 0;
}

static int _ch_edit_pop_save_common2(PopDlgRet ret)
{
    uint32_t sel;
    GxBusPmViewInfo  view_info_temp = {0};
    GxMsgProperty_NodeNumGet node_num_get = {0};

    if (POP_VAL_OK == ret)
    {
        // do save
        g_AppChOps.save();
        s_save_flag = true;

#if SAT2IP_SERVER_SUPPORT
        app_sat2ip_update_prog_total();
#endif
#if DVB2IP_SERVER_SUPPORT
        app_dvb2ip_prog_num_update();
#endif
        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
        node_num_get.node_type = NODE_PROG;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
        if(node_num_get.node_num == 0 && view_info_temp.stream_type == GXBUS_PM_PROG_TV)
        {
            GxBus_ConfigSetInt(ALL_TV_KEY, ALL_TV_ID);
        }
        if(node_num_get.node_num == 0 && view_info_temp.stream_type == GXBUS_PM_PROG_RADIO)
        {
            GxBus_ConfigSetInt(ALL_RADIO_KEY, ALL_RADIO_ID);
        }
        view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;

        if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&view_info_temp))
        {
            return -1;
        }
        GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
    }
    else
    {
        g_AppChOps.create(g_AppPlayOps.normal_play.play_total);
        sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
        if(GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS)
        {
            GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
            GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
        }
    }

    return 0 ;
}

static int _ch_edit_key_tvradio_popdlg_exit_cb(PopDlgRet ret)
{
    int save_result ;

    _ch_edit_pop_save_common1(ret);
    save_result = _ch_edit_pop_save_common2(ret);

    if ( save_result < 0 )
    {
        macro_ch_ed_no_prog_after_save();
    }
    else
    {
        channel_edit_create_tv_radio_menu();
    }

    return 0 ;
}

static int _ch_edit_key_exit_ending_proc(void)
{
    GxBusPmViewInfo  view_info_temp = {0};

    if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
    {
        return 0;//错误43583
    }

    s_VideoStart = 0;

    // recover view mode & list, without skip prog
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
    view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
    g_AppPlayOps.play_list_create(&view_info_temp);
    g_AppChOps.update_prog_num();
    if((g_AppChOps.get_tv_num()+g_AppChOps.get_radio_num() == 0))
    {
#if BOUQUET_SUPPORT
        g_AppBat.stop(&g_AppBat);
        app_bouquet_clear_all();
#endif
        if(g_AppFullArb.state.mute == STATE_ON)
            g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
    }

    if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
    {
        int32_t prog_pos = -1;
        int32_t prog_id = 0;

        channel_edit_change_prog_state();
        GxBusPmViewInfo view_info;
        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
        if(view_info.stream_type == GXBUS_PM_PROG_TV)
        {
            prog_id = g_AppPlayOps.normal_play.view_info.tv_prog_cur;
        }
        else
        {
            prog_id = g_AppPlayOps.normal_play.view_info.radio_prog_cur;
        }
        app_channel_edit_end_dialog();
        if(true == channel_edit_prog_find(prog_id, &prog_pos))
        {
            if((prog_pos != g_AppPlayOps.normal_play.play_count)
                ||(view_info_temp.stream_type != view_info.stream_type)
                || g_AppFullArb.tip != FULL_STATE_DUMMY)
            {
                g_AppPlayOps.program_stop();
                g_AppPlayOps.program_play(PLAY_MODE_POINT, prog_pos);
            }
            else
            {
#if AUTO_UPDATE_SUPPORT
                GxMsgProperty_NodeByPosGet node = {0};
                node.node_type = NODE_PROG;
                node.pos = prog_pos;
                app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
                app_auto_update_start(&node);
#endif
                app_update_recall_info();
            }
        }
        else
        {
            g_AppPlayOps.program_stop();
            app_play_current_prog(PLAY_MODE_POINT);
        }
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
        if(g_AppChOps.get_list_total() == 0)
        {
            GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
            STOP_VIDEO();
        }
        else
        {
            marco_ch_ed_start_full_video();
        }
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    }
    else
    {
        GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
        STOP_VIDEO();
        app_channel_edit_end_dialog();
        /*when back to main menu, make sure the spp layer is on top*/
        GUI_SetInterface("video_off", NULL);
        if(s_save_flag == TRUE)
            channel_edit_change_prog_state();
    }

    return 0 ;
}

static int _ch_edit_key_exit_popdlg_exit_cb(PopDlgRet ret)
{
    _ch_edit_pop_save_common1(ret);
    _ch_edit_pop_save_common2(ret);

    _ch_edit_key_exit_ending_proc();

    return 0 ;
}

static int _ch_edit_exit_dialog_recovery_change(void)
{
    GxBusPmViewInfo  view_info_temp = {0};

    if ( (g_AppChOps.verity != NULL ) && (g_AppChOps.verity() == true) )
    {
        // recover view mode & list, without skip prog
        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
        view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
        g_AppPlayOps.play_list_create(&view_info_temp);
        g_AppChOps.update_prog_num();
    }
    return 0 ;
}

static int _ch_edit_key_red_popdlg_proc_ret_ok(void)
{
    uint32_t pos=0;
    PopDlgPos pop_pos = {0};
    uint32_t sel;
    GxBusPmViewInfo  viewinfo_old, viewinfo_new;
    GxMsgProperty_NodeByPosGet node;
    uint16_t id_bak;

    sel = app_pop_get_temp_data(1);
    viewinfo_old.group_mode = app_pop_get_temp_data(2);
    id_bak = app_pop_get_temp_data(3);

    if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
    {
        return 0;//错误43583
    }

    memset(&node, 0, sizeof(GxMsgProperty_NodeByPosGet));
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&viewinfo_new));
    bool play_already_flag = false;
    if((id_bak != node.prog_data.id)
            && (viewinfo_new.group_mode == viewinfo_old.group_mode)) //û?и???viewinfo
    {
        pos = g_AppChOps.real_pos(sel);
        g_AppPlayOps.normal_play.play_count = pos;//old_playcount ;
        app_log_info("-----[LINE:%d][play_count:%d]-----\n", __LINE__, pos);
        g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
        app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
        play_already_flag = true;
    }

    memset(&node, 0, sizeof(GxMsgProperty_NodeByPosGet));
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
    if( !play_already_flag
         && (viewinfo_new.group_mode != viewinfo_old.group_mode//??????viewinfo
         || (false == app_check_av_running() && GXBUS_PM_PROG_BOOL_DISABLE == node.prog_data.lock_flag
         && node.prog_data.id != app_play_id_get())))
    {
         g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
         app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
    }

    //此处不能去掉，否则底部tips消失
    sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
    GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
    GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
    _channel_edit_list_change();

    channel_edit_cmb_group_build();
	channel_edit_show_tip(NULL, NULL, NULL, NULL, NULL);
    app_pop_sort_create(pop_pos, g_AppPlayOps.normal_play.play_count, _ch_ed_pop_sort_exit);

    //错误 #57772 后面新增问题
    sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
    GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
    GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
    _channel_edit_list_change();

    return 0 ;
}

static int _ch_edit_key_red_popdlg_exit_cb(PopDlgRet ret)
{
    int save_result ;

    _ch_edit_pop_save_common1(ret);
    save_result = _ch_edit_pop_save_common2(ret);

    if ( save_result < 0 )
    {
        macro_ch_ed_no_prog_after_save();
    }
    else
    {
        _ch_edit_key_red_popdlg_proc_ret_ok();
    }

    return 0 ;
}

static uint32_t _get_cmb_sel_after_save(uint32_t total_id, uint32_t *id_buffer, uint32_t total_id_bak,
                                        uint32_t *id_buffer_bak, uint32_t cmb_sel_bak, unsigned short key);

static int _ch_edit_key_leftright_popdlg_proc_ret_ok(void)
{
    uint32_t cmb_sel = 0;
    uint32_t id_total_bak = 0;
    uint32_t *id_buffer_bak = NULL;
    uint32_t sym;

    id_total_bak = app_pop_get_temp_data(1);
    id_buffer_bak = (uint32_t *)(app_pop_get_temp_data(2));
    cmb_sel = app_pop_get_temp_data(3);
    sym = app_pop_get_temp_data(4);

    if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
    {
        GxCore_Free(id_buffer_bak);
        return 0;//错误43583
    }
    if (id_total_bak <= 1)
    {//增加sattv后，有可能只有1个卫星，那不需要切换。兼容以前。
        GxCore_Free(id_buffer_bak);
        ch_ed_mode_change(CH_ED_MODE_NONE);
        return 0;
    }

    channel_edit_cmb_group_build();
    ch_ed_mode_change(CH_ED_MODE_NONE);

    //get all of id after save
    uint32_t id_total = 0;
    uint32_t *id_buffer = NULL;
    if(channel_id_total_and_buffer_dump(sp_GpIdOps, &id_total, &id_buffer) == GXCORE_ERROR)
    {
        app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
        GxCore_Free(id_buffer_bak);
        return GXCORE_ERROR;
    }

    uint32_t cmb_sel_set = 0;
    cmb_sel_set = _get_cmb_sel_after_save(id_total, id_buffer, id_total_bak, id_buffer_bak, cmb_sel, sym);
    GUI_SetProperty(GROUP_CH_ED, "select", &cmb_sel_set);
    app_channel_edit_group_change();
    _channel_edit_list_change();

    GxCore_Free(id_buffer_bak);
    GxCore_Free(id_buffer);
    return 0 ;
}

static int _ch_edit_key_leftright_popdlg_exit_cb(PopDlgRet ret)
{
    int save_result ;

    _ch_edit_pop_save_common1(ret);
    save_result = _ch_edit_pop_save_common2(ret);

    if (save_result < 0)
    {
        macro_ch_ed_no_prog_after_save();
        uint32_t *id_buffer_bak = NULL;
        id_buffer_bak = (uint32_t *)(app_pop_get_temp_data(2));
        GxCore_Free(id_buffer_bak);
    }
    else
    {
        _ch_edit_key_leftright_popdlg_proc_ret_ok();
    }

    return 0 ;
}

static int _ch_edit_popdlg_is_create(ExitCb  exit_cb)
{
    if (g_AppChOps.verity() == true)
    {
        GUI_SetProperty("img_ch_edit_tip_ok", "state", "hide");
        GUI_SetProperty("text_ch_edit_tip_ok", "state", "hide");
        if( APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE ){
            channel_edit_show_tip(NULL, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE, NULL);
        }else{
            channel_edit_show_tip(STR_ID_SORT, STR_ID_RENAME, STR_ID_PID, STR_ID_MODE, NULL);
        }
        s_save_flag = false;

        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_SAVE_INFO;
        pop.format = POP_FORMAT_DLG;
        pop.exit_cb = exit_cb;
        popdlg_create(&pop);

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////////


//sattv时候，按钮key按键功能无效。
static int _channel_edit_check_keypress(unsigned int scan_code, unsigned int sym)
{
	if ( app_fuse_spec_is_sattv_work_mode() )
	{
		switch(find_virtualkey_ex(scan_code,sym))
		{
			case STBK_1:
			case STBK_2:
			case STBK_3:
			case STBK_4:
			case STBK_5:
			case STBK_RED:
			case STBK_GREEN:
				return 1 ;
			default:
				break;
		}
	}
	return 0 ;
}

int channel_edit_mode_off_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t sel=0;
    uint32_t pos=0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			if ( _channel_edit_check_keypress(event->key.scancode,event->key.sym) )
				return ret ;
			channel_edit_pid_modify_func(find_virtualkey_ex(event->key.scancode,event->key.sym));

			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_EXIT:
				case STBK_MENU:
				case STBK_DOWN:
				case STBK_UP:
				case STBK_LEFT:
				case STBK_RIGHT:
				case STBK_TV_RADIO:
				    ret = EVENT_TRANSFER_KEEPON;
					break;
				case STBK_PAGE_UP:
				{
					uint32_t sel;
					GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
					if (sel == 0)
					{
						sel = g_AppChOps.get_list_total() - 1;
						GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
						ret =   EVENT_TRANSFER_STOP;
					}
					else
					{
						ret =  EVENT_TRANSFER_KEEPON;
					}
				}
					break;
				case STBK_PAGE_DOWN:
				{
					uint32_t sel;
					GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
					if (g_AppChOps.get_list_total() == sel+1)
					{
						sel = 0;
						GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
						ret =  EVENT_TRANSFER_STOP;
					}
					else
					{
						ret =  EVENT_TRANSFER_KEEPON;
					}
				}
					break;

				case STBK_1:
				    // del
				    #if TKGS_SUPPORT
				    if(GX_TKGS_OFF != app_tkgs_get_operatemode())
				    {
				    	app_tkgs_cantwork_popup();
					break;
				    }
				    #endif
					ch_ed_mode_change(CH_ED_MODE_DEL);
				    break;
				case STBK_2:
                    {
                        // move
                        #if TKGS_SUPPORT
                        if(app_check_tkgs_control_db() != 0)
                            break;
                        else
                            ch_ed_mode_change(CH_ED_MODE_SWAP);
                        #elif LCN_SUPPORT
                            int32_t lcn_config;
                            int32_t sort_flag;
                            GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);
                            GxBus_ConfigGetInt(T2_SORT_FLAG, &sort_flag, T2_SORT_FLAG_VALUE);
                            if(lcn_config ==1 && sort_flag == 2)
                            {
                                PopDlg  pop = {0};
                                pop.type = POP_TYPE_OK;
                                pop.mode = POP_MODE_UNBLOCK;
                                pop.str = STR_ID_SORT_BY_LCN_MOVE;
                                pop.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
                                pop.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
                                pop.exit_cb = NULL;
                                popdlg_create(&pop);
                            }
                            else
                                ch_ed_mode_change(CH_ED_MODE_MOVE);
                        #else
                            ch_ed_mode_change(CH_ED_MODE_MOVE);
                        #endif
                    }
                    break;
				case STBK_3:
				    // skip
					ch_ed_mode_change(CH_ED_MODE_SKIP);
				    break;
				case STBK_4:
				    // lock
				    {
				        int32_t lock_state = 0;
				        GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &lock_state, CHANNEL_LOCK);
				        if (CHANNEL_LOCK_ENABLE == lock_state)
				        {
				            PasswdDlg passwd_dlg;
				            memset(&passwd_dlg, 0, sizeof(PasswdDlg));
							passwd_dlg.passwd_exit_cb = ch_ed_lock_passwd_cb;
							passwd_dlg.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
							passwd_dlg.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
							passwd_dlg_create(&passwd_dlg);
				        }
				        else
				        {
							ch_ed_mode_change(CH_ED_MODE_LOCK);
				        }
				    }
				    break;
				case STBK_5:
				    // fav
					ch_ed_mode_change(CH_ED_MODE_FAV);
				    break;
				case STBK_OK:
					{
						GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
						pos = g_AppChOps.real_pos(sel);
                        if((pos != g_AppPlayOps.normal_play.play_count)
                                || (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK))
                        {
                            GUI_SetProperty("txt_channel_edit_signal", "state", "show");
                            if(g_AppFullArb.state.tv  == STATE_ON)
                            {
                                GUI_SetInterface("video_off", NULL);
                            }
                            GUI_SetProperty("txt_channel_edit_signal", "draw_now", NULL);

							g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
							g_AppPlayOps.program_play(PLAY_MODE_POINT,pos);
							sel = g_AppChOps.real_select(pos);
							GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);//20141224
                        }
                    }
				    break;

                case STBK_RED:
                    if( APP_THEME_CLASSIC_DARK || APP_THEME_CLASSIC_BLUE || APP_THEME_GRID_PURPLE ){
                    #if TKGS_SUPPORT
                    if(app_check_tkgs_control_db() != 0)
                    {
                        break;
                    }
                    else
                    #endif
                    {
                        uint32_t sel;
                        GUI_GetProperty(LISTVIEW_CH_ED, "active", &sel);

                        GxBusPmViewInfo  viewinfo_old; //, viewinfo_new;
                        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&viewinfo_old));

                        uint16_t id_bak;
                        GxMsgProperty_NodeByPosGet node;
                        memset(&node, 0, sizeof(GxMsgProperty_NodeByPosGet));
                        node.node_type = NODE_PROG;
                        node.pos = g_AppPlayOps.normal_play.play_count;
                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
                        id_bak = node.prog_data.id;

                        app_pop_set_temp_data(sel,viewinfo_old.group_mode,id_bak,0);
                        if ( false == _ch_edit_popdlg_is_create(_ch_edit_key_red_popdlg_exit_cb))
                        {
                            _ch_edit_key_red_popdlg_proc_ret_ok();
                        }
                        break;
                    }
                    }
                    else{
                        break;
                    }

				case STBK_GREEN:
					#if TKGS_SUPPORT
					if(app_check_tkgs_control_db() != 0)
					{
						break;
					}
					else
					#endif
				    {
				        static PopKeyboard keyboard;
				        static GxMsgProperty_NodeByIdGet node;

						channel_edit_show_tip(NULL, NULL, NULL, NULL, NULL);
				        GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
				        node.node_type = NODE_PROG;
				        node.id = g_AppChOps.get_id(sel);
				        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
				        memset(&keyboard, 0, sizeof(PopKeyboard));
				        keyboard.in_name    = (char*)(node.prog_data.prog_name);
				        keyboard.max_num = MAX_PROG_NAME - 1;
				        keyboard.out_name   = NULL;
				        keyboard.change_cb  = NULL;
				        keyboard.release_cb = ch_ed_progname_release_cb;
                        keyboard.usr_data   = FILE_CHAR_NAME_INVALID;
						keyboard.pos.x = 200;
						keyboard.pos.y = 130;
                        multi_language_keyboard_create(&keyboard);
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

static void ch_ed_all_select(uint64_t mode)
{
    uint32_t i=0;
    uint32_t sel=0;
    uint32_t flag = 0;

    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);

    for (i=0; i<g_AppChOps.get_list_total(); i++)
    {
        if (g_AppChOps.check(i, mode) == FALSE)
        {
            flag = 1;
            break;
        }
    }

    if (flag == 1)
    {
        for (i=0; i<g_AppChOps.get_list_total(); i++)
        {
            if (g_AppChOps.check(i, mode) == FALSE)
                g_AppChOps.set(i, mode);

        }
    }
    else
    {
        for (i=0; i<g_AppChOps.get_list_total(); i++)
        {
            g_AppChOps.set(i, mode);
        }
    }

    GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
    GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
	sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);/*after move, list in channel edit, is differ with play list*/
	GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);

}
int channel_edit_mode_on_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t sel=0;
#define ONE_PAGE_NUM 10
    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_EXIT:
                case STBK_MENU:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_DOWN:
                case STBK_UP:
#if TKGS_SUPPORT
#else
                    // move mode & normal move
                    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                    if ((s_Mode == CH_ED_MODE_MOVE) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
                    {
                        if (event->key.sym == STBK_UP)
                        {
                            if (sel != 0)
                            {
                                g_AppChOps.move(sel,sel-1);
                            }
                            else
                            {
                                g_AppChOps.move(sel,(g_AppChOps.get_list_total()-1));
                            }
                        }
                        else
                        {
                            if (sel != (g_AppChOps.get_list_total()-1))
                            {
                                g_AppChOps.move(sel,sel+1);
                            }
                            else
                            {
                                g_AppChOps.move(sel,0);
                            }
                        }
                        sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);/*after move, list in channel edit, is differ with play list*/
                        GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                    }
                    else
                    {

                    }
#endif
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_PAGE_UP:
                    {
                        uint32_t sel;
                        GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
						uint32_t row;
						GUI_GetProperty(LISTVIEW_CH_ED, "visible_row", &row);
#if TKGS_SUPPORT
                        if((s_Mode == CH_ED_MODE_SWAP) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
#else
                        if((s_Mode == CH_ED_MODE_MOVE) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
#endif
                        {
                            if(sel == 0)
                            {
                                g_AppChOps.move(sel,(g_AppChOps.get_list_total()-1));
                                sel = g_AppChOps.get_list_total()-1;
                            }
							else if(sel < row)
                            {
                                g_AppChOps.move(sel,0);
                                sel = 0;
                            }
                            else
                            {
								g_AppChOps.move(sel,sel-row);
								sel = sel-row;
                            }
                            GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                            sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);/*after move, list in channel edit, is differ with play list*/
                            GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                            ret =  EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            if (sel == 0)
                            {
                                sel = g_AppChOps.get_list_total()-1;
                                GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                                ret = EVENT_TRANSFER_STOP;
                            }
                            else
                            {
                                ret =  EVENT_TRANSFER_KEEPON;
                            }
                        }
                    }
                    break;
                case STBK_PAGE_DOWN:
                    {
                        uint32_t sel;
                        GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
						uint32_t row;
						GUI_GetProperty(LISTVIEW_CH_ED, "visible_row", &row);
#if TKGS_SUPPORT
                        if((s_Mode == CH_ED_MODE_SWAP) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
#else
                        if((s_Mode == CH_ED_MODE_MOVE) && (g_AppChOps.check(sel, MODE_FLAG_MOVING)))
#endif
                        {
                            if(sel == g_AppChOps.get_list_total()-1)
                            {
                                g_AppChOps.move(sel,0);
                                sel = 0;
                            }
							else if((sel+row) > (g_AppChOps.get_list_total()-1))
                            {
                                g_AppChOps.move(sel,(g_AppChOps.get_list_total()-1));
                                sel = g_AppChOps.get_list_total()-1;
                            }
                            else
                            {
								g_AppChOps.move(sel,sel+row);
								sel = sel+row;

                            }
                            GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                            sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);/*after move, list in channel edit, is differ with play list*/
                            GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                            ret =  EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            if (g_AppChOps.get_list_total() == sel+1)
                            {
                                sel = 0;
                                GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                                ret = EVENT_TRANSFER_STOP;
                            }
                            else
                            {
                                ret =  EVENT_TRANSFER_KEEPON;
                            }
                        }
                    }
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:
                case STBK_TV_RADIO:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_1:
                case STBK_2:
                case STBK_3:
                case STBK_4:
                case STBK_5:
                    {
                        if(g_AppChOps.group_check(MODE_FLAG_MOVE) > 0)
                        {
							GUI_GetProperty(LISTVIEW_CH_ED, "active", &sel);
                            g_AppChOps.group_clear(MODE_FLAG_MOVE);
                            GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
							GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                        }
                        else
                        {
                            GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                            if(g_AppChOps.check(sel, MODE_FLAG_MOVING) == true)
                            {
                                g_AppChOps.clear(sel, MODE_FLAG_MOVING);
                                GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            }
                        }
					ch_ed_mode_change(CH_ED_MODE_NONE);

                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }

                case STBK_OK:

                    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);

                    switch (s_Mode)
                    {
                        case CH_ED_MODE_DEL:
                            g_AppChOps.set(sel, MODE_FLAG_DEL);
                            GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            break;
                        case CH_ED_MODE_LOCK:
                            g_AppChOps.set(sel, MODE_FLAG_LOCK);
                            GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            break;
                        case CH_ED_MODE_SKIP:
                            g_AppChOps.set(sel, MODE_FLAG_SKIP);
                            GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                            break;
#if TKGS_SUPPORT
                        case CH_ED_MODE_SWAP:
                            {
                                if(g_AppChOps.group_check(MODE_FLAG_MOVE) > 0&&selbackuppos!=0xffffffff)
                                {
                                    g_AppChOps.move(selbackuppos,sel);
                                    g_AppChOps.move(sel-1,selbackuppos);
                                    g_AppChOps.clear(sel, MODE_FLAG_MOVING);
                                    g_AppChOps.group_clear(MODE_FLAG_MOVE);
                                    if(g_AppChOps.check(sel, MODE_FLAG_MOVING) == false)
                                    {
										channel_edit_show_tip(NULL, NULL,NULL, NULL, STR_ID_SELECT);
                                    }
                                    else
                                    {
										channel_edit_show_tip(NULL, NULL ,NULL, NULL, STR_ID_MOVE);
                                    }
                                    GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
                                    GUI_SetProperty(LISTVIEW_CH_ED, "select", &sel);
                                    selbackuppos=0xffffffff;
                                }
                                else
                                {
                                    g_AppChOps.set(sel, MODE_FLAG_MOVE);
                                    selbackuppos = sel;
									channel_edit_show_tip(NULL,NULL,NULL,NULL, STR_ID_SWAP);
                                    GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                                }

                                break;
                            }
#else
                        case CH_ED_MODE_MOVE:
                            {
                                if(g_AppChOps.group_check(MODE_FLAG_MOVE) > 0)
                                {
                                    g_AppChOps.group_move(sel);
                                    if(g_AppChOps.check(sel, MODE_FLAG_MOVING) == false)
                                    {
										channel_edit_show_tip(NULL, STR_ID_SELECT_GROUP_CHANNELS,NULL, NULL, STR_ID_SELECT);
                                    }
                                    else
                                    {
										channel_edit_show_tip(NULL, NULL ,NULL, NULL, STR_ID_MOVE);
                                    }

                                    GUI_SetProperty(LISTVIEW_CH_ED, "update_all", NULL);
                                    sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);/*after move, list in channel edit, is differ with play list*/
                                    GUI_SetProperty(LISTVIEW_CH_ED, "active", &sel);
                                    _channel_edit_list_change();
                                }
                                else
                                {

                                    g_AppChOps.set(sel, MODE_FLAG_MOVING);
                                    if(g_AppChOps.check(sel, MODE_FLAG_MOVING) == false)
                                    {
										channel_edit_show_tip(NULL, STR_ID_SELECT_GROUP_CHANNELS,NULL, NULL, STR_ID_SELECT);
                                    }
                                    else
                                    {
										channel_edit_show_tip(NULL,NULL,NULL,NULL, STR_ID_MOVE);
                                    }

                                    GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                                }

                                break;
                            }
#endif
                        case CH_ED_MODE_FAV:
                            s_ListSel = sel;
                            GUI_CreateDialog(WND_POP_FAV);
                            break;
                        default:
                            break;
                    }
                    break;

                case STBK_RED:

                    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);

                    switch (s_Mode)
                    {
                        case CH_ED_MODE_DEL:
                            ch_ed_all_select(MODE_FLAG_DEL);
                            break;
                        case CH_ED_MODE_LOCK:
                            ch_ed_all_select(MODE_FLAG_LOCK);
                            break;
                        case CH_ED_MODE_SKIP:
                            ch_ed_all_select(MODE_FLAG_SKIP);
                            break;
                        default:
                            break;
                    }
                    break;
#if TKGS_SUPPORT
#else
                case STBK_GREEN:
                    GUI_GetProperty(LISTVIEW_CH_ED, "select", &sel);
                    if((s_Mode == CH_ED_MODE_MOVE)
                            && (g_AppChOps.check(sel, MODE_FLAG_MOVING) == false))
                    {
                        g_AppChOps.set(sel, MODE_FLAG_MOVE);
                        if(g_AppChOps.group_check(MODE_FLAG_MOVE) > 0)
                        {
                            channel_edit_show_tip(NULL,STR_ID_SELECT_GROUP_CHANNELS,NULL,NULL, STR_ID_MOVE);
                        }
                        else
                        {
                            channel_edit_show_tip(NULL, STR_ID_SELECT_GROUP_CHANNELS,NULL, NULL, STR_ID_SELECT);
                        }
                        GUI_SetProperty(LISTVIEW_CH_ED, "update_row", &sel);
                    }
                    break;
#endif

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_channel_edit_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;

    if (s_Mode == CH_ED_MODE_NONE)
    {
        ret = channel_edit_mode_off_keypress(widget, usrdata);
    }
    else
    {
        ret = channel_edit_mode_on_keypress(widget, usrdata);
    }

    return ret;
}

static int ch_ed_timeout_cb(void* usrdata)
{
    char time_str[24] = {0};
    char time_str_temp[6] = {0};

    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str));

	// time
	if(strlen(time_str) > 24)
	{
		app_log_error("\nlenth error\n");
		return 1;
	}
	memcpy(time_str_temp,&time_str[strlen(time_str)-5],5);// 00:00
	GUI_SetProperty("text_channel_edit_time", "string", time_str_temp);

	// ymd
	time_str[strlen(time_str)-5] = '\0';
	GUI_SetProperty("text_channel_edit_ymd", "string", time_str);

    return 0;
}

static uint32_t _get_cmb_sel_after_save(uint32_t total_id, uint32_t *id_buffer, uint32_t total_id_bak,
                                        uint32_t *id_buffer_bak, uint32_t cmb_sel_bak, unsigned short key)
{
    int i, j;
    bool cmb_sel_set_flag = false;
    uint32_t cmb_sel_set = 0;

    if(NULL == id_buffer || NULL == id_buffer_bak)
    {
        return 0;
    }

    if(key == STBK_LEFT)
    {
        if(0 == cmb_sel_bak) //all
        {
            cmb_sel_set = total_id - 1;
        }
		else if((id_buffer_bak[cmb_sel_bak] & FAV_MASK) != 0) //fav
        {
            for(i = total_id - 1; i >= 0; i--)
            {
				if((((id_buffer[i] & FAV_MASK) != 0) && (id_buffer[i] < id_buffer_bak[cmb_sel_bak]))
						|| (0 == (id_buffer[i] & FAV_MASK)))
                {
                    cmb_sel_set = i;
                    break;
                }
            }
        }
        else //sat
        {
            for(i = total_id - 1; i >= 0; i--)
            {
                for(j = 0; j < cmb_sel_bak; j++)
                {
                    if(id_buffer[i] == id_buffer_bak[j])
                    {
                        cmb_sel_set = i;
                        cmb_sel_set_flag = true;
                        break;
                    }
                }
                if(cmb_sel_set_flag)
                {
                    break;
                }
            }
            if(!cmb_sel_set_flag)
            {
                cmb_sel_set = total_id-1;
            }
        }
    }
    else
    {
        if(cmb_sel_bak == 0) //all
        {
            cmb_sel_set = 1;
        }
		else if((id_buffer_bak[cmb_sel_bak] & FAV_MASK) != 0) //fav
        {
            for(i = 1; i < total_id; i++)
            {
                if(id_buffer[i] > id_buffer_bak[cmb_sel_bak])
                {
                    cmb_sel_set = i;
                    cmb_sel_set_flag = true;
                    break;
                }
            }
            if(!cmb_sel_set_flag)
            {
                cmb_sel_set = 0;
            }
        }
        else //sat
        {
            for(i = 1; i < total_id; i++)
            {
				if((id_buffer[i] & FAV_MASK) != 0)
                {
                    cmb_sel_set = i;
                    cmb_sel_set_flag = true;
                    break;
                }
                for(j = cmb_sel_bak+1; j < total_id_bak; j++)
                {
                    if(id_buffer[i] == id_buffer_bak[j])
                    {
                        cmb_sel_set = i;
                        cmb_sel_set_flag = true;
                        break;
                    }
                }
                if(cmb_sel_set_flag)
                {
                    break;
                }
            }
            if(!cmb_sel_set_flag)
            {
                cmb_sel_set = 0;
            }
        }
    }

    return cmb_sel_set;
}

// signal channel edit
SIGNAL_HANDLER int app_channel_edit_create(GuiWidget *widget, void *usrdata)
{
	uint32_t sel = 0;
	int init_value = 0;

    ch_ed_mode_change(CH_ED_MODE_NONE);
	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

	//  g_AppFullArb.timer_start();
	GUI_SetFocusWidget(LISTVIEW_CH_ED);

    sp_GpIdOps = new_id_ops();

    if (sp_GpIdOps == NULL)
        return GXCORE_ERROR;

    channel_edit_cmb_group_build();

    sel = channel_edit_get_cur_group_num();

	GUI_SetProperty(GROUP_CH_ED, "select", &sel);
	app_channel_edit_group_change();

//modify 20141224

    _channel_edit_list_change();

    // CH ED TIMER
    if (reset_timer(sp_ChEdTimer) != 0)
    {
        sp_ChEdTimer = create_timer(ch_ed_timeout_cb,
                TIME_PRECISION*SEC_TO_MILLISEC, NULL, TIMER_REPEAT);
    }
    // first show
    ch_ed_timeout_cb(NULL);
    s_save_flag= false;

    START_VIDEO();
    g_AppFullArb.timer_start();// 20121108 处理由暗变亮过程
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_edit_destroy(GuiWidget *widget, void *usrdata)
{
    int init_value = 0;

    //有变化，但是被其它应用比如投屏打断退出菜单了，调用这个还原相关的设置。
    _ch_edit_exit_dialog_recovery_change();

    del_id_ops(sp_GpIdOps);
    sp_GpIdOps = NULL;

    remove_timer(sp_ChEdTimer);
    sp_ChEdTimer = NULL;
    GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
    g_AppFullArb.timer_stop();
    g_AppBook.invalid_remove();
#if RECALL_LIST_SUPPORT
#else
    g_AppPlayOps.recall_play.view_info.status = VIEW_INFO_DISABLE;
#endif

    back_to_main_menu_update_background();

    //从此菜单退出后，模式可能已经改变，需要更新当前模式下的所有数据
    app_fuse_spec_switch_work_edit_exit();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_edit_show(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_channel_edit_got_focus(GuiWidget *widget, void *usrdata)
{
	if ( app_fuse_spec_is_sattv_work_mode() )
	{
		return EVENT_TRANSFER_STOP;
	}
	ch_ed_mode_change_tips(s_Mode);
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_channel_edit_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	GxBusPmViewInfo  view_info_temp = {0};

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
					view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
					g_AppPlayOps.play_list_create(&view_info_temp);
                    g_AppChOps.update_prog_num();
					GUI_EndDialog("after wnd_full_screen");
					STOP_VIDEO();
					g_AppPlayOps.normal_play.rec = PLAY_KEY_DUMMY;
                    GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
					break;
				case STBK_EXIT:
				case STBK_MENU:
				{
					if ( false == _ch_edit_popdlg_is_create(_ch_edit_key_exit_popdlg_exit_cb) )
					{
						_ch_edit_key_exit_ending_proc();
					}
                }
                break;

                case STBK_LEFT:
                case STBK_RIGHT:
                {
                    uint32_t cmb_sel = 0;
                    GUI_GetProperty(GROUP_CH_ED, "select", &cmb_sel);

                    uint32_t id_total_bak = 0;
                    uint32_t *id_buffer_bak = NULL;
                    if(channel_id_total_and_buffer_dump(sp_GpIdOps, &id_total_bak, &id_buffer_bak) == GXCORE_ERROR)
                    {
                        app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
                        return GXCORE_ERROR;
                    }

                    app_pop_set_temp_data(id_total_bak,(uint32_t)id_buffer_bak,cmb_sel,event->key.sym);
                    if ( false == _ch_edit_popdlg_is_create(_ch_edit_key_leftright_popdlg_exit_cb))
                    {
                        _ch_edit_key_leftright_popdlg_proc_ret_ok();
                    }
                    break;
                }
                case STBK_TV_RADIO:
                {
                    if ( false == _ch_edit_popdlg_is_create(_ch_edit_key_tvradio_popdlg_exit_cb))
                    {
                        channel_edit_create_tv_radio_menu();
                    }
                }
                break;
                default:
                break;
            }
		default:
			break;
	}

    return EVENT_TRANSFER_STOP;
}

static void _create_ch_edit(void)
{
#if AUTO_UPDATE_SUPPORT
    app_auto_update_stop();
#endif
    GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);
    g_AppFullArb.timer_stop();
    app_subt_pause();
    if(g_AppFullArb.state.pause == STATE_ON)
    {
        g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    }
    GUI_CreateDialog(WND_CHANNEL_EDIT);
}

static int _ch_edit_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result ==  PASSWD_OK)
	{
		_create_ch_edit();
	}
	else if(ret->result ==  PASSWD_ERROR)
	{
		int init_value;
	GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
        if(init_value == 1)
        {
            PasswdDlg passwd_dlg;
            memset(&passwd_dlg, 0, sizeof(PasswdDlg));
            passwd_dlg.passwd_exit_cb = _ch_edit_passwd_dlg_cb;
            passwd_dlg.str = STR_ID_ERR_PASSWD;
            if(GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
            {
                passwd_dlg.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
                passwd_dlg.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
            }

            passwd_dlg_create(&passwd_dlg);
        }
    }
    else if(ret->result ==  PASSWD_CANCEL)
    {
        app_channel_info_create_dialog();
    }

	return 0;
}

static int _ch_edit_pvr_pop_cb(PopDlgRet ret)
{
    if(g_AppFullArb.state.tv == STATE_OFF)
    {
        if(0 == strcasecmp(WND_FULL_SCREEN, GUI_GetFocusWindow()))
            app_channel_info_create_dialog();
    }
    return 0;
}

static int _pop_fav_list_get_total(void)
{
    GxMsgProperty_NodeNumGet node = {0};

    node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node));
#if TKGS_SUPPORT
	if (node.node_num>8)
	{
		node.node_num -= 8;
	}
#endif

    return node.node_num;
}

void app_create_ch_edit_fullsreeen(void)
{
    if (g_AppChOps.get_list_total() != 0)
    {

        if (g_AppPvrOps.state != PVR_DUMMY)
        {
            app_pvr_create_no_btn_popdlg(_ch_edit_pvr_pop_cb);
        }
        else
        {
            int init_value = 0;
            GxBus_ConfigGetInt(MENU_LOCK_KEY, &init_value, MENU_LOCK);
            if(init_value == 1)
            {
                PasswdDlg passwd_dlg;
                memset(&passwd_dlg, 0, sizeof(PasswdDlg));
                passwd_dlg.passwd_exit_cb = _ch_edit_passwd_dlg_cb;
                if(GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
                {
                    passwd_dlg.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
                    passwd_dlg.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
                }

                passwd_dlg_create(&passwd_dlg);
            }
            else
            {
                if( APP_THEME_CLASSIC_PURPLE ){
                    app_unset_window_back_ground(WND_FULL_SCREEN, true, false);
                }
                _create_ch_edit();
            }
        }
     }
}

#define POP_FAV______
SIGNAL_HANDLER int app_pop_fav_list_create(GuiWidget *widget, void *usrdata)
{
	channel_edit_show_tip(NULL, STR_ID_RENAME, NULL, NULL, STR_ID_SELECT);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_fav_list_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_fav_keypress(GuiWidget *widget, void *usrdata)
{

	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					STOP_VIDEO();
					GUI_EndDialog("after wnd_full_screen");
                    GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
					break;

				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int app_pop_fav_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t sel=0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			//switch(event->key.sym)
			{
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_POP_FAV);
					if(s_favname_flag == true)
					{
						uint32_t sel = 0;

						s_favname_flag = false;

						GUI_GetProperty(GROUP_CH_ED,"select",&sel);
						channel_edit_cmb_group_build();
						GUI_SetProperty(GROUP_CH_ED,"select",&sel);
					}
					break;
                case STBK_UP:
                case STBK_DOWN:
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;
		        case STBK_PAGE_UP:
			        {
				        uint32_t sel;
				        GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
				        if (sel == 0)
				        {
					        sel = _pop_fav_list_get_total() - 1;
					        GUI_SetProperty(LISTVIEW_POP_FAV, "select", &sel);
					        ret =   EVENT_TRANSFER_STOP;
				        }
				        else
				        {
				        	ret =  EVENT_TRANSFER_KEEPON;
				        }
			        }
			        break;
                case STBK_PAGE_DOWN:
			        {
				        uint32_t sel;
				        GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
				        if (_pop_fav_list_get_total() == sel+1)
				        {
					        sel = 0;
					        GUI_SetProperty(LISTVIEW_POP_FAV, "select", &sel);
                            ret = EVENT_TRANSFER_STOP;
				        }
				        else
				        {
				        	ret =  EVENT_TRANSFER_KEEPON;
				        }
                    }
                        break;
                case STBK_OK:
                    GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
#if TKGS_SUPPORT
					g_AppChOps.set(s_ListSel, ((uint64_t)1<<(sel+9)));
#else
                    g_AppChOps.set(s_ListSel, ((uint64_t)1<<sel));
#endif
                    GUI_SetProperty(LISTVIEW_POP_FAV, "update_row", &sel);
                    break;
                case STBK_GREEN:
                    {
                        static PopKeyboard keyboard;
                        static GxMsgProperty_NodeByPosGet node;

                        GUI_GetProperty(LISTVIEW_POP_FAV, "select", &sel);
#if TKGS_SUPPORT
						sel += 9;
#endif
                        node.node_type = NODE_FAV_GROUP;
                        node.pos = sel;
                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

                        memset(&keyboard, 0, sizeof(PopKeyboard));
                        keyboard.in_name    = (char*)(node.fav_item.fav_name);
                        keyboard.max_num = MAX_FAV_GROUP_NAME - 1;
                        keyboard.out_name   = NULL;
                        keyboard.change_cb  = NULL;
                        keyboard.release_cb = ch_ed_favname_release_cb;
                        keyboard.usr_data   = KEY_LAN_CMB_INVALID_CHAR_NAME;
						keyboard.pos.x = 200;
						keyboard.pos.y = 130;
                        multi_language_keyboard_create(&keyboard);
                    }
                    break;
	            }
    }

    return ret;
}


SIGNAL_HANDLER int app_pop_fav_list_get_total(GuiWidget *widget, void *usrdata)
{
    return _pop_fav_list_get_total();
}


SIGNAL_HANDLER int app_pop_fav_list_get_data(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeByPosGet node = {0};
	ListItemPara* item = NULL;
    static char fav_name[MAX_FAV_GROUP_NAME];

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	return GXCORE_ERROR;
	if(0 > item->sel)   return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
    item->image = NULL;
    // 2nd param: sel the same value with MODE_FLAG_FAV*
    #if TKGS_SUPPORT
	if (g_AppChOps.check(s_ListSel, (uint64_t)1<<(item->sel+9)))
	#else
    if (g_AppChOps.check(s_ListSel, (uint64_t)1<<item->sel))
	#endif
	    item->image = "s_choice";
	item->string = NULL;

	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	node.node_type = NODE_FAV_GROUP;
	node.pos = item->sel;
	#if TKGS_SUPPORT
	node.pos += 9;
	#endif
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
    strcpy(fav_name, (char*)node.fav_item.fav_name);
    item->string = fav_name;

    return GXCORE_SUCCESS;
}

#define POP_SORT______

#if TKGS_SUPPORT || LCN_SUPPORT
static char *s_SortMode[SORT_MODE_TOTAL] = {
    "Name(A-Z)","Name(Z-A)",STR_ID_TP,STR_ID_FREE_SCRAMBLE,STR_ID_FAV_UNFAV,STR_ID_LOCK_UNLOCK,"HD/SD","LCN"};
#else
static char *s_SortMode[SORT_MODE_TOTAL] = {
    "Name(A-Z)","Name(Z-A)",STR_ID_TP,STR_ID_FREE_SCRAMBLE,STR_ID_FAV_UNFAV,STR_ID_LOCK_UNLOCK,"HD/SD"};
#endif

static void (*s_pop_sort_exit)(PopDlgRet,uint32_t);
static uint32_t s_cur_prog_pos = ~(uint32_t)0;

SIGNAL_HANDLER int app_pop_sort_list_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_sort_list_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static int s_sort_mode = -1;
static int _sort_wait_pop_create(void)
{
#if TKGS_SUPPORT || LCN_SUPPORT
    int value = PROGRAM_LCN_SORT_VALUE;
    int lcn_value = 0;

    GxBus_ConfigGetInt(LCN_KEY, &lcn_value, LCN_VALUE);
    if(1 == lcn_value)
    {
        if(SORT_BY_LCN == s_sort_mode)
            value = 1;
        else
            value = 0;

        GxBus_ConfigSetInt(PROGRAM_LCN_SORT_KEY, value);
    }
#endif
    g_AppChOps.sort((sort_mode)s_sort_mode, &s_cur_prog_pos);
	g_AppChOps.save();
    return 0;
}

SIGNAL_HANDLER int app_pop_sort_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	//uint32_t mode =0;
	//PopDlg  pop;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_POP_SORT);
					if(s_pop_sort_exit != NULL)
					{
						(*s_pop_sort_exit)(POP_VAL_CANCEL, s_cur_prog_pos);
					}
					break;

				case STBK_OK:
					GUI_GetProperty("listview_pop_sort", "select", &s_sort_mode);
					_sort_wait_pop_create();
					GUI_EndDialog(WND_POP_SORT);
					if(s_pop_sort_exit != NULL)
                    {
                        (*s_pop_sort_exit)(POP_VAL_OK, s_cur_prog_pos);
                    }

                    break;

				case STBK_UP:
				case STBK_DOWN:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
	}

	return ret;
}

SIGNAL_HANDLER int app_pop_sort_list_get_total(GuiWidget *widget, void *usrdata)
{
    return SORT_MODE_TOTAL;
}

SIGNAL_HANDLER int app_pop_sort_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item) 	return GXCORE_ERROR;
	if(0 > item->sel)   return GXCORE_ERROR;

	//col-0: num
	item->x_offset = 0;
    item->image = NULL;
	item->string = NULL;

	//col-1:sort mode
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
    item->string = s_SortMode[item->sel];

    return GXCORE_SUCCESS;
}

void app_pop_sort_create(PopDlgPos pop_pos, uint32_t prog_pos, void (*exit_cb)(PopDlgRet,uint32_t))
{
	s_cur_prog_pos = prog_pos;
	s_pop_sort_exit = exit_cb;
	GUI_CreateDialog(WND_POP_SORT);

	if(pop_pos.x == 0 && pop_pos.y == 0)
	{
		pop_pos.x = APP_WND_SORT_POS_X;
		pop_pos.y = APP_WND_SORT_POS_Y;
	}

	GUI_SetProperty(WND_POP_SORT, "move_window_x", &(pop_pos.x));
	GUI_SetProperty(WND_POP_SORT, "move_window_y", &(pop_pos.y));
}


#define POP_CH_PID______
#define TEXT_CH_PID_V_TYPE "text_ch_pid_v_type"
#define TEXT_CH_PID_A_TYPE "text_ch_pid_a_type"
#define EDIT_V "edit_ch_pid_v"
#define EDIT_A "edit_ch_pid_a"
#define EDIT_PCR "edit_ch_pid_pcr"

#define MAX_PID_LEN (4)

static void (*s_pop_ch_pid_exit)(PopDlgRet,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
static uint32_t s_ch_pid_v = ~(uint32_t)0;
static uint32_t s_ch_pid_a = ~(uint32_t)0;
static uint32_t s_ch_pid_pcr = ~(uint32_t)0;
static uint32_t s_video_sel = 0;
static uint32_t s_audio_sel = 0;

SIGNAL_HANDLER int app_ch_pid_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_UP:
					{
						uint32_t sel;
						GUI_GetProperty("box_pop_ch_pid_opt", "select", &sel);
						if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
							(sel>2) ? (sel--) : (sel=5);
						else
							(sel>0) ? (sel--) : (sel=5);
						GUI_SetProperty("box_pop_ch_pid_opt", "select", &sel);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				case STBK_DOWN:
					{
						uint32_t sel;
						GUI_GetProperty("box_pop_ch_pid_opt", "select", &sel);
						if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
							(sel<5) ? (sel++) : (sel=2);
						else
							(sel<5) ? (sel++) : (sel=0);
						GUI_SetProperty("box_pop_ch_pid_opt", "select", &sel);
					}
					ret = EVENT_TRANSFER_STOP;
					break;
				default:
					break;
			}
	}

	return ret;
}

SIGNAL_HANDLER int app_pop_ch_pid_create(GuiWidget *widget, void *usrdata)
{
	char pid_buff[MAX_PID_LEN + 1] = {0};

	sprintf(pid_buff, "%04d", s_ch_pid_v);
	GUI_SetProperty(EDIT_V, "string", pid_buff);

	memset(pid_buff, 0, MAX_PID_LEN);
	sprintf(pid_buff, "%04d", s_ch_pid_a);
	GUI_SetProperty(EDIT_A, "string", pid_buff);

	memset(pid_buff, 0, MAX_PID_LEN);
	sprintf(pid_buff, "%04d", s_ch_pid_pcr);
	GUI_SetProperty(EDIT_PCR, "string", pid_buff);

	//box部件存在问题，第一个boxitem不能disable，采用此补丁方式
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		GUI_SetProperty("boxitem_ch_type_v", "disable", NULL);
		GUI_SetProperty("boxitem_ch_pid_v", "disable", NULL);
		uint32_t sel = 2;
		GUI_SetProperty("box_pop_ch_pid_opt", "select", &sel);
	}
	else
	{
		GUI_SetProperty("boxitem_ch_type_v", "enable", NULL);
		GUI_SetProperty("boxitem_ch_pid_v", "enable", NULL);
		uint32_t sel = 0;
		GUI_SetProperty("box_pop_ch_pid_opt", "select", &sel);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_ch_pid_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}
#if 0
static int _invalid_pid_pop_cb(PopDlgRet ret)
{
    char pid_buff[MAX_PID_LEN + 1] = {0};

    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        sprintf(pid_buff, "%04d", s_ch_pid_v);
        GUI_SetProperty(EDIT_V, "string", pid_buff);
    }

    memset(pid_buff, 0, MAX_PID_LEN);
    sprintf(pid_buff, "%04d", s_ch_pid_a);
    GUI_SetProperty(EDIT_A, "string", pid_buff);

    memset(pid_buff, 0, MAX_PID_LEN);
    sprintf(pid_buff, "%04d", s_ch_pid_pcr);
    GUI_SetProperty(EDIT_PCR, "string", pid_buff);

    return 0;
}
#endif

SIGNAL_HANDLER int  app_edit_ch_pid_v_reach_end(GuiWidget *widget, void *usrdata)
{

    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CH_PID_V_LEN 4
    GUI_GetProperty("edit_ch_pid_v", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CH_PID_V_LEN - 1 : 0);
    GUI_SetProperty("edit_ch_pid_v", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int  app_edit_ch_pid_a_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CH_PID_A_LEN 4
    GUI_GetProperty("edit_ch_pid_a", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CH_PID_A_LEN - 1 : 0);
    GUI_SetProperty("edit_ch_pid_a", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int  app_edit_ch_pid_pcr_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_CH_PID_PCR_LEN 4
    GUI_GetProperty("edit_ch_pid_pcr", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_CH_PID_PCR_LEN - 1 : 0);
    GUI_SetProperty("edit_ch_pid_pcr", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pop_ch_pid_keypress(GuiWidget *widget, void *usrdata)
{
	char *pid_str = NULL;
	uint32_t box_sel;
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					STOP_VIDEO();
					GUI_EndDialog("after wnd_full_screen");
                    GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
					break;
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_POP_CH_PID);
					if(s_pop_ch_pid_exit != NULL)
					{
						(*s_pop_ch_pid_exit)(POP_VAL_CANCEL, s_ch_pid_v, s_ch_pid_a, s_ch_pid_pcr,s_video_sel,s_audio_sel);
					}
					break;

				case STBK_OK:
					GUI_GetProperty("box_pop_ch_pid_opt", "select", &box_sel);
					if(box_sel == 5) //save item
					{
						uint32_t ch_pid_v = 0;
						uint32_t ch_pid_a = 0;
						uint32_t ch_pid_pcr = 0;
						uint32_t video_sel = 0;
						uint32_t audio_sel = 0;
						bool invalid_flag = false;
						GUI_GetProperty(EDIT_V, "string", &pid_str);
						if(pid_str != NULL)
						{
							ch_pid_v = atoi(pid_str);
							pid_str = NULL;
						}

						GUI_GetProperty(EDIT_A, "string", &pid_str);
						if(pid_str != NULL)
						{
							ch_pid_a = atoi(pid_str);
							pid_str = NULL;
						}

						GUI_GetProperty(EDIT_PCR, "string", &pid_str);
						if(pid_str != NULL)
						{
							ch_pid_pcr = atoi(pid_str);
							pid_str = NULL;
						}
						GUI_GetProperty("cmb_ch_type_v","select",&video_sel);
						GUI_GetProperty("cmb_ch_type_a","select",&audio_sel);

						//GUI_EndDialog(WND_POP_CH_PID);

						if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
						{
							if( (ch_pid_v > 0 && ch_pid_v < 0x1fff)\
									&&(ch_pid_a > 0 && ch_pid_a < 0x1fff )\
									&&(ch_pid_pcr > 0 && ch_pid_pcr < 0x1fff) )
							{
								if(s_ch_pid_v != ch_pid_v || s_ch_pid_a != ch_pid_a || s_ch_pid_pcr != ch_pid_pcr || s_video_sel != video_sel || s_audio_sel != audio_sel)
								{
									invalid_flag = true;
								}
								else
								{
									GUI_EndDialog(WND_POP_CH_PID);
									if(s_pop_ch_pid_exit != NULL)
									{
										(*s_pop_ch_pid_exit)(POP_VAL_CANCEL, s_ch_pid_v, s_ch_pid_a, s_ch_pid_pcr,s_video_sel,s_audio_sel);
									}
									break;
								}
							}
						}
						else //radio
						{
							if( (ch_pid_a > 0 && ch_pid_a < 0x1fff )\
									&&(ch_pid_pcr > 0 && ch_pid_pcr < 0x1fff) )
							{
								if(s_ch_pid_a != ch_pid_a || s_ch_pid_pcr != ch_pid_pcr || s_audio_sel != audio_sel)
								{
									invalid_flag = true;
								}
								else
								{
									GUI_EndDialog(WND_POP_CH_PID);
									if(s_pop_ch_pid_exit != NULL)
									{
										(*s_pop_ch_pid_exit)(POP_VAL_CANCEL, s_ch_pid_v, s_ch_pid_a, s_ch_pid_pcr,s_video_sel,s_audio_sel);
									}
									break;
								}
							}
						}

						if(invalid_flag == true && ch_pid_v != ch_pid_a)
						{
							s_ch_pid_v = ch_pid_v;
							s_ch_pid_a = ch_pid_a;
							s_ch_pid_pcr = ch_pid_pcr;
							s_video_sel = video_sel;
							s_audio_sel = audio_sel;

							GUI_EndDialog(WND_POP_CH_PID);
							if(s_pop_ch_pid_exit != NULL)
							{
								(*s_pop_ch_pid_exit)(POP_VAL_OK, s_ch_pid_v, s_ch_pid_a, s_ch_pid_pcr,s_video_sel,s_audio_sel);
							}
						}
						else /*pid invalid,pop a dailog*/
						{
							PopDlg  pop;
							memset(&pop, 0, sizeof(PopDlg));
							pop.type = POP_TYPE_OK;
							pop.mode = POP_MODE_UNBLOCK;
							pop.str = STR_ID_INVALID_PID;
							pop.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
							pop.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
							pop.exit_cb = NULL;
							char pid_buff[MAX_PID_LEN + 1] = {0};

							if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
							{
								sprintf(pid_buff, "%04d", s_ch_pid_v);
								GUI_SetProperty(EDIT_V, "string", pid_buff);
							}

							memset(pid_buff, 0, MAX_PID_LEN);
							sprintf(pid_buff, "%04d", s_ch_pid_a);
							GUI_SetProperty(EDIT_A, "string", pid_buff);

							memset(pid_buff, 0, MAX_PID_LEN);
							sprintf(pid_buff, "%04d", s_ch_pid_pcr);
							GUI_SetProperty(EDIT_PCR, "string", pid_buff);

							popdlg_create(&pop);
							int init_value = 0;
							GUI_SetProperty(EDIT_A, "select", &init_value);
							GUI_SetProperty(EDIT_V, "select", &init_value);
							GUI_SetProperty(EDIT_PCR, "select",&init_value);
						}

					}

					break;

				case STBK_UP:
				case STBK_DOWN:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
	}

	return ret;
}

void app_ch_pid_pop(PopDlgPos pop_pos, uint32_t prog_id, void (*exit_cb)(PopDlgRet,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t))
{
	if(prog_id == 0)
		return;

	GxMsgProperty_NodeByIdGet prog_node = {0};
	prog_node.node_type = NODE_PROG;
	prog_node.id = prog_id;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&prog_node)))
    {
        s_ch_pid_v = prog_node.prog_data.video_pid;
        s_ch_pid_a = prog_node.prog_data.cur_audio_pid;
        s_ch_pid_pcr = prog_node.prog_data.pcr_pid;
        s_pop_ch_pid_exit = exit_cb;

        GUI_CreateDialog(WND_POP_CH_PID);
		s_video_sel = (uint32_t)channel_get_app_video_type(prog_node.prog_data.video_type);
		s_audio_sel = (uint32_t)channel_get_app_audio_type(prog_node.prog_data.cur_audio_type);
		GUI_SetProperty("cmb_ch_type_v", "select", &s_video_sel);
		GUI_SetProperty("cmb_ch_type_a", "select", &s_audio_sel);




        if(pop_pos.x == 0 && pop_pos.y == 0)
        {
            pop_pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
            pop_pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
        }

        GUI_SetProperty(WND_POP_CH_PID, "move_window_x", &(pop_pos.x));
		GUI_SetProperty(WND_POP_CH_PID, "move_window_y", &(pop_pos.y));
	}
}

void app_tv_channel_menu_exec(void)
{
    unsigned int tv_num = app_check_channel_num(GXBUS_PM_PROG_TV);
    if (tv_num == 0)
    {//freescan节目为空后，再判断下sattv节目是否为空
        tv_num = app_fuse_spec_tv_radio_num(GXBUS_PM_PROG_TV);
    }

    if(tv_num == 0)
    {
        app_mainmenu_tip_exec(MM_NO_CHANNEL);
    }
    else
    {
#if SAT2IP_SERVER_SUPPORT
        if (app_sat2ip_operate_tip_get_force() < 0)
            return;
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
#endif
        // get total earlier than create, so move from channel_edit to here
        g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
        // set tv mode

        // this will change full screen state
        g_AppFullArb.state.tv = STATE_ON;
        GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);
        GUI_CreateDialog(WND_CHANNEL_EDIT);
    }
}

void app_radio_channel_menu_exec(void)
{
    unsigned int radio_num = app_check_channel_num(GXBUS_PM_PROG_RADIO);
    if (radio_num == 0)
    {//freescan节目为空后，再判断下sattv节目是否为空
        radio_num = app_fuse_spec_tv_radio_num(GXBUS_PM_PROG_RADIO);
    }

    if(radio_num == 0)
    {
        app_mainmenu_tip_exec(MM_NO_CHANNEL);
    }
    else
    {
#if SAT2IP_SERVER_SUPPORT
        if (app_sat2ip_operate_tip_get_force() < 0)
            return;
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
#endif
        // get total earlier than create, so move from channel_edit to here
        g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_RADIO;
        // set radio mode

        // this will change full screen state
        g_AppFullArb.state.tv = STATE_OFF;
        GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);
        GUI_CreateDialog(WND_CHANNEL_EDIT);
    }
}

void app_channel_edit_menu_exec(void)
{
    PopDlg pop;

    g_AppChOps.update_prog_num();
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 3;
    pop.pos.x = APP_POP_DLG_POS_X;
    pop.pos.y = APP_POP_DLG_POS_Y;
    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        if(g_AppChOps.get_tv_num() == 0)
        {
            pop.str = STR_ID_NO_TV_CH;
            popdlg_create(&pop);
            return;
        }
    }
    else
    {
        if(g_AppChOps.get_radio_num() == 0)
        {
            pop.str = STR_ID_NO_RADIO_CH;
            popdlg_create(&pop);
            return;
        }
    }

#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
#endif
    GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);
    GUI_CreateDialog(WND_CHANNEL_EDIT);
}

#if VOICE_CONTROL_SUPPORT
// ret: 0-normal mode; 1-edit mode
int app_vc_channel_edit_mode_flag(void)
{
    if(s_Mode == CH_ED_MODE_NONE)
        return 0;
    else
        return 1;
}

int app_vc_channel_edit_switch_channel(void)
{
    if((s_Mode == CH_ED_MODE_NONE)
            && (1 < g_AppChOps.get_list_total()))
        return 0;
    else
        return 1;

}
#endif


