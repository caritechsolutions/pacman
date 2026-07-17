#include "app.h"
#include "stdlib.h"
#include "app_default_params.h"
#include "media_info.h"
#include "app_volume_value.h"
#include "app_windows.h"
#include "bus/module/player/gxaudio_dongle.h"
#include "bus/service/gxusbbtplug.h"
#include "module/app_network_service.h"

#if BLUETOOTH_SUPPORT

#include "gx_bt_api.h"

//widget
#define BT_CANVAS_SPECTRUM              "sink_view_canvas_spectrum"
#define BT_BUTTON_PLAY_STOP				"sink_view_boxitem2_button"
#define BT_TEXT_SINK_NAME1				"sink_view_text_name1"
#define BT_IMAGE_MUTE                   "sink_view_imag_mute"
#define BT_IMAGE_POPMSG                 "sink_view_image_popmsg"
#define BT_IMG_BT_VOL             "img_sink_volume"
#define BT_BMP_VOL_MUTE           "s_vol_mute"
#define BT_BMP_VOL_UNMUTE         "s_vol"
#define BT_IMG_POP_PLAY                    "MP_BUTTON_PLAY"
#define BT_IMG_POP_PAUSE                   "s_pause"

#define NET_USB_DEV_BT_SINK    "BT0SINK"
#define NET_BT_SINK_AUTHOR     "Nationalchip"

typedef enum
{
	PLAY_SINK_CTROL_PLAY,
	PLAY_SINK_CTROL_PAUSE,
	PLAY_SINK_CTROL_RESUME,
	PLAY_SINK_CTROL_STOP,
}play_sink_ctrol_state;

static play_sink_ctrol_state sink_box_button_ctrol = PLAY_SINK_CTROL_PLAY;

typedef enum
{
	SINK_BOX_BUTTON_PLAY_PAUSE,
	SINK_BOX_BUTTON_STOP,
	SINK_BOX_BUTTON_PAUSE,
	SINK_BOX_BUTTON_PLAY,
}sink_box_button;
extern struct SpectrumView  SpectrumViewData;

extern status_t app_send_msg_exec(uint32_t msg_id,void* params);
extern int app_get_mute_state(void);
extern uint32_t app_system_vol_get(void);
extern void spectrum_volumebak_clear(void);
extern int app_check_bluetooth_status(void);
extern int app_check_bt_connect_status(void);
extern int app_bluetooth_source_set_volume(int volume);
extern int app_bluetooth_source_set_channel(int channel);
extern int app_bluez_dev_auto_sink_connect(void);

static unsigned int app_bluetooth_sink_get_cap(GxAudioDongleCap  *cap)
{
    GxBtA2dpCodecInfo info;
    GxCore_BTSourceGetCodecInfo(&info);
    app_log_info("****sample_rate=%d,channel_num=%d,bit_width=%d\n",info.sample_rate,info.channel_num,info.bit_width);

    GxCore_BTSinkGetCodecInfo(&info);
    app_log_info("--------sample_rate=%d,channel_num=%d,bit_width=%d,interlace=%d,little_endian=%d\n",info.sample_rate,
            info.channel_num,info.bit_width,info.interlace,info.little_endian);
    cap->format = GXAUDIO_DONGLE_SBC;
    cap->keep_pulse = 1;
    //cap->pcm.sample_freq = 44100;
	//cap->pcm.channel_num = 2;
	//cap->pcm.bits        = 16;
	//cap->pcm.interlace   = 1;
	//cap->pcm.endian      = 0;

    return 0;
}

static unsigned int app_bluetooth_sink_get_bufinfo(GxAudioDongleBufInfo *data)
{
    GxBtFifoInfo info;

    GxCore_BTSinkGetFifoInfo(&info);
    data->datalen = (info.total_size - info.free_size);
    data->freelen = info.free_size;
    return 0;
}

static int app_bluetooth_sink_pull_data(GxAudioDongleData *data)
{

    return GxCore_BTSinkRecvData(data->buf_ptr,data->buf_size);
}


static GxAudioDongleOps thiz_sink_ops = {
    .name = NET_USB_DEV_BT_SINK,
    .author = NET_BT_SINK_AUTHOR,
    .get_cap = app_bluetooth_sink_get_cap,
    .get_bufinfo = app_bluetooth_sink_get_bufinfo,
    .push = NULL,
    .pull = app_bluetooth_sink_pull_data,
};

status_t app_bluetooth_register_sink_dogle(void)
{

    GxAudioRegisterDongle(&thiz_sink_ops);
    return GXCORE_SUCCESS;
}

status_t app_bluetooth_unregister_sink_dogle(void)
{
    GxAudioUnRegisterDongle(&thiz_sink_ops);
    return GXCORE_SUCCESS;
}

unsigned int app_bluetooth_sink_audio_enable(void)
{
    int ret = 0;
    GxAudioDongleParam param;

    param.mute = app_get_mute_state();
    param.volume = app_system_vol_get();
    param.channel = 0;

    ret = GxAudioDongleEnable(thiz_sink_ops.name,&param);
    return ret;
}

unsigned int app_bluetooth_sink_audio_disable(void)
{
    int ret = 0;

    ret = GxAudioDongleDisable(thiz_sink_ops.name);
    return ret;
}

int app_bluetooth_sink_pause(void)
{
    int ret = 0;

    if ( app_check_bluetooth_status() != BT_A2DP_SINK_DISCONNECT )
    {
        ret = GxCore_BTSinkPause();
        app_bluetooth_sink_audio_disable();
    }
    return ret;
}

int app_bluetooth_sink_play(void)
{
    int ret = 0;

    if ( app_check_bluetooth_status() != BT_A2DP_SINK_DISCONNECT )
    {
        app_bluetooth_sink_audio_enable();
        ret = GxCore_BTSinkPlay();
    }
    return ret;
}


int app_bluetooth_sink_set_mute(int mute)
{
    int ret = 0;
    ret = GxAudioDongleSetMute(thiz_sink_ops.name, mute);
    return ret;
}

int app_bluetooth_set_volume(int volume)
{
    int ret = 0;
    int status ;
    status = app_check_bluetooth_status() ;

    if ( status == BT_A2DP_SINK_CONNECT || status == BT_A2DP_SINK_PAUSE ||
         status == BT_A2DP_SINK_PLAY)
    {
        ret = GxAudioDongleSetVolume(thiz_sink_ops.name,volume);
    }
    else
    {
        app_bluetooth_source_set_volume(volume);
    }
    return ret;
}

int app_bluetooth_set_channel(int channel)
{
    int ret = 0;
    int status ;
    status = app_check_bluetooth_status() ;

    if ( status == BT_A2DP_SINK_CONNECT || status == BT_A2DP_SINK_PAUSE ||
         status == BT_A2DP_SINK_PLAY)
    {
        ret = GxAudioDongleSetChannel(thiz_sink_ops.name,channel);
    }
    else
    {
        app_bluetooth_source_set_channel(channel);
    }
    return ret;

}

void sink_view_spectrum_create(void)
{
#define SPECTRUM_X_OFFSET	0
#define SPECTRUM_Y_YOFFSET  50
	int x,y,w,h = 0;
	sscanf((widget_get_property(gui_get_widget(NULL,BT_CANVAS_SPECTRUM),"rect")),"[%d,%d,%d,%d]",&x,&y,&w,&h);
	if(w>SPECTRUM_X_OFFSET)
	{
		w = w-SPECTRUM_X_OFFSET;
	}
	if(h>SPECTRUM_Y_YOFFSET)
	{
		h = h-SPECTRUM_Y_YOFFSET;
	}
	spectrum_create(BT_CANVAS_SPECTRUM, SPECTRUM_X_OFFSET, SPECTRUM_Y_YOFFSET, w, h);

}

void sink_view_mode_create(void)
{

	GUI_SetProperty(BT_CANVAS_SPECTRUM, "state", "hide");
	GUI_SetProperty(BT_CANVAS_SPECTRUM, "record", "enable");

    GUI_SetProperty("image_win_sink_view_back", "state", "show");
    sink_view_spectrum_create();
    spectrum_start();
}

void sink_view_mode_pause(void)
{
//		spectrum_pause();
		spectrum_stop();
}

void sink_view_mode_resume(void)
{
		if((PLAY_SINK_CTROL_PLAY == sink_box_button_ctrol)
                    &&GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_BOOK))
		{
            if(spectrum_get_FlagForStop() == true)
            {
                spectrum_start();
            }
            else
            {
                spectrum_resume();
            }
		}
}

status_t sink_popmsg_mute_init(void)
{
	uint32_t value = 0;

	value=pmpset_get_int(PMPSET_MUTE);

	switch (value)
	{
		case PMPSET_MUTE_ON:
			GUI_SetProperty(BT_IMAGE_MUTE, "state", "show");
			break;

		case PMPSET_MUTE_OFF:
			GUI_SetProperty(BT_IMAGE_MUTE, "state", "hide");
			break;

		default:
			return GXCORE_ERROR;
			break;
	}
	return GXCORE_SUCCESS;
}

status_t sink_popmsg_mute_ctrl(void)
{
	uint32_t value = 0;

	value=pmpset_get_int(PMPSET_MUTE);

	switch (value)
	{
		case PMPSET_MUTE_ON:
			pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
            app_bluetooth_sink_set_mute(PMPSET_MUTE_OFF);
			GUI_SetProperty(BT_IMAGE_MUTE, "state", "hide");
            GUI_SetProperty(WND_SINK_VIEW, "update", NULL);
            GUI_SetInterface("flush", NULL);
            spectrum_volumebak_clear();
			//spectrum_resume();
			break;

		case PMPSET_MUTE_OFF:
			pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_ON);
            app_bluetooth_sink_set_mute(PMPSET_MUTE_ON);
			GUI_SetProperty(BT_IMAGE_MUTE, "state", "show");
			break;

		default:
			return GXCORE_ERROR;
			break;
	}
	return GXCORE_SUCCESS;
}


status_t sink_box_ctrl(sink_box_button button)
{
	switch(button)
	{
		case SINK_BOX_BUTTON_PLAY_PAUSE:
            if(PLAY_SINK_CTROL_PLAY == sink_box_button_ctrol)
			{
				sink_box_button_ctrol = PLAY_SINK_CTROL_PAUSE;
				app_bluetooth_sink_pause();
                sink_view_mode_pause();
                GUI_SetProperty(BT_IMAGE_POPMSG, "img",BT_IMG_POP_PAUSE);
			}
			else if(PLAY_SINK_CTROL_PAUSE == sink_box_button_ctrol)
			{
				sink_box_button_ctrol = PLAY_SINK_CTROL_PLAY;
                GUI_SetProperty(BT_IMAGE_POPMSG, "img", BT_IMG_POP_PLAY);
                sink_view_mode_resume();
                app_bluetooth_sink_play();
            }

            break;

		case SINK_BOX_BUTTON_PAUSE:
            GUI_SetProperty(BT_IMAGE_POPMSG, "img",BT_IMG_POP_PAUSE);
            sink_view_mode_pause();
            app_bluetooth_sink_pause();
            break;

		case SINK_BOX_BUTTON_PLAY:
            GUI_SetProperty(BT_IMAGE_POPMSG, "img", BT_IMG_POP_PLAY);
            sink_view_mode_resume();
            app_bluetooth_sink_play();
            break;

		case SINK_BOX_BUTTON_STOP:
            break;

		default:
			return GXCORE_ERROR;
			break;
	}

	return GXCORE_SUCCESS;
}

void app_bluetooth_sink_ui_status(GxMessage *msg)
{
    int ret = 0;
    int index = 0;
    GxMsgProperty_UsbBtPlayingInfo *sink_info;
    switch(msg->msg_id)
    {
        case APPMSG_BLUETOOTH_UI_CONTROL:
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
            {
                break;
            }
            if(BT_A2DP_SINK_PAUSE == app_check_bluetooth_status())
            {
                GUI_SetProperty(BT_IMAGE_POPMSG, "img",BT_IMG_POP_PAUSE);
                sink_view_mode_pause();
            }
            else
            {
                gx_bt_a2dp_sink_get_playing_info();
                GUI_SetProperty(BT_IMAGE_POPMSG, "img", BT_IMG_POP_PLAY);
                sink_view_mode_resume();
            }
            break;
        case GXMSG_USBBT_SINK_PLAYING_TITLE_INFO:
            sink_info = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtPlayingInfo);
            if(sink_info != NULL)
            {
                if (isalpha((unsigned char)sink_info->string[0]))
                {
                    GUI_SetProperty(BT_TEXT_SINK_NAME1, "string", sink_info->string);
                }
                app_log_info("*Title****string=%s\n", sink_info->string);
            }
            break;
        case GXMSG_USBBT_SINK_PLAYING_ARTIST_INFO:
            sink_info = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtPlayingInfo);
            if(sink_info != NULL)
            {
                app_log_info("*****string=%s\n", sink_info->string);
            }
            break;
        case GXMSG_USBBT_SINK_PLAYING_ALBUM_INFO:
            sink_info = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtPlayingInfo);
            if(sink_info != NULL)
            {
                app_log_info("**___***string=%s\n", sink_info->string);
            }
            break;
        case GXMSG_USBBT_SINK_TRACK_CHANGED:
			app_log_info(" ****** SINK Track Changed\n");
            for(index=0;index<20;index++)
            {
                ret = GxCore_BTSinkGetPlayingInfo();
                if (ret == 0)
                {
                    app_log_info("playininfo, index=%d\n",index);
                    break;
                }
            }
			break;

        default:
            break;
    }

}

SIGNAL_HANDLER  int app_sink_view_got_focus(const char* widgetname, void *usrdata)
{
	if((app_check_bluetooth_status() != BT_A2DP_SINK_PLAY)||(spectrum_get_FlagForStop() == true))
	{
        sink_view_mode_resume();
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_sink_view_lost_focus(const char* widgetname, void *usrdata)
{
	if ((GXCORE_SUCCESS != GUI_CheckDialog(WND_VOLUME))
            || (app_check_bluetooth_status() != BT_A2DP_SINK_PLAY))
	{
		sink_view_mode_pause();
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int sink_view_canvas_init(const char* widgetname, void *usrdata)
{
	/*bean: canvas redraw lost static action*/
	spectrum_redraw();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sink_view_create(const char* widgetname, void *usrdata)
{
    int32_t audio_vol = 0;
    app_bluetooth_register_sink_dogle();
	/*popmsg topright*/
	//GUI_SetProperty(BT_IMAGE_POPMSG, "state", "hide");
    /*set volume*/
	GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
	app_system_vol_set(audio_vol);

	/*popmsg_mute*/
	sink_popmsg_mute_init();
   	sink_view_mode_create();

    sink_box_ctrl(SINK_BOX_BUTTON_PLAY);
    gx_bt_a2dp_sink_get_playing_info();
	return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int app_sink_view_destroy(const char* widgetname, void *usrdata)
{
	spectrum_destroy();
    app_bluetooth_sink_pause();
    app_bluetooth_unregister_sink_dogle();

	return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int app_sink_view_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);

	event = (GUI_Event *)usrdata;
	switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
	{
		case VK_BOOK_TRIGGER:
            GUI_SetProperty(BT_TEXT_SINK_NAME1, "string", NULL);
			GUI_EndDialog("after wnd_full_screen");
			break;
		case APPK_BACK:
		case APPK_MENU:

			GUI_EndDialog(WND_SINK_VIEW);
			return EVENT_TRANSFER_STOP;
		case APPK_PAUSE_PLAY:
            sink_box_ctrl(SINK_BOX_BUTTON_PLAY_PAUSE);
            return EVENT_TRANSFER_STOP;

		case APPK_PAUSE:
			if(PLAY_SINK_CTROL_STOP != sink_box_button_ctrol)
			{
				sink_box_ctrl(SINK_BOX_BUTTON_PAUSE);
			}
			return EVENT_TRANSFER_STOP;

		case APPK_PLAY:
			sink_box_ctrl(SINK_BOX_BUTTON_PLAY);

            return EVENT_TRANSFER_STOP;
		case APPK_STOP:
			sink_box_ctrl(SINK_BOX_BUTTON_STOP);
			return EVENT_TRANSFER_STOP;
		case APPK_MUTE:
            sink_popmsg_mute_ctrl();
            return EVENT_TRANSFER_STOP;
		case APPK_VOLUP:
		case APPK_VOLDOWN:
        case APPK_LEFT:
        case APPK_RIGHT:
            {
				uint32_t value;
				value=pmpset_get_int(PMPSET_MUTE);

				if(value==PMPSET_MUTE_ON)
				{
                    app_bluetooth_sink_set_mute(PMPSET_MUTE_OFF);
					pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
					GUI_SetProperty(BT_IMAGE_MUTE, "state", "hide");
                    GUI_SetProperty(WND_SINK_VIEW, "update", NULL);
                    GUI_SetInterface("flush", NULL);
                    spectrum_volumebak_clear();
				}
				int32_t pos_y = 488;
				GUI_CreateDialog(WND_VOLUME);
				GUI_SetProperty(WND_VOLUME, "move_window_y", &pos_y);
				return EVENT_TRANSFER_STOP;
			}
            break;
        case APPK_0:
            gx_bt_a2dp_sink_get_playing_info();
            break;
		default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}

static int s_enter_sink_menu_flag = 0 ;

int app_is_sink_menu_mode(void)
{
    return s_enter_sink_menu_flag ;
}

static int _sink_menu_pop_cb(PopDlgRet ret)
{
    s_enter_sink_menu_flag = 0 ;
    return 0 ;
}

void app_create_sink_menu(void)
{
    int ret = 1 ;
    int init_connect = 0;
    char buff[100] ;

    init_connect = app_check_bt_connect_status();

    if( (init_connect == BT_A2DP_SINK_CONNECT) ||
        (init_connect == BT_A2DP_SINK_PLAY)    ||
        (init_connect == BT_A2DP_SINK_PAUSE) )
    {
        popdlg_destroy();
        GUI_CreateDialog(WND_SINK_VIEW);
    }
    else
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.pos.x = APP_POP_DLG_POS_X;
        pop.pos.y = APP_POP_DLG_POS_Y;

        if ( (init_connect == BT_A2DP_SOURCE_CONNECT) ||
             (init_connect == BT_A2DP_SOURCE_PLAY)    ||
             (init_connect == BT_A2DP_SOURCE_PAUSE)   ||
             (init_connect == BT_A2DP_SOURCE_CONNETING) )
        {
            pop.str = STR_ID_BLUETOOTH_CONNECT_DIS;
        }
        else
        {
            if( init_connect != BT_A2DP_SINK_CONNETING )
            {
                ret = app_bluez_dev_auto_sink_connect();
            }
            else
            {
                ret = 0 ;
            }
            if ( ret == 0 )
            {
                s_enter_sink_menu_flag = 1 ;
                pop.exit_cb = _sink_menu_pop_cb;
                pop.str = STR_ID_BLUETOOTH_CONNECT_ING;
            }
            else
            {
                memset(buff,0,100);
                sprintf(buff,"%s  [ %s ] ",STR_ID_LINK_BLUETOOTH,net_if_dev_usb_bt);
                pop.str = buff;
            }
        }
        popdlg_create(&pop);
    }
}

#endif
