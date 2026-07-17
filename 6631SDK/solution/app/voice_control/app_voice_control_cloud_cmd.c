#include "app_config.h"

#if VOICE_CONTROL_SUPPORT
#include "include/vc_cmd.h"
#include "app_key.h"
#include "app_windows.h"
#include "voice_control/include/app_voice_control_api.h"
#if NETAPPS_SUPPORT
#include "app_netapps_common.h"
#endif

typedef struct _APPCMDFunctionClss{
    VoiceCMDIDEnum   cmd_id;
    union {
        int (*cmd_func)(void* params);
        unsigned int remote_key;
    }p;
}APPCMDFunctionClss;

// strings
#define VC_VOLUME_MAX "Max volume"//"The volume is already maximum and cannot be turned up again!!"
#define VC_VOLUME_MIN "Min volume"//"The volume is already minimum and cannot be turned down again!!"
#define VC_MUTE   "Mute"//"It's already on mute."
#define VC_UNMUTE "Unmute"//"It's already off mute."
#define VC_NOT_SUPPORT "The current scenario does not support"//"Not support"//"Sorry, but i can't do it in the current situation!"
#define VC_NOT_SUPPORT_ERR "Not support"//"Sorry, but i can't do it in the current situation!"
#define VC_NOT_SUPPORT_EX "Not support ex"//"Sorry, I don't understand!"
#define VC_NOT_SUPPORT_CMD "Sorry, I don't understand!"
#define VC_REPEAT "Repeat"//"It's already like this. No need to do anything else!"
#define VC_ERROR  "Wrong"//"Something is wrong, please try it again!"
// audio volume
#define VC_A_CUR_STR "Current volume is"
#define VC_A_UP_STR "Increase volume by"
#define VC_A_DOWN_STR "Decrease volume by"
#define VC_A_TO_STR "Change volume to"
#define VC_A_MUTE_STR "Mute the voice!"
#define VC_A_UNMUTE_STR "Open the voice!"
// play control
#define VC_P_PLAY_STR "Start playing!"
#define VC_P_PAUSE_STR "Pause playback!"
#define VC_P_STOP_STR "Stop playing!"
#define VC_P_PREVIOUS_STR "Switch to the previous!"
#define VC_P_NEXT_STR "Switch to the next!"
#define VC_P_RECALL_STR "Switch to the last channel watched!"
#define VC_P_CHANNEL_STR "Switch to channel"
#define VC_P_NAME_STR "Switch to"
// basic control
#define VC_B_BASIC_EXIT  "Response to the exit key!"
#define VC_B_BASIC_UP    "Respond to the up key!"
#define VC_B_BASIC_DOWN  "Response to the down key!"
#define VC_B_BASIC_LEFT  "Response to the left key!"
#define VC_B_BASIC_RIGHT "Response to the right key!"
#define VC_B_BASIC_CONFIRM "Response to the confirm key!"
#define VC_B_BASIC_CANCEL "Response to abort operation!"
// GOTO MENU
#define VC_M_EPG_STR "Welcome to program guide!"
#define VC_M_FULLSCREEN_STR "Welcome to full screen!"
#define VC_M_MAINMENU_STR "Welcome to main menu!"
#define VC_M_IPTV_STR "Welcome to iptv menu!"
#define VC_M_XTREAM_STR "Welcome to xtream menu!"
#define VC_M_STALKER_STR "Welcome to stalker menu!"
// PVR
#define VC_R_START_REC_STR "Start recording the current program!"
#define VC_R_STOP_REC_STR "End current recording!"

static unsigned int thiz_open_applets_status = 0;
unsigned int app_get_applets_status(void) {
    return thiz_open_applets_status;
}

void app_set_applets_status(unsigned int status) {
    thiz_open_applets_status = status;
}
// volume set
// volume up
static int _stb_volume_up(void* params)
{
    int ret = 0;
    unsigned int volume = 0;
    unsigned int max_volume = 0;
    int volume_params = 2;
    char buf[128] = {0};

    if(params)
    {
        if(0 < (((VoiceCMDParamsClass*)params)->value))
        {
            volume_params = ((VoiceCMDParamsClass*)params)->value;
        }
    }
    ret = app_vc_volume_quick_set_ex(volume_params, 0, 1, &volume, &max_volume);
    if(ret == 1)
    {
    // max volume for display
       app_voice_control_notify_ex((char*)VC_VOLUME_MAX);
    }
    else
    {
        if(volume_params > 0x10000000)
            sprintf(buf, "%s %d%%! %s (%d/%d).", app_vc_transform_strings(VC_A_UP_STR), (volume_params - 0x10000000),
                    app_vc_transform_strings(VC_A_CUR_STR),volume, max_volume);
        else
            sprintf(buf, "%s %d! %s (%d/%d).", app_vc_transform_strings(VC_A_UP_STR), volume_params, app_vc_transform_strings(VC_A_CUR_STR), volume, max_volume);
        app_voice_control_notify_ex(buf);
    }
    return ret;
}
// volume down
static int _stb_volume_down(void* params)
{
    int ret = 0;
    unsigned int volume = 0;
    unsigned int max_volume = 0;
    int volume_params = -2;
    char buf[128] = {0};

    if(params)
    {
        if(0 < (((VoiceCMDParamsClass*)params)->value))
        {
            volume_params = -(((VoiceCMDParamsClass*)params)->value);
        }
    }

    ret = app_vc_volume_quick_set_ex(volume_params, 0, 1, &volume, &max_volume);
    if(ret == 2)
    {
    // min volume for display
       app_voice_control_notify_ex((char*)VC_VOLUME_MIN);
    }
    else
    {
        if(volume_params < (int)0xF0000000)
            sprintf(buf, "%s %d%%! %s (%d/%d).",app_vc_transform_strings(VC_A_DOWN_STR), ((0 - volume_params) - 0x10000000),app_vc_transform_strings(VC_A_CUR_STR), volume, max_volume);
        else
            sprintf(buf, "%s %d! %s (%d/%d).", app_vc_transform_strings(VC_A_DOWN_STR),(-volume_params), app_vc_transform_strings(VC_A_CUR_STR),volume, max_volume);
        app_voice_control_notify_ex(buf);
    }
    return ret;
}

static int _stb_volume(void* params)
{
    int ret = 0;
    unsigned int volume = 0;
    unsigned int max_volume = 0;
    int volume_params = 0;
    char buf[128] = {0};

    if(params)
    {
        if(0 < (((VoiceCMDParamsClass*)params)->value))
        {
            volume_params = ((VoiceCMDParamsClass*)params)->value;
        }
    }
    else
    {
        return -1;
    }

    ret = app_vc_volume_quick_set_ex(volume_params, 1, 1, &volume, &max_volume);
    if(ret == 1)
    {
    // max volume for display
       app_voice_control_notify_ex((char*)VC_VOLUME_MAX);
    }
    else if(ret == 2)
    {
    // min volume for display
       app_voice_control_notify_ex((char*)VC_VOLUME_MIN);
    }
    else
    {
        sprintf(buf, "%s %d! %s (%d/%d).", app_vc_transform_strings(VC_A_TO_STR), volume_params, app_vc_transform_strings(VC_A_CUR_STR), volume, max_volume);
        app_voice_control_notify_ex(buf);
    }
    return ret;
}

// mute control
// mute
static int _stb_mute(void* params)
{
    int ret = 0;

    ret = app_vc_set_mute_quick_ex(1, 1);
    if(ret == STATUS_TYPE_NOT_SUPPORT)
    {
    // already mute
       app_voice_control_notify_ex((char*)VC_MUTE);
    }
    else
    {
    // mute for display
        app_voice_control_notify_ex((char*)VC_A_MUTE_STR);
    }

    return ret;
}

// unmute
static int _stb_unmute(void* params)
{
    int ret = 0;

    ret = app_vc_set_mute_quick_ex(1, 0);
    if(ret == 1)
    {
    // already unmute
       app_voice_control_notify_ex((char*)VC_UNMUTE);
    }
    else
    {
    // unmute for display
        app_voice_control_notify_ex((char*)VC_A_UNMUTE_STR);
    }

    return ret;
}


// play & stop play,
// play
static int _stb_play_start(void* params)
{
    int ret = 0;

    ret = app_vc_set_play_quick(2);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_ERR);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else if(STATUS_TYPE_WINDOWN_UNSUPPORTED == ret)
    {
        app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_P_PLAY_STR);
    }

    return ret;
}

// pause play
static int _stb_play_pause(void* params)
{
    int ret = 0;

    ret = app_vc_set_play_quick(1);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_ERR);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else if(STATUS_TYPE_WINDOWN_UNSUPPORTED == ret)
    {
        app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_P_PAUSE_STR);
    }

    return ret;
}

// stop play
static int _stb_play_stop(void* params)
{
    int ret = 0;

    ret = app_vc_set_play_quick(0);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_ERR);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else if(STATUS_TYPE_WINDOWN_UNSUPPORTED == ret)
    {
        app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_P_STOP_STR);
    }

    return ret;
}


// switch channel
// switch to preview
static int _stb_switch_to_prev_channel(void *params)
{
    int ret = 0;

    ret = app_vc_switch_channel_quick(0);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_P_PREVIOUS_STR);
    }

    return ret;
}
// switch to next
static int _stb_switch_to_next_channel(void *params)
{
    int ret = 0;

    ret = app_vc_switch_channel_quick(1);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_P_NEXT_STR);
    }

    return ret;
}

// play recall
static int _stb_play_recall(void *params)
{
    int ret = 0;

    ret = app_vc_play_recall_quick();
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_P_RECALL_STR);
    }

    return ret;
}

// play by name
static int _stb_play_by_name(void *params)
{
    int ret = 0;
    char *name = NULL;
    char buf[100] = {0};
    char real_name[64] = {0};
    unsigned int real_name_len = 64;

    if(NULL == params)
        return -1;

    name = (char*)(((VoiceCMDParamsClass*)params)->str);

    ret = app_vc_play_by_name_quick(name, real_name, real_name_len);
    if((0 == ret) && (0 < real_name_len))
    {
    // command for display
        sprintf(buf, "%s %s", app_vc_transform_strings(VC_P_NAME_STR), real_name);
        app_voice_control_notify_ex((char*)buf);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else if(STATUS_TYPE_SELECT_CHANNEL == ret)
    {
        app_voice_control_notify_ex((char*)VC_P_CHANNEL_STR);
    }
    else
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }

    return ret;
}

// play by num
static int _stb_play_by_num(void *params)
{
    int ret = 0;
    char name[32] = {0};
    unsigned int num = 0;
    char buf[100] = {0};

    if(NULL == params)
        return -1;

    num = ((VoiceCMDParamsClass*)params)->value;
    ret = app_vc_play_by_num_quick(num, name, sizeof(name));
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        sprintf(buf, "%s %d", app_vc_transform_strings(VC_P_CHANNEL_STR), num);
        app_voice_control_notify_ex((char*)buf);
    }
    return ret;
}

// epg
static int _stb_goto_epg(void *params)
{
    int ret = 0;

    ret = app_vc_goto_epg();
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_M_EPG_STR);
    }

    return ret;
}

// main menu
static int _stb_goto_mainmenu(void *params)
{
    int ret = 0;
    ret = app_vc_goto_mainmenu();
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_M_MAINMENU_STR);
    }

    return ret;
}

// full screen
static int _stb_goto_fullscreen(void *params)
{
    int ret = 0;
    ret = app_vc_goto_fullscreen();
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_M_FULLSCREEN_STR);
    }

    return ret;
}
// iptv
static int _stb_goto_iptv(void *params)
{
    int ret = 0;

    ret = app_vc_goto_iptv();
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_M_IPTV_STR);
    }

    return ret;
}

static int _stb_goto_xtream(void *params)
{
    int ret = 0;

    ret = app_vc_goto_xtream();
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_M_XTREAM_STR);
    }

    return ret;
}

static int _stb_goto_stalker(void *params)
{
    int ret = 0;

    ret = app_vc_goto_xtream();
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_ERR);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_M_STALKER_STR);
    }

    return ret;
}

typedef struct _MenuOpenClass{
    char *name;
    int (*open_process)(void *params);
}MenuOpenClass;

static MenuOpenClass thiz_menu_open[] = {
    {"full screen", _stb_goto_fullscreen},
    {"epg, program guide", _stb_goto_epg},
    {"main menu", _stb_goto_mainmenu},
    {"iptv", _stb_goto_iptv},
    {"xtream", _stb_goto_xtream},
    {"stalker",_stb_goto_stalker},
};

static int _stb_open(void *params)
{
    int ret = 0;
    char *menu_name = NULL;
    int i = 0;
    unsigned int count = sizeof(thiz_menu_open)/sizeof(MenuOpenClass);
    if(params)
    {
        menu_name = (char*)(((VoiceCMDParamsClass*)params)->str);
        if(0 == strlen(menu_name))
        {
            return -1;
        }
    }

    for(i = 0; i < count; i++)
    {
        if((strstr(thiz_menu_open[i].name, menu_name))
                && (thiz_menu_open[i].open_process))
        {
            thiz_menu_open[i].open_process(NULL);
            break;
        }
    }
    if(i == count)
    {
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_CMD);
    }
    return ret;
}

static int _applets_open(void *params)
{
    int ret = 0;
    char *applets_id = NULL;

#if APPLETS_SUPPORT
    extern int app_applets_mainmenu_entry(char *id);
    if(params)
    {
        applets_id = (char*)(((VoiceCMDParamsClass*)params)->str);
        if(!applets_id||(0 == strlen(applets_id)))
        {
            return -1;
        }
    }
    GUI_EndDialog("after wnd_full_screen");
    app_set_applets_status(1);
    app_applets_mainmenu_entry(applets_id);
    return 0;
#endif

    app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_CMD);
    return ret;
}

static int _netapps_close(void *params)
{
    int ret = 0;
    char *app_id = NULL;
    char buffer[32];

#if NETAPPS_SUPPORT
    if(params)
    {
        app_id = (char*)(((VoiceCMDParamsClass*)params)->str);
        if(app_id&&(strlen(app_id)>0))
        {
            app_netapps_get_app_id(buffer, sizeof(buffer)-1);
            if((strlen(buffer)>0)&&(strcmp(buffer, "none") != 0)&&(strcmp(app_id, buffer) == 0))
            {
                GUI_EndDialog("after wnd_full_screen");
                return 0;
            }
        }
    }
#endif

    app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_CMD);
    return ret;
}

static int _netapps_process(void *params)
{
    int ret = EVENT_TRANSFER_STOP;
    char buffer[32];

#if NETAPPS_SUPPORT
    memset(buffer, 0, sizeof(buffer));
    app_netapps_get_app_id(buffer, sizeof(buffer)-1);
    if((strlen(buffer)>0)&&(strcmp(buffer, "none") != 0))
    {
    	ret = EVENT_TRANSFER_KEEPON;
    }
    else
    {
        app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_CMD);
    }
#else
    app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_CMD);
#endif

    return ret;
}

// record
// record start
static int _stb_record_start(void* params)
{
    int ret = 0;

    ret = app_vc_record_control(0);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_R_START_REC_STR);
    }

    return ret;
}

// record stop
static int _stb_record_stop(void* params)
{
    int ret = 0;

    ret = app_vc_record_control(1);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else if(STATUS_TYPE_REPEAT == ret)
    {// repeat operator
       app_voice_control_notify_ex((char*)VC_REPEAT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_R_STOP_REC_STR);
    }

    return ret;
}

static int _stb_basic_control_exit(void *params)
{
    int ret = 0;

    ret = app_vc_switch_basic_keypress(0);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_B_BASIC_EXIT);
    }

    return ret;
}

static int _stb_basic_control_up(void *params)
{
    int ret = 0;

    ret = app_vc_switch_basic_keypress(1);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
        //sprintf(buf, "%s %s", app_vc_transform_strings(VC_P_NAME_STR), real_name);
        //app_voice_control_notify_ex((char*)buf);

        app_voice_control_notify_ex((char*)VC_B_BASIC_UP);
    }

    return ret;
}

static int _stb_basic_control_down(void *params)
{
    int ret = 0;

    ret = app_vc_switch_basic_keypress(2);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_B_BASIC_DOWN);
    }

    return ret;
}

static int _stb_basic_control_left(void *params)
{
    int ret = 0;

    ret = app_vc_switch_basic_keypress(3);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_B_BASIC_LEFT);
    }

    return ret;
}

static int _stb_basic_control_right(void *params)
{
    int ret = 0;

    ret = app_vc_switch_basic_keypress(4);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_B_BASIC_RIGHT);
    }

    return ret;
}

static int _stb_basic_control_confirm(void *params)
{
    int ret = 0;

    ret = app_vc_switch_basic_keypress(5);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_B_BASIC_CONFIRM);
    }

    return ret;
}

static int _stb_basic_control_cancel(void *params)
{
    int ret = 0;

    ret = app_vc_switch_basic_keypress(6);
    if(STATUS_TYPE_NOT_SUPPORT == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }
    else
    {
    // command for display
        app_voice_control_notify_ex((char*)VC_B_BASIC_CANCEL);
    }

    return ret;
}

#if 0
// standby
static int _stb_standby(void *params)
{
    int ret = 0;
    ret = app_vc_standby_control(1);
    if(1 == ret)
    {
    // not support
       app_voice_control_notify_ex((char*)VC_NOT_SUPPORT);
    }

    return ret;
}

static int _stb_wakeup(void *params)
{
    int ret = 0;
    ret = app_vc_standby_control(0);
    return ret;
}
#endif

static int _data_begin(void *params)
{
    app_voice_control_notify_waiting();
    return 0;
}

static int _not_support(void *params)
{
    app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_EX);
    return 0;
}

static int _cmd_not_support(void *params)
{
    app_voice_control_notify_ex((char*)VC_NOT_SUPPORT_CMD);
    return 0;
}


static int _error(void *params)
{
    if(params)
    {
        app_voice_control_notify_ex((char*)params);
    }
    else
    {
        app_voice_control_notify_ex((char*)VC_ERROR);
    }
    return 0;
}

static APPCMDFunctionClss thiz_cmd_normal[] = {
    { VOICE_CMD_DATA_BEGIN,           .p = {.cmd_func = _data_begin }},
    { VOICE_CMD_DATA_END,             .p = {.cmd_func = NULL }},
    { VOICE_CMD_ERR,                  .p = {.cmd_func = _error }},

    // Voice CMD
    { VOICE_CMD_VOLUME_UP ,           .p = {.cmd_func = _stb_volume_up }},
    { VOICE_CMD_VOLUME_DOWN,          .p = {.cmd_func = _stb_volume_down }},
    { VOICE_CMD_VOLUME,               .p = {.cmd_func = _stb_volume }},
    { VOICE_CMD_MUTE,                 .p = {.cmd_func = _stb_mute }},
    { VOICE_CMD_UNMUTE,               .p = {.cmd_func = _stb_unmute }},

    { VOICE_CMD_PLAY,                 .p = {.cmd_func = _stb_play_start }},
    { VOICE_CMD_STOP_PLAY,            .p = {.cmd_func = _stb_play_stop }},
    { VOICE_CMD_PAUSE,                .p = {.cmd_func = _stb_play_pause }},
    { VOICE_CMD_FAST_BACKWARD,        .p = {.cmd_func = NULL }},
    { VOICE_CMD_FAST_FORWARD,         .p = {.cmd_func = NULL }},
    { VOICE_CMD_SEEK_FORWARD,         .p = {.cmd_func = NULL }},
    { VOICE_CMD_SEEK_BACKWARD,        .p = {.cmd_func = NULL }},
    { VOICE_CMD_SEEK_TO,              .p = {.cmd_func = NULL }},

    { VOICE_CMD_REC,                  .p = {.cmd_func = _stb_record_start }},
    { VOICE_CMD_END_RECORD,           .p = {.cmd_func = _stb_record_stop }},

    { VOICE_CMD_RECALL_PROG,          .p = {.cmd_func = _stb_play_recall }},
    { VOICE_CMD_PLAY_BY_NMAE,         .p = {.cmd_func = _stb_play_by_name }},
    { VOICE_CMD_PLAY_BY_NUM,          .p = {.cmd_func = _stb_play_by_num }},
    { VOICE_CMD_PRE,                  .p = {.cmd_func = _stb_switch_to_prev_channel }},
    { VOICE_CMD_NEXT,                 .p = {.cmd_func = _stb_switch_to_next_channel }},

    { VOICE_CMD_OPEN,                 .p = {.cmd_func = _stb_open }},

    { VOICE_CMD_MARK_PROG,            .p = {.cmd_func = NULL }},
    { VOICE_CMD_UNMARK_PROG,          .p = {.cmd_func = NULL }},

    { VOICE_CMD_STANDBY,              .p = {.cmd_func = NULL/*_stb_standby*/ }},
    { VOICE_CMD_WAKEUP,               .p = {.cmd_func = NULL/*_stb_wakeup*/ }},

    { VOICE_CMD_OPEN_APPLETS,                 .p = {.cmd_func = _applets_open }},
    { VOICE_CMD_NETAPPS_EXIT,                 .p = {.cmd_func = _netapps_close }},
    { VOICE_CMD_NETAPPS_SEARCH,                 .p = {.cmd_func = _netapps_process }},
    { VOICE_CMD_NETAPPS_CATEGORY,                 .p = {.cmd_func = _netapps_process }},
    { VOICE_CMD_NETAPPS_GROUP,                 .p = {.cmd_func = _netapps_process }},
    { VOICE_CMD_NETAPPS_PAGE,                 .p = {.cmd_func = _netapps_process }},
    { VOICE_CMD_NETAPPS_PLAY,                 .p = {.cmd_func = _netapps_process }},
    { VOICE_CMD_NETAPPS_SETTING,                 .p = {.cmd_func = _netapps_process }},
    { VOICE_CMD_NETAPPS_USER,                 .p = {.cmd_func = _netapps_process }},
    { VOICE_CMD_NETAPPS_PRIVATE,                 .p = {.cmd_func = _netapps_process }},

    { VOICE_CMD_KEY_EXIT,               .p = {.cmd_func = _stb_basic_control_exit }},
    { VOICE_CMD_KEY_UP,                 .p = {.cmd_func = _stb_basic_control_up }},
    { VOICE_CMD_KEY_DOWN,               .p = {.cmd_func = _stb_basic_control_down }},
    { VOICE_CMD_KEY_LEFT,               .p = {.cmd_func = _stb_basic_control_left }},
    { VOICE_CMD_KEY_RIGHT,              .p = {.cmd_func = _stb_basic_control_right }},
    { VOICE_CMD_KEY_OK,                 .p = {.cmd_func = _stb_basic_control_confirm }},
    { VOICE_CMD_KEY_CANCEL,             .p = {.cmd_func = _stb_basic_control_cancel }},

};

// not support menu
static int _support_check_result(void)
{
    if((GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH))
        || (GXCORE_SUCCESS == GUI_CheckDialog(WND_OTA_UPGRADE))
        || (GXCORE_SUCCESS == GUI_CheckDialog(WND_UPGRADE_PROCESS))
        || (app_vc_osdttx_flag_get())
            )
    {
        return STATUS_TYPE_NOT_SUPPORT;// not support
    }
    return STATUS_TYPE_CAN_SUPPORT;
}

int app_vc_cloud_cmd_process(unsigned int cmd_id, char* des, unsigned int des_len)
{
    int ret = 0;
    int i = 0;
    int count = sizeof(thiz_cmd_normal)/sizeof(APPCMDFunctionClss);


    if(STATUS_TYPE_NOT_SUPPORT == _support_check_result())
    {
        _not_support(NULL);
        return -1;
    }
    // normal cmd
    for(i = 0; i < count; i++)
    {
        if(thiz_cmd_normal[i].cmd_id == cmd_id)
        {
            if(thiz_cmd_normal[i].p.cmd_func)
            {
                ret = thiz_cmd_normal[i].p.cmd_func(des);
            }
            return ret;
        }
    }
    _cmd_not_support(NULL);
    return ret;
}

#endif

