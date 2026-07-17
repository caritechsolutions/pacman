#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_epg.h"
#include "app_pop.h"
#include "full_screen.h"
#include "app_default_params.h"
#include "app_windows.h"
#include "app_channel_info.h"

#if THEME_CLASSIC_BLUE
extern bool music_play_state(void);
extern void music_status_init(void);
extern void app_create_pvr_media_menu(void);

static bool s_media_on_fullscreen = false;

//resourse

//widget
#define BUTTON_DOWNLOAD			"media_centre_button_download"
#define IMAGE_DOWNLOAD			"media_centre_image_downloadsecant"
#define BUTTON_SETTING			"media_centre_button_setting"
#define IMAGE_SETTING			"media_centre_image_settingsecant"

#define TEXT_TITLE				"win_media_centre_text_title"
#define BOX_WINDOWN				"win_media_centre_box_main"
enum
{
	TYPE_FILE = 0,
	TYPE_VIDEO,
	TYPE_MUSIC,
	TYPE_PICTURE,
	TYPE_TEXT,
	TYPE_TOTAL,
};

int8_t *TextBuffer[] =
{
	(int8_t*)"File",
	(int8_t*)"Video",
	(int8_t*)"Music",
	(int8_t*)"Picture",
	(int8_t*)"Text",
};

void app_create_media_centre_fullsreeen(void)
{
    if(app_channel_info_check_dialog()== GXCORE_SUCCESS)
    {
        app_channel_info_end_dialog();
        GUI_SetInterface("flush", NULL);
    }
    if (g_AppPvrOps.state != PVR_DUMMY)
    {
        app_pvr_create_no_btn_popdlg(NULL);
    }
    else
    {
        app_clean_fullscreen_tip(false, false, false);
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        g_AppPlayOps.program_stop();
        GUI_CreateDialog(WIN_MEDIA_CENTRE);
        s_media_on_fullscreen = true;
    }
}

void app_media_centre_quit(void)
{
	GUI_EndDialog(WIN_MEDIA_CENTRE);
	GUI_SetProperty(WIN_MEDIA_CENTRE,"draw_now",NULL);
	if(s_media_on_fullscreen == true)
	{
	    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
	    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
	    if (g_AppPlayOps.normal_play.play_total != 0)
	    {
	        app_play_current_prog(PLAY_MODE_POINT);
	    }
	}
	s_media_on_fullscreen = false;
}

SIGNAL_HANDLER int media_centre_init(const char* widgetname, void *usrdata)
{
	int32_t init_value;
	init_value = 0;
	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
	pmpset_init();
	init_value = pmpset_get_int(PMPSET_MUTE);
	if (init_value == 0)
		{pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);}
	else
		{pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_ON);}

#ifdef PMP_ATTACH_DVB
	GUI_SetProperty(BUTTON_DOWNLOAD, "state", "hide");
	GUI_SetProperty(IMAGE_DOWNLOAD, "state", "hide");
	GUI_SetProperty(BUTTON_SETTING, "state", "hide");
	GUI_SetProperty(IMAGE_SETTING, "state", "hide");
#endif
	//FocusType = TYPE_FILE;
	GUI_SetProperty(TEXT_TITLE,"string",TextBuffer[0]);

	return 0;
}

SIGNAL_HANDLER int media_centre_destroy(const char* widgetname, void *usrdata)
{
	int32_t init_value;
	//for dvb
	pmpset_exit();
	/*stop all player av*/
    play_av_by_stop();

	/*stop all player pic*/
	GxMessage* new_msg_pic = NULL;
	new_msg_pic = GxBus_MessageNew(GXMSG_PLAYER_STOP);
	APP_CHECK_P(new_msg_pic, GXCORE_ERROR);

	GxMsgProperty_PlayerStop *stop_pic;
	stop_pic = GxBus_GetMsgPropertyPtr(new_msg_pic, GxMsgProperty_PlayerStop);
	APP_CHECK_P(stop_pic, GXCORE_ERROR);
	stop_pic->player = PMP_PLAYER_PIC;

	GxBus_MessageSendWait(new_msg_pic);
	GxBus_MessageFree(new_msg_pic);

	if(music_play_state() == TRUE)
	{
		music_status_init();
	}

	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

	return 0;
}
SIGNAL_HANDLER int media_centre_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	if(NULL == usrdata) return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    switch(event->key.sym)
    {
        case VK_BOOK_TRIGGER:
            GUI_SetProperty("win_media_centre_box_main", "state", "hide");
            GUI_SetProperty("win_media_centre_text_title", "state", "hide");
            GUI_SetProperty("win_media_centre_image_title", "state", "hide");
			GUI_SetInterface("flush", NULL);
			GUI_EndDialog("after wnd_full_screen");
			break;
		case APPK_BACK:
		case APPK_MENU:
			{
				GUI_SetProperty("win_media_centre_box_main", "state", "hide");
				GUI_SetProperty("win_media_centre_text_title", "state", "hide");
				GUI_SetProperty("win_media_centre_text_title", "draw_now", NULL);
				GUI_SetProperty("win_media_centre_image_title", "state", "hide");
				GUI_SetProperty("win_media_centre_image_title", "draw_now", NULL);
				app_media_centre_quit();
           	}
			break;
		case APPK_UP:
		case APPK_DOWN:
			break;
			//return EVENT_TRANSFER_KEEPON;

        case APPK_RIGHT:
        case APPK_LEFT:
            return EVENT_TRANSFER_KEEPON;

        default:
            break;
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int media_centre_button_video_clicked(const char* widgetname, void *usrdata)
{
	file_view_set_record_group(FILE_VIEW_GROUP_MOVIE);
    GUI_SetInterface("clear_image",NULL);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int media_centre_button_music_clicked(const char* widgetname, void *usrdata)
{
	file_view_set_record_group(FILE_VIEW_GROUP_MUSIC);
    GUI_SetInterface("clear_image",NULL);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int media_centre_button_picture_clicked(const char* widgetname, void *usrdata)
{
	file_view_set_record_group(FILE_VIEW_GROUP_PIC);
    GUI_SetInterface("clear_image",NULL);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int media_centre_button_text_clicked(const char* widgetname, void *usrdata)
{
	file_view_set_record_group(FILE_VIEW_GROUP_TEXT);
    GUI_SetInterface("clear_image",NULL);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int media_centre_button_file_clicked(const char* widgetname, void *usrdata)
{
	file_view_set_record_group(FILE_VIEW_GROUP_ALL);
    GUI_SetInterface("clear_image",NULL);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int media_centre_item_change(const char* widgetname, void *usrdata)
{
	int32_t Sel = 0;
	GUI_GetProperty(BOX_WINDOWN,"select",&Sel);
	GUI_SetProperty(TEXT_TITLE,"string",TextBuffer[Sel]);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int media_centre_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint32_t box_sel;

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
				case STBK_UP:
				case STBK_LEFT:
					GUI_GetProperty(BOX_WINDOWN, "select", &box_sel);
					if(TYPE_FILE == box_sel)
					{
						box_sel = TYPE_TEXT;
						GUI_SetProperty(BOX_WINDOWN, "select", &box_sel);
						GUI_SetProperty(TEXT_TITLE,"string",TextBuffer[box_sel]);
						ret = EVENT_TRANSFER_STOP;
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;

				case STBK_DOWN:
				case STBK_RIGHT:
					GUI_GetProperty(BOX_WINDOWN, "select", &box_sel);
					if(TYPE_TEXT == box_sel)
					{
						box_sel = TYPE_FILE;
						GUI_SetProperty(BOX_WINDOWN, "select", &box_sel);
						GUI_SetProperty(TEXT_TITLE,"string",TextBuffer[box_sel]);
						ret = EVENT_TRANSFER_STOP;
					}
					else
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;

				case STBK_OK:
				case STBK_MENU:
				case STBK_EXIT:
				//case STBK_UP:
				//case STBK_DOWN:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
		default:
			break;
	}

    return ret;
}

void app_media_centre_menu_exec(void)
{
    g_AppPvrOps.tms_delete(&g_AppPvrOps);
    GUI_CreateDialog(WIN_MEDIA_CENTRE);
}
#endif
