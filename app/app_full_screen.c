#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "full_screen.h"
#include "app_pop.h"
#include "app_book.h"
#include "app_default_params.h"
#include "app_epg.h"
#include "app_windows.h"
#include "app_channel_info.h"
#include "channel_common.h"
#include "app_netaudio.h"
#include "app_fuse_api.h"
#if WEBLET_SUPPORT
#include "app_weblet.h"
#endif
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if FM_SUPPORT
#include "app_fm.h"
#endif
#ifdef SUBTTRANS_SUPPORT
#include "gx_subttrans.h"
#endif
#if (DEMOD_DTMB > 0)
bool s_menuBack = false;
#endif
extern status_t app_create_epg_menu(void);
extern void app_ttx_magz_open(void);
extern bool app_check_av_running(void);
extern void app_sysinfo_menu_exec(void);
extern void app_create_pvr_media_fullsreeen(void);
extern void app_create_timer_setting_fullsreeen(void);
extern bool app_check_play_rec_flag(void);
extern int app_pvr_stop_cb(PopDlgRet ret);
extern status_t app_get_current_program(GxMsgProperty_NodeByPosGet *node);
extern bool app_program_need_predemux(GxMsgProperty_NodeByPosGet *node);
#if (IPTV_LITE_SUPPORT > 0)
extern int app_iptv_lite_clean_menu_flag(void);
#endif
#if ATSCCC_SUPPORT
extern void app_atsc_mode_set(bool flag);
extern void app_subt_close(void);
extern  void app_atsc_close_sub(void);
#endif
#if THEME_CLASSIC_BLUE
extern void app_create_media_centre_fullsreeen(void);
#endif

void full_arbitrate(uint32_t key);

#if (DEMOD_DTMB > 0)
void app_full_screen_is_menu_back_set(bool in)
{
    s_menuBack = in;
}

bool app_full_screen_is_menu_back_get(void)
{
    return s_menuBack;
}
#endif

static event_list* sp_shortcuts = NULL;
void shortcuts_timer_remove(void)
{
	if(sp_shortcuts != NULL)
	{
	    remove_timer(sp_shortcuts);
		sp_shortcuts = NULL;
	}
}

static int shortcuts_timeout(void* usrdata)
{
	GUI_SetProperty("txt_full_screen_shortcuts","state","osd_trans_hide");
	shortcuts_timer_remove();
	return 0;
}

void shortcuts_timer_start(void)
{
    if (reset_timer(sp_shortcuts) != 0)
    {
        sp_shortcuts = create_timer(shortcuts_timeout, 3000, NULL, TIMER_ONCE);
    }
}

void app_full_screen_hide_tips(void)
{
    app_clean_fullscreen_tip(false, true, true);
}

static int app_create_epg_wnd(void)
{
#define IMG_PAUSE    "img_full_pause"
    GUI_SetProperty(IMG_PAUSE, "state", "hide");
    app_clean_fullscreen_tip(false, false, true);
    GUI_SetInterface("flush",NULL);
    if(g_AppFullArb.state.pause == STATE_ON)
    {
        g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    }
    app_fuse_common_epg_create_dialog();
    return 0;
}

static int _stop_pvr_to_epg(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        app_create_epg_wnd();
    }
    return 0;
}

int app_create_menu_wnd(void)
{
    if( APP_THEME_CLASSIC_PURPLE||APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE){
        shortcuts_timeout(NULL);
    }
    if (APP_THEME_CLASSIC_PURPLE || APP_THEME_GRID_PURPLE){
        app_unset_window_back_ground(WND_FULL_SCREEN, true, false);
    }
    app_clean_fullscreen_tip(false, false, true);
    g_AppFullArb.timer_stop();
    g_AppFullArb.state.pause = STATE_OFF;
    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    g_AppPlayOps.program_stop();
#if BOUQUET_SUPPORT
    g_AppBat.stop(&g_AppBat);
#endif
    //the following 4 line codes for #47966
    uint32_t state_mute_bak = g_AppFullArb.state.mute;
    g_AppFullArb.state.mute = STATE_OFF;
    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
    g_AppFullArb.state.mute = state_mute_bak;
    GUI_CreateDialog(WND_MAIN_MENU);
    g_AppPlayOps.normal_play.rec = PLAY_KEY_DUMMY;
#if (DEMOD_DTMB > 0)
    s_menuBack = true;
#endif
#if PANEL_SETTING_SUPPORT
    int32_t led_display = 0;
    GxBus_ConfigGetInt(PANEL_LED_DISPLAY_KEY, &led_display, PANEL_LED_DISPLAY);
    if (led_display == 0) /*0 show channel No 1 Show time*/
    {
        app_panel_show_prog_num(PANEL_CLEAN_PROG_NUM);
    }
#endif

    return 0;
}

static int _stop_pvr_to_menu(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        app_create_menu_wnd();
    }
    return 0;
}

static int _stop_pvr_to_tv_radio(PopDlgRet ret)
{
    uint32_t key = STBK_TV_RADIO;
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        if(app_channel_info_check_dialog() == GXCORE_SUCCESS)
            app_channel_info_end_dialog();
        full_arbitrate(key);
    }
    return 0;
}

static int _stop_pvr_to_zoom(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        GUI_CreateDialog(WND_ZOOM);
    }
    return 0;
}

static int app_create_audio_wnd(void)
{
    extern int app_audio_create_dialog(void);
    app_clean_fullscreen_tip(false, false, true);
    app_audio_create_dialog();
    return 0;
}

static int _stop_pvr_to_audio(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        app_create_audio_wnd();
    }
    return 0;
}

#if AUDIO_DESCRIPTOR_SUPPORT
static int app_create_audio_description_wnd(void)
{
    extern int app_audio_description_create_dialog(void);
    app_clean_fullscreen_tip(false, false, true);
    app_audio_description_create_dialog();
    return 0;
}

static int _stop_pvr_to_audio_desc(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        app_create_audio_description_wnd();
    }
    return 0;
}
#endif

extern int app_subtitle_create_dialog(void);
static int app_create_sub_wnd(void)
{
    app_clean_fullscreen_tip(false, false, true);
#if ATSCCC_SUPPORT
    app_atsc_close_sub();
    app_atsc_mode_set(false);
#endif
    app_subtitle_create_dialog();

    return 0;
}

#if ATSCCC_SUPPORT
static int app_create_atsccc_sub_wnd(void)
{
    app_clean_fullscreen_tip(false, false, true);
    app_subt_close();
    app_atsc_mode_set(true);
    app_subtitle_create_dialog();

    return 0;
}
#endif

static int _stop_pvr_to_sub(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        app_create_sub_wnd();
    }
    return 0;
}

#if MULTIFEED_LIST_SUPPORT
extern int app_video_multifeed_fullscreen(void);
static int _stop_pvr_to_multifeed(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_pvr_stop_cb(ret);
        app_video_multifeed_fullscreen();
    }
    return 0;
}
#endif

static void app_pvr_stop_popdlg(void)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = app_pvr_stop_cb;
    pop.str = app_pvr_get_tips(g_AppPvrOps.state);
    popdlg_create(&pop);
}

static void app_pvr_stop_and_jump_popdlg(ExitCb jump_cb)
{
	PopDlg pop;
	memset(&pop, 0, sizeof(PopDlg));
	pop.type = POP_TYPE_YES_NO;
	pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = jump_cb;
    pop.str = app_pvr_get_tips(g_AppPvrOps.state);
    popdlg_create(&pop);
}

static int _fullscreen_up_or_down_normal(unsigned short key)
{
    int num ;
    num = g_AppChOps.get_list_total();
    if (num != 0)
    {
        /*
        - 节目只有一个，并且视频暂停的情况下，会不播放此节目。但是把右上角的暂停按钮消失了，造成问题。
        - 所以需要恢复播放。              add 20230522
        */
        if ((num == 1) && (g_AppFullArb.state.pause == STATE_ON ) && (g_AppPvrOps.state == PVR_DUMMY))
        {
            if ( g_AppFullArb.exec[EVENT_PAUSE] != NULL )
                g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
            if ( g_AppFullArb.draw[EVENT_PAUSE] != NULL )
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
            return 0 ;
        }
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        g_AppFullArb.tip = FULL_STATE_DUMMY;
        g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
        if(STBK_UP == key)
        {
            g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_NEXT, 0);
        }
        else if(STBK_DOWN == key)
        {
            g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_PREV, 0);
        }
    }

#if HDMI_CEC_SUPPORT
    extern int app_cec_change_service(void);
	app_cec_change_service();
#endif
    return 0;
}

static int _fullscreen_key_up_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if(POP_VAL_OK == ret)
    {
        if(GXCORE_ERROR == app_channel_info_check_dialog())
        {
            GUI_SetInterface("flush",NULL);
        }
        if(PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _fullscreen_up_or_down_normal(STBK_UP);
    }
    else
    {
        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            app_channel_info_create_dialog();
    }
    return 0;
}

static int _fullscreen_key_down_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if(POP_VAL_OK == ret)
    {
        if(GXCORE_ERROR == app_channel_info_check_dialog())
        {
            GUI_SetInterface("flush",NULL);
        }
        if(PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _fullscreen_up_or_down_normal(STBK_DOWN);
    }
    else
    {
        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            app_channel_info_create_dialog();
    }
    return 0;
}

int _ext_app_bypass_one_program_pvr_tms_state(void)
{
    pvr_state cur_pvr = PVR_DUMMY ;

    if ( g_AppChOps.get_list_total() == 1 )
    {
        cur_pvr = app_pvr_get_state();
        if ( (PVR_TIMESHIFT == cur_pvr) || (PVR_TMS_AND_REC == cur_pvr) )
        {
             return 1 ;
        }
    }

    return 0 ;
}

#if 0 == RECALL_LIST_SUPPORT
static int _fullscreen_recall_normal(void)
{
    FullArb *arb = &g_AppFullArb;
    uint32_t state = STATE_OFF;

    if(g_AppFullArb.tip == FULL_STATE_DUMMY)
    {
        app_clean_fullscreen_tip(true, false, false);
    }
    if(arb->state.pause == STATE_ON)
    {
        arb->state.pause = STATE_OFF;
        arb->draw[EVENT_PAUSE](arb);
    }
    state = arb->state.tv;
    arb->exec[EVENT_RECALL](arb);
    if(state != arb->state.tv)
        arb->draw[EVENT_RECALL](arb);
    GUI_SetProperty(WND_FULL_SCREEN, "draw_now", NULL);

    return 0;
}

static int _fullscreen_recall_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if(POP_VAL_OK == ret)
    {
        if(GXCORE_ERROR == app_channel_info_check_dialog())
        {
            GUI_SetInterface("flush",NULL);
        }
        if(PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _fullscreen_recall_normal();
    }
    else
    {
        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            app_channel_info_create_dialog();
    }
    return 0;
}
#endif

static int32_t tms_flag_check(void)
{
    int32_t tms_flag = 0;

    GxBus_ConfigGetInt(PVR_TIMESHIFT_KEY, &tms_flag, PVR_TIMESHIFT_FLAG);
    return tms_flag;
}
#define PROG_STREAM_TYPE(play_ops)   play_ops.normal_play.view_info.stream_type


void app_stop_play_and_clean_full_screen(bool clean_bg)
{
    FullArb *arb = &g_AppFullArb;
    app_full_screen_hide_tips();
    arb->timer_stop();
    arb->state.pause = STATE_OFF;
    arb->draw[EVENT_PAUSE](&g_AppFullArb);
    g_AppPlayOps.program_stop();

    //the following 4 line codes for #47966
    uint32_t state_mute_bak = arb->state.mute;
    arb->state.mute = STATE_OFF;
    arb->draw[EVENT_MUTE](&g_AppFullArb);
    arb->state.mute = state_mute_bak;

    if (clean_bg)
    {
        uint32_t state_tv_bak = arb->state.tv;
        arb->state.tv = STATE_ON;
        arb->draw[EVENT_TV_RADIO](&g_AppFullArb);
        arb->state.tv = state_tv_bak;
    }
}

#if FM_SUPPORT
void app_entry_fm_play_from_full_screen(void)
{
    uint32_t tuner = 0;
    AppFrontend_Monitor monitor;

    app_stop_play_and_clean_full_screen(false);
    app_update_window_back_ground(WND_FULL_SCREEN, NULL, false);
    app_set_window_back_ground(WND_FULL_SCREEN, NULL, true);

    tuner = app_tuner_cur_tuner_get();
    monitor = FRONTEND_MONITOR_OFF;
    app_ioctl(tuner, FRONTEND_MONITOR_SET, &monitor);
    GxFrontend_SetUnlock(tuner);
    GxFrontend_CleanRecordPara(tuner);
    if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_VOLUME);
    }
    app_fm_play_exec();
    return;
}
#endif
void app_tv_radio_key_exec(void)
{
 #if ( IPTV_LITE_SUPPORT > 0)
    app_iptv_lite_clean_menu_flag();
    GxPlayer_MediaStop(PLAYER_FOR_IPTV);
#endif
#if FM_SUPPORT
    if(g_AppChOps.get_tv_num() == 0 && g_AppChOps.get_radio_num() == 0)
        return;
    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
    {
        if(GUI_CheckDialog(WND_FM_PLAY) != GXCORE_SUCCESS)
        {
            app_entry_fm_play_from_full_screen();
            return;
        }
        else
        {
            GUI_EndDialog("after wnd_full_screen");
        }
    }
    else if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        if(GUI_CheckDialog(WND_FM_PLAY) == GXCORE_SUCCESS)
        {
            GUI_EndDialog("after wnd_full_screen");
        }
    }
#endif
    app_clean_fullscreen_tip(false, false, false);
    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
    g_AppFullArb.exec[EVENT_TV_RADIO](&g_AppFullArb);
    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    GUI_SetProperty(WND_FULL_SCREEN, "draw_now", NULL);
}

void full_arbitrate(uint32_t key)
{
    FullArb *arb = &g_AppFullArb;
	usb_check_state usb_state = USB_OK;
	PopDlg pop;
    GxMsgProperty_NodeByPosGet node = {0};

    switch (key)
    {
        case STBK_TV:
		case STBK_RADIO:
			if(((PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_TV)
					&& (STBK_TV == key))
				|| ((PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_RADIO)
					&& (STBK_RADIO == key)))
			{
                break;
			}
        case STBK_TV_RADIO:
#if ( IPTV_LITE_SUPPORT > 0)
            if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            {
                extern int app_iptv_lite_is_active(void);
                if(app_iptv_lite_is_active())
                {
                    extern void app_iptv_lite_create_menu(void);
                    app_stop_play_and_clean_full_screen(true);
                    app_iptv_lite_create_menu();
                }
                else
                {
                    app_tv_radio_key_exec();
                }
                break;
            }
            else
#endif
            {
                app_tv_radio_key_exec();
            }
            break;

        case STBK_MUTE:
            arb->exec[EVENT_MUTE](arb);
            arb->draw[EVENT_MUTE](arb);
            break;

        case STBK_PAUSE:
        case STBK_PAUSE_STB:
        case STBK_TMS:
            if((GXCORE_SUCCESS != app_get_current_program(&node)) || (app_netaudio_check_flag() == 1))
                break;

            if(tms_flag_check() == STATE_ON &&
                    (app_program_need_predemux(&node)
#ifdef SUBTTRANS_SUPPORT
                      || (1 == Gx_SubtTransGetStatus())
#endif
                      ))
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.timeout_sec= 3;
                pop.str = STR_ID_CANT_PVR;
                popdlg_create(&pop);
                break;
            }

            if(tms_flag_check() == STATE_ON && g_AppPvrOps.state == PVR_DUMMY)
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.timeout_sec= 3;

                usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
                if(usb_state == USB_NO_SPACE)
                {
                    pop.str = STR_ID_NO_SPACE;
                    popdlg_create(&pop);
                    break;
                }
                else if(usb_state == USB_READ_ONLY)
                {
                    pop.str = STR_ID_NOWRITE_PERMISSIONS;
                    popdlg_create(&pop);
                    break;
                }
                else if(USB_ERROR == usb_state)
                {
                    pop.str = STR_ID_INSERT_USB;
                    if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
                        popdlg_create(&pop);
                    break;
                }
                else if(USB_OK == usb_state)
                {
                    int section_record = 0;
                    GxBus_ConfigGetInt(PVR_SECTIONRECORD_KEY, &section_record, PVR_SECTIONRECORD_FLAG);
                    if(1 == section_record)
                    {
                        if(app_pvr_get_free_space(&g_AppPvrOps) < 256)
                        {
                            pop.str = STR_ID_NO_SPACE;
                            popdlg_create(&pop);
                            break;
                        }
                    }

                    if(!app_check_av_running())
                    {
                        pop.str = STR_ID_CANT_PVR;
                        popdlg_create(&pop);
                        break;
                    }
                }
            }

            if ( (PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_RADIO) && (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
				&& (arb->state.pause == STATE_OFF ) && (g_AppPvrOps.state == PVR_DUMMY) )
            {
                //printf("############## current is audio & scramble \n");
                if(  app_check_av_running() == true )
                {
                    PlayerProgInfo  *cur_info = app_player_prog_info_get();
                    GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, cur_info); //solution check
                    //printf("############## current frame_cnt = %lld \n",cur_info->audio_stat.play_frame_cnt);

                    if ( cur_info->audio_stat.play_frame_cnt <= 0)
                    {
                        //printf("#### audio pass \n");
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.format = POP_FORMAT_DLG;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.timeout_sec = 3;
                        pop.str = STR_ID_CANT_PVR;
                        popdlg_create(&pop);
                        break;
                    }
                }
            }

            if((app_check_av_running() == true) || (arb->state.pause == STATE_ON) ||
                ((app_check_av_running() == false) && (node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                 &&(node.prog_data.lock_flag != GXBUS_PM_PROG_BOOL_ENABLE)))
            {
                arb->exec[EVENT_PAUSE](arb);
                arb->draw[EVENT_PAUSE](arb);
            }

            break;

        case STBK_RECALL:
            {
#if RECALL_LIST_SUPPORT
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
#else
                if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_fullscreen_recall_stop_pvr_cb))
                {
                    _fullscreen_recall_normal();
                }
#endif
            }
            break;

        default:
            break;
    }
}
void app_prog_play_resume_exec(void)
{
    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
    if (g_AppChOps.get_list_total() != 0)
    {
        app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

        if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
            g_AppFullArb.tip = FULL_STATE_LOCKED;
        else
            g_AppFullArb.tip = FULL_STATE_DUMMY;
    }
}


SIGNAL_HANDLER int app_full_screen_create(GuiWidget *widget, void *usrdata)
{
    g_AppFullArb.init(&g_AppFullArb);
    g_AppChOps.update_prog_num();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_full_screen_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

static void _fullscreen_create_channel_list(GUI_Event *event)
{
    bool change_flag = true;

    if (event->key.sym == STBK_OK)
        change_flag = false;

    if (change_flag)
    {
        if (g_AppPvrOps.state != PVR_DUMMY && app_pvr_check_change_state())
        {
            app_pvr_create_no_btn_popdlg(NULL);
            return;
        }
    }

    if  ( app_fuse_spec_full_screen_create_dialog(event,change_flag) )
        return ;

    if (g_AppChOps.get_list_total() != 0)
    {
        app_clean_fullscreen_tip(false, true, true);
        GUI_CreateDialog(WND_CHANNEL_LIST);
        if(change_flag)
        {
            GUI_SendEvent(WND_CHANNEL_LIST, event);
        }
    }
}

SIGNAL_HANDLER int app_full_screen_keypress(GuiWidget *widget, void *usrdata)
{
    GxMsgProperty_NodeByPosGet node = {0};
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    AppPvrOps *pvr = &g_AppPvrOps;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            {
                if(app_check_play_rec_flag() == true)
                    return ret;
                if (g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt,
                            find_virtualkey_ex(event->key.scancode,event->key.sym)) == GXCORE_SUCCESS)
                {
#define IMG_STATE   "img_full_state"
#define IMG_REC     "img_full_rec"
#define IMG_MUTE    "img_full_mute"
#define IMG_PAUSE   "img_full_pause"
                    if(event->key.sym == STBK_EXIT)
                    {
                        GUI_SetProperty(IMG_STATE, "img", "s_icon_ts");
                        GUI_SetProperty(IMG_REC, "img", "s_pvr_rec");
                        GUI_SetProperty(IMG_MUTE, "img", "s_mute");
                        if(g_AppFullArb.state.mute == STATE_ON)
                        {
                            g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
                        }
                        GUI_SetProperty(IMG_PAUSE, "img", "s_pause");
                    }

                    return ret;
                }

                switch(event->key.sym)
                {
#if !ATSCCC_SUPPORT
                    case STBK_FIND:
#endif
                    case STBK_SAT:
                    case STBK_FAV:
                    case STBK_OK:
                        _fullscreen_create_channel_list(event);
                        break;
#if ATSCCC_SUPPORT
                    case STBK_FIND:
                        if(g_AppChOps.get_list_total() == 0)
                        {
                            break;
                        }
                        if (g_AppPvrOps.state != PVR_DUMMY)
                        {
                            app_pvr_stop_and_jump_popdlg(_stop_pvr_to_sub);
                        }
                        else
                        {
                            app_create_atsccc_sub_wnd();
                        }
                        break;
#endif
                    case STBK_MENU:
						{
                            if (pvr->state != PVR_DUMMY)
                            {
                                //app_pvr_stop_menu_popdlg();
                                app_pvr_stop_and_jump_popdlg(_stop_pvr_to_menu);
                            }
                            else
                            {
                                app_create_menu_wnd();
                            }
                            break;
                        }

					case STBK_MEDIA:
						if (APP_THEME_CLASSIC_BLUE){
#if THEME_CLASSIC_BLUE
							app_create_media_centre_fullsreeen();
#endif
						}
						else{
                        if (pvr->state != PVR_DUMMY)
                        {
                            app_pvr_stop_popdlg();
                        }
                        else
                        {
                            app_full_screen_hide_tips();
                            g_AppFullArb.timer_stop();
                            g_AppFullArb.state.pause = STATE_OFF;
                            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                            g_AppPlayOps.program_stop();
                            gxapp_main_menu_theme_media_entry();
                        }
                        }
                        break;

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
                            if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_fullscreen_key_up_stop_pvr_cb))
                            {
                                _fullscreen_up_or_down_normal(STBK_UP);
                            }
                            break;
                        }

                    case STBK_CH_DOWN:
                    case STBK_DOWN:
                        {
#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
                            if (g_AppChOps.get_list_total() != 0)
                            {
                                int prog_pos = g_AppChOps.get_prev_pos(g_AppPlayOps.normal_play.play_count);
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
                            if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_fullscreen_key_down_stop_pvr_cb))
                            {
                                _fullscreen_up_or_down_normal(STBK_DOWN);
                            }
                            break;
                        }

                    case STBK_EPG:
                        if (g_AppChOps.get_list_total() != 0)
                        {
							if (pvr->state != PVR_DUMMY)
                            {
								//app_pvr_stop_epg_popdlg();
                                app_pvr_stop_and_jump_popdlg(_stop_pvr_to_epg);
                            }
                            else
                            {
#if ATSCCC_SUPPORT
                                extern handle_t app_atsc_unload(void);
                                app_atsc_unload();
#endif
                                app_create_epg_wnd();
                            }
                        }
                        break;

                    case STBK_TV:
                    case STBK_RADIO:
                        if(((PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_TV)
                                    && (STBK_TV == event->key.sym))
                                || ((PROG_STREAM_TYPE(g_AppPlayOps) == GXBUS_PM_PROG_RADIO)
                                    && (STBK_RADIO == event->key.sym)))
                        {
                            break;
                        }
                    case STBK_TV_RADIO:
                        {
#if SAT2IP_SERVER_SUPPORT
                            if (app_sat2ip_switch_tip_get_force() < 0)
                                break;
#endif
#if DVB2IP_SERVER_SUPPORT
                            if (app_dvb2ip_switch_tip_get_force() < 0)
                                break;
#endif
                            if(pvr->state != PVR_DUMMY)
                            {
                                app_pvr_stop_and_jump_popdlg(_stop_pvr_to_tv_radio);
                            }
                            else
                            {
                                if(app_channel_info_check_dialog() == GXCORE_SUCCESS)
                                    app_channel_info_end_dialog();
                                full_arbitrate(event->key.sym);
                            }
                            break;
                        }

                    case STBK_PAUSE:
                    case STBK_PAUSE_STB:
                    case STBK_TMS:
                    case STBK_MUTE:
                        if (g_AppChOps.get_list_total() != 0)
                        {
                            full_arbitrate(event->key.sym);
                        }
                        break;

                    case STBK_RECALL:
                        if ((g_AppChOps.get_list_total() != 0)
                                && (pvr->state == PVR_DUMMY))
                        {
#if SAT2IP_SERVER_SUPPORT
                            if (app_sat2ip_switch_tip_get_force() < 0)
                                break;
#endif
#if DVB2IP_SERVER_SUPPORT
                            if (app_dvb2ip_switch_tip_get_force() < 0)
                                break;
#endif
                            full_arbitrate(event->key.sym);
                        }
                        break;

                    case STBK_REC_START:
                        {
                            PopDlg pop;
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_NO_BTN;
                            pop.format = POP_FORMAT_DLG;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.timeout_sec= 3;

#if (0 == REDIO_RECODE_SUPPORT)
                            if(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_TV)
                                break;
#endif
                            if((GXCORE_SUCCESS != app_get_current_program(&node)) ||  (app_netaudio_check_flag() == 1))
                                break;

                            if(app_program_need_predemux(&node))
                            {
                                pop.str = STR_ID_CANT_PVR;
                                popdlg_create(&pop);
                                break;
                            }
#ifdef SUBTTRANS_SUPPORT
                            if(1 == Gx_SubtTransGetStatus()) {
                                pop.str = STR_ID_CANT_PVR;
                                popdlg_create(&pop);
                                break;
                            }
#endif

                            if(g_AppPvrOps.state == PVR_DUMMY)
                            {
                                usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
                                if(usb_state == USB_ERROR)
                                {
                                    pop.str = STR_ID_INSERT_USB;
                                    popdlg_create(&pop);
                                    break;
                                }
                                else if(usb_state == USB_NO_SPACE)
                                {
                                    pop.str = STR_ID_NO_SPACE;
                                    popdlg_create(&pop);
                                    break;
                                }
                                else if(usb_state == USB_READ_ONLY)
                                {
                                    pop.str = STR_ID_NOWRITE_PERMISSIONS;
                                    popdlg_create(&pop);
                                    break;
                                }

#if SAT2IP_SERVER_SUPPORT
                                if (app_sat2ip_check_playing())
                                {
                                    pop.str = STR_ID_SAT2IP_STOP_TIP2;
                                    popdlg_create(&pop);
                                    break;
                                }
#endif

								if((app_check_av_running() == true) && (0xff != g_AppPmt.ver))
                                {
									if(g_AppFullArb.state.pause == STATE_ON)
									{
										g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
										g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
									}
									if(app_pvr_rec_start(0) == -1)
                                    {
                                        pop.str = STR_ID_CANT_PVR;
                                        popdlg_create(&pop);
                                        break;
                                    }
                                    GUI_CreateDialog(WND_PVR_BAR);
                                    GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
                                    break;
                                }
                                else if(g_AppFullArb.state.pause == STATE_ON)
                                {
                                    g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
                                    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                                    g_AppPlayOps.set_rec_time(0);
                                    app_play_current_prog(PLAY_MODE_WITH_REC);
                                }
                                else
                                {
                                    AppFrontend_LockState lock;
                                    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);

                                    node.node_type = NODE_PROG;
                                    node.pos = g_AppPlayOps.normal_play.play_count;
                                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
                                    if((g_AppPlayOps.normal_play.parent_key == PLAY_KEY_LOCK
                                            || node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
                                            && node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE
                                            && lock == FRONTEND_LOCKED)
                                    {
                                        if(app_pvr_rec_start(0) == -1)
                                        {
                                            pop.str = STR_ID_CANT_PVR;
                                            popdlg_create(&pop);
                                            break;
                                        }
                                        if(app_channel_info_check_dialog() == GXCORE_SUCCESS)
                                            app_channel_info_end_dialog();
                                        GUI_CreateDialog(WND_PVR_BAR);
                                        GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
                                        break;
                                    }
                                    else
                                    {
                                        pop.str = STR_ID_CANT_PVR;
                                        popdlg_create(&pop);
                                    }
                                }
                            }
                            else
                            {
                                if(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_RADIO)
                                {
                                    if(app_channel_info_check_dialog() == GXCORE_SUCCESS)
                                        app_channel_info_end_dialog();
                                }
                                GUI_CreateDialog(WND_PVR_BAR);
                                GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);

                                //#4329
                                if(pvr->state != PVR_RECORD)
                                    GUI_SendEvent(WND_PVR_BAR, event);
                            }
                            break;
                        }

                    case STBK_REC_STOP:
                        {
                            PopDlg  pop;

                            if ((g_AppPvrOps.state != PVR_RECORD)
                                    && (g_AppPvrOps.state != PVR_TIMESHIFT)
                                    && (g_AppPvrOps.state != PVR_TMS_AND_REC)
                                    && (g_AppPvrOps.state != PVR_TP_RECORD))
                                break;

                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_YES_NO;
                            pop.format = POP_FORMAT_DLG;
                            pop.str = app_pvr_get_tips(g_AppPvrOps.state);
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.exit_cb = app_pvr_stop_cb;
                            popdlg_create(&pop);
                            break;
                        }

                    case STBK_PLAY:

                        if((g_AppPvrOps.state == PVR_DUMMY) || (g_AppPvrOps.state == PVR_TP_RECORD))
                        {
                            if(g_AppFullArb.state.pause == STATE_ON)
                            {
                                g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
                                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                            }
                            else
                            {
                                if(g_AppPvrOps.state == PVR_DUMMY)
                                    app_fuse_spec_pvr_record_create_fullsreeen();
                            }
                        }
                        else if((g_AppPvrOps.state == PVR_TIMESHIFT)
                                ||(g_AppPvrOps.state == PVR_TMS_AND_REC))
                        {
                            GUI_CreateDialog(WND_PVR_BAR);
                            GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
                            if(g_AppFullArb.state.pause == STATE_ON)
                            {
                                GUI_SendEvent("slider_pvr_seek", event);
                            }
                            else
                            {
                                GUI_SendEvent("slider_pvr_process", event);
                            }
                        }

                        break;

                    case STBK_FF:
                    case STBK_FB:
                        if((g_AppPvrOps.state == PVR_TIMESHIFT)
                                ||(g_AppPvrOps.state == PVR_TMS_AND_REC))
                        {
                            GUI_CreateDialog(WND_PVR_BAR);
                            GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
                            GUI_SendEvent("slider_pvr_seek", event);
                        }

                        break;

                    case STBK_VOLDOWN:
                    case STBK_VOLUP:
                    case STBK_LEFT:
                    case STBK_RIGHT:
						if (g_AppChOps.get_list_total() != 0)
						{
                        	GUI_EndDialog(WND_NUMBER);
                        	GUI_CreateDialog(WND_VOLUME);
                        	GUI_SendEvent(WND_VOLUME, event);
						}
                        break;

                    case STBK_1:
                    case STBK_2:
                    case STBK_3:
                    case STBK_4:
                    case STBK_5:
                    case STBK_6:
                    case STBK_7:
                    case STBK_8:
                    case STBK_9:
                    case STBK_0:
					    if(g_AppChOps.get_list_total() == 0)
                        {
                            break;
                        }

                        GUI_EndDialog(WND_VOLUME);
                        GUI_CreateDialog(WND_NUMBER);
                        GUI_SendEvent(WND_NUMBER, event);
                        break;

                    case STBK_TTX:
						if(g_AppFullArb.tip == FULL_STATE_DUMMY)
						{
                        #if ATSCCC_SUPPORT
                            break;
                        #endif
                            if(g_AppFullArb.state.pause != STATE_ON)
                            {
                                app_ttx_magz_open();
                            }
                        }
                        break;

#if !ATSCCC_SUPPORT
                    case STBK_SUBT:
                        if(g_AppChOps.get_list_total() == 0)
                        {
                            break;
                        }

                        if (g_AppPvrOps.state == PVR_TIMESHIFT || g_AppPvrOps.state == PVR_TMS_AND_REC)
                        {
                            app_pvr_stop_and_jump_popdlg(_stop_pvr_to_sub);
                        }
                        else
                        {
                            app_create_sub_wnd();
                        }
                        break;
#endif

                    case STBK_INFO:
                        if (g_AppChOps.get_list_total() != 0)
                        {
                            if (pvr->state != PVR_DUMMY)
                                GUI_CreateDialog(WND_PVR_BAR);
                            else
                                app_channel_info_create_dialog();
                        }
                        break;

#if AUDIO_DESCRIPTOR_SUPPORT
                    case STBK_MUL_PIC:
                        {
                            if((app_netaudio_check_flag() == 1))
                                break;
                            if(GXCORE_SUCCESS == app_get_current_program(&node))
                            {
                                if(g_AppPvrOps.state != PVR_DUMMY)
                                    app_pvr_stop_and_jump_popdlg(_stop_pvr_to_audio_desc);
                                else
                                    app_create_audio_description_wnd();
                            }
                        }

                        break;
#endif

                    case STBK_ZOOM:
                        if((g_AppChOps.get_list_total() == 0)
                                || (g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_TV)
                                || (g_AppFullArb.state.pause == STATE_ON)
                                || (g_AppFullArb.scramble_check() == TRUE)
                                || (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
                                || (g_AppFullArb.tip == FULL_STATE_NO_SIGNAL))
                        {
                            break;
                        }
                        if (g_AppPvrOps.state != PVR_DUMMY)
                        {
                            app_pvr_stop_and_jump_popdlg(_stop_pvr_to_zoom);
                        }
                        else
                        {
                            GUI_CreateDialog(WND_ZOOM);
                        }
                        break;
#if WEBLET_SUPPORT
                    case STBK_QR:
                        app_clean_fullscreen_tip(false, false, false);
                        app_weblet_pop_rc_qr();
                        if(g_AppFullArb.state.tv == STATE_ON)
                        {
                            app_channel_info_end_dialog();
                        }
                        break;
#endif
#if BOUQUET_SUPPORT
                    case STBK_F1:
                        if(g_AppPvrOps.state != PVR_DUMMY)
                        {
                            app_pvr_create_no_btn_popdlg(NULL);
                            break;
                        }

                        if(app_get_bouquet_num() <= 0)
                        {
                            PopDlg pop;

                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_NO_BTN;
                            pop.str = STR_ID_NO_BOUQUET;
                            pop.timeout_sec = 2;
                            pop.mode = POP_MODE_UNBLOCK;
                            popdlg_create(&pop);
                            break;
                        }
                        else
                        {
                            app_clean_fullscreen_tip(false, true, true);
                            GUI_CreateDialog(WND_BOUQUET);
                        }
                        break;
#endif
#if MULTIFEED_LIST_SUPPORT
                    case STBK_LAST:
                        {
                            if(g_AppPvrOps.state != PVR_DUMMY)
                                app_pvr_stop_and_jump_popdlg(_stop_pvr_to_multifeed);
                            else
                                app_video_multifeed_fullscreen();

                        }
                        break;
#endif
                    case STBK_AUDIO:
                        if(GXCORE_SUCCESS == app_get_current_program(&node))
                        {
                            if (g_AppPvrOps.state == PVR_TIMESHIFT || g_AppPvrOps.state == PVR_TMS_AND_REC)
                            {
                                app_pvr_stop_and_jump_popdlg(_stop_pvr_to_audio);
                            }
                            else
                            {
                                app_create_audio_wnd();
                            }
                        }
                        break;
                    case STBK_TIMER:
                        {
                            app_create_timer_setting_fullsreeen();
                        }
                    break;
                    case STBK_DISK_INFO:
                        app_create_pvr_media_fullsreeen();
                    break;
                    case STBK_ASPECT:
                        if(STATE_OFF == g_AppFullArb.state.ttx
                                && PVR_DUMMY == g_AppPvrOps.state
#if AUTO_UPDATE_SUPPORT
                                && 0 == app_get_auto_update_flag()
#endif
                                )
                        {
                            if (APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE){
                            extern void app_t2_video_set_aspect_shortcuts(void);
                            shortcuts_timer_start();
                            app_t2_video_set_aspect_shortcuts();
                            }else{
                            extern status_t app_video_aspect_hotkey(void);
                            app_video_aspect_hotkey();
                            }
                        }
                        break;

                    case STBK_PN:
                        if(STATE_OFF == g_AppFullArb.state.ttx
                                && PVR_DUMMY == g_AppPvrOps.state
#if AUTO_UPDATE_SUPPORT
                                && 0 == app_get_auto_update_flag()
#endif
                                )
                        {
                            if( APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE ){
                            extern void app_t2_video_set_tv_format_shortcuts(void);
                            extern void shortcuts_timer_start(void);
                            shortcuts_timer_start();
                            app_t2_video_set_tv_format_shortcuts();
                            }else{
                            extern status_t app_video_standard_hotkey(void);
                            app_video_standard_hotkey();
                            }
                        }
                        break;

                    default:
                        break;
                }

                default:
                break;
            }
    }
    return ret;
}

SIGNAL_HANDLER int app_full_screen_got_focus(GuiWidget *widget, void *usrdata)
{
#if VOICE_CONTROL_SUPPORT
    extern unsigned int app_get_applets_status(void);
    extern void app_set_applets_status(unsigned int status);
    GxMsgProperty_NodeByPosGet node_prog = {0};
    node_prog.node_type = NODE_PROG;
    node_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
    if((!app_check_av_running()) && (node_prog.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE)
            &&(app_get_applets_status()))
    {
        app_play_current_prog(PLAY_MODE_POINT);
        app_set_applets_status(0);
    }
#endif
#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_set_dialog_resume();
    app_sat2ip_set_dlg_ok();
    app_sat2ip_set_gui_ok();
#endif
#if AUTO_UPDATE_SUPPORT
    if(0 != app_get_auto_update_flag())
         return EVENT_TRANSFER_STOP;
#endif
    g_AppFullArb.timer_start();

    return EVENT_TRANSFER_STOP;
}
