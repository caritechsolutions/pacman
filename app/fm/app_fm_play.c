/*************************************************************************
	> File Name: app/app_fm_play.c
	> Author:
	> Mail:
	> Created Time: 2021年11月18日 星期四 17时28分32秒
 ************************************************************************/
#include "app.h"
#if FM_SUPPORT
#include "app_windows.h"
#include "app_keyboard_language.h"
#include "dlna.h"
#include "app_module.h"
#include "full_screen.h"
#include "app_common_select.h"
#include "app_fm.h"
#include "app_volume_value.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#define IMG_FM_PLAY_PAUSE           "img_fm_play_pause"
#define IMG_FM_PLAY_POPUP           "img_fm_play_popup"
#define TXT_FM_PLAY_POPUP           "txt_fm_play_popup"
#define IMG_POP_PLAY                "MP_BUTTON_PLAY"
#define IMG_POP_PAUSE               "s_pause"

#define TXT_FM_INFO_NAME            "text_fm_play_info_name"
#define TXT_FM_INFO_NAME_NUM        "text_fm_play_info_name_num"
#define TXT_FM_INFO_TIME            "text_fm_play_info_time"
#define TXT_FM_INFO_DATE            "text_fm_play_info_date"
#define PROGBAR_FM_INFO_STRENGTH    "progbar_fm_play_info_strength"
#define TXT_FM_INFO_STRENGTH        "text_fm_play_info_strength_value"
#define IMG_FM_INFO_MODE            "img_fm_play_info_mode"
#define TXT_FM_INFO_MODE            "text_fm_play_info_mode"
#define IMG_FM_INFO_STEP            "img_fm_play_info_step"
#define TXT_FM_INFO_STEP            "text_fm_play_info_step"
#define IMG_FM_INFO_SAVE            "img_fm_play_info_save"
#define TXT_FM_INFO_SAVE            "text_fm_play_info_save"
#define FM_MIN_FREQ     64000
#define FM_MAX_FREQ     108000
static Fm_play_state s_fm_play_state = STATE_STOP;
static event_list* sp_FmPlayPauseTimer = NULL;
static event_list* sp_FmPlayInfoTimer = NULL;
static uint32_t* sp_fm_freq_array = NULL;
static Fm_mode s_fm_play_mode = STORA_MODE;
static int s_fm_cur_freq = 0;
static int s_fm_step_value = 1;
static int s_fm_play_index = 0;
extern void app_tv_radio_key_exec(void);
extern int app_create_menu_wnd(void);

Fm_play_state app_fm_get_play_state(void)
{
    return s_fm_play_state;
}

void app_fm_play_prog(uint32_t freq)
{
    int32_t tuner = 0;
    char *url = NULL;

    if(s_fm_play_state == STATE_PAUSE)
        GUI_SetProperty(IMG_FM_PLAY_PAUSE, "state", "hide");
    s_fm_play_state = STATE_PLAY;
    url = GxCore_Mallocz(PLAYER_URL_LONG);
    if(url == NULL)
    {
         app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
         return;
    }

    tuner = app_get_fm_tuner_id();
    if(tuner >= 0)
    {
        sprintf(url, "fm://fre:%d&tuner:%d", freq, tuner);
        GxPlayer_MediaPlay(PLAYER_FOR_FM, url, 0, 0, NULL);
    }
    GXCORE_FREE(url);
}

void app_fm_play_stop(void)
{
    s_fm_play_state = STATE_STOP;
    GxPlayer_MediaStop(PLAYER_FOR_FM);
}

static void app_fm_play_info_show_signal(void)
{
#define PROGBAR_STRENGTH_L  "s_progress3_l"
#define PROGBAR_STRENGTH_M  "s_progress3_m"
#define PROGBAR_STRENGTH_R  "s_progress3_r"
#define PROGBAR_UNLOCKED_L  "s_progress3_l_red"
#define PROGBAR_UNLOCKED_M  "s_progress3_m_red"
#define PROGBAR_UNLOCKED_R  "s_progress3_r_red"

    unsigned int value = 0;
    char Buffer[20] = {0};
    GxFeSignalStrength strength = {0};
    int num = 0;
    int handle = -1;

    num = app_fm_freq_num_get();
    if((app_fm_play_mode_get() == STORA_MODE) && (num == 0))
        value = 0;
    else
    {
        handle = app_fm_get_handle();
        if(handle < 0)
            value = 0;
        else
        {
            GxFeGetStrength(handle, &strength);
            value = strength.percent;
        }
    }

    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%", value);

    GUI_SetProperty(PROGBAR_FM_INFO_STRENGTH, "fore_image_l", PROGBAR_STRENGTH_L);
    GUI_SetProperty(PROGBAR_FM_INFO_STRENGTH, "fore_image_m", PROGBAR_STRENGTH_M);
    GUI_SetProperty(PROGBAR_FM_INFO_STRENGTH, "fore_image_r", PROGBAR_STRENGTH_R);
    GUI_SetProperty(TXT_FM_INFO_STRENGTH, "string", Buffer);
    GUI_SetProperty(PROGBAR_FM_INFO_STRENGTH, "value", &value);

    return;
}

static int app_fm_play_info_mode_update(void)
{
    char buff[20]={0};
    if(app_fm_play_mode_get() == STORA_MODE)
    {
        GUI_SetProperty(IMG_FM_INFO_MODE, "state", "show");
        GUI_SetProperty(IMG_FM_INFO_STEP, "state", "hide");
        GUI_SetProperty(IMG_FM_INFO_SAVE, "state", "hide");
        GUI_SetProperty(TXT_FM_INFO_MODE, "state", "show");
        GUI_SetProperty(TXT_FM_INFO_STEP, "state", "hide");
        GUI_SetProperty(TXT_FM_INFO_SAVE, "state", "hide");
        GUI_SetProperty(TXT_FM_INFO_MODE, "string", STR_ID_FM_STOR_MODE);
    }
    else
    {
        GUI_SetProperty(IMG_FM_INFO_MODE, "state", "show");
        GUI_SetProperty(IMG_FM_INFO_STEP, "state", "show");
        GUI_SetProperty(IMG_FM_INFO_SAVE, "state", "show");
        GUI_SetProperty(TXT_FM_INFO_MODE, "state", "show");
        GUI_SetProperty(TXT_FM_INFO_STEP, "state", "show");
        GUI_SetProperty(TXT_FM_INFO_SAVE, "state", "show");
        GUI_SetProperty(TXT_FM_INFO_MODE, "string", STR_ID_FM_FREQ_MODE);
        GUI_SetProperty(TXT_FM_INFO_SAVE, "string", "Save");
        sprintf(buff,"Step: %.1fMHz",s_fm_step_value/10.0);
        GUI_SetProperty(TXT_FM_INFO_STEP, "string", buff);
    }

    return 0;
}

static void app_fm_play_info_tip(char* str)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = str;
    pop.timeout_sec = 2;
    popdlg_create(&pop);
}

int app_fm_play_get_index()
{
    return s_fm_play_index;
}

int app_fm_play_set_index(int index)
{
    if(index >=0 && index< app_fm_freq_num_get())
    {
        s_fm_play_index = index;
        GxBus_ConfigSetInt(FM_PLAY_INDEX, s_fm_play_index);
    }
    return 0;
}

Fm_mode app_fm_play_mode_get(void)
{
    return s_fm_play_mode;
}

void app_fm_play_mode_set(Fm_mode mode)
{
    s_fm_play_mode = mode;
}

static int app_fm_play_info_show_name(void)
{
    int num = 0;
    int index = 0;
    char buff[20]= {0};

    num = app_fm_freq_num_get();
    if(app_fm_play_mode_get() == STORA_MODE)
    {
        if(num > 0)
        {
            index = app_fm_play_get_index();
            sprintf(buff,"%03d", index + 1);
            GUI_SetProperty(TXT_FM_INFO_NAME_NUM, "string", buff);
            memset(buff, 0, sizeof(buff));
#if FM_FREQ_NAME_SUPPORT
            sprintf(buff, "%.1f MHz %s", sp_fm_freq_array[index]/1000.0,app_fm_get_freq_name(sp_fm_freq_array[index]));
#else
            sprintf(buff, "%.1f MHz", sp_fm_freq_array[index]/1000.0);
#endif
            GUI_SetProperty(TXT_FM_INFO_NAME, "string", buff);
        }
        else
        {
            GUI_SetProperty(TXT_FM_INFO_NAME_NUM, "string", " ");
            GUI_SetProperty(TXT_FM_INFO_NAME, "string", " ");
        }
    }
    else
    {
        GUI_SetProperty(TXT_FM_INFO_NAME_NUM, "string", " ");
#if FM_FREQ_NAME_SUPPORT
        sprintf(buff,"%.1f MHz %s",s_fm_cur_freq/1000.0,app_fm_get_freq_name(s_fm_cur_freq));
#else
        sprintf(buff,"%.1f MHz",s_fm_cur_freq/1000.0);
#endif
        GUI_SetProperty(TXT_FM_INFO_NAME, "string", buff);
    }
    return 0;
}

static int app_fm_play_info_update(void)
{
    char time_str[24] = {0};

    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str));
    app_set_widget_string(TXT_FM_INFO_TIME, time_str+MIN_SEC_OFFSET);
    memset(time_str,0,sizeof(time_str));
    g_AppTime.time_get(&g_AppTime, time_str, sizeof(time_str));
    time_str[MIN_SEC_OFFSET] = '\0';
    app_set_widget_string(TXT_FM_INFO_DATE, time_str);

    app_fm_play_info_show_signal();

    return 0;
}

static int timer_fm_play_info_timeout(void *usrdata)
{
    app_fm_play_info_update();

    return 0;
}

static void app_fm_play_step_set(void)
{
    char* buffer1;
    char buffer2[20]={0};
    int step_value = 1;

    GUI_GetProperty("edit_fm_play_info_step_set", "string", &buffer1);
    step_value = app_float_edit_str_to_value(buffer1)*10;
    if(step_value>10 || step_value<1)
    {
        app_fm_play_info_tip(STR_ID_FM_FREQ_OUT);
    }
    else
    {
        sprintf(buffer2,"Step: %.1f MHz",step_value/10.0);
        GUI_SetProperty(TXT_FM_INFO_STEP, "string", buffer2);
        GxBus_ConfigSetInt(FM_STEP,step_value);
        GUI_EndDialog(WND_FM_PLAY_INFO_STEP_SET);
        s_fm_step_value=step_value;
    }
}


SIGNAL_HANDLER int app_fm_play_info_step_set_create(GuiWidget *widget, void *usrdata)
{
    char str[20]={0};

    sprintf(str,"%0.1f",s_fm_step_value/10.0);
    GUI_SetProperty("edit_fm_play_info_step_set","string",str);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_play_info_step_set_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_play_info_step_set_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

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
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_FM_PLAY_INFO_STEP_SET);
                    break;
                case STBK_OK:
                    app_fm_play_step_set();
                    break;
                default:
                    break;
            }
    }

    return EVENT_TRANSFER_STOP;
}

void app_fm_number_response(int count)
{
    int num = 0;


    num = app_fm_freq_num_get();
    if(app_fm_play_mode_get() == STORA_MODE)
    {
        if((num > 0) && (count > 0) && (count <= app_fm_freq_num_get()))
        {
#if DVB2IP_SERVER_SUPPORT
            if(app_dvb2ip_fm_stop_rec_tip_by_freq(sp_fm_freq_array[count-1])<0)
                return;
#endif
            s_fm_cur_freq = sp_fm_freq_array[count-1];
            app_fm_play_set_index(count - 1);
            app_fm_play_prog(s_fm_cur_freq);
        }
        else
            app_fm_play_info_tip(STR_ID_FM_NO_FREQ);
    }
    else
    {
        if((count*100 > FM_MAX_FREQ)||(count*100 < FM_MIN_FREQ))
            app_fm_play_info_tip(STR_ID_FM_FREQ_OUT);
        else
        {
#if DVB2IP_SERVER_SUPPORT
            if(app_dvb2ip_fm_stop_rec_tip_by_freq(count*100)<0)
                return;
#endif
            s_fm_cur_freq = count*100;
            app_fm_play_prog(s_fm_cur_freq);
        }
    }
    app_fm_play_info_show_name();
    app_fm_play_info_update();
}

static void app_fm_mode_change()
{
    uint32_t i,num;
    int index = 0;
    int temp_freq;

    num = app_fm_freq_num_get();
    index = app_fm_play_get_index();
    if(app_fm_play_mode_get() == STORA_MODE)
    {
        if(num>0)
            temp_freq = sp_fm_freq_array[index];
        else
            temp_freq = FM_MIN_FREQ;
#if DVB2IP_SERVER_SUPPORT
        if(app_dvb2ip_fm_stop_rec_tip_by_freq(temp_freq)<0)
            return;
#endif
        s_fm_cur_freq=temp_freq;
        app_fm_play_mode_set(FREQ_MODE);
        app_fm_play_prog(s_fm_cur_freq);
    }
    else
    {
        app_fm_play_mode_set(STORA_MODE);
        if(num <= 0)
        {
            index = 0;
            s_fm_cur_freq = FM_MIN_FREQ;
            app_fm_play_stop();
            app_fm_play_info_tip(STR_ID_FM_NO_FREQ);
        }
        else
        {
            for(i=0;i<num;i++)
            {
                if(sp_fm_freq_array[i] == s_fm_cur_freq)
                {
                    index = i;
                    break;
                }
            }
            if(i>=num)
                index = 0;
#if DVB2IP_SERVER_SUPPORT
            if(app_dvb2ip_fm_stop_rec_tip_by_freq(sp_fm_freq_array[index])<0)
                return;
#endif
            s_fm_cur_freq = sp_fm_freq_array[index];
            app_fm_play_prog(s_fm_cur_freq);
        }
    }
    app_fm_play_set_index(index);
    app_fm_play_info_mode_update();
    app_fm_play_info_show_name();
}

static void app_fm_save_cur_freq(uint32_t freq)
{
    int num;
    if(app_fm_freq_check_exist(freq))
    {
        app_fm_play_info_tip(STR_ID_FM_EXIST);
    }
    else
    {
        app_fm_freq_add_one(s_fm_cur_freq);
        num = app_fm_freq_num_get();
        sp_fm_freq_array = (uint32_t*)GxCore_Realloc(sp_fm_freq_array, num*sizeof(uint32_t));
        if(sp_fm_freq_array)
            app_fm_freq_array_get(sp_fm_freq_array);
#if DVB2IP_SERVER_SUPPORT
        app_dvb2ip_prog_num_update();
#endif
    }
}

static int _fm_play_pause_update(void* usrdata)
{
    APP_TIMER_REMOVE(sp_FmPlayPauseTimer);
    GUI_SetProperty(IMG_FM_PLAY_PAUSE, "state", "hide");

    return 0;
}

SIGNAL_HANDLER int app_fm_play_info_create(GuiWidget *widget, void *usrdata)
{
    int num = 0;
    int value = 0;

    app_fm_play_mode_set(STORA_MODE);
    GxBus_ConfigGetInt(FM_STEP, &value, FM_STEP_VAL);
    s_fm_step_value = value;

    num = app_fm_freq_num_get();
    if(num > 0)
    {
        sp_fm_freq_array = (uint32_t*)GxCore_Malloc(num * sizeof(uint32_t));
        if(sp_fm_freq_array)
        {
            memset(sp_fm_freq_array, 0, num * sizeof(uint32_t));
            app_fm_freq_array_get(sp_fm_freq_array);
            GxBus_ConfigGetInt(FM_PLAY_INDEX, &value, FM_PLAY_INDEX_VAL);
#if DVB2IP_SERVER_SUPPORT
            if(app_dvb2ip_fm_stop_rec_tip_by_freq(sp_fm_freq_array[value])<0)
                return EVENT_TRANSFER_STOP;
#endif
            s_fm_cur_freq = sp_fm_freq_array[value];
            app_fm_play_set_index(value);
            if(s_fm_play_state != STATE_PLAY)
                app_fm_play_prog(s_fm_cur_freq);
        }
    }
    else
    {
        s_fm_play_state = STATE_STOP;
        GUI_SetProperty(IMG_FM_PLAY_PAUSE, "state", "hide");
        app_fm_play_info_tip(STR_ID_FM_NO_FREQ);
    }

    app_fm_play_info_mode_update();
    app_fm_play_info_show_name();
    app_fm_play_info_update();

    APP_TIMER_ADD(sp_FmPlayInfoTimer, timer_fm_play_info_timeout, 1000, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_fm_play_info_destroy(GuiWidget *widget, void *usrdata)
{
    GXCORE_FREE(sp_fm_freq_array);
    APP_TIMER_REMOVE(sp_FmPlayInfoTimer);
    APP_TIMER_REMOVE(sp_FmPlayPauseTimer);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_play_info_got_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_play_info_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

#if DVB2IP_SERVER_SUPPORT
static int _dvb2ip_fm_exit_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        app_dvb2ip_use_flag_set(0);
        GUI_EndDialog(WND_FM_PLAY_INFO);
        GUI_EndDialog(WND_FM_PLAY);
        if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
            app_create_menu_wnd();
    }
    return 0;
}

static int _dvb2ip_fm_tv_radio_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        app_dvb2ip_use_flag_set(0);
        app_tv_radio_key_exec();
    }
    return 0;
}
#endif

SIGNAL_HANDLER int app_fm_play_info_keypress(GuiWidget *widget, void *usrdata)
{
    int num = 0;
    int index = 0;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                {
#if DVB2IP_SERVER_SUPPORT
                    if(app_dvb2ip_fm_get_client_connect())
                    {
                        PopDlg pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_YES_NO;
                        pop.format = POP_FORMAT_DLG;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.str = STR_ID_STOP_DVB2IP_FM;
                        pop.exit_cb = _dvb2ip_fm_exit_cb;
                        popdlg_create(&pop);
                        return EVENT_TRANSFER_STOP;
                    }
#endif
                    GUI_EndDialog(WND_FM_PLAY_INFO);
                    GUI_EndDialog(WND_FM_PLAY);
                    if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
                        app_create_menu_wnd();
                    return EVENT_TRANSFER_STOP;
                }
            case STBK_TV_RADIO:
                {
#if DVB2IP_SERVER_SUPPORT
                    if(app_dvb2ip_fm_get_client_connect())
                    {
                        PopDlg pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_YES_NO;
                        pop.format = POP_FORMAT_DLG;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.str = STR_ID_STOP_DVB2IP_FM;
                        pop.exit_cb = _dvb2ip_fm_tv_radio_cb;
                        popdlg_create(&pop);
                        return EVENT_TRANSFER_STOP;
                    }
#endif
                    app_tv_radio_key_exec();
                    return EVENT_TRANSFER_STOP;
                }
            case STBK_OK:
                if(app_fm_play_mode_get() == STORA_MODE)
                {
                    num = app_fm_freq_num_get();
                    if(num > 0)
                    {
                        GUI_EndDialog(WND_FM_PLAY_INFO);
                        app_create_fm_freq_list();
                    }
                    else
                    {
                        app_fm_play_info_tip(STR_ID_FM_NO_FREQ);
                    }
                }
                else
                    app_fm_play_info_tip(STR_ID_FM_CHANGE_MODE);
                break;
            case STBK_MUTE:
                {
                    g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
                    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
                }
                break;
            case STBK_VOLDOWN:
            case STBK_VOLUP:
            case STBK_LEFT:
            case STBK_RIGHT:
                {
                    if( g_AppFullArb.state.mute == STATE_ON)
                    {
                        g_AppFullArb.exec[EVENT_MUTE](&g_AppFullArb);
                        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
                    }
                    GUI_EndDialog(WND_NUMBER);
                    GUI_CreateDialog(WND_VOLUME);
                    GUI_SendEvent(WND_VOLUME, event);
                }
                break;
            case STBK_PAUSE:
                if(s_fm_play_state == STATE_PLAY)
                {
                    s_fm_play_state = STATE_PAUSE;
                    GUI_SetProperty(IMG_FM_PLAY_PAUSE, "state", "show");
                    GUI_SetProperty(IMG_FM_PLAY_PAUSE, "img", IMG_POP_PAUSE);
                    APP_TIMER_REMOVE(sp_FmPlayPauseTimer);
                    GxPlayer_MediaPause(PLAYER_FOR_FM);
                }
                else if(s_fm_play_state == STATE_PAUSE)
                {
                    s_fm_play_state = STATE_PLAY;
                    GUI_SetProperty(IMG_FM_PLAY_PAUSE, "state", "show");
                    GUI_SetProperty(IMG_FM_PLAY_PAUSE, "img", IMG_POP_PLAY);
                    APP_TIMER_ADD(sp_FmPlayPauseTimer,_fm_play_pause_update, 3000, TIMER_REPEAT);
                    GxPlayer_MediaResume(PLAYER_FOR_FM);
                }
                break;
            case STBK_PLAY:
                if(s_fm_play_state == STATE_PAUSE)
                {
                    s_fm_play_state = STATE_PLAY;
                    GUI_SetProperty(IMG_FM_PLAY_PAUSE, "state", "show");
                    GUI_SetProperty(IMG_FM_PLAY_PAUSE, "img", IMG_POP_PLAY);
                    APP_TIMER_ADD(sp_FmPlayPauseTimer,_fm_play_pause_update, 3000, TIMER_REPEAT);
                    GxPlayer_MediaResume(PLAYER_FOR_FM);
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
                GUI_CreateDialog(WND_NUMBER);
                GUI_SendEvent(WND_NUMBER, event);
                break;
            case STBK_UP:
                num = app_fm_freq_num_get();
                if(app_fm_play_mode_get() == STORA_MODE)
                {
                    if(num > 1)
                    {
                        index = app_fm_play_get_index();
                        if(++index >= num)
                            index = 0;
                #if DVB2IP_SERVER_SUPPORT
                        if(app_dvb2ip_fm_stop_rec_tip_by_freq(sp_fm_freq_array[index])<0)
                            return EVENT_TRANSFER_STOP;
                #endif
                        s_fm_cur_freq = sp_fm_freq_array[index];
                        app_fm_play_set_index(index);
                        app_fm_play_prog(s_fm_cur_freq);
                        app_fm_play_info_show_name();
                    }
                    else if(num == 0)
                    {
                        app_fm_play_info_tip(STR_ID_FM_NO_FREQ);
                    }
                }
                else
                {
                    uint32_t temp;
                    temp = (s_fm_cur_freq + s_fm_step_value*100)>FM_MAX_FREQ?FM_MAX_FREQ:(s_fm_cur_freq+s_fm_step_value*100);
                #if DVB2IP_SERVER_SUPPORT
                        if(app_dvb2ip_fm_stop_rec_tip_by_freq(temp)<0)
                            return EVENT_TRANSFER_STOP;
                #endif
                    s_fm_cur_freq=temp;
                    app_fm_play_prog(s_fm_cur_freq);
                    app_fm_play_info_show_name();
                }
                break;
            case STBK_DOWN:
                num = app_fm_freq_num_get();
                if(app_fm_play_mode_get() == STORA_MODE)
                {
                    if(num > 1)
                    {
                        index = app_fm_play_get_index();
                        if(--index < 0)
                            index = num-1;
#if DVB2IP_SERVER_SUPPORT
                        if(app_dvb2ip_fm_stop_rec_tip_by_freq(sp_fm_freq_array[index])<0)
                            return EVENT_TRANSFER_STOP;
#endif
                        s_fm_cur_freq = sp_fm_freq_array[index];
                        app_fm_play_set_index(index);
                        app_fm_play_prog(s_fm_cur_freq);
                        app_fm_play_info_show_name();
                    }
                    else if(num == 0)
                    {
                        app_fm_play_info_tip(STR_ID_FM_NO_FREQ);
                    }
                }
                else
                {
                    uint32_t temp;
                    temp = (s_fm_cur_freq - s_fm_step_value*100)<FM_MIN_FREQ?FM_MIN_FREQ:(s_fm_cur_freq-s_fm_step_value*100);
#if DVB2IP_SERVER_SUPPORT
                        if(app_dvb2ip_fm_stop_rec_tip_by_freq(temp)<0)
                            return EVENT_TRANSFER_STOP;
#endif
                    s_fm_cur_freq = temp;
                    app_fm_play_prog(s_fm_cur_freq);
                    app_fm_play_info_show_name();
                }
                break;
            case STBK_RED:
#if DVB2IP_SERVER_SUPPORT
                if(app_dvb2ip_fm_stop_rec_tip_by_freq(0)<0)
                    return EVENT_TRANSFER_STOP;
#endif
                if(s_fm_play_state == STATE_PAUSE)
                {
                    s_fm_play_state = STATE_PLAY;
                    GUI_SetProperty(IMG_FM_PLAY_PAUSE, "state", "hide");
                }
                app_fm_mode_change();
                break;
            case STBK_GREEN:
                if(app_fm_play_mode_get() == FREQ_MODE)
                    GUI_CreateDialog(WND_FM_PLAY_INFO_STEP_SET);
                break;
            case STBK_YELLOW:
                if(app_fm_play_mode_get() == FREQ_MODE)
                {
                    app_fm_play_info_tip(STR_ID_SAVING);
                    app_fm_save_cur_freq(s_fm_cur_freq);
                }
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_fm_play_create(GuiWidget *widget, void *usrdata)
{
    int fe_handle = -1;
    int tuner_id = 0;
    struct fe_buffer_parameters params= {0};

    tuner_id = app_get_fm_tuner_id();
    fe_handle = GxFrontend_IdToHandle(tuner_id);
    if ((fe_handle < 0))
        return EVENT_TRANSFER_STOP;

    params.buffer_threshold = 16*1024;
    params.sw_buffer_size = 1024*1024;
    params.hw_buffer_size = 256*1024;
    if(ioctl(fe_handle, FE_CONFIG_BUFFER, &params) < 0)
        return EVENT_TRANSFER_STOP;


    app_fm_data_init();

    /*set volume*/
    int32_t audio_vol = 0;
    GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
    app_system_vol_set(audio_vol);
    s_fm_play_state = STATE_STOP;
    GUI_CreateDialog(WND_FM_PLAY_INFO);
    g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_play_destroy(GuiWidget *widget, void *usrdata)
{
    app_fm_play_stop();
#if FM_FREQ_NAME_SUPPORT
    fm_freq_name_list_destroy();
#endif

    if(padmux_check(34, 1) != 0)
        padmux_set(34, 1);
#if DVB2IP_SERVER_SUPPORT
    if(app_dvb2ip_fm_get_client_connect())
    {
        app_dvb2ip_use_flag_set(0);
    }
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_play_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
#if DVB2IP_SERVER_SUPPORT
                if(app_dvb2ip_fm_stop_rec_tip_by_freq(0)<0)
                    return EVENT_TRANSFER_STOP;
#endif
                    GUI_EndDialog(WND_FM_PLAY);
                break;
        }
    }
    return EVENT_TRANSFER_STOP;

}

void app_fm_play_exec(void)
{
    if(padmux_check(34, 4) != 0)
        padmux_set(34, 4);

    GUI_CreateDialog(WND_FM_PLAY);
}

#endif
