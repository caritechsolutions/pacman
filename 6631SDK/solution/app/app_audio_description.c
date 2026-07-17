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
#include "app_send_msg.h"
#include "full_screen.h"
#include "app_audio.h"
#include "app_windows.h"
#include "app_ui_style.h"
#include "app_common_select.h"
#include "app_channel_info.h"

#if AUDIO_DESCRIPTOR_SUPPORT

static AudioLang_t s_adLang = {0,0};
static uint32_t s_select_pid = 0x1fff;
static handle_t s_ad_mutex = -1;
static event_list* s_ad_list_refresh_timer = NULL;

extern uint8_t app_get_audio_count_num(void);
extern int app_pmt_get_muli_audio_type_by_elempid(int elem_pid);

void app_audio_description_free(void)
{
    if(s_ad_mutex == -1)
    {
        GxCore_MutexCreate(&s_ad_mutex);
    }

    GxCore_MutexLock(s_ad_mutex);
    app_audio_lang_free(&s_adLang);
    GxCore_MutexUnlock(s_ad_mutex);
}

static void app_set_first_audio_description(void)
{
    char lang_buff[60];
    char lang[3][4];
    char audiolang[4] = {0};
    int i = 0;
    char* p = NULL;
    char* ptr = NULL;
    uint8_t count = 0;
    uint32_t index = 0;
    GxMsgProperty_PlayerAdAudio ad_audio = {0};
    if(NULL ==  GxBus_ConfigGet(PRIORITY_LANG_KEY,lang_buff,sizeof(lang_buff),PRIORITY_LANG_VALUE))
    {
        strcpy(lang_buff,PRIORITY_LANG_VALUE);
    }
    memset(lang,0,3*4);
    p = strtok_r(lang_buff, "_",&ptr);
    while(p)
    {
        if(count == 3)
            break;
        memcpy(lang[count],p,3);
        count++;
        p = strtok_r(NULL, "_",&ptr);
    }
    for(i=0; i < s_adLang.audioLangCnt; i++)
    {
        memset(audiolang,0,4);
        memcpy(audiolang,s_adLang.audioLangList[i].name,3);
        if(0 == app_language_code_cmp(audiolang, (char *)lang[0]))
        {
            index = i;
            break;
        }
        else if(0 == app_language_code_cmp(audiolang, (char *)lang[1]))
        {
            index = i;
        }
    }
    ad_audio.player = PLAYER_FOR_NORMAL;
    ad_audio.apid = s_adLang.audioLangList[index].id;
    ad_audio.admxid = 1;
    ad_audio.atype = s_adLang.audioLangList[index].audiotype;;
    app_send_msg_exec(GXMSG_PLAYER_AD_AUDIO_ENABLE,(void*)&ad_audio);
    s_select_pid = s_adLang.audioLangList[index].id;
}

static void app_audio_description_init(void)
{
	int i = 0;
    int audio_desc = 0;
    uint32_t sel = 0;
    GxBus_ConfigGetInt(AD_CONF_KEY, &audio_desc, AD_CONFIG);
    if(audio_desc == 0)
    {
        GUI_SetProperty(g_CommonSelect.widget.list,"select",&i);
        GUI_SetProperty(g_CommonSelect.widget.list, "active", &i);
        return;
    }
    else
    {
        GxCore_MutexLock(s_ad_mutex);
        if(s_adLang.audioLangCnt == 0)
        {
            GxCore_MutexUnlock(s_ad_mutex);
            GUI_SetProperty(g_CommonSelect.widget.list,"select",&i);
            GUI_SetProperty(g_CommonSelect.widget.list, "active", &i);
            return;
        }
        for(i = 0; i<s_adLang.audioLangCnt; i++)
        {
            if(s_adLang.audioLangList[i].id == s_select_pid)
            {
                sel = i+1;
                GxCore_MutexUnlock(s_ad_mutex);
                GUI_SetProperty(g_CommonSelect.widget.list,"select",&sel);
                GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
                break;
            }
        }
        GxCore_MutexUnlock(s_ad_mutex);
    }
}



static int app_ad_list_refresh(void *userdata)
{
	GUI_SetProperty(g_CommonSelect.widget.list,"update_all",NULL);
    app_update_pop_dlg(WND_PASSWD);
	app_audio_description_init();
	s_ad_list_refresh_timer = NULL;
    return 0;
}

void app_ad_track_set(PmtInfo* audio)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t num = 0;
    uint8_t NormalCount = 0;
    uint8_t Ac3Count = 0;
    uint8_t EAc3Count = 0;
    uint8_t FindAudio = 0;
    uint8_t SpaceCount = 0;
    int audio_desc = 0;
    AudioCodecType audioType = AUDIO_CODEC_UNKNOWN;

    GxCore_MutexLock(s_ad_mutex);
    app_audio_lang_free(&s_adLang);

    if(NULL == audio ||0 == audio->stream_count)
    {
        GxCore_MutexUnlock(s_ad_mutex);
        return;
    }


    s_adLang.audioLangCnt = app_get_audio_count_num();

    if(0 == s_adLang.audioLangCnt)
    {
        GxCore_MutexUnlock(s_ad_mutex);
        return;
    }

    s_adLang.audioLangList = GxCore_Calloc(s_adLang.audioLangCnt, sizeof(AudioList_t));
    if(NULL == s_adLang.audioLangList)
    {
        s_adLang.audioLangCnt = 0;
        GxCore_MutexUnlock(s_ad_mutex);
        return;
    }

    for(i = 0; i<audio->stream_count; i++)
    {
        audioType = app_audio_type(audio->stream_info[i].elem_pid);

        if(audioType != AUDIO_CODEC_UNKNOWN)
        {
            if(audio->stream_info[i].iso639_count == 0 || audio->stream_info[i].iso639_info == NULL)
            {
                s_adLang.audioLangCnt = s_adLang.audioLangCnt - 1;
                continue;
            }
            if(audio->stream_info[i].iso639_info->audio_type != 2 && audio->stream_info[i].iso639_info->audio_type != 3 )
            {
                s_adLang.audioLangCnt = s_adLang.audioLangCnt - 1;
                continue;
            }
            if(audioType != AUDIO_CODEC_EAC3)
            {
				#define MAX_ID_LEN    6
                char buf[MAX_ID_LEN] = {0};
                s_adLang.audioLangList[j].audiotype = audioType;
                s_adLang.audioLangList[j].id = audio->stream_info[i].elem_pid;
                if((NULL != audio->stream_info[i].iso639_info)
                        &&(0 != app_audio_get_array_len(audio->stream_info[i].iso639_info->iso639,3)))
                {
                    s_adLang.audioLangList[j].name = app_audio_get_str_duplication(audio->stream_info[i].iso639_info->iso639,3);
                    if(*s_adLang.audioLangList[j].name == ' ') {
                        SpaceCount++;
                        sprintf(buf,"%d", SpaceCount);
                        buf[MAX_ID_LEN-1] = '\0';
                        s_adLang.audioLangList[j].name = app_audio_get_string_strcat("Audio",buf,NULL);
                    }
                }
                else
                {
                    if(audioType == AUDIO_CODEC_AC3)
                    {
                        Ac3Count++;
                        sprintf(buf, "%d",Ac3Count);
                        buf[MAX_ID_LEN-1] = '\0';
                        s_adLang.audioLangList[j].name = app_audio_get_string_strcat("Dolby",buf,NULL);
                    }
                    else
                    {
                        NormalCount++;
                        sprintf(buf, "%d",NormalCount);
                        buf[MAX_ID_LEN-1] = '\0';
                        s_adLang.audioLangList[j].name = app_audio_get_string_strcat("Audio",buf,NULL);
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
                        s_adLang.audioLangList[j].audiotype = AUDIO_CODEC_AC3;
                        s_adLang.audioLangList[j].id = ((Ac3Descriptor*)audio->ac3_info[num])->elem_pid;
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
                            s_adLang.audioLangList[j].audiotype = AUDIO_CODEC_EAC3;
                            s_adLang.audioLangList[j].id = ((Eac3Descriptor*)audio->eac3_info[num])->elem_pid;
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
                        s_adLang.audioLangList[j].name = app_audio_get_str_duplication(audio->stream_info[i].iso639_info->iso639,3);
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
                            s_adLang.audioLangList[j].name = app_audio_get_string_strcat("Dolby",buf,NULL);
                        }
                        else if (audioType == AUDIO_CODEC_EAC3)
                        {
                            EAc3Count++;
                            sprintf(buf, "%d",EAc3Count);
                            buf[MAX_ID_LEN-1] = '\0';
                            s_adLang.audioLangList[j].name = app_audio_get_string_strcat("Dolby+",buf,NULL);
                        }
                        else
                        {
                            NormalCount++;
                            sprintf(buf, "%d",NormalCount);
                            buf[MAX_ID_LEN-1] = '\0';
                            s_adLang.audioLangList[j].name = app_audio_get_string_strcat("Audio",buf,NULL);
                        }
                    }
                    j++;
                }
            }
        }
    }
    s_adLang.audioLangCnt = j;
    GxBus_ConfigGetInt(AD_CONF_KEY, &audio_desc, AD_CONFIG);
    if(audio_desc != 0 && s_adLang.audioLangCnt != 0)
    {
        app_set_first_audio_description();
    }

    GxCore_MutexUnlock(s_ad_mutex);
    if(GUI_CheckDialog(WND_COMMON_SELECT) == GXCORE_SUCCESS && strcmp(g_CommonSelect.title, STR_ID_AUDIO_DESCRIPTION) == 0)
    {
        if (reset_timer(s_ad_list_refresh_timer) != 0)
        {
            s_ad_list_refresh_timer = create_timer(app_ad_list_refresh, 10, NULL, TIMER_ONCE);
        }
    }
}

static int app_audio_description_list_get_total(GuiWidget *widget, void *usrdata)
{
    return s_adLang.audioLangCnt+1;
}

static int app_audio_description_list_get_data(GuiWidget *widget, void *usrdata)
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
    if(item->sel == 0)
    {
        item->string = "Off";
    }
    else
    {
        GxCore_MutexLock(s_ad_mutex);
        if(s_adLang.audioLangList[item->sel-1].name !=NULL)
        {
            item->string = s_adLang.audioLangList[item->sel-1].name;//get_str_duplication(s_adLang.audioLangList[item->sel].name, buf);
        }
        GxCore_MutexUnlock(s_ad_mutex);
    }
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

static int app_audio_description_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_KEEPON;
    int sel = 0;
    int audio_desc = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_COMMON_SELECT);
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_COMMON_SELECT);
                    GUI_SetInterface("flush",NULL);
                    app_channel_info_create_dialog();
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_OK:
                    {
                        GxCore_MutexLock(s_ad_mutex);
                        GUI_GetProperty(g_CommonSelect.widget.list,"select",&sel);
                        GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
                        GxBus_ConfigGetInt(AD_CONF_KEY, &audio_desc, AD_CONFIG);
                        GxMsgProperty_PlayerAdAudio ad_audio = {0};
                        if(sel == 0)
                        {
                            if(audio_desc != 0)
                            {
                                audio_desc = 0;
                                ad_audio.player = PLAYER_FOR_NORMAL;
                                app_send_msg_exec(GXMSG_PLAYER_AD_AUDIO_DISABLE,(void*)&ad_audio);
                                s_select_pid = 0x1fff;
                                GxBus_ConfigSetInt(AD_CONF_KEY, audio_desc);
                            }
                            else
                            {
                                GxCore_MutexUnlock(s_ad_mutex);
                                break;
                            }
                        }
                        else
                        {
                            sel = sel-1;
                            if(audio_desc == 0)
                            {
                                audio_desc = 1;
                                GxBus_ConfigSetInt(AD_CONF_KEY, audio_desc);
                            }
                            if(s_adLang.audioLangList[sel].id != s_select_pid)
                            {
                                ad_audio.player = PLAYER_FOR_NORMAL;
                                ad_audio.apid = s_adLang.audioLangList[sel].id;
                                ad_audio.admxid = 1;
                                ad_audio.atype = s_adLang.audioLangList[sel].audiotype;;
                                app_send_msg_exec(GXMSG_PLAYER_AD_AUDIO_ENABLE,(void*)&ad_audio);
                                s_select_pid = s_adLang.audioLangList[sel].id;
                            }
                        }
                        GxCore_MutexUnlock(s_ad_mutex);
                    }
					return EVENT_TRANSFER_STOP;
                    break;
                default:
                    break;
            }
    }

    return ret;
}

static int app_audio_description_create(GuiWidget *widget, void *usrdata)
{
    if(s_ad_mutex == -1)
    {
        GxCore_MutexCreate(&s_ad_mutex);
    }
	app_audio_description_init();

	return EVENT_TRANSFER_STOP;
}

static int app_audio_description_destroy(GuiWidget *widget, void *usrdata)
{
	if(s_ad_list_refresh_timer)
	{
		remove_timer(s_ad_list_refresh_timer);
		s_ad_list_refresh_timer = NULL;
	}
	return EVENT_TRANSFER_STOP;
}

static int app_audio_description_keypress(GuiWidget *widget, void *usrdata)
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

int app_audio_description_create_dialog(void)
{
    CommonSelectParas paras = {0};
    paras.choice = COMMON_SELECT_TITLE_ONLY;
    paras.title = STR_ID_AUDIO_DESCRIPTION;
    paras.signal.create = app_audio_description_create;
    paras.signal.destroy = app_audio_description_destroy;
    paras.signal.keypress = app_audio_description_keypress;
    paras.signal.list_get_total = app_audio_description_list_get_total;
    paras.signal.list_get_data = app_audio_description_list_get_data;
    paras.signal.list_keypress = app_audio_description_list_keypress;
    app_common_select_create_dialog(&paras);

    return 0;
}
#endif
