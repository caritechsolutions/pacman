#include "app.h"
#include "gdi_play.h"
#include "app_windows.h"

#define TEXT_INFO0			"movie_info_text_info0"
#define TEXT_INFO1          	"movie_info_text_info1"
#define TEXT_INFO2		       "movie_info_text_info2"
#define TEXT_INFO3	              "movie_info_text_info3"
#define TEXT_INFO4                 "movie_info_text_info4"
#define TEXT_INFO5                 "movie_info_text_info5"

static void _show_movie_info(void)
{
	char string[100];
	play_movie_info movie_info;
	PlayerProgInfo *info = app_player_prog_info_get();

	play_movie_get_info(&movie_info);
	GxPlayer_MediaGetProgInfoByName(PMP_PLAYER_AV, info);

	GUI_SetProperty(TEXT_INFO0, "string", movie_info.name);

	if(info->file_size)
	{
		memset(string,0,sizeof(string));
        play_av_display_size_str(info->file_size,string);
		GUI_SetProperty(TEXT_INFO1, "string", string);
	}

	if(info->video.width)
	{
		memset(string,0,sizeof(string));
		sprintf(string, "%d x %d", info->video.width, info->video.height);
		GUI_SetProperty(TEXT_INFO2, "string", string);
	}
	else
		GUI_SetProperty(TEXT_INFO2, "string", " ");

	if(info->video.codec_id)
	{
		GUI_SetProperty(TEXT_INFO3, "string", info->video.codec_id);
	}
	else
		GUI_SetProperty(TEXT_INFO3, "string", " ");

	if(info->video.bitrate)
	{
		memset(string,0,sizeof(string));
		sprintf(string, "%d bps",info->video.bitrate);
		GUI_SetProperty(TEXT_INFO4, "string", string);
	}
	else
	{
		GUI_SetProperty(TEXT_INFO4, "string", " ");
	}


	if(info->video.fps)
	{
		memset(string,0,sizeof(string));
		sprintf(string, "%3.1f fps",info->video.fps);
		GUI_SetProperty(TEXT_INFO5, "string", string);
	}
	else
		GUI_SetProperty(TEXT_INFO5, "string", " ");
}

SIGNAL_HANDLER  int movie_info_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	GUI_SendEvent(WIN_MOVIE_VIEW, event);

	_show_movie_info();

	GUI_SetProperty(WIN_MOVIE_INFO, "update", NULL);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int movie_info_init(const char* widgetname, void *usrdata)
{
	_show_movie_info();
	return 0;
}

SIGNAL_HANDLER int movie_info_destroy(const char* widgetname, void *usrdata)
{
	return 0;
}


SIGNAL_HANDLER int movie_info_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	if(NULL == usrdata) return EVENT_TRANSFER_STOP;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;
				case APPK_LEFT:
					break;

				case APPK_RIGHT:
					break;

				case APPK_OK:
					GUI_EndDialog(WIN_MOVIE_INFO);
					break;

				default:
					break;
			}
			break;
		default:
			break;
	}


	return EVENT_TRANSFER_KEEPON;
}



