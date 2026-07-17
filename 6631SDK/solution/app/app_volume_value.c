/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_full_screen.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.3.8	           shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "full_screen.h"
#include "app_volume_value.h"
#include "app_windows.h"
#include "app_channel_info.h"
#if OSD_SUBTITLE_SUPPORT
#include "module/app_osd_subtitle.h"
#endif

#define BMP_VOL_MUTE        "s_vol_mute"
#define BMP_VOL_UNMUTE      "s_vol"
#define IMG_VOL             "img_volume"
#define PROGBAR_VOL         "progbar_volume"
#define TXT_VOL             "txt_volume"

static int volume_bar_timeout(void* data);
extern int app_audio_prog_track_set_mode(int value,uint8_t mode);

// from app_mute_pause.c
static event_list* sp_timer = NULL;

/* static variable ------------------------------------------------------ */
static GxMsgProperty_PlayerAudioVolume s_Volume;
static GxMsgProperty_PlayerAudioVolume s_sys_volume;

status_t app_volume_scope_status(void)
{
	int ret_value = 0;
	GxBus_ConfigGetInt(VOLUME_MODE_KEY, &ret_value, VOLUME_SCOMPE_);
	return ret_value;
}

static int app_audio_volume_prog_get_mode(void)
{
	int volume_sel = 0;
	GxMsgProperty_NodeByPosGet node = {0};

	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	{
		volume_sel = node.prog_data.audio_volume;
	}

	return volume_sel;
}

status_t app_system_vol_save(GxMsgProperty_PlayerAudioVolume val)
{
    if(val > 99)
        val = 99;
	GxBus_ConfigSetInt(AUDIO_VOLUME_KEY, val);

    return GXCORE_SUCCESS;
}
#if AUDIO_DESCRIPTOR_SUPPORT
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
/*offset value -3 -2 -1 0 1 2 3 */
/*ad volume 0(-32db)-32(0db)
*/
#define MAX_AD_VOL_OUTPUT 32
#define MIN_AD_VOL_OUTPUT 26
#define DEFAULT_AD_VOL_OUTPUT (MAX_AD_VOL_OUTPUT-(MAX_AD_VOL_OUTPUT-MIN_AD_VOL_OUTPUT)/2)

int app_system_ad_volume_set(int32_t offset)
{
    int volume = 0;
    int Ret;

    if(offset < -3)
    {
        offset = -3;
    }
    if(offset > 3)
    {
        offset = 3;
    }
    if(offset >=0)
    {
        volume = DEFAULT_AD_VOL_OUTPUT + offset*(MAX_AD_VOL_OUTPUT -DEFAULT_AD_VOL_OUTPUT)/3;
    }
    else
    {
        volume = DEFAULT_AD_VOL_OUTPUT + offset*(DEFAULT_AD_VOL_OUTPUT-MIN_AD_VOL_OUTPUT)/3;
    }
    if (volume > 32)
    {
        volume = 32;
    }
    Ret = GxPlayer_SetAudioDescriptorBoostVolume(volume);
    if (Ret != 0)
    {
        app_log_error("error GxPlayer_SetAudioVolume failed %d", Ret);
    }
    return 0;
}
#endif
#endif
status_t app_system_vol_set(GxMsgProperty_PlayerAudioVolume val)
{
    if(val > 99)
        val = 99;
    s_sys_volume = val;
    GxPlayer_SetAudioVolume(s_sys_volume);
#if BLUETOOTH_SUPPORT
{
    extern int app_bluetooth_set_volume(int volume);
    app_bluetooth_set_volume(s_sys_volume);
}
#endif

    return GXCORE_SUCCESS;
}

status_t app_system_vol_set_val(GxMsgProperty_PlayerAudioVolume val)
{
    s_sys_volume = val;
    return GXCORE_SUCCESS;
}

uint32_t app_system_vol_get(void)
{
    return s_sys_volume;
}

void app_volume_value_init(void)
{
	int32_t val = 0;
    if(( app_volume_scope_status() == VOLUME_CHANNEL) && GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_ERROR
            && GUI_CheckDialog(WND_NETVIDEO_PLAY) == GXCORE_ERROR
            && GUI_CheckDialog(WND_FM_PLAY) == GXCORE_ERROR
            && GUI_CheckDialog(WND_DLNA_DMR) == GXCORE_ERROR
            && GUI_CheckDialog(WND_WEBAPI_NETPLAY) == GXCORE_ERROR) //Multimedia /netapp /iptv /DMR volume is system volume
	{
		val = app_audio_volume_prog_get_mode();
	}
	else
	{
        val = app_system_vol_get();
	}
	if(val == 99)
	{
		s_Volume = 25;
	}
	else
	{
		s_Volume = val / VOLUME_NUM;
	}
    if(s_Volume > 0)
    {
		GUI_SetProperty(IMG_VOL, "img", BMP_VOL_UNMUTE);
    }
    else
    {
		GUI_SetProperty(IMG_VOL, "img", BMP_VOL_MUTE);
    }


	char volume_buf[4] = {0};
	GUI_SetProperty(PROGBAR_VOL, "value", (void *)(&s_Volume));
	sprintf(volume_buf, "%d", s_Volume);
	GUI_SetProperty(TXT_VOL, "string", volume_buf);
}

void app_volume_value_set(int volume)
{
	char volume_buf[4] = {0};

    app_system_vol_set(volume);
    app_system_vol_save(volume);

    if(GUI_CheckDialog(WND_VOLUME) == GXCORE_ERROR)
    {
        GUI_CreateDialog(WND_VOLUME);
    }
    else
    {
        int init_value = INFOBAR_TIMEOUT;
        app_volume_value_init();
        GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
        if (reset_timer(sp_timer) != 0)
        {
            // reset time failed, do
            sp_timer = create_timer(volume_bar_timeout, init_value*SEC_TO_MILLISEC, NULL, TIMER_REPEAT);
        }
    }

	GUI_SetProperty(PROGBAR_VOL, "value", (void *)(&s_Volume));
	sprintf(volume_buf, "%d", s_Volume);
	GUI_SetProperty(TXT_VOL, "string", volume_buf);
}

/* Private functions ------------------------------------------------------ */
static void volume_change(uint16_t key)
{
    FullArb *arb = &g_AppFullArb;
	uint32_t audio_vol = 0;
	GxMsgProperty_PlayerAudioVolume dispValue = 0;
	char volume_buf[4];

	// when change volue, enable mute
	if ((key == STBK_LEFT) || (key == STBK_VOLDOWN) || STBK_GREEN == key)
	{
		if (s_Volume > 1)
        {
			s_Volume--;
		}
        else
        {
			s_Volume = 0;
		}
	}
	else if ((key == STBK_RIGHT) || (key == STBK_VOLUP) || STBK_YELLOW == key)
	{
		if (s_Volume >= MAX_DISPLAY_VOL)
        {
			s_Volume = MAX_DISPLAY_VOL;
		}
        else
        {
			s_Volume++;
		}
	}

    if(s_Volume > 0)
    {
		GUI_SetProperty(IMG_VOL, "img", BMP_VOL_UNMUTE);
    }
    else
    {
		GUI_SetProperty(IMG_VOL, "img", BMP_VOL_MUTE);
    }

	// player do sth
	audio_vol = AUDIO_MAP_25(s_Volume);
	app_system_vol_set(audio_vol);

    if(arb->state.mute == STATE_ON)
    {
        arb->exec[EVENT_MUTE](arb);
        arb->draw[EVENT_MUTE](arb);
    }


    dispValue = s_Volume;
	GUI_SetProperty(PROGBAR_VOL, "value", (void *)(&(dispValue)));

	// set number value
	memset(volume_buf, 0, 4);
	sprintf(volume_buf, "%d", s_Volume);
	GUI_SetProperty(TXT_VOL, "string", volume_buf);

    if (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
    {
        extern status_t popmsg_mute_init(void);
        popmsg_mute_init();
    }
}

static int volume_bar_timeout(void* data)
{
	if(sp_timer != NULL)
	{
		remove_timer(sp_timer);
		sp_timer = NULL;
	}
	GUI_EndDialog(WND_VOLUME);
	GUI_SetProperty(WND_VOLUME, "draw_now", NULL);

	return GXCORE_SUCCESS;
}

static void _left_right_key_press(uint16_t key)
{
	int init_value = INFOBAR_TIMEOUT;

	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);

	if (reset_timer(sp_timer) != 0) {
		// reset time failed, do
	    sp_timer = create_timer(volume_bar_timeout, init_value*SEC_TO_MILLISEC, NULL, TIMER_REPEAT);
	}

	volume_change(key);
}

/* Exported functions ----------------------------------------------------- */
SIGNAL_HANDLER int app_volume_value_create(const char* widgetname, void *usrdata)
{
    int pos_x;
    int pos_y;
	int init_value = INFOBAR_TIMEOUT;

    if(app_channel_info_check_dialog() == GXCORE_SUCCESS)
    {
        pos_x = APP_WND_VOL_POS_X;
        pos_y = APP_WND_VOL_POS_Y;
        GUI_SetProperty(WND_VOLUME, "move_window_x", &pos_x);
        GUI_SetProperty(WND_VOLUME, "move_window_y", &pos_y);
    }
#if FM_SUPPORT
    if(GUI_CheckDialog(WND_FM_PLAY_INFO) == GXCORE_SUCCESS)
    {
        pos_x = APP_WND_VOL_POS_X;
        pos_y = APP_WND_VOL_POS_Y;
        GUI_SetProperty(WND_VOLUME, "move_window_x", &pos_x);
        GUI_SetProperty(WND_VOLUME, "move_window_y", &pos_y);
    }
#endif
#if WEBAPI_SUPPORT
    extern int app_webapi_netplay_menu_is_show(void);
    if ( app_webapi_netplay_menu_is_show() )
    {
        pos_x = APP_WND_VOL_POS_X;
        pos_y = APP_WND_VOL_POS_Y;
        GUI_SetProperty(WND_VOLUME, "move_window_x", &pos_x);
        GUI_SetProperty(WND_VOLUME, "move_window_y", &pos_y);
    }
#endif
	s_Volume = 0;
	app_volume_value_init();

	GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
    if (reset_timer(sp_timer) != 0) {
		// reset time failed, do
		sp_timer = create_timer(volume_bar_timeout, init_value*SEC_TO_MILLISEC, NULL, TIMER_REPEAT);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_volume_value_destroy(const char* widgetname, void *usrdata)
{
	if(sp_timer != NULL)
	{
		remove_timer(sp_timer);
		sp_timer = NULL;
	}
    extern int app_iptv_lite_check(void);
    if(((app_volume_scope_status()==VOLUME_CHANNEL) && GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_ERROR)
#if ( IPTV_LITE_SUPPORT > 0)
    &&(app_iptv_lite_check() == 0)
#endif
#if (FM_SUPPORT >0)
    &&(GUI_CheckDialog(WND_FM_PLAY)==GXCORE_ERROR)
#endif
    &&(GUI_CheckDialog(WND_DLNA_DMR) == GXCORE_ERROR)
    &&(GUI_CheckDialog(WND_WEBAPI_NETPLAY) == GXCORE_ERROR))
	{
		 app_audio_prog_track_set_mode(AUDIO_MAP_25(s_Volume),0xfe);
	}
	else
	{
        app_system_vol_save(AUDIO_MAP_25(s_Volume));
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_volume_value_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;

	event = (GUI_Event *)usrdata;
	msg = (GxMessage*)event->msg.service_msg;

	if(GXMSG_PLAYER_SPEED_REPORT == msg->msg_id || GXMSG_PLAYER_STATUS_REPORT == msg->msg_id)
	{
		if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
			GUI_SendEvent(WIN_MOVIE_VIEW, event);
		if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
			GUI_SendEvent(WIN_MUSIC_VIEW, event);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_volume_value_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode, event->key.sym))
		{
            case VK_BOOK_TRIGGER:
			    GUI_EndDialog("after wnd_full_screen");
			break;

		    case APPK_BACK:
		    case APPK_MENU:
				volume_bar_timeout(NULL);
				const char *wnd = GUI_GetFocusWindow();
				if(0 == strcasecmp(WND_FULL_SCREEN, wnd))
				{
					GUI_SendEvent(wnd, event);
				}
				break;

			case STBK_VOLDOWN:
			case STBK_VOLUP:
			case STBK_LEFT:
			case STBK_RIGHT:
				_left_right_key_press(find_virtualkey_ex(event->key.scancode, event->key.sym));
				break;
			case STBK_GREEN:
			case STBK_YELLOW:
			#if FM_SUPPORT
				if(GUI_CheckDialog(WND_FM_PLAY_INFO) == GXCORE_SUCCESS)
				{
					volume_bar_timeout(NULL);
					GUI_SendEvent(WND_FM_PLAY_INFO, event);
				}
                else
			#endif
                {
                    _left_right_key_press(find_virtualkey_ex(event->key.scancode, event->key.sym));
                }
				break;
			default:
				volume_bar_timeout(NULL);
				GUI_SendEvent(GUI_GetFocusWindow(), event);
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

#if WEBAPI_SUPPORT
void app_webapi_volume_get(int *current, int *max)
{
	*current = app_system_vol_get();
	*max = 99;
}

void app_webapi_volume_set(int value)
{
    FullArb *arb = &g_AppFullArb;
	app_system_vol_set(value);
	app_system_vol_save(value);
    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_WEBAPI_NETPLAY))
    {
        extern void app_webapi_unmute_change(void);
        app_webapi_unmute_change();
    }
    else if (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
    {
        extern void app_movie_unmute_change(void);
        app_movie_unmute_change();
    }
#ifdef FM_SUPPORT
    else if (GXCORE_SUCCESS == GUI_CheckDialog(WND_FM_PLAY))
    {
        if( g_AppFullArb.state.mute == STATE_ON)
        {
            g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
            g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
        }
    }
#endif
    else if (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
    {
        extern void app_music_unmute_change(void);
        app_music_unmute_change();
    }
#if IPTV_EPG_PLAYBACK_SUPPORT
    else if (GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG_PLAYBACK))
    {
        void app_iptv_playback_unmute_change(void);
        app_iptv_playback_unmute_change();
    }
#endif
    else if (GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
    {
        extern void app_net_video_unmute_change(void);
        app_net_video_unmute_change();
    }
#if DLNADMR_SUPPORT
    else if (GXCORE_SUCCESS == GUI_CheckDialog(WND_DLNA_DMR))
    {
        void app_dlna_dmr_unmute_change(void);
        app_dlna_dmr_unmute_change();
    }
#endif
#if PLUGINSTORE_SUPPORT
    else if (GXCORE_SUCCESS == GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY))
    {
        void app_plugin_listview_play_unmute_change(void);
        app_plugin_listview_play_unmute_change();
    }
#endif
    else
    {
        arb->exec[EVENT_MUTE](arb);
        arb->draw[EVENT_MUTE](arb);
    }
}
#endif

/* End of file -------------------------------------------------------------*/

