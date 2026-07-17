#include "app.h"
#include "app_pop.h"
#include "app_windows.h"
#include "pmp_subtitle.h"

#define AUDIO_DELAY_SUPPORT 0

#define BOX_MOVIE_SET				"movie_set_box"
#define EDIT_SEEK_TIME				"movie_set_edit_seek"
#define COMBOBOX_AUDIO_CHANNEL		"movie_set_combo_audio_channel"
#define COMBOBOX_SWITCH_SEQUENCE	"movie_set_combo_play_sequence"
#define BUTTON_SET_DELAY            "movie_set_button_delay"
#define COMBOBOX_DISPLAY_MODE		"movie_set_combo_display_mode"

enum
{
    MOV_SEEK = 0,
    MOV_AUDIO_CH,
    MOV_PLAY_MODE,
#if AUDIO_DELAY_SUPPORT > 0
    MOV_AUDIO_DELAY,
#endif
    MOV_VIEW_MODE
};

static event_list *s_audio_switch_timer = NULL;
int audio_delay_ms = 0;
static int s_movie_set_total_time = 0;
static pmpset_movie_play_sequence s_quence_mode = 0;
extern status_t app_seek_set_mediaplay(int seek_ms,SeekFlag flag);
static int combobox_init_audio_channel(void)
{
	PlayerProgInfo *info = app_player_prog_info_get();

	GxPlayer_MediaGetProgInfoByName(PMP_PLAYER_AV, info);
	if((0 >= info->audio_num)||(PLAYER_MAX_TRACK_AUDIO < info->audio_num))
	{
		GUI_SetProperty(COMBOBOX_AUDIO_CHANNEL, "content", (void*)"[None]");
		return 1;
	}

	char* combobox_buf = NULL;
	char* combobox_node = NULL;
	char* default_buf_data = "aud";
	char default_buf_index[20] = {0};
	int i = 0;
	int sel;

	combobox_buf = GxCore_Calloc(1, PLAYER_MAX_TRACK_AUDIO*PLAYER_TARCK_NAME_LONG + PLAYER_MAX_TRACK_AUDIO + 100);
	if(NULL == combobox_buf) {
		return 1;
	}

	/*combobox context*/
	strcpy(combobox_buf,  "[");
	for(i = 0; i < info->audio_num; i++)
	{
		/*add ,*/
		if(0 != i) strcat(combobox_buf, ",");

		/*add index*/
		memset(default_buf_index, 0, sizeof(default_buf_index));
		sprintf(default_buf_index, "%d.", i);
		strcat(combobox_buf, default_buf_index);

		/*add node*/
		if('\0' != info->audio[i].track_name[0])
			combobox_node = info->audio[i].track_name;
		else
			combobox_node = info->audio[i].lang;

		//default combobox_node
		if((NULL == combobox_node) || (0 == strlen(combobox_node)))
		{
			combobox_node = default_buf_data;
		}

		strcat(combobox_buf, combobox_node);
	}
	strcat(combobox_buf,  "]");

	app_log_debug("AUD: combobox_buf %s\n", combobox_buf);
	sel = info->cur_audio_id;
	GUI_SetProperty(COMBOBOX_AUDIO_CHANNEL, "content", (void*)combobox_buf);
	GUI_SetProperty(COMBOBOX_AUDIO_CHANNEL, "select", &sel);
	APP_FREE(combobox_buf);

	return 0;
}


static int combobox_set_audio_channel(int index)
{
    APP_TIMER_REMOVE(s_audio_switch_timer);
    if (GXCORE_SUCCESS != GUI_CheckDialog(WIN_MOVIE_VIEW))
    {
        return 0;
    }

	PlayerProgInfo *info = app_player_prog_info_get();
	GxMessage *msg;

	GxPlayer_MediaGetProgInfoByName(PMP_PLAYER_AV, info);
	if(index >= info->audio_num)
	{
		return 1;
	}

	GxMsgProperty_PlayerAudioSwitch *audio_switch;
	msg = GxBus_MessageNew(GXMSG_PLAYER_AUDIO_SWITCH);
	APP_CHECK_P(msg, 1);
	audio_switch = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerAudioSwitch);
	APP_CHECK_P(audio_switch, 1);
	audio_switch->player = PMP_PLAYER_AV;
	audio_switch->pid = index;
	audio_switch->type = info->audio[index].codec_type;
	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);

	return 0;
}

static int _timer_movie_set_audio(void* userdata)
{
    APP_TIMER_REMOVE(s_audio_switch_timer);
    int index = 0;
    if (GXCORE_SUCCESS != GUI_CheckDialog(WIN_MOVIE_VIEW))
    {
        return 0;
    }
    GUI_GetProperty(COMBOBOX_AUDIO_CHANNEL, "select", (void*)&index);
    combobox_set_audio_channel(index);

	return 0;
}

#if AUDIO_DELAY_SUPPORT > 0
static int button_set_audio_delay(int delay_ms)
{
	GxMessage *msg;

	GxMsgProperty_PlayerAudioSync *audio_delay;
	msg = GxBus_MessageNew(GXMSG_PLAYER_AUDIO_SYNC);
	APP_CHECK_P(msg, 1);
	audio_delay = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerAudioSync);
	APP_CHECK_P(audio_delay, 1);
	audio_delay->player = PMP_PLAYER_AV;
	audio_delay->timems = delay_ms;
	GxBus_MessageSendWait(msg);
	GxBus_MessageFree(msg);

	return 0;
}
#endif

static status_t key_ok(void)
{
	uint32_t item_sel = 0;
    int ret = GXCORE_ERROR;
	GUI_GetProperty(BOX_MOVIE_SET, "select", (void*)&item_sel);

	switch(item_sel)
	{
		/*seek*/
		case 0:
			{
				/*get*/
				char* str_seek = NULL;
				int ms_seek = 0;
				int hour = 0;
				int minite = 0;
				int second = 0;
				PlayerProgInfo *info = app_player_prog_info_get();
				PlayTimeInfo  tinfo = {0};

				GxPlayer_MediaGetProgInfoByName(PMP_PLAYER_AV, info);
				s_movie_set_total_time = info->duration;

				GUI_GetProperty(EDIT_SEEK_TIME, "string", (void*)&str_seek);
				if(NULL == str_seek) break;
				sscanf(str_seek, "%d:%d:%d", &hour, &minite, &second);
				app_log_debug("[MOVIE] seek %d:%d:%d\n", hour, minite, second);
				ms_seek = (hour*60*60 + minite*60 + second) * 1000;
				GxPlayer_MediaGetTime(PMP_PLAYER_AV, &tinfo);

				/*check*/

				/*dealy with ms*/
				if((ms_seek < s_movie_set_total_time + 1000) && (ms_seek > s_movie_set_total_time))
				{
					ms_seek = s_movie_set_total_time; //有余数显示时间会加一秒
				}

				if(ms_seek > s_movie_set_total_time)
				{
                    GUI_SetProperty(EDIT_SEEK_TIME, "string", "00:00:00");
					PopDlg pop;
					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_OK;
					pop.format = POP_FORMAT_DLG;
					pop.str = "Seek time error";
					pop.mode = POP_MODE_UNBLOCK;
					pop.timeout_sec = 6;
					popdlg_create(&pop);
					break;
				}
				ms_seek=ms_seek+tinfo.seek_min;
				/*set*/
				app_seek_set_mediaplay(ms_seek, SEEK_ORIGIN_SET);
                ret = GXCORE_SUCCESS;
				break;
			}

		default:
			break;
	}
	app_log_debug("[MOVIE] keypress_movie_view_set_ok item:%d, value:0\n", item_sel);

	return ret;
}

static void app_movie_set_play_mode(void)
{
    pmpset_movie_play_sequence sequence = 0;
    sequence = pmpset_get_int(PMPSET_MOVIE_PLAY_SEQUENCE);
    if(sequence != s_quence_mode)
        pmpset_set_int(PMPSET_MOVIE_PLAY_SEQUENCE, s_quence_mode);
}

static status_t key_left_right(unsigned short value)
{
    uint32_t item_sel = 0;
    uint32_t value_sel = 0;
    GUI_GetProperty(BOX_MOVIE_SET, "select", (void*)&item_sel);

    switch(item_sel)
    {
        /*audio channel*/
        case MOV_AUDIO_CH:
            {
                APP_TIMER_ADD(s_audio_switch_timer, _timer_movie_set_audio,500,TIMER_REPEAT);
                break;
            }

            /*sequence*/
        case MOV_PLAY_MODE:
            {
                GUI_GetProperty(COMBOBOX_SWITCH_SEQUENCE, "select", (void*)&value_sel);
                s_quence_mode = value_sel;
                break;
            }

#if AUDIO_DELAY_SUPPORT > 0
            /*audio delay*/
        case MOV_AUDIO_DELAY:
            {
                char string[15] = {0};
                if(APPK_LEFT==value)
                {
                    if(audio_delay_ms >= -4800)
                        audio_delay_ms -= 200;
                }
                else
                {
                    if(audio_delay_ms < 5000)
                        audio_delay_ms += 200;
                }
                memset(string,0,sizeof(string));
                sprintf(string, "%dms", audio_delay_ms);
                GUI_SetProperty(BUTTON_SET_DELAY, "string", (void*)string);

                button_set_audio_delay(audio_delay_ms);
            }
            break;
#endif
            // for display mode
        case MOV_VIEW_MODE:
            GUI_GetProperty(COMBOBOX_DISPLAY_MODE, "select", (void*)&value_sel);
            pmpset_set_int(PMPSET_ASPECT_RATIO, value_sel);
            break;

        default:
            break;
    }

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER  int movie_set_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	GxMessage * msg;
	event = (GUI_Event *)usrdata;
	GxMsgProperty_PlayerStatusReport* player_status = NULL;
	//error#4252
	msg = (GxMessage*)event->msg.service_msg;
	switch(msg->msg_id)
	{
		case GXMSG_PLAYER_STATUS_REPORT:
			player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
            //app_log_debug("dgw----> movie set  %d  %d\n",player_status->status,PLAYER_STATUS_PLAY_END);
			if(PLAYER_STATUS_PLAY_END == player_status->status || PLAYER_STATUS_ERROR == player_status->status)
				GUI_EndDialog(WIN_MOVIE_SET);
			break;
		default:
			break;
	}
	GUI_SendEvent(WIN_MOVIE_VIEW, event);

	return EVENT_TRANSFER_STOP;
}



SIGNAL_HANDLER int movie_set_init(const char* widgetname, void *usrdata)
{
    int32_t value = 0;
    /*seek*/

    /*audio channel*/
    combobox_init_audio_channel();

    /*sequence*/
    s_quence_mode = pmpset_get_int(PMPSET_MOVIE_PLAY_SEQUENCE);
    GUI_SetProperty(COMBOBOX_SWITCH_SEQUENCE, "select", (void*)&s_quence_mode);

#if AUDIO_DELAY_SUPPORT > 0
    /*audio delay*/
    //TODO: get audio_delay_ms from player
    char string[10] = {0};
    sprintf(string, "%dms", audio_delay_ms);
    GUI_SetProperty(BUTTON_SET_DELAY, "string", (void*)string);
#endif

    /*display mode*/
    value = pmpset_get_int(PMPSET_ASPECT_RATIO);
    GUI_SetProperty(COMBOBOX_DISPLAY_MODE, "select", (void*)&value);

    PlayerProgInfo *info = app_player_prog_info_get();

    GxPlayer_MediaGetProgInfoByName(PMP_PLAYER_AV, info);
    if(info->is_radio == 1)//Radio
    {
        //GUI_SetProperty("movie_set_boxitem3","state","disable");
        GUI_SetProperty("movie_set_boxitem4","disable","NULL");
    }
    else
    {
        //GUI_SetProperty("movie_set_boxitem3","state","enable");
		GUI_SetProperty("movie_set_boxitem4", "enable", NULL);
//        GUI_SetProperty("movie_set_boxitem4","state","enable");
    }

    return 0;
}

SIGNAL_HANDLER int movie_set_destroy(const char* widgetname, void *usrdata)
{
    app_movie_set_play_mode();
	return 0;
}

SIGNAL_HANDLER int app_movie_set_edit_seek_reach_end(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define MOVIE_SET_EDIT_SEEK_LEN 8
    GUI_GetProperty("movie_set_edit_seek", "select", &cur_sel);
    sel = (cur_sel == 0 ? MOVIE_SET_EDIT_SEEK_LEN - 1 : 0);
    GUI_SetProperty("movie_set_edit_seek", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int movie_set_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	if(NULL == usrdata) return EVENT_TRANSFER_STOP;
	event = (GUI_Event *)usrdata;
	switch(event->key.sym)
	{
		case VK_BOOK_TRIGGER:
			GUI_EndDialog("after wnd_full_screen");
			break;

		case APPK_LEFT:
			key_left_right(event->key.sym);
			break;
		case APPK_RIGHT:
			key_left_right(event->key.sym);
			break;

		case APPK_OK:
			if(key_ok() == GXCORE_SUCCESS)
            {
                if(s_audio_switch_timer != NULL)
                {
                    _timer_movie_set_audio(NULL);
                }
                GUI_EndDialog(WIN_MOVIE_SET);
            }
			break;

		case APPK_REPEAT:
		{
			int value;

			value = pmpset_get_int(PMPSET_MOVIE_PLAY_SEQUENCE);
			if(value == PMPSET_MOVIE_PLAY_SEQUENCE_SEQUENCE)
			{
				value = PMPSET_MOVIE_PLAY_SEQUENCE_ONLY_ONCE;
			}
			else
			{
				value++;
			}
				pmpset_set_int(PMPSET_MOVIE_PLAY_SEQUENCE, value);

			GUI_SetProperty(COMBOBOX_SWITCH_SEQUENCE, "select", (void*)&value);
			break;
		}
        case APPK_BACK:
            {
                if(s_audio_switch_timer != NULL)
                {
                    _timer_movie_set_audio(NULL);
                }
            }
            break;

		default:
			break;
	}


	return EVENT_TRANSFER_KEEPON;
}


