/*
 * =====================================================================================
 *
 *       Filename:  app_audio.c
 *
 *    Description:  select dvb subtitle or ttx subtitle
 *
 *        Version:  1.0
 *        Created:  2011年10月13日 10时14分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_module.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_send_msg.h"
#include "full_screen.h"
#include "app_utility.h"
#include "app_audio.h"
#include "app_windows.h"
#include "app_ui_style.h"
#include "app_common_select.h"
#include "app_channel_info.h"
#include "app_wnd_file_list.h"
#include "app_netaudio.h"


#define SUBT_OFF_MODE_NUM    1

static AudioLang_t audioLang = {0,0};
static handle_t s_audio_mutex = -1;
static bool netaudio_to_audio_flag = false;
static event_list* s_audio_list_refresh_timer = NULL;
static void app_audio_init(void);

extern uint8_t app_get_audio_count_num(void);
extern int app_pmt_get_muli_audio_type_by_elempid(int elem_pid);
extern void app_show_channel_pid(GxBusPmDataProg *prog_data);
extern int app_set_first_audio_lang(void);

AudioLang_t* app_get_audio_lang(void)
{
    return &audioLang;
}

void app_audio_lang_free(AudioLang_t *audio_lang)
{
    if((audio_lang != NULL) && (NULL != audio_lang->audioLangList))
    {
        uint32_t i = 0;

        for(i=0; i<audio_lang->audioLangCnt; i++)
        {
            if(audio_lang->audioLangList[i].name != NULL)
            {
                GxCore_Free(audio_lang->audioLangList[i].name);
                audio_lang->audioLangList[i].name = NULL;
            }
        }
        GxCore_Free(audio_lang->audioLangList);
        audio_lang->audioLangList = NULL;
        audio_lang->audioLangCnt = 0;
    }
}

void app_audio_free(void)
{
    if(s_audio_mutex == -1)
    {
        GxCore_MutexCreate(&s_audio_mutex);
    }
    if(app_netaudio_check_flag())
        return;

    GxCore_MutexLock(s_audio_mutex);
    app_audio_lang_free(&audioLang);
    GxCore_MutexUnlock(s_audio_mutex);
#if AUDIO_DESCRIPTOR_SUPPORT
    app_audio_description_free();
#endif
}

char *app_audio_get_str_duplication(uint8_t *array, uint32_t strLen)
{
	char *desStr = NULL;
	uint32_t i = 0;
	if(NULL == array || 0 == strLen)
		return NULL;

	desStr = GxCore_Malloc((strLen+1)*sizeof(char));
	for(i=0; i<strLen; i++)
	{
		desStr[i] = array[i];
	}
	desStr[strLen] = '\0';
	return desStr;

}

uint32_t app_audio_get_array_len(uint8_t *array, uint32_t strLen)
{
	uint32_t i = 0;

	if(NULL == array)
		return 0;

	for(i = 0; i< strLen; i++)
	{
		if('\0' == array[i])
			break;
	}
	return i;
}

AudioCodecType app_audio_type(uint32_t elem_pid)
{
	AudioCodecType ret = AUDIO_CODEC_UNKNOWN;
	uint32_t audio_type = 0;

	audio_type = app_pmt_get_muli_audio_type_by_elempid(elem_pid);
	switch(audio_type)
	{
		case GXBUS_PM_AUDIO_MPEG1:
			ret = AUDIO_CODEC_MPEG1;
			break;
		case GXBUS_PM_AUDIO_MPEG2:
			ret = AUDIO_CODEC_MPEG2;
			break;
		case GXBUS_PM_AUDIO_DRA:
			ret = AUDIO_CODEC_DRA1;
			break;
		case GXBUS_PM_AUDIO_AC3:
			ret = AUDIO_CODEC_AC3;
			break;
		case GXBUS_PM_AUDIO_DTS:
			ret = AUDIO_CODEC_DTS;
			break;
		case GXBUS_PM_AUDIO_EAC3:
			ret = AUDIO_CODEC_EAC3;
			break;
		case GXBUS_PM_AUDIO_AAC_LATM:
			ret = AUDIO_CODEC_AAC_LATM;
			break;
		case GXBUS_PM_AUDIO_AAC_ADTS:
			ret = AUDIO_CODEC_AAC_ADTS;
			break;
		default:
			ret = AUDIO_CODEC_UNKNOWN;
			break;
	}
	return ret;
}

int app_audio_prog_track_get_mode(void)
{
	int track_sel = 0;
	GxMsgProperty_NodeByPosGet node = {0};

	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	{

		if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_MONO)
		{
			track_sel = 0;
		}
		else if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_LEFT)
		{
			track_sel = 1;
		}
		else if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_RIGHT)
		{
			track_sel = 2;
		}
		else
		{
			track_sel = 3;
		}

	}

	return track_sel;
}

int app_audio_prog_track_set_mode(int value,uint8_t mode)
{
	GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_NodeModify node_modify = {0};
	GxBusPmDataProgAduioTrack prog_track = AUDIO_TRACK_MONO;

	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	{
		node_modify.node_type = NODE_PROG;
		node_modify.prog_data = node.prog_data;
		if(value == 0)
		{
			prog_track = GXBUS_PM_PROG_AUDIO_MODE_MONO;
		}
		else if(value == 1)
		{
			prog_track =GXBUS_PM_PROG_AUDIO_MODE_LEFT;
		}
		else if(value == 2)
		{
			prog_track =GXBUS_PM_PROG_AUDIO_MODE_RIGHT;
		}
		else
		{
			prog_track =GXBUS_PM_PROG_AUDIO_MODE_STEREO;
		}

		if(0xfe == mode)
		{
			node_modify.prog_data.audio_volume = value;
		}
		else
		{
			node_modify.prog_data.audio_mode = prog_track;
		}

		app_send_msg_exec(GXMSG_PM_NODE_MODIFY, (void *)(&node_modify));

		if(0xfe != mode)
		{
			//value 0,1,2,3  MONO,LEFT,RIGHT,STEREO. from AudioTrack gxbus gxplayer_setting.h
			app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &value);
		}
	}

	return 0;
}

static int app_audio_list_refresh(void *userdata)
{
	GUI_SetProperty(g_CommonSelect.widget.list,"update_all",NULL);
	app_audio_init();
    app_update_pop_dlg(WND_PASSWD);
	s_audio_list_refresh_timer = NULL;
    return 0;
}

/*please GxCore_Free the memory by yourself*/
char* app_audio_get_string_strcat(char *src,char *dst,char *separator)
{
	int src_len = 0;
	int des_len = 0;
	int total_len = 0;
	int sep_len = 0;

	char *str = NULL;

	if(NULL != src)
		src_len = strlen(src);

	if(NULL != dst)
		des_len = strlen(dst);

	if(NULL != separator)
		sep_len = strlen(separator);


	total_len = src_len + sep_len+des_len+1;

	str = GxCore_Malloc(total_len*sizeof(char));
	if(NULL != str)
	{
		memset(str,0,total_len*sizeof(char));

		if(NULL != src)
			strcat(str,src);
		if(NULL != separator)
			strcat(str,separator);
		if(NULL != dst)
			strcat(str,dst);

		str[total_len-1] = '\0';
	}

	return str;
}

void app_track_set(PmtInfo* audio)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t num = 0;
    uint8_t NormalCount = 0;
    uint8_t Ac3Count = 0;
    uint8_t EAc3Count = 0;
    uint8_t FindAudio = 0;
    uint8_t SpaceCount = 0;
    AudioCodecType audioType = AUDIO_CODEC_UNKNOWN;

    GxCore_MutexLock(s_audio_mutex);
    app_audio_lang_free(&audioLang);

    if(NULL == audio ||0 == audio->stream_count)
    {
        GxCore_MutexUnlock(s_audio_mutex);
        return;
    }

    audioLang.audioLangCnt = app_get_audio_count_num();

    if(0 == audioLang.audioLangCnt)
    {
        GxCore_MutexUnlock(s_audio_mutex);
        return;
    }

    audioLang.audioLangList = GxCore_Calloc(audioLang.audioLangCnt, sizeof(AudioList_t));
    if(NULL == audioLang.audioLangList)
    {
        audioLang.audioLangCnt = 0;
        GxCore_MutexUnlock(s_audio_mutex);
        return;
    }

    for(i = 0; i<audio->stream_count; i++)
    {
        audioType = app_audio_type(audio->stream_info[i].elem_pid);

        if(audioType != AUDIO_CODEC_UNKNOWN)
        {
            if((audio->stream_info[i].iso639_count != 0) && (audio->stream_info[i].iso639_info))
            {
                if(audio->stream_info[i].iso639_info->audio_type == 2 || audio->stream_info[i].iso639_info->audio_type == 3 )
                {
                    audioLang.audioLangCnt = audioLang.audioLangCnt - 1;
                    continue;
                }
            }
            if(audioType != AUDIO_CODEC_EAC3)
            {

				#define MAX_ID_LEN    6
                char buf[MAX_ID_LEN] = {0};
                audioLang.audioLangList[j].audiotype = audioType;
                audioLang.audioLangList[j].id = audio->stream_info[i].elem_pid;
                if((NULL != audio->stream_info[i].iso639_info)
                        &&(0 != app_audio_get_array_len(audio->stream_info[i].iso639_info->iso639,3)))
                {
                    audioLang.audioLangList[j].name = app_audio_get_str_duplication(audio->stream_info[i].iso639_info->iso639,3);
                    if(*audioLang.audioLangList[j].name == ' ') {
                        SpaceCount++;
                        sprintf(buf,"%d", SpaceCount);
                        buf[MAX_ID_LEN-1] = '\0';
                        audioLang.audioLangList[j].name = app_audio_get_string_strcat("Audio",buf,NULL);
                    }
#if AUDIO_LANG_QAA2OST_SUPPORT
                    if(strncmp(audioLang.audioLangList[j].name,"qaa",3) == 0)
                    {
                        memset(audioLang.audioLangList[j].name, 0, 4*sizeof(char));
                        memcpy(audioLang.audioLangList[j].name, "OST", 3*sizeof(char));
                    }
#endif
                }
                else
                {
                    if(audioType == AUDIO_CODEC_AC3)
                    {
                        Ac3Count++;
                        sprintf(buf, "%d",Ac3Count);
                        buf[MAX_ID_LEN-1] = '\0';
                        audioLang.audioLangList[j].name = app_audio_get_string_strcat("Dolby",buf,NULL);
                    }
                    else
                    {
                        NormalCount++;
                        sprintf(buf, "%d",NormalCount);
                        buf[MAX_ID_LEN-1] = '\0';
                        audioLang.audioLangList[j].name = app_audio_get_string_strcat("Audio",buf,NULL);
                    }
                }
                j++;
            }
            else
            {
                FindAudio = 0;
                for(num=0; num<audio->ac3_count;num++)
                {

                    if(((Ac3Descriptor*)audio->ac3_info[num])->elem_pid == audio->stream_info[i].elem_pid)
                    {
                        audioLang.audioLangList[j].audiotype = AUDIO_CODEC_AC3;
                        audioLang.audioLangList[j].id = ((Ac3Descriptor*)audio->ac3_info[num])->elem_pid;
                        FindAudio = 1;
                        break;
                    }
                }

                if(num == audio->ac3_count)
                {
                    for(num=0; num<audio->eac3_count;num++)
                    {
                        if(((Eac3Descriptor*)audio->eac3_info[num])->elem_pid == audio->stream_info[i].elem_pid)
                        {
                            audioLang.audioLangList[j].audiotype = AUDIO_CODEC_EAC3;
                            audioLang.audioLangList[j].id = ((Eac3Descriptor*)audio->eac3_info[num])->elem_pid;
                            FindAudio = 1;
                            break;
                        }
                    }
                }

                if(FindAudio == 1)
                {// find the audio, AC3 OR EAC3
                    if((NULL != audio->stream_info[i].iso639_info)
                            &&(0 != app_audio_get_array_len(audio->stream_info[i].iso639_info->iso639,3)))
                    {
                        audioLang.audioLangList[j].name = app_audio_get_str_duplication(audio->stream_info[i].iso639_info->iso639,3);
                    #if AUDIO_LANG_QAA2OST_SUPPORT
                    if(strncmp(audioLang.audioLangList[j].name,"qaa",3) == 0)
                    {
                        memset(audioLang.audioLangList[j].name, 0, 4*sizeof(char));
                        memcpy(audioLang.audioLangList[j].name, "OST", 3*sizeof(char));
                    }
                    #endif
                    }
                    else
                    {
		            			#define MAX_ID_LEN    6
                        char buf[MAX_ID_LEN] = {0};

                        if(audioType == AUDIO_CODEC_AC3 )
                        {
                            Ac3Count++;
                            sprintf(buf, "%d",Ac3Count);
                            buf[MAX_ID_LEN-1] = '\0';
                            audioLang.audioLangList[j].name = app_audio_get_string_strcat("Dolby",buf,NULL);
                        }
                        else if (audioType == AUDIO_CODEC_EAC3)
                        {
                            EAc3Count++;
                            sprintf(buf, "%d",EAc3Count);
                            buf[MAX_ID_LEN-1] = '\0';
                            audioLang.audioLangList[j].name = app_audio_get_string_strcat("Dolby+",buf,NULL);
                        }
                        else
                        {
                            NormalCount++;
                            sprintf(buf, "%d",NormalCount);
                            buf[MAX_ID_LEN-1] = '\0';
                            audioLang.audioLangList[j].name = app_audio_get_string_strcat("Audio",buf,NULL);
                        }
                    }
                    j++;
                }
            }
        }
    }
    audioLang.audioLangCnt = j;
    int sel = 0;
    if(app_netaudio_check_flag())
    {
        GxCore_MutexUnlock(s_audio_mutex);
        return;
    }
    GxBus_ConfigGetInt(ADVANCED_AUDIO_REMEMBER_KEY, &sel, ADVANCED_AUDIO_REMEMBER_VALUE);
    if(sel == 0 && netaudio_to_audio_flag == false)
    {
        app_set_first_audio_lang();
    }
    netaudio_to_audio_flag = false;
#ifdef RECOVER_AUDIO_ID_TYPE
    GxMsgProperty_NodeByPosGet node = {0};
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        for(i = 0; i < audioLang.audioLangCnt; i++)
        {
            if(audioLang.audioLangList[i].id == node.prog_data.cur_audio_pid)
            {
                break;
            }
        }
        if(i >= audioLang.audioLangCnt) //can't find track
        {
            GxMsgProperty_NodeModify  modify_node = {0};
            GxPlayer_MediaAudioSwitch(PLAYER_FOR_NORMAL, audioLang.audioLangList[0].id, audioLang.audioLangList[0].audiotype, NULL);

            memcpy(&(modify_node.prog_data),&(node.prog_data),sizeof(GxBusPmDataProg));
            modify_node.node_type = NODE_PROG;
            modify_node.prog_data.cur_audio_pid = audioLang.audioLangList[0].id;
            if(AUDIO_CODEC_AC3 == audioLang.audioLangList[0].audiotype)
            {
                modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_AC3;
            }
            else if(AUDIO_CODEC_EAC3 == audioLang.audioLangList[0].audiotype)
            {
                modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_EAC3;
            }
            else if(AUDIO_CODEC_MPEG1 == audioLang.audioLangList[0].audiotype)
            {
                modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_MPEG1;
            }
            else if(AUDIO_CODEC_MPEG2 == audioLang.audioLangList[0].audiotype)
            {
                modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_MPEG2;
            }
            else if(AUDIO_CODEC_AAC_ADTS == audioLang.audioLangList[0].audiotype)
            {
                modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_AAC_ADTS;
            }
            else if(AUDIO_CODEC_DRA1 == audioLang.audioLangList[0].audiotype)
            {
                modify_node.prog_data.cur_audio_type = GXBUS_PM_AUDIO_DRA;
            }

            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, (void *)(&modify_node));

            if (GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
                app_show_channel_pid(&modify_node.prog_data);
        }
    }
	#endif


    GxCore_MutexUnlock(s_audio_mutex);
    if(GUI_CheckDialog(WND_COMMON_SELECT) == GXCORE_SUCCESS && strcmp(g_CommonSelect.title, STR_ID_AUDIO) == 0)
    {
        if (reset_timer(s_audio_list_refresh_timer) != 0)
        {
            s_audio_list_refresh_timer = create_timer(app_audio_list_refresh, 10, NULL, TIMER_ONCE);
        }
    }
}

static void app_audio_list_end_diadlg(void)
{
    uint32_t box_sel = 0;
    GUI_GetProperty(g_CommonSelect.widget.group, "select", &box_sel);
    app_audio_prog_track_set_mode(box_sel,0);
    GUI_EndDialog(WND_COMMON_SELECT);
    GUI_SetInterface("flush",NULL);
    app_channel_info_create_dialog();

}

static int app_audio_list_change_state(int sel,bool flag)
{
    GxMsgProperty_NodeByPosGet node = {0};
    uint8_t cur_audio_type = GXBUS_PM_AUDIO_MPEG1;

    if(sel>=audioLang.audioLangCnt)
    {
        return 0;
    }
    else
    {
        AudioCodecType audiotype = AUDIO_CODEC_UNKNOWN;
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        {
            GxMsgProperty_NodeModify  modify_node = {0};
            modify_node.node_type = NODE_PROG;
            memcpy(&(modify_node.prog_data),&(node.prog_data),sizeof(GxBusPmDataProg));

            audiotype = audioLang.audioLangList[sel].audiotype;
            if(AUDIO_CODEC_AC3 == audiotype)
            {
                cur_audio_type = GXBUS_PM_AUDIO_AC3;
            }
            else if(AUDIO_CODEC_EAC3 == audiotype)
            {
                cur_audio_type = GXBUS_PM_AUDIO_EAC3;
            }
            else if(AUDIO_CODEC_AAC_LATM == audiotype)
            {
                cur_audio_type = GXBUS_PM_AUDIO_AAC_LATM;
            }
            else if(AUDIO_CODEC_MPEG1 == audiotype)
            {
                cur_audio_type = GXBUS_PM_AUDIO_MPEG1;
            }
            else if(AUDIO_CODEC_MPEG2 == audiotype)
            {
                cur_audio_type = GXBUS_PM_AUDIO_MPEG2;
            }
            else if(AUDIO_CODEC_AAC_ADTS == audiotype)
            {
                cur_audio_type = GXBUS_PM_AUDIO_AAC_ADTS;
            }
            else if(AUDIO_CODEC_DRA1 == audiotype)
            {
                cur_audio_type = GXBUS_PM_AUDIO_DRA;
            }
            if(audioLang.audioLangList[sel].id != node.prog_data.cur_audio_pid)
            {
                modify_node.prog_data.cur_audio_pid = audioLang.audioLangList[sel].id;
                modify_node.prog_data.cur_audio_type = cur_audio_type;
                app_send_msg_exec(GXMSG_PM_NODE_MODIFY, (void *)(&modify_node));
                if(flag == false)
                {
                    GxPlayer_MediaAudioSwitch(PLAYER_FOR_NORMAL,audioLang.audioLangList[sel].id,audiotype,NULL);
#if CA_SUPPORT
                    {
                        app_extend_change_audio(node.prog_data.cur_audio_pid,audioLang.audioLangList[sel].id, CONTROL_NORMAL);
                        //app_descrambler_set_cw(handle, even, odd, 0);// TODO: 0 for cas2, 128 for 3des
                        // TODO:
                    }
#endif
                }

            }
        }
    }
    return 1;
}

#if NET_AUDIO_SUPPORT
#define MAX_IN_PUT_URL_LEN 256
static char *cur_path = NULL;
static char *dest_path = NULL;
static event_list* s_audio_list_update_timer = NULL;
static int s_netaudio_count = 0;

static int _net_audio_local_filedlg_exit_cb(WndStatus ret)
{
    int str_len = 0;
    uint32_t sel = 0;

    if (ret == WND_OK)
    {
        app_get_file_real_path(&dest_path);
        if (dest_path != NULL)
        {
            APP_FREE(cur_path);
            str_len = strlen(dest_path);
            cur_path = (char *)GxCore_Malloc(str_len + 1);
            if (cur_path != NULL)
            {
                memcpy(cur_path, dest_path, str_len);
                cur_path[str_len] = '\0';
            }
            app_netaudio_list_update(cur_path);
            s_netaudio_count = app_get_netaudio_tatal_num();
            GUI_GetProperty(g_CommonSelect.widget.list,"active",&sel);
            GUI_SetProperty("listview_common_select","update_all",NULL);
            GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
        }
    }
    return 0 ;
}

static int app_audio_update_timer(void *userdata)
{
    uint32_t sel = 0;
    if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
        return 0;
    if(app_netaudio_get_download_data_state() == 0)
    {
        GUI_GetProperty(g_CommonSelect.widget.list,"active",&sel);
        GUI_SetProperty(g_CommonSelect.widget.list,"update_all",NULL);
        GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
        APP_TIMER_REMOVE(s_audio_list_update_timer);
    }
    return 0;
}

static void audio_cloud_release_cb(PopKeyboard *data)
{

    if(data->in_ret == POP_VAL_CANCEL)
    {
        return;
    }

    if (data->out_name == NULL)
        return;

    app_netaudio_auto_get_data_start(data->out_name);
    GxBus_ConfigSet(NET_AUDIO_SER_KEY,data->out_name);
    APP_TIMER_ADD(s_audio_list_update_timer, app_audio_update_timer, 100, TIMER_REPEAT);
    return;
}

static int app_netaudio_create(GuiWidget *widget, void *usrdata)
{
    int sel = 0;
    if(s_audio_mutex == -1)
    {
        GxCore_MutexCreate(&s_audio_mutex);
    }

    if(app_netaudio_check_flag())
    {
        sel = app_get_netaudio_current_play_index();
        sel = sel + audioLang.audioLangCnt;
        GUI_SetProperty(g_CommonSelect.widget.list,"select",&sel);
        GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
        GUI_SetProperty(g_CommonSelect.widget.tips.text_red, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_red, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.text_green, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_green, "state", "hide");
        return EVENT_TRANSFER_STOP;
    }
    else
    {
        GUI_SetProperty(g_CommonSelect.widget.tips.text_yellow, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_yellow, "state", "hide");
        app_audio_init();
    }
	return EVENT_TRANSFER_STOP;
}

static int app_netaudio_list_get_total(GuiWidget *widget, void *usrdata)
{
    if(IF_STATE_CONNECTED == app_network_get_cur_dev_and_state(NULL))
    {
        s_netaudio_count = app_get_netaudio_tatal_num();
        return audioLang.audioLangCnt + s_netaudio_count;
    }
    return audioLang.audioLangCnt;

}

static int app_netaudio_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    // col-0: num
    item->x_offset = 0;
    item->image = NULL;
    item->string = NULL;

    //col-1: channel name
    item = item->next;
    item->image = NULL;
    GxCore_MutexLock(s_audio_mutex);
	if(item->sel>=audioLang.audioLangCnt)
	{
        item->string = app_netaudio_get_name(item->sel-audioLang.audioLangCnt);
	}
	else if(audioLang.audioLangList[item->sel].name !=NULL)
    {
        item->string = audioLang.audioLangList[item->sel].name;//get_str_duplication(audioLang.audioLangList[item->sel].name, buf);
    }
    GxCore_MutexUnlock(s_audio_mutex);
#ifdef WND_COMMONSELECT_CHOINCESHOW_SUPORT
    uint32_t active_sel = 0;
    GUI_GetProperty(g_CommonSelect.widget.list, "active", &active_sel);
    item = item->next;
    if(item->sel == active_sel)
    {
        item->image = "s_choice";
    }
    else
    {
        item->image = NULL;
    }
    item->string = NULL;
#endif
    return EVENT_TRANSFER_STOP;
}

static int app_netaudio_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    int ret = EVENT_TRANSFER_KEEPON;
    PopDlg  pop;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_COMMON_SELECT);
                    break;
                case STBK_YELLOW:
                    {
                        if((!app_netaudio_check_flag())||(IF_STATE_CONNECTED != app_network_get_cur_dev_and_state(NULL)))
                        {
                            ret = EVENT_TRANSFER_STOP;
                            break;
                        }

                        GUI_CreateDialog(WND_NETAUDIO);
                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }
                case STBK_GREEN:
                    {
                        if(app_netaudio_check_flag())
                            break;
                        if(IF_STATE_CONNECTED == app_network_get_cur_dev_and_state(NULL))
                        {
                            static PopKeyboard keyboard;

                            char *input_url = NULL;
                            input_url = GxCore_Calloc(1, MAX_IN_PUT_URL_LEN);
                            GxBus_ConfigGet(NET_AUDIO_SER_KEY, input_url, MAX_IN_PUT_URL_LEN, NET_AUDIO_SER_VALUE);
                            memset(&keyboard, 0, sizeof(PopKeyboard));
                            keyboard.in_name    = input_url;
                            keyboard.max_num = 256;
                            keyboard.out_name   = NULL;
                            keyboard.change_cb  = NULL;
                            keyboard.release_cb = audio_cloud_release_cb;
                            multi_language_keyboard_create(&keyboard);
                            GxCore_Free(input_url);
                            input_url = NULL;
                        }

                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;
                case STBK_RED:
                    {
                        if(app_netaudio_check_flag())
                            break;
                        if(check_usb_status() == false)
                        {
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_OK;
                            pop.str = STR_ID_INSERT_USB;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.pos.x = APP_POP_DLG_POS_X;
                            pop.pos.y = APP_POP_DLG_POS_Y;
                            popdlg_create(&pop);

                            ret = EVENT_TRANSFER_STOP;
                            break;
                        }
                        app_update_xml_m3u_path(cur_path, &dest_path,_net_audio_local_filedlg_exit_cb);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    app_audio_list_end_diadlg();
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_OK:
                    {
                        GxCore_MutexLock(s_audio_mutex);
                        if((audioLang.audioLangCnt + s_netaudio_count) == 0)
                        {
                            GxCore_MutexUnlock(s_audio_mutex);
                            break;
                        }
                        int ret = 0;
                        int sel = 0;
                        GUI_GetProperty(g_CommonSelect.widget.list,"select",&sel);
                        if(app_netaudio_check_flag())
                        {
                            ret = app_audio_list_change_state(sel,true);
                            GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
                            GxCore_MutexUnlock(s_audio_mutex);
                            if(ret != 0)
                            {
                                netaudio_to_audio_flag = true;
                                app_netaudio_nomal_replay();
                                GUI_SetProperty(g_CommonSelect.widget.tips.text_yellow, "state", "hide");
                                GUI_SetProperty(g_CommonSelect.widget.tips.img_yellow, "state", "hide");
                                GUI_SetProperty(g_CommonSelect.widget.tips.text_red, "state", "show");
                                GUI_SetProperty(g_CommonSelect.widget.tips.img_red, "state", "show");
                                GUI_SetProperty(g_CommonSelect.widget.tips.text_green, "state", "show");
                                GUI_SetProperty(g_CommonSelect.widget.tips.img_green, "state", "show");
                            }
                            else
                                app_netaudio_online_data_play(sel-audioLang.audioLangCnt);
                            GUI_GetProperty(g_CommonSelect.widget.group, "select", &box_sel);
                            app_audio_prog_track_set_mode(box_sel, 0);
                            return EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret = app_audio_list_change_state(sel,false);
                            GxCore_MutexUnlock(s_audio_mutex);
                            if(ret == 0)
                            {
                                if(PVR_DUMMY != g_AppPvrOps.state)
                                {
                                    memset(&pop, 0, sizeof(PopDlg));
                                    pop.type = POP_TYPE_OK;
                                    pop.str = STR_ID_STOP_PVR_FIRST;
                                    pop.mode = POP_MODE_UNBLOCK;
                                    pop.pos.x = APP_POP_DLG_POS_X;
                                    pop.pos.y = APP_POP_DLG_POS_Y;
                                    popdlg_create(&pop);
                                    return EVENT_TRANSFER_STOP;
                                }
                                GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
                                app_netaudio_online_data_play(sel-audioLang.audioLangCnt);
                                GUI_SetProperty(g_CommonSelect.widget.tips.text_yellow, "state", "show");
                                GUI_SetProperty(g_CommonSelect.widget.tips.img_yellow, "state", "show");
                                GUI_SetProperty(g_CommonSelect.widget.tips.text_red, "state", "hide");
                                GUI_SetProperty(g_CommonSelect.widget.tips.img_red, "state", "hide");
                                GUI_SetProperty(g_CommonSelect.widget.tips.text_green, "state", "hide");
                                GUI_SetProperty(g_CommonSelect.widget.tips.img_green, "state", "hide");
                            }
                            else
                            {
                                GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
                            }
                            GUI_GetProperty(g_CommonSelect.widget.group, "select", &box_sel);
                            app_audio_prog_track_set_mode(box_sel, 0);
                            return EVENT_TRANSFER_STOP;
                        }
                    }
                    break;

				case STBK_LEFT:
				case STBK_RIGHT:
					GUI_SendEvent(g_CommonSelect.widget.group, event);
					return EVENT_TRANSFER_STOP;
					break;
                default:
                    break;
            }
    }

    return ret;
}




static void _netaudio_video_delay_rebuild(void)
{
#define BUFFER_LENGTH   (16)
    int i = 0;
    char strbuf[BUFFER_LENGTH] = {0};
    char buffer[BUFFER_LENGTH] = {0};
    char *buffer_all = NULL;
    uint32_t max_num = 0;
    float seconds = 0;
    max_num = app_get_netaudio_max_tsbuff_time();
    buffer_all = GxCore_Mallocz(max_num * BUFFER_LENGTH);
    if(buffer_all == NULL)
    {
        return;
    }
    sprintf(buffer_all,"[");
    for(i = 0; i < max_num; i++)
    {
        seconds = app_get_netaudio_tsbuff_time(i);
        seconds = seconds/1000;
        if(i == 0)
        {
            sprintf(buffer,"%.1fs",seconds);
        }
        else
        {
            memset(buffer, 0, BUFFER_LENGTH);
            sprintf(buffer,",%.1fs",seconds);
        }
        strcat(buffer_all, buffer);
        memset(strbuf, 0, BUFFER_LENGTH);
    }

    strcat(buffer_all, "]");
    GUI_SetProperty("netaudio_combo_audio_channel","content",buffer_all);
    GxCore_Free(buffer_all);
    return;
}

SIGNAL_HANDLER int app_netaudio_init(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;
    _netaudio_video_delay_rebuild();
    sel = app_get_netaudio_tsbuff_index();
    GUI_SetProperty("netaudio_combo_audio_channel","select",&sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netaudio_destroy(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_netaudio_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    uint32_t old_sel = 0;
    uint32_t sel = 0;
    if(event->type == GUI_KEYDOWN)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog(WND_NETAUDIO);
                GUI_EndDialog(WND_COMMON_SELECT);
                break;

            case STBK_MENU:
            case STBK_EXIT:
                old_sel = app_get_netaudio_tsbuff_index();
                GUI_GetProperty("netaudio_combo_audio_channel","select",&sel);
                GUI_EndDialog(WND_NETAUDIO);
                GUI_EndDialog(WND_COMMON_SELECT);
                if(old_sel != sel)
                {
                    app_set_netaudio_tsbuff_time(sel);
                    app_netaudio_tsbuff_replay();
                }
                break;
            default:
                break;
        }

    }
    return EVENT_TRANSFER_STOP;
}
#endif

static void app_audio_init(void)
{
	int init_value = 0;
	int i = 0;
	GxMsgProperty_NodeByPosGet node = {0};
	uint32_t selectPid = 0;

	GxBus_ConfigGetInt(SOUND_MODE_KEY, &init_value, SOUND_MODE);

	node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;

	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
			selectPid = node.prog_data.cur_audio_pid;

	if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_MONO)
	{
		init_value = 0;
	}
	else if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_LEFT)
	{
		init_value = 1;
	}
	else if(node.prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_RIGHT)
	{
		init_value = 2;
	}
	else
	{
		init_value = 3;
	}
	GUI_SetProperty(g_CommonSelect.widget.group,"select",&init_value);
    GxCore_MutexLock(s_audio_mutex);
	for(i = 0; i<audioLang.audioLangCnt; i++)
	{
		if(audioLang.audioLangList[i].id == selectPid)
		{
            GxCore_MutexUnlock(s_audio_mutex);
			GUI_SetProperty(g_CommonSelect.widget.list,"select",&i);
			GUI_SetProperty(g_CommonSelect.widget.list, "active", &i);
			return;
		}
	}
    GxCore_MutexUnlock(s_audio_mutex);
}

static int app_audio_list_get_total(GuiWidget *widget, void *usrdata)
{
    return audioLang.audioLangCnt;
}

static int app_audio_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    // col-0: num
    item->x_offset = 0;
    item->image = NULL;
    item->string = NULL;

    //col-1: channel name
    item = item->next;
    item->image = NULL;
    GxCore_MutexLock(s_audio_mutex);
    if(audioLang.audioLangList[item->sel].name !=NULL)
    {
        item->string = audioLang.audioLangList[item->sel].name;//get_str_duplication(audioLang.audioLangList[item->sel].name, buf);
    }
    GxCore_MutexUnlock(s_audio_mutex);
#ifdef WND_COMMONSELECT_CHOINCESHOW_SUPORT
    //col-.3 select status
    uint32_t active_sel = 0;
    GUI_GetProperty(g_CommonSelect.widget.list, "active", &active_sel);
    item = item->next;
    if(item->sel == active_sel)
    {
        item->image = "s_choice";
    }
    else
    {
        item->image = NULL;
    }
    item->string = NULL;
#endif
    return EVENT_TRANSFER_STOP;
}

static int app_audio_track_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel = 0;
	GUI_GetProperty(g_CommonSelect.widget.group, "select", &box_sel);
	app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &box_sel);
#if BLUETOOTH_SUPPORT
{
    extern  int app_bluetooth_set_channel(int channel);
    app_bluetooth_set_channel(box_sel);
}
#endif
	return EVENT_TRANSFER_STOP;
}

static int app_audio_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    int ret = EVENT_TRANSFER_KEEPON;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_COMMON_SELECT);
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    app_audio_list_end_diadlg();
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_OK:
                    {
                        GxCore_MutexLock(s_audio_mutex);
                        if(audioLang.audioLangCnt == 0)
                        {
                            GxCore_MutexUnlock(s_audio_mutex);
                            break;
                        }
                        int sel = 0;
                        GUI_GetProperty(g_CommonSelect.widget.list,"select",&sel);
                        GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
                        app_audio_list_change_state(sel,false);
                        GxCore_MutexUnlock(s_audio_mutex);
                        GUI_GetProperty(g_CommonSelect.widget.group, "select", &box_sel);
                        app_audio_prog_track_set_mode(box_sel, 0);

                    }
                    return EVENT_TRANSFER_STOP;
                    break;

				case STBK_LEFT:
				case STBK_RIGHT:
					GUI_SendEvent(g_CommonSelect.widget.group, event);
					return EVENT_TRANSFER_STOP;
					break;
                default:
                    break;
            }
    }

    return ret;
}

static int app_audio_create(GuiWidget *widget, void *usrdata)
{
    if(s_audio_mutex == -1)
    {
        GxCore_MutexCreate(&s_audio_mutex);
    }
    app_audio_init();

	return EVENT_TRANSFER_STOP;
}

static int app_audio_destroy(GuiWidget *widget, void *usrdata)
{
	if(s_audio_list_refresh_timer)
	{
		remove_timer(s_audio_list_refresh_timer);
		s_audio_list_refresh_timer = NULL;
	}
	return EVENT_TRANSFER_STOP;
}

static int app_audio_keypress(GuiWidget *widget, void *usrdata)
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
                    GUI_EndDialog(WND_COMMON_SELECT);
                    if(g_AppFullArb.state.tv == STATE_OFF)
                    {
                        app_channel_info_create_dialog();
                    }

                    break;

                case STBK_UP:
                case STBK_DOWN:
                    return EVENT_TRANSFER_STOP;
                case STBK_LEFT:
                case STBK_RIGHT:
                    return EVENT_TRANSFER_STOP;


                case STBK_EXIT:
                    break;

                default:

                    break;
            }
        default:
            break;
    }

    return ret;
}

int app_audio_create_dialog(void)
{
    CommonSelectParas paras = {0};

    paras.choice = COMMON_SELECT_TITLE_GROUP;
    paras.title = STR_ID_AUDIO;
    paras.content = "[Mono,Left,Right,Stereo]";

#if NET_AUDIO_SUPPORT
    if(IF_STATE_CONNECTED == app_network_get_cur_dev_and_state(NULL))
    {
		paras.tip_strs.str_green = "Cloud";
        paras.tip_strs.str_red = "Import";
        paras.tip_strs.str_yellow = "Video Cache";
        paras.signal.create = app_netaudio_create;
        paras.signal.destroy = app_audio_destroy;
        paras.signal.keypress = app_audio_keypress;
        paras.signal.group_change = app_audio_track_change;
        paras.signal.list_get_total = app_netaudio_list_get_total;
        paras.signal.list_get_data = app_netaudio_list_get_data;
        paras.signal.list_keypress = app_netaudio_list_keypress;
        app_common_select_create_dialog(&paras);
        return 0;
    }
#endif
    paras.signal.create = app_audio_create;
    paras.signal.destroy = app_audio_destroy;
    paras.signal.keypress = app_audio_keypress;
    paras.signal.group_change = app_audio_track_change;
    paras.signal.list_get_total = app_audio_list_get_total;
    paras.signal.list_get_data = app_audio_list_get_data;
    paras.signal.list_keypress = app_audio_list_keypress;
    app_common_select_create_dialog(&paras);

    return 0;
}
