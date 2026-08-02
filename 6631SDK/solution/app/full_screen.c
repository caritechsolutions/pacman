/*TS
 * =====================================================================================
 *
 *       Filename:  full_screen.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年08月26日 08时47分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#include "channel_common.h"
#include "full_screen.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_book.h"
#include "app_windows.h"
#include "app_keyboard_language.h"
#include "app_utility.h"
#include "app_channel_info.h"

#define IMG_STATE       "img_full_state"
#define TXT_STATE       "txt_full_state"
#define IMG_REC         "img_full_rec"
#define IMG_TMS         "img_full_tms"

extern AppPlayOps g_AppPlayOps;
static event_list* sp_FullTimer = NULL;
static event_list *s_play_pause_timer = NULL;
#define STR_ID_PARENTAL_LOCK    "Parental Lock"
#if PARENTAL_LOCK_SUPPORT
#if DVB2IP_SERVER_SUPPORT
static char* tip_str[] = {STR_ID_NO_SIGNAL, STR_ID_LOCKED,STR_ID_SCRAMBLE,STR_ID_NO_PROGRAM,STR_ID_VIDEO_UNSUPPORT,STR_ID_PARENTAL_LOCK, " ", STR_ID_WAITING};
#else
static char* tip_str[] = {STR_ID_NO_SIGNAL, STR_ID_LOCKED,STR_ID_SCRAMBLE,STR_ID_NO_PROGRAM,STR_ID_VIDEO_UNSUPPORT,STR_ID_PARENTAL_LOCK, " "};
#endif
#else
#if DVB2IP_SERVER_SUPPORT
static char* tip_str[] = {STR_ID_NO_SIGNAL, STR_ID_LOCKED,STR_ID_SCRAMBLE,STR_ID_NO_PROGRAM,STR_ID_VIDEO_UNSUPPORT," ", STR_ID_WAITING};
#else
static char* tip_str[] = {STR_ID_NO_SIGNAL, STR_ID_LOCKED,STR_ID_SCRAMBLE,STR_ID_NO_PROGRAM,STR_ID_VIDEO_UNSUPPORT," "};
#endif
#endif
extern bool app_check_av_running(void);
extern void pvr_percent_init(void);
extern int app_play_get_recall_total_count(void);
extern void app_recall_list_set_play_num(int recall_num);
extern pvr_speed app_get_pvr_speed(unsigned int speed);
extern bool app_get_avcodec_flag(void);
extern void app_set_avcodec_flag(bool state);
extern uint32_t app_get_subtitling_mode(void);

#if ATSCCC_SUPPORT
extern int app_atsc_get_num(void);
#endif


void app_clean_fullscreen_tip(bool osd_trans_hide, bool end_pop_dlg, bool end_info_wnd)
{
    if (end_pop_dlg)
    {
        popdlg_destroy();//#54418
    }
    if (osd_trans_hide)
    {
        GUI_SetProperty(TXT_STATE, "string", STR_ID_BLANK);
        GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
        GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
    }
    else
    {
        GUI_SetProperty(TXT_STATE, "state", "hide");
        GUI_SetProperty(IMG_STATE, "state", "hide");
    }
    if (end_info_wnd)
    {
        app_channel_info_end_dialog();
    }
#if HDMI_CEC_SUPPORT
    extern void pop_dialog_destroy(void);
    pop_dialog_destroy();
#endif
}

static void full_mute_exec(FullArb* arb)
{
    GxMsgProperty_PlayerAudioMute player_mute;

    if (arb->state.mute == STATE_ON)
    {
        player_mute = 0;
        arb->state.mute = STATE_OFF;
        app_set_hardware_unmute();
    }
    else
    {
        player_mute = 1;
        arb->state.mute = STATE_ON;
        app_set_hardware_mute();
    }
#if BLUETOOTH_SUPPORT
{
    extern int app_bluetooth_source_set_mute(int mute);
    int ret_value = 0;
    app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    ret_value = app_bluetooth_source_set_mute(player_mute);
    if(ret_value == 0)
    {
        player_mute = 1;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }

}
#else
    app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
#endif
}


static void full_mute_draw(FullArb* arb)
{
#define IMG_MUTE_LEN    20
    char IMG_MUTE[IMG_MUTE_LEN]="img_full_mute";
#ifdef FM_SUPPORT
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_FM_PLAY))
    {
        memset(IMG_MUTE,0,IMG_MUTE_LEN);
        strncpy(IMG_MUTE,"img_fm_play_mute",strlen("img_fm_play_mute"));
    }
#endif
    if (arb->state.mute == STATE_ON)
    {
        GUI_SetProperty(IMG_MUTE, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_MUTE, "state", "hide");
    }
    GUI_SetProperty(IMG_MUTE, "draw_now", NULL);
}

static void full_speed_draw(FullArb* arb)
{
#define IMG_SPEED       "img_full_pause"
#define TXT_SPEED       "txt_full_speed"
#define FF_IMG          "s_pvr_ff"
#define FB_IMG          "s_pvr_fb"
    AppPvrOps *pvr = &g_AppPvrOps;
    char buffer[6] = {0};

    if (pvr->spd != 0)
    {
        if(pvr->spd > 0)
            GUI_SetProperty(IMG_SPEED, "img", FF_IMG);
        else
            GUI_SetProperty(IMG_SPEED, "img", FB_IMG);
        sprintf(buffer,"X%d", app_get_pvr_speed(abs(pvr->spd)));
        GUI_SetProperty(TXT_SPEED, "string", buffer);

        GUI_SetProperty(IMG_SPEED, "state", "show");
        GUI_SetProperty(TXT_SPEED, "state", "show");
    }
    else
    {
		if(arb->state.pause != STATE_ON)
		{
			GUI_SetProperty(IMG_SPEED, "state", "hide");
			GUI_SetProperty(TXT_SPEED, "state", "hide");
		}
    }
    GUI_SetProperty(IMG_SPEED, "draw_now", NULL);
}


static int32_t tms_flag_check(void)
{
    int32_t tms_flag = 0;

    GxBus_ConfigGetInt(PVR_TIMESHIFT_KEY, &tms_flag, PVR_TIMESHIFT_FLAG);
    return tms_flag;
}
static int thiz_pause_delay_cnt = 0;
static int _play_pause_cb(void* usrdata)
{
    #define RECORD_WAIT_COUNT      9 // time = 500ms*(7+1)=4000ms
    usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
    if ((app_check_av_running() == true) && (0xff != g_AppPmt.ver) && (usb_state == USB_OK))
    {
        GUI_Event event = {0};
        if (s_play_pause_timer != NULL)
        {
            remove_timer(s_play_pause_timer);
            s_play_pause_timer = NULL;
        }
        thiz_pause_delay_cnt = 0;

        event.type = GUI_KEYDOWN;
        event.key.sym = STBK_PAUSE;

        GUI_CreateDialog(WND_PVR_BAR);
        GUI_SendEvent(WND_PVR_BAR, &event);
    }
    else
    {
        if(app_check_av_running() == true && g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO )
        {
            if (app_channel_info_check_dialog() != GXCORE_SUCCESS)
            {
                app_channel_info_create_dialog();
            }
        }
        if (thiz_pause_delay_cnt++ >= RECORD_WAIT_COUNT)
        {
            if (s_play_pause_timer != NULL)
            {
                remove_timer(s_play_pause_timer);
                s_play_pause_timer = NULL;
            }
            thiz_pause_delay_cnt = 0;

            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_CANT_PVR;
            pop.mode = POP_MODE_UNBLOCK;
			pop.create_cb = NULL;
            pop.exit_cb = NULL;
            pop.timeout_sec = 3;
            popdlg_create(&pop);
        }
    }
    return 0;
}

static void event_to_pvr(void)
{
	if (0xff == g_AppPmt.ver)
	{
	    thiz_pause_delay_cnt = 0;
	    if (0 != reset_timer(s_play_pause_timer))
	    {
	        s_play_pause_timer = create_timer(_play_pause_cb, 300, NULL, TIMER_REPEAT);
	    }
	}
    else
    {
        GUI_Event event = {0};
        event.type = GUI_KEYDOWN;
        event.key.sym = STBK_PAUSE;
        if(g_AppPvrOps.state == PVR_DUMMY)
        {
            pvr_percent_init();
        }
        GUI_CreateDialog(WND_PVR_BAR);
        GUI_SendEvent(WND_PVR_BAR, &event);
    }
}

static void full_pause_exec(FullArb* arb)
{
    GxMsgProperty_PlayerPause player_pause;
    GxMsgProperty_PlayerResume player_resume;
    int freeze = 0;
#if INDEPEND_RECORD
    goto NORMAL_PAUSE;
#endif
    if(g_AppPvrOps.state != PVR_DUMMY)
    {
        GxMsgProperty_NodeByPosGet node_prog = {0};
        node_prog.node_type = NODE_PROG;
        node_prog.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
        if((g_AppPvrOps.state != PVR_TP_RECORD)
               && (node_prog.prog_data.id == g_AppPvrOps.env.prog_id)
            /*&&(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)*/)
            //Modified 2012-11-06 for Radio TMS
        {
            event_to_pvr();
        }
        else
        {
            goto NORMAL_PAUSE;
        }
    }
    else if ((tms_flag_check() == STATE_ON)
                && (g_AppPlayOps.normal_play.play_total > 0)
                && (app_check_av_running() == true)
                &&(arb->state.pause == STATE_OFF))
    {
        if(g_AppPvrOps.usb_check(&g_AppPvrOps) == USB_OK)
        {
            //tms only start
            if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
#if REDIO_RECODE_SUPPORT
                    || (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
#endif
              )
            {
                event_to_pvr();
            }
            else
            {
                goto NORMAL_PAUSE;
            }
        }
        else
        {
            PopDlg pop;
            usb_check_state usb_state = USB_OK;
            // check usb state
            usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
            if(usb_state == USB_ERROR)
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.format = POP_FORMAT_DLG;
                pop.str = STR_ID_INSERT_USB;
                pop.mode = POP_MODE_UNBLOCK;
                pop.create_cb = NULL;
                pop.exit_cb = NULL;
                pop.timeout_sec= 3;
                popdlg_create(&pop);
            }
            goto NORMAL_PAUSE;
        }
    }
    else
    {
NORMAL_PAUSE:
        if (arb->state.pause == STATE_ON)
        {
            GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

            player_resume.player = PLAYER_FOR_NORMAL;
            app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
            arb->state.pause = STATE_OFF;
            app_subt_resume();
#if BLUETOOTH_SUPPORT
        {
            extern int app_bluez_source_play(void);
            app_bluez_source_play();
        }
#endif
        }
        else
        {
            GxMsgProperty_PlayerTimeShift time_shift;

            freeze = 1;
            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

            memset(&time_shift, 0, sizeof(GxMsgProperty_PlayerTimeShift));
            time_shift.enable = 0;
			time_shift.shift_dmxid = 1;
#if CA_SUPPORT
		{
			uint16_t ts_src = 0;
			uint16_t DemuxId = time_shift.shift_dmxid;
		    app_demux_param_adjust(&ts_src, &DemuxId, 0);
			time_shift.shift_dmxid = DemuxId;
		}
#endif
            app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));
            app_subt_pause();
            player_pause.player = PLAYER_FOR_NORMAL;
            app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));
            arb->state.pause = STATE_ON;
#if BLUETOOTH_SUPPORT
        {
            extern int app_bluez_source_pause(void);
            app_bluez_source_pause();
        }
#endif
        }
    }

}

static void full_pause_draw(FullArb* arb)
{
#define IMG_PAUSE    "img_full_pause"
#define PAUSE_IMG    "s_pause"

    if (arb->state.pause == STATE_ON)
    {
		GUI_SetProperty(IMG_PAUSE, "img", PAUSE_IMG);
        GUI_SetProperty(IMG_PAUSE, "state", "show");
		GUI_SetProperty(TXT_SPEED, "string", " ");

    }
    else
    {
        GUI_SetProperty(IMG_PAUSE, "state", "hide");
        GUI_SetProperty(TXT_SPEED, "string", " ");
    }
    GUI_SetProperty(IMG_PAUSE, "draw_now", NULL);
}

static void full_tv_radio_exec(FullArb* arb)
{
#define STREAM_TYPE(play_ops)   play_ops.normal_play.view_info.stream_type
#define CUR_TV(play_ops)        play_ops.normal_play.view_info.tv_prog_cur
#define CUR_RADIO(play_ops)     play_ops.normal_play.view_info.radio_prog_cur
    if (STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_TV)
    {
        STREAM_TYPE(g_AppPlayOps) =  GXBUS_PM_PROG_RADIO;
        arb->state.tv = STATE_OFF;
    }
    else
    {
        STREAM_TYPE(g_AppPlayOps) =  GXBUS_PM_PROG_TV;
        arb->state.tv = STATE_ON;
    }

	if(arb->state.pause == STATE_ON)
	{
		arb->state.pause = STATE_OFF;
		full_pause_draw(arb);
	}
    g_AppPlayOps.program_stop();
    if (GXCORE_ERROR != g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info)))
    {
		// all list have, then play
        g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(g_AppPlayOps.normal_play.play_count);
        app_play_current_prog(PLAY_TYPE_FORCE);
	}
    else if((STREAM_TYPE(g_AppPlayOps) ==  GXBUS_PM_PROG_RADIO) && ( g_AppChOps.get_list_total() ==0))
    {
        g_AppFullArb.timer_start();
    }
}

static void full_tv_radio_draw(FullArb* arb)
{
    bool flag = false;

    if(g_AppChOps.get_tv_num() == 0 && g_AppChOps.get_radio_num() == 0)
    {
        app_unset_window_back_ground(WND_FULL_SCREEN, true, true);
        flag = true;
    }

    if(flag == false)
    {
        if(arb->state.tv == STATE_OFF)
        {
            app_set_window_back_ground(WND_FULL_SCREEN, NULL, true);
        }
        else
        {
#if ATSCCC_SUPPORT
            if(app_atsc_get_num() != 0)
                return;
#endif
            if ((arb->state.subt == STATE_OFF)
                    || ( g_AppTtxSubt.ttx_num  + g_AppTtxSubt.subt_num ==0))
            {
                app_unset_window_back_ground(WND_FULL_SCREEN, true, true);
            }
        }
    }
}


static void full_recall_exec(FullArb* arb)
{
#define CURRENT_TYPE    g_AppPlayOps.current_play.view_info.stream_type
#define RECALL_TYPE     tmp_info->view_info.stream_type

    PlayInfo *tmp_info = NULL;
    uint32_t prog_id = 0;
    int32_t prog_pos = 0;

#if RECALL_LIST_SUPPORT
    app_play_get_recall_total_count(); // 排除已删除节目
    if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV
        && g_AppPlayOps.normal_play.view_info.tv_prog_cur == g_AppPlayOps.recall_play[1].view_info.tv_prog_cur)
    ||(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO
        && g_AppPlayOps.normal_play.view_info.radio_prog_cur == g_AppPlayOps.recall_play[1].view_info.radio_prog_cur))
    {
        tmp_info = &g_AppPlayOps.recall_play[0];
    }
    else
    {
        tmp_info = &g_AppPlayOps.recall_play[1];
    }
#else
    tmp_info = &g_AppPlayOps.recall_play;

    if(memcmp(&g_AppPlayOps.recall_play.view_info , &g_AppPlayOps.current_play.view_info, sizeof(GxBusPmViewInfo)) == 0)
    {
        arb->state.pause = STATE_ON;
        full_pause_exec(arb);
        return;
    }
    if(g_AppPlayOps.recall_play.view_info.status == VIEW_INFO_DISABLE)
    {
        app_update_recall_info();
    }
#endif
    tmp_info->view_info.skip_view_switch = g_AppPlayOps.normal_play.view_info.skip_view_switch;
    prog_id = (RECALL_TYPE == GXBUS_PM_PROG_TV) ? (tmp_info->view_info.tv_prog_cur)
                                                : (tmp_info->view_info.radio_prog_cur);

    if (1 == app_fuse_spec_prepare_switch_work_env(0,tmp_info->view_info.group_mode,tmp_info->view_info.sat_id))
    {
        if((GXCORE_ERROR == g_AppPlayOps.play_list_create(&tmp_info->view_info))
            || (GXCORE_ERROR == app_get_prog_pos_by_id(&tmp_info->view_info, prog_id, &prog_pos)))
        {
            app_fuse_spec_prepare_switch_work_env(1,g_AppPlayOps.current_play.view_info.group_mode,g_AppPlayOps.current_play.view_info.sat_id);
        }
        else
        {
            app_fuse_spec_prepare_switch_work_env(1,g_AppPlayOps.current_play.view_info.group_mode,g_AppPlayOps.current_play.view_info.sat_id);
            memcpy(&(g_AppPlayOps.normal_play.view_info),&(g_AppPlayOps.current_play.view_info), sizeof(GxBusPmViewInfo));
            app_fuse_spec_switch_work_env(tmp_info->view_info.group_mode,tmp_info->view_info.sat_id);

        }
    }
    else
    {
        app_fuse_spec_switch_work_env(tmp_info->view_info.group_mode,tmp_info->view_info.sat_id);
    }
    if((GXCORE_ERROR == g_AppPlayOps.play_list_create(&tmp_info->view_info))
            || (GXCORE_ERROR == app_get_prog_pos_by_id(&tmp_info->view_info, prog_id, &prog_pos)))
    {
        g_AppPlayOps.play_list_create(&(g_AppPlayOps.current_play.view_info));
        arb->state.pause = STATE_ON;
        full_pause_exec(arb);
        return;
    }

    if(CURRENT_TYPE != RECALL_TYPE)
        arb->state.tv = (RECALL_TYPE == GXBUS_PM_PROG_TV) ? STATE_ON : STATE_OFF;
    prog_pos = g_AppChOps.get_cur_pos(prog_pos);
    g_AppPlayOps.program_play(PLAY_TYPE_FORCE, prog_pos);
}

static int BadSignalCount = 0;

void app_bad_signal_count_clean(void)
{
    BadSignalCount = 0;
    return;
}

#if NO_SIGNAL_ALERT_TIP_SUPPORT
#include "app_alert_tip.h"
#define NO_SIGNAL_ALERT_TIP_COUNT (10) //10*0.8=8s
#define NO_SIGNAL_ALERT_TIP_POP_SEC 300
#define NO_SIGNAL_ALERT_STANDBY_POP_SEC 180
static int no_signal_alert_count = 0;
extern int top_stbk_halt(uint32_t scan_code);
extern void app_dvbt2_auto_search(bool delete_prog);
extern void app_dvbc_auto_search(bool delete_prog);
extern void app_reset_menu_exec(void);

static void app_no_signal_alert_reset_count(void)
{
    no_signal_alert_count = 0;
}

static void app_no_signal_alert_tip_stop(void)
{
    app_no_signal_alert_reset_count();
    app_alert_end_dialog();
}

static int app_no_signal_alert_standy_pop_exec(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        top_stbk_halt(0);
    }
    else
    {
        app_no_signal_alert_reset_count();
    }
    return 0;
}
static void app_no_signal_alert_auto_standby(void)
{
    PopDlg pop;

    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = STR_ID_POWER_OFF;
    pop.timeout_sec = NO_SIGNAL_ALERT_STANDBY_POP_SEC;
    pop.default_ret = POP_VAL_OK;
    pop.show_time = true;
    pop.exit_cb = app_no_signal_alert_standy_pop_exec;
    popdlg_create(&pop);
}

static void app_no_signal_alert_tip_red_key_func(void)
{
    app_no_signal_alert_tip_stop();
    app_reset_menu_exec();
}

static void app_no_signal_alert_tip_yellow_key_func(void)
{
    int dvb_search_mode = 0;

    app_no_signal_alert_tip_stop();
    g_AppPlayOps.program_stop();
    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(dvb_search_mode == 0)
        app_dvbc_auto_search(false);
    else if(dvb_search_mode == 1)
        app_dvbt2_auto_search(true);
}

static void app_no_signal_alert_tip_timeout_cb_func(void)
{
    app_no_signal_alert_tip_stop();
    app_no_signal_alert_auto_standby();
}

static void app_no_signal_alert_tip_start(void)
{
    AlertParas params;

    memset(&params, 0, sizeof(AlertParas));
    params.str_title = STR_ID_RECEPTION;
    params.str_content = STR_ID_SIGNAL_WARNNING;
    params.str_red = STR_ID_SIGNAL_CLEAR;
    params.key_red = app_no_signal_alert_tip_red_key_func;
    params.str_yellow = STR_ID_SIGNAL_FIND;
    params.key_yellow = app_no_signal_alert_tip_yellow_key_func;
    params.key_exit = app_no_signal_alert_reset_count;
    params.timeout = NO_SIGNAL_ALERT_TIP_POP_SEC;
    params.timeout_cb = app_no_signal_alert_tip_timeout_cb_func;
    app_alert_create_dialog(&params);
}

#endif

#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
#define NO_SIGNAL_STANDBY_COUNT (375) //375*0.8=300s
#define NO_SIGNAL_STANDBY_POP_SEC (180)
static int no_signal_count = 0;
extern int top_stbk_halt(uint32_t scan_code);
static int app_no_signal_standy_pop_exec(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        top_stbk_halt(0);
    }
    else
    {
        no_signal_count = 0;
    }
    return 0;
}
static void app_no_signal_auto_standby(void)
{
    PopDlg pop;

    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = STR_ID_POWER_OFF;
    pop.timeout_sec = NO_SIGNAL_STANDBY_POP_SEC;
    pop.default_ret = POP_VAL_OK;
    pop.show_time = true;
    pop.exit_cb = app_no_signal_standy_pop_exec;
    popdlg_create(&pop);
}
#endif

static void full_state_exec(FullArb* arb)
{
    GxMsgProperty_NodeByPosGet prog_node = {0};
    AppFrontend_LockState lock;
    int led_lock_state = 0;

    if (g_AppPvrOps.state == PVR_RECORD
            || g_AppPvrOps.state == PVR_TMS_AND_REC)
    {
        // pvr rec state get
        GxMsgProperty_NodeByPosGet node;
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
        if (g_AppPvrOps.env.prog_id == node.prog_data.id)
        {
            arb->state.rec = STATE_ON;
        }
        else
        {
            arb->state.rec = STATE_OFF;
        }
    }
    else if(g_AppPvrOps.state == PVR_TP_RECORD)
    {
        arb->state.rec = STATE_ON;
    }
    else
    {
        arb->state.rec = STATE_OFF;
    }

    if (g_AppPvrOps.state == PVR_TIMESHIFT
            || g_AppPvrOps.state == PVR_TMS_AND_REC)
    {
        arb->state.tms = STATE_ON;
    }
    else
    {
        arb->state.tms = STATE_OFF;
    }

    if (g_AppChOps.get_list_total() == 0)
    {
        arb->tip = FULL_STATE_NOT_EXIST;
        return;
    }

    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
    //g_AppNim.property_get(&g_AppNim, AppFrontendID_Status, &status,
    //								sizeof(AppFrontend_Status));
#if DVB2IP_SERVER_SUPPORT
    {
        /* RIST/FSR is carrying this service. The tuner is legitimately unlocked
         * (that IS the FSR use case -- satellite dead, recovery delivering), so
         * the no-signal tip would be actively wrong: there is a real picture.
         *
         * Gated on app_rist_screen_delivering(), which requires the player to be
         * RUNNING on the receiver's output -- NOT merely on "chain active". If
         * the chain is up but the recovery peer is unreachable or silent, this
         * is false and the normal no-signal path below still runs, so a genuine
         * failure is never hidden behind a blank screen.
         *
         * The counters are reset too, so the no-signal auto-standby and alert
         * tip cannot fire against a working RIST picture if they are ever
         * enabled in this build. */
        extern int app_rist_screen_connecting(void);
        extern int app_rist_screen_delivering(void);

        /* A RIST channel takes ~10s to come up (receiver buffer + FSR handshake),
         * during which the chain is active but not yet delivering -- exactly the
         * branch that used to show "No signal" for the whole startup, telling the
         * user something is broken while everything is working. Show the
         * platform's neutral "Waiting..." instead.
         *
         * Bounded by the first-frame probe: once it resolves (picture, or gave up
         * after 20s) this goes false and the genuine no-signal/error path below
         * takes over, so a real failure is still surfaced. Chain zaps only --
         * factory zaps are ~1s and never reach here. */
        if (app_rist_screen_connecting())
        {
            arb->tip = FULL_STATE_CONNECTING;
            return;
        }

        if (lock == FRONTEND_UNLOCK && app_rist_screen_delivering())
        {
            BadSignalCount = 0;
#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
            no_signal_count = 0;
#endif
#if NO_SIGNAL_ALERT_TIP_SUPPORT
            app_no_signal_alert_reset_count();
#endif
            led_lock_state = 1;
            app_panel_ioctl_handle(PANEL_SHOW_LOCK, &led_lock_state);
            arb->tip = FULL_STATE_DUMMY;
            return;
        }
    }
#endif
#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
    if(lock == FRONTEND_LOCKED)
    {
        no_signal_count = 0;
    }
    else
    {
        if(no_signal_count++ >= NO_SIGNAL_STANDBY_COUNT)
        {
            no_signal_count = 0;
            app_no_signal_auto_standby();
        }
    }
#endif
    if (lock == FRONTEND_UNLOCK)
    {
#if NO_SIGNAL_ALERT_TIP_SUPPORT
        no_signal_alert_count++;
#endif
		if(BadSignalCount++ >= 3)
		{
            arb->tip = FULL_STATE_NO_SIGNAL;
			led_lock_state = 0;
			app_panel_ioctl_handle(PANEL_SHOW_LOCK, &led_lock_state);
		}
		else
		{
			if(arb->tip == FULL_STATE_NOT_EXIST)
			{
				arb->tip = FULL_STATE_DUMMY;
			}
		}
	}
    else
    {
        BadSignalCount = 0;
#if NO_SIGNAL_ALERT_TIP_SUPPORT
        app_no_signal_alert_reset_count();
#endif
        led_lock_state = 1;
        app_panel_ioctl_handle(PANEL_SHOW_LOCK, &led_lock_state);

        if(g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
        {
#if PARENTAL_LOCK_SUPPORT
            if(FULL_STATE_PARENTAL_LOCK == arb->tip)
                return;
#endif
            arb->tip = FULL_STATE_LOCKED;
            return;
        }

        // get current program info
        prog_node.node_type = NODE_PROG;
        prog_node.pos = g_AppPlayOps.normal_play.play_count;
        if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node)))
        {
            return;
        }

        if(prog_node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE
                || arb->scramble_check() == FALSE)
        {
            PlayerStatusInfo player_status;
            memset(&player_status, 0, sizeof(player_status));
            GxPlayer_MediaGetStatus(PLAYER_FOR_NORMAL, &player_status);
            if(player_status.status == PLAYER_STATUS_PLAY_RUNNING && player_status.error == PLAYER_ERROR_VIDEO_DECODER_ERROR)
            {
                arb->tip = FULL_STATE_UNSUPPORT;
            }
            else
            {
                arb->tip = FULL_STATE_DUMMY;
            }
        }
        else
        {
            arb->tip = FULL_STATE_SCRAMBLE;
        }
    }
}


static int _player_hide_show(unsigned char hide_show)
{
    GxMsgProperty_PlayerVideoHide config_video_hide = {0};
    GxMsgProperty_PlayerVideoShow config_video_show = {0};
    if(hide_show == 0){
        config_video_hide.player = PLAYER_FOR_NORMAL;
        GUI_SetInterface("video_off", NULL);
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_HIDE,&config_video_hide);
	} else {
        config_video_show.player = PLAYER_FOR_NORMAL;
        GUI_SetInterface("video_on", NULL);
        app_send_msg_exec(GXMSG_PLAYER_VIDEO_SHOW,&config_video_show);
	}
	return 0;
}

static void ch_ed_state_draw(FullArb* arb)
{
#define CH_ED_SIGNAL        "txt_channel_edit_signal"
	char *pstr = NULL;
    char value_str[20] = {0};
    GUI_GetProperty(CH_ED_SIGNAL, "state", value_str);
    if(strlen(value_str) == 0)
        strcpy(value_str,"show");
	GUI_GetProperty(CH_ED_SIGNAL, "string", &pstr);

	if((arb->tip == FULL_STATE_DUMMY))
	{
		if(((arb->state.tv != STATE_ON) && ((pstr == NULL) || (strcmp(pstr,"Radio")))))
		{
			GUI_SetProperty(CH_ED_SIGNAL, "string", "Radio");
            if(strcmp(value_str,"show"))
                GUI_SetProperty(CH_ED_SIGNAL, "state", "show");
            app_update_keyboard_dlg();
            app_update_pop_dlg(WND_POP_FAV);
        }
    }
	else if((pstr == NULL) || (strcmp(pstr,tip_str[arb->tip])))
	{
		GUI_SetProperty(CH_ED_SIGNAL, "string", tip_str[arb->tip]);
        if(strcmp(value_str,"show"))
            GUI_SetProperty(CH_ED_SIGNAL, "state", "show");
		app_update_keyboard_dlg();
        app_update_pop_dlg(WND_POP_FAV);
        app_update_pop_dlg(WND_PASSWD);
	}

	if (arb->tip == FULL_STATE_DUMMY)
	{
        PlayerStatusInfo player_status;
        memset(&player_status, 0, sizeof(player_status));
        GxPlayer_MediaGetStatus(PLAYER_FOR_NORMAL, &player_status);
        if((arb->state.tv == STATE_ON) && AVCODEC_RUNNING == player_status.vcodec.state)
        {
            PlayerProgInfo *info = app_player_prog_info_get();
            int32_t ret = 0;

            GxMsgProperty_NodeByPosGet node = {0};
            node.node_type = NODE_PROG;
            node.pos = g_AppPlayOps.normal_play.play_count;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

            ret = GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info);
            if(ret == 0)
            {
                int32_t prog_id;
                app_getvalue(info->url, "progid", &prog_id);
                if(prog_id == node.prog_data.id)
                {
                    if(app_get_avcodec_flag() == false)
                        app_set_avcodec_flag(true);
                    _player_hide_show(1);
                    GUI_SetProperty(CH_ED_SIGNAL, "string", " ");
                    GUI_GetProperty(CH_ED_SIGNAL, "state", value_str);
                    if(strcmp(value_str,"show") == 0)
                    {
                        GUI_SetProperty(CH_ED_SIGNAL, "state", "hide");
                        GUI_SetProperty(CH_ED_SIGNAL, "draw_now", NULL);
                    }
                }
            }
        }
        else
        {
            if(arb->state.tv == STATE_ON)
                _player_hide_show(0);

            if(strcmp(value_str,"show"))
            {
                GUI_SetProperty(CH_ED_SIGNAL, "state", "show");
                app_update_keyboard_dlg();
            }
        }
	}
	else
	{
        if(arb->state.tv == STATE_ON)
            _player_hide_show(0);

        if(strcmp(value_str,"show"))
        {
            GUI_SetProperty(CH_ED_SIGNAL, "state", "show");
            app_log_info("\n################ line:%d   [ channeledit_fullstate have changed ]\n",__LINE__);
            app_update_keyboard_dlg();
        }
    }
    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);
}

static void epg_state_draw(FullArb* arb)
{
#define EPG_SIGNAL        "txt_epg_signal"
	char *pstr = NULL;

	if (GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS
			|| GUI_CheckDialog(WND_SYSTEM_SETTING) == GXCORE_SUCCESS
			)
	{
		return;
	}

    char value_str[20] = {0};
    GUI_GetProperty(EPG_SIGNAL, "state", value_str);
    if(strlen(value_str) == 0)
        strcpy(value_str,"show");
	GUI_GetProperty(EPG_SIGNAL, "string", &pstr);

	if((arb->tip == FULL_STATE_DUMMY) && (arb->state.tv != STATE_ON))
	{
		if((pstr == NULL) || (strcmp(pstr,"Radio")))
		{
			GUI_SetProperty(EPG_SIGNAL, "string", "Radio");
            if(strcmp(value_str,"show"))
                GUI_SetProperty(EPG_SIGNAL, "state", "show");
		}

        app_update_pop_dlg(WND_POP_BOOK);
    }
	else if((pstr == NULL) || (strcmp(pstr,tip_str[arb->tip])))
	{
		GUI_SetProperty(EPG_SIGNAL, "string", tip_str[arb->tip]);
	}

    if (arb->tip == FULL_STATE_DUMMY)
    {
        PlayerStatusInfo player_status;
        memset(&player_status, 0, sizeof(player_status));
        GxPlayer_MediaGetStatus(PLAYER_FOR_NORMAL, &player_status);
        if((arb->state.tv == STATE_ON) && (AVCODEC_RUNNING == player_status.vcodec.state))//
        {
            int32_t ret = 0;
            GxMsgProperty_NodeByPosGet node = {0};
            node.node_type = NODE_PROG;
            node.pos = g_AppPlayOps.normal_play.play_count;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
            PlayerProgInfo *info = app_player_prog_info_get();
            ret = GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info);
            if(ret == 0)
            {
                int32_t prog_id;
                app_getvalue(info->url, "progid", &prog_id);
                if(prog_id == node.prog_data.id)
                {
                    if(app_get_avcodec_flag() == false)
                        app_set_avcodec_flag(true);
                    _player_hide_show(1);
                    if(strcmp(value_str,"show") == 0)
                    {
                        GUI_SetProperty(EPG_SIGNAL, "state", "hide");
                        GUI_SetProperty(EPG_SIGNAL, "draw_now", NULL);
                    }
                }
            }
        }
        else
        {
            if(arb->state.tv == STATE_ON)
                _player_hide_show(0);

            if(strcmp(value_str,"show"))
            {
                GUI_SetProperty(EPG_SIGNAL, "state", "show");
                app_update_keyboard_dlg();
            }
        }
	}
    else
    {
        if(arb->state.tv == STATE_ON)
            _player_hide_show(0);

        if(strcmp(value_str,"show"))
        {
            _player_hide_show(0);
            GUI_SetProperty(EPG_SIGNAL, "state", "show");
            if(0 == strcasecmp(WND_EPG, GUI_GetFocusWindow()))//when password created not update
                GUI_SetProperty(EPG_SIGNAL,"update",NULL);
        }
    }
    app_update_pop_dlg(WND_POP_OPTION_LIST);
    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);
    app_update_pop_dlg(WND_PASSWD);
}

#define PLAY_KEY_EXIST()    false
#define CH_ED_STATE_DRAW(arb)  ch_ed_state_draw(arb)
#define EPG_STATE_DRAW(arb)  epg_state_draw(arb)
static void full_state_draw(FullArb* arb)
{
    const char *wnd = GUI_GetFocusWindow();
    if(NULL != wnd)
    {
        // signal state , wnd add here
        if (GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT) || GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG))
        {
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
                CH_ED_STATE_DRAW(arb);
            else
                EPG_STATE_DRAW(arb);

            return;
        }
    }

    if(GUI_CheckDialog(WND_EPG) == GXCORE_SUCCESS)
        return;
#if FM_SUPPORT
    if(GUI_CheckDialog(WND_FM_PLAY) == GXCORE_SUCCESS)
        return;
#endif
#if NO_SIGNAL_ALERT_TIP_SUPPORT
    int mode = 0;
    GxBus_ConfigGetInt(NO_SIGNAL_ALERT_TIP_KEY, &mode, NO_SIGNAL_ALERT_TIP_VALUE);
    if(no_signal_alert_count >= NO_SIGNAL_ALERT_TIP_COUNT && mode ==1)
    {
        if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_ERROR && GXCORE_SUCCESS != app_channel_list_check_dialog())
        {
            app_no_signal_alert_tip_start();
        }
    }
    else
    {
        if(GUI_CheckDialog(WND_ALERT_TIP) == GXCORE_SUCCESS)
        {
            app_no_signal_alert_tip_stop();
        }
    }
#endif

    char value_str[20] = {0};
    GUI_GetProperty(IMG_REC, "state", value_str);
    if(strlen(value_str) == 0)
        strcpy(value_str,"show");

          // rec here
    if (arb->state.rec == STATE_ON)
    {
		 if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) != GXCORE_SUCCESS && strcmp(value_str,"show"))
		{
			GUI_SetProperty(IMG_REC, "state", "show");
		}
    }
    else
    {
        //从原来是在键盘框不在时用osd_trans_hide,先改为hide,就算键盘窗口存在也
        //不会出现把键盘消一块
		if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS && (strcmp(value_str,"show") == 0))
		{
			GUI_SetProperty(IMG_REC, "state", "hide");
		}
		else if(strcmp(value_str,"show") == 0)
		{
			GUI_SetProperty(IMG_REC, "state", "osd_trans_hide");
		}
    }

     // tms here
    memset(value_str,0,sizeof(value_str));
    GUI_GetProperty(IMG_TMS, "state", value_str);
    if(strlen(value_str) == 0)
        strcpy(value_str,"show");

    if (arb->state.tms == STATE_ON)
    {
        if(strcmp(value_str,"show"))
            GUI_SetProperty(IMG_TMS, "state", "show");
    }
    else
    {
		if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS && (strcmp(value_str,"show") == 0))
		{
			GUI_SetProperty(IMG_TMS, "state", "hide");
		}
		else if((strcmp(value_str,"show") == 0))
		{
			GUI_SetProperty(IMG_TMS, "state", "osd_trans_hide");
		}
    }

    memset(value_str,0,sizeof(value_str));
    GUI_GetProperty(TXT_STATE, "state", value_str);
    if(strlen(value_str) == 0)
        strcpy(value_str,"show");

    if (GUI_CheckDialog(WND_PASSWD) != GXCORE_SUCCESS
        && app_channel_info_signal_check_dialog() != GXCORE_SUCCESS
        && NULL != wnd)
    {
            // signal state , wnd add here
            if(!(0 == strcmp(wnd,WND_FULL_SCREEN)
                || 0 == app_channel_info_compare_dialog(wnd)
                || 0 == app_channel_list_compare_dialog(wnd)
                || (0 == app_channel_list_check_dialog()
                    && 0 == strcmp(wnd, WND_KEYBOARD_LANGUAGE))
                || 0 == strcmp(wnd,WND_VOLUME)
                || 0 == strcmp(wnd,WND_NUMBER)
                || 0 == strcmp(wnd,WND_PVR_BAR)
#if BOUQUET_SUPPORT
                || 0 == strcmp(wnd,WND_BOUQUET)
#endif
                || 0 == strcmp(wnd,WND_SLEEP)
                || 0 == strcmp(wnd,WND_POP_QR)
                || 0 == strcmp(wnd,WND_DURATION_SET)))

            {
                GUI_SetProperty(TXT_STATE, "string", tip_str[FULL_STATE_DUMMY]);
                if(strcmp(value_str,"show") == 0)
                {
                    GUI_SetProperty(TXT_STATE, "state", "hide");
                    GUI_SetProperty(IMG_STATE, "state", "hide");
                }
                return;
            }

        if((0 != app_channel_list_compare_dialog(wnd) && (!(0 == app_channel_list_check_dialog()
                    && 0 == strcmp(wnd, WND_KEYBOARD_LANGUAGE))))
			&& (app_fuse_spec_full_screen_check_dialog(wnd))
#if BOUQUET_SUPPORT
			&& (0 != strcmp(wnd,WND_BOUQUET))
#endif
        )

        {
            if ( arb->tip == FULL_STATE_DUMMY )
            {
                if(strcmp(value_str,"show") ==  0)
                {
                    GUI_SetProperty(IMG_STATE, "state", "osd_trans_hide");
                    GUI_SetProperty(TXT_STATE, "state", "osd_trans_hide");
                }
            }
            else
            {
                if (!PLAY_KEY_EXIST())
                {
                    GUI_SetProperty(TXT_STATE, "string", tip_str[arb->tip]);
                    if(strcmp(value_str,"show"))
                    {
                        GUI_SetProperty(TXT_STATE, "state", "show");
                        GUI_SetProperty(IMG_STATE, "state", "show");
                    }
                }
#if (DEMOD_DVB_T || DEMOD_ISDBT) && 0 == DEMOD_DVB_S
                if(g_AppChOps.get_tv_num() == 0 && g_AppChOps.get_radio_num() == 0)
                {
                    GUI_SetProperty(IMG_STATE, "state", "hide");
                    GUI_SetProperty(TXT_STATE, "state", "hide");
                    if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
                        GUI_CreateDialog(WND_INSTALLATION_GUIDE);
                }
                else
                {
                    GUI_SetProperty(TXT_STATE, "string", tip_str[arb->tip]);
                    if(strcmp(value_str,"show"))
                    {
                        GUI_SetProperty(TXT_STATE, "state", "show");
                        GUI_SetProperty(IMG_STATE, "state", "show");
                        app_update_pop_dlg(WND_POP_TIP);
                    }
                }
#endif
            }
        }
    }
}

static int full_state_timer(void* usrdata)
{
    FullArb *arb = &g_AppFullArb;

    arb->exec[EVENT_STATE](arb);
    arb->draw[EVENT_STATE](arb);

	return 0;
}

static void full_timer_start(void)
{
    if (reset_timer(sp_FullTimer) != 0)
    {
        // reset time failed, do
        sp_FullTimer = create_timer(full_state_timer, 800, NULL, TIMER_REPEAT);
    }

}

static void full_timer_stop(void)
{
    if (sp_FullTimer != NULL)
    {
        timer_stop(sp_FullTimer);
    }
#if NO_SIGNAL_ALERT_TIP_SUPPORT
    app_no_signal_alert_tip_stop();
#endif
#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
    no_signal_count = 0;
#endif
}

// flag 0 : get scramble status 1 : set scramble   2 : set free
static bool full_scramble_manage(uint32_t flag)
{
	static uint32_t scramble = 2;

	if (flag == 0)
	{
		if (scramble == 1)    return TRUE;
		else if (scramble == 2)    	return FALSE;
		else    return FALSE;
	}
	else if (flag == 1 || flag == 2)
	{
		scramble = flag;
		return TRUE;
	}
	else
	{
		return TRUE;
	}

	return TRUE;
}

static bool full_scramble_check(void)
{
    return full_scramble_manage(0);
}
static void full_scramble_set(uint32_t flag)
{
    full_scramble_manage(flag);
}


static void full_arbitrate_init(FullArb* arb)
{
    int32_t init_value = 0;

    if (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        arb->state.tv       = STATE_ON;
    }
    else
    {
        arb->state.tv       = STATE_OFF;
    }
    arb->state.mute     = STATE_OFF;
    arb->state.pause    = STATE_OFF;
    arb->state.rec      = STATE_OFF;
    arb->state.tms      = STATE_OFF;
    arb->state.ttx      = STATE_OFF;

    init_value = app_get_subtitling_mode();
    (init_value==0)?(arb->state.subt=STATE_OFF):(arb->state.subt=STATE_ON);

    arb->tip            = FULL_STATE_DUMMY;//TODO  !!! MUST get !!!!!!! shenbin

    arb->draw[EVENT_TV_RADIO]   = full_tv_radio_draw;
    arb->exec[EVENT_TV_RADIO]   = full_tv_radio_exec;

    arb->draw[EVENT_MUTE]       = full_mute_draw;
    arb->exec[EVENT_MUTE]       = full_mute_exec;

    arb->draw[EVENT_PAUSE]      = full_pause_draw;
    arb->exec[EVENT_PAUSE]      = full_pause_exec;

    arb->exec[EVENT_STATE]      = full_state_exec;
    arb->draw[EVENT_STATE]      = full_state_draw;

    arb->draw[EVENT_RECALL]     = full_tv_radio_draw;
    arb->exec[EVENT_RECALL]     = full_recall_exec;

    arb->draw[EVENT_SPEED]      = full_speed_draw;
    arb->exec[EVENT_SPEED]      = NULL;

}


FullArb g_AppFullArb =
{
    .init               = full_arbitrate_init,
    .timer_start        = full_timer_start,
    .timer_stop         = full_timer_stop,
    .scramble_check     = full_scramble_check,
    .scramble_set       = full_scramble_set,
};

//////////for media center////////////
void app_save_mute_state(int value)
{
    if(value == 0)
    {
        g_AppFullArb.state.mute = STATE_OFF;
    }
    else
    {
        g_AppFullArb.state.mute = STATE_ON;
    }
}

int app_get_mute_state(void)
{
    pmpset_mute ret;

    if(g_AppFullArb.state.mute == STATE_OFF)
    {
        ret = 0;
    }
    else
    {
       ret = 1;
    }

    return ret;
}

int app_get_motor_state_in_fullscreen(void)
{
	int value = 0;

	 if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS
		|| GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
	 {
		value = 1;
	 }
	 else
	 {
		value = 0;
	 }

	 return value;
}

#if VOICE_CONTROL_SUPPORT
enum _VCFEnum{
    VC_F_PAUSE,
    VC_F_PLAY,
    VC_F_REC,
    VC_F_REC_STOP,
};
//cmd: 0-pause; 1-play; 2-record; 3-stop record
//ret: 0-success, 1-not support, 2-repeat operator, -1-err
int app_vc_fullscreen_reaction(int cmd)
{
    FullArb *arb = &g_AppFullArb;
    if(VC_F_PAUSE == cmd)
    {
        if((arb->state.pause == STATE_OFF)
                && (g_AppPlayOps.normal_play.play_total > 0)
                && (app_check_av_running() == true)
                )
        {
            return 0;
        }
        else if(arb->state.pause == STATE_ON)
        {
            return 2;
        }
        else
            return 1;
    }
    else if(VC_F_PLAY == cmd)
    {
        if(arb->state.pause == STATE_ON)
        {
            return 0;
        }
        else if((arb->state.tms == STATE_ON)
            ||(app_check_av_running() == true)
                )
        {
            return 2;
        }
        else
            return 1;
    }
    else if(VC_F_REC == cmd)
    {
        if((g_AppPlayOps.normal_play.play_total > 0)
                && ((app_check_av_running() == true)
                     || ((app_check_av_running() == false)
                          && ((arb->tip == FULL_STATE_LOCKED)
#if PARENTAL_LOCK_SUPPORT
                               || (arb->tip == FULL_STATE_PARENTAL_LOCK)
#endif
                             )
                        )
                   )
                && (arb->state.rec != STATE_ON)
                && (g_AppPvrOps.usb_check(&g_AppPvrOps) == USB_OK))
        {
            return 0;
        }
        else if(arb->state.rec == STATE_ON)
        {
            return 2;
        }
        else
            return 1;
    }
    else if(VC_F_REC_STOP == cmd)
    {
        if((arb->state.rec == STATE_ON)
                || (arb->state.tms == STATE_ON))
        {
            return 0;
        }
        else
            return 1;
    }
    return -1;
}
#endif

