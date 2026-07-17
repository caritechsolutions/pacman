#include <fcntl.h>
#include "app.h"
#include "stdlib.h"
#include "app_module.h"
#include "app_default_params.h"
#include "media_info.h"
#include "app_volume_value.h"
#include "app_windows.h"
#include "full_screen.h"
#if USB_MICROPHONE_SUPPORT
#include "module/mircophone/app_microphone_view.h"

typedef enum {
    ITEM_MIKE_USB_VOLUME = 0,
    ITEM_MIKE_STB_VOLUME,
    ITEM_MIKE_TOTAL
} MicrophoneVolumeMode;

//widget
#define MIKE_IMAGE_MUTE                   "mike_view_imag_mute"

#define MIKE_BMP_VOL_MUTE        "s_vol_mute"
#define MIKE_BMP_VOL_UNMUTE      "s_vol"
#define MIKE_USB_IMG_VOL         "img_usb_volume"
#define MIKE_IMG_VOL             "img_stb_volume"
#define MIKE_PROGBAR_VOL         "stb_progbar_volume"
#define MIKE_USB_PROGBAR_VOL     "usb_progbar_volume"
#define MIKE_TXT_VOL             "txt_stb_volume"
#define MIKE_USB_TXT_VOL         "txt_usb_volume"
#define MIKE_MAX_ITEM_NUM     2

static char *s_img_mike_focus[MIKE_MAX_ITEM_NUM] =
{
    "img_mike_item_focus1",
    "img_mike_item_focus2",
};


extern status_t app_send_msg_exec(uint32_t msg_id,void* params);
extern uint32_t app_system_vol_get(void);
extern int app_iptv_lite_check(void);
extern status_t app_volume_scope_status(void);
extern int app_audio_prog_track_set_mode(int value,uint8_t mode);


static GxMsgProperty_PlayerAudioVolume thiz_volume;
static GxMsgProperty_PlayerAudioVolume thiz_usb_volume;
static unsigned int thiz_cur_item = 0;
static event_list* thiz_timer = NULL;

status_t mike_popmsg_mute_init(void)
{
    uint32_t value = 0;

    value=pmpset_get_int(PMPSET_MUTE);

    switch (value)
    {
        case PMPSET_MUTE_ON:
            GUI_SetProperty(MIKE_IMAGE_MUTE, "state", "show");
            break;

        case PMPSET_MUTE_OFF:
            GUI_SetProperty(MIKE_IMAGE_MUTE, "state", "hide");
            break;

        default:
            return GXCORE_ERROR;
            break;
    }
    return GXCORE_SUCCESS;
}

status_t mike_popmsg_mute_ctrl(void)
{
    uint32_t value = 0;

    value=pmpset_get_int(PMPSET_MUTE);

    switch (value)
    {
        case PMPSET_MUTE_ON:
            pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_OFF);
            app_microphone_set_mute(PMPSET_MUTE_OFF);
            GUI_SetProperty(MIKE_IMAGE_MUTE, "state", "hide");
            GUI_SetProperty(WND_MIKE_VIEW, "update", NULL);
            GUI_SetInterface("flush", NULL);
            break;

        case PMPSET_MUTE_OFF:
            pmpset_set_int(PMPSET_MUTE, PMPSET_MUTE_ON);
            app_microphone_set_mute(PMPSET_MUTE_ON);
            GUI_SetProperty(MIKE_IMAGE_MUTE, "state", "show");
            break;

        default:
            return GXCORE_ERROR;
            break;
    }
    return GXCORE_SUCCESS;
}

static void microphone_focus_item(int index)
{
    if((index >= 0) && (index < MIKE_MAX_ITEM_NUM))
    {
        GUI_SetProperty(s_img_mike_focus[index], "state", "show");
    }
}

static void microphone_unfocus_item(int index)
{
    if((index >= 0) && (index < MIKE_MAX_ITEM_NUM))
    {
        GUI_SetProperty(s_img_mike_focus[index], "state", "hide");
    }
}

static void _microphone_up_keypress(void)
{
    int val = thiz_cur_item;

    if(thiz_cur_item == ITEM_MIKE_USB_VOLUME)
    {
        thiz_cur_item = MIKE_MAX_ITEM_NUM -1;
    }
    else
    {
        thiz_cur_item --;
    }

    microphone_unfocus_item(val);
    microphone_focus_item(thiz_cur_item);
    if(thiz_timer != NULL)
    {
        remove_timer(thiz_timer);
        thiz_timer = NULL;
    }

}

static void _microphone_down_keypress(void)
{
    int sel = thiz_cur_item;
    if(thiz_cur_item == MIKE_MAX_ITEM_NUM - 1)
    {
        thiz_cur_item = ITEM_MIKE_USB_VOLUME;
    }
    else
    {
        thiz_cur_item ++;
    }
    microphone_unfocus_item(sel);
    microphone_focus_item(thiz_cur_item);
    if(thiz_timer != NULL)
    {
        remove_timer(thiz_timer);
        thiz_timer = NULL;
    }
}

static status_t _mircophone_vol_save(GxMsgProperty_PlayerAudioVolume val,unsigned int mike_val)
{
    if(val > 99)
        val = 99;
    if(thiz_cur_item == ITEM_MIKE_USB_VOLUME)
    {
        GxBus_ConfigSetInt(USB_AUDIO_VOLUME_KEY, mike_val);
    }
    else
    {
        GxBus_ConfigSetInt(AUDIO_VOLUME_KEY, val);
    }

    return GXCORE_SUCCESS;
}

static void _mircophone_volume_value_init(void)
{
    int32_t val = 0;
    int32_t usb_value = 0;
    char volume_buf[4] = {0};
    char usb_volume_buf[4] = {0};

    val = app_system_vol_get();
    if(val == 99)
    {
        thiz_volume = 25;
    }
    else
    {
        thiz_volume = val / VOLUME_NUM;
    }

    if(thiz_volume > 0)
    {
        GUI_SetProperty(MIKE_IMG_VOL, "img", MIKE_BMP_VOL_UNMUTE);
    }
    else
    {
        GUI_SetProperty(MIKE_IMG_VOL, "img", MIKE_BMP_VOL_MUTE);
    }

    GUI_SetProperty(MIKE_PROGBAR_VOL, "value", (void *)(&thiz_volume));
    sprintf(volume_buf, "%d", thiz_volume);
    GUI_SetProperty(MIKE_TXT_VOL, "string", volume_buf);

    GxBus_ConfigGetInt(USB_AUDIO_VOLUME_KEY, &usb_value, AUDIO_VOLUME);
    if(usb_value == 99)
    {
        thiz_usb_volume = 25;
    }
    else
    {
        thiz_usb_volume = usb_value / VOLUME_NUM;
    }

    if(thiz_usb_volume > 0)
    {
        GUI_SetProperty(MIKE_USB_IMG_VOL, "img", MIKE_BMP_VOL_UNMUTE);
    }
    else
    {
        GUI_SetProperty(MIKE_USB_IMG_VOL, "img", MIKE_BMP_VOL_MUTE);
    }

    GUI_SetProperty(MIKE_USB_PROGBAR_VOL, "value", (void *)(&thiz_usb_volume));
    sprintf(usb_volume_buf, "%d", thiz_usb_volume);
    GUI_SetProperty(MIKE_USB_TXT_VOL, "string", usb_volume_buf);

}

static void _microphone_volume_change(uint16_t key)
{
    FullArb *arb = &g_AppFullArb;
    uint32_t audio_vol = 0;
    GxMsgProperty_PlayerAudioVolume disp_value = 0;
    char volume_buf[4];
    char usb_volume_buf[4];

    // when change volue, enable mute
    if ((key == STBK_LEFT) || (key == STBK_VOLDOWN) || STBK_GREEN == key)
    {
        if(thiz_cur_item == ITEM_MIKE_USB_VOLUME)
        {
            if (thiz_usb_volume > 1)
            {
                thiz_usb_volume--;
            }
            else
            {
                thiz_usb_volume = 0;
            }

        }
        else
        {
            if (thiz_volume > 1)
            {
                thiz_volume--;
            }
            else
            {
                thiz_volume = 0;
            }
        }
    }
    else if ((key == STBK_RIGHT) || (key == STBK_VOLUP) || STBK_YELLOW == key)
    {
        if(thiz_cur_item == ITEM_MIKE_USB_VOLUME)
        {
            if (thiz_usb_volume >= MAX_DISPLAY_VOL)
            {
                thiz_usb_volume = MAX_DISPLAY_VOL;
            }
            else
            {
                thiz_usb_volume++;
            }

        }
        else
        {
            if (thiz_volume >= MAX_DISPLAY_VOL)
            {
                thiz_volume = MAX_DISPLAY_VOL;
            }
            else
            {
                thiz_volume++;
            }
        }
    }

    // player do sth
    if(thiz_cur_item == ITEM_MIKE_USB_VOLUME)
    {
        if(thiz_usb_volume > 0)
        {
            GUI_SetProperty(MIKE_USB_IMG_VOL, "img", MIKE_BMP_VOL_UNMUTE);
        }
        else
        {
            GUI_SetProperty(MIKE_USB_IMG_VOL, "img", MIKE_BMP_VOL_MUTE);
        }

        audio_vol = AUDIO_MAP_25(thiz_usb_volume);
        app_microphone_set_volume(audio_vol);
        GUI_SetProperty(MIKE_USB_PROGBAR_VOL, "value", (void *)(&(thiz_usb_volume)));
        memset(usb_volume_buf, 0, 4);
        sprintf(usb_volume_buf, "%d", thiz_usb_volume);
        GUI_SetProperty(MIKE_USB_TXT_VOL, "string", usb_volume_buf);
    }
    else
    {
        if(thiz_volume > 0)
        {
            GUI_SetProperty(MIKE_IMG_VOL, "img", MIKE_BMP_VOL_UNMUTE);
        }
        else
        {
            GUI_SetProperty(MIKE_IMG_VOL, "img", MIKE_BMP_VOL_MUTE);
        }

        audio_vol = AUDIO_MAP_25(thiz_volume);
        app_system_vol_set(audio_vol);

        if(arb->state.mute == STATE_ON)
        {
            arb->exec[EVENT_MUTE](arb);
            arb->draw[EVENT_MUTE](arb);
        }

        disp_value = thiz_volume;
        GUI_SetProperty(MIKE_PROGBAR_VOL, "value", (void *)(&(disp_value)));

        // set number value
        memset(volume_buf, 0, 4);
        sprintf(volume_buf, "%d", thiz_volume);
        GUI_SetProperty(MIKE_TXT_VOL, "string", volume_buf);

        if (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
        {
            extern status_t popmsg_mute_init(void);
            popmsg_mute_init();
        }
    }
}

static int _mike_volume_bal_timeout(void* data)
{
    if(thiz_timer != NULL)
    {
        remove_timer(thiz_timer);
        thiz_timer = NULL;
    }
    GUI_EndDialog(WND_MIKE_VIEW);
    GUI_SetProperty(WND_MIKE_VIEW, "draw_now", NULL);

    return GXCORE_SUCCESS;
}

static void _microphone_left_right_key_press(uint16_t key)
{
    int init_value = INFOBAR_TIMEOUT;

    GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);

    if (reset_timer(thiz_timer) != 0) {
        // reset time failed, do
        thiz_timer = create_timer(_mike_volume_bal_timeout, init_value*SEC_TO_MILLISEC*3, NULL, TIMER_REPEAT);
    }

    _microphone_volume_change(key);
}


SIGNAL_HANDLER  int app_mike_view_got_focus(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_mike_view_lost_focus(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_mike_view_create(const char* widgetname, void *usrdata)
{
    int32_t audio_vol = 0;
    //int32_t usb_audio_vol = 0;
    int init_value = INFOBAR_TIMEOUT;
    //app_microphone_register_mike_dogle();
    /*set volume*/
    GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
    //GxBus_ConfigGetInt(USB_AUDIO_VOLUME_KEY, &usb_audio_vol, AUDIO_VOLUME);
    app_system_vol_set(audio_vol);

    microphone_focus_item(thiz_cur_item);
    //app_microphone_play();
    /*popmsg_mute*/
    mike_popmsg_mute_init();

    thiz_volume = 0;
    _mircophone_volume_value_init();

    GxBus_ConfigGetInt(INFOBAR_TIMEOUT_KEY, &init_value, INFOBAR_TIMEOUT);
    if (reset_timer(thiz_timer) != 0) {
        // reset time failed, do
        thiz_timer = create_timer(_mike_volume_bal_timeout, init_value*SEC_TO_MILLISEC*3, NULL, TIMER_REPEAT);
    }

    return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int app_mike_view_destroy(const char* widgetname, void *usrdata)
{
    if(thiz_timer != NULL)
    {
        remove_timer(thiz_timer);
        thiz_timer = NULL;
    }
    _mircophone_vol_save(AUDIO_MAP_25(thiz_volume),AUDIO_MAP_25(thiz_usb_volume));

    //app_microphone_unregister_mike_dogle();

    return GXCORE_SUCCESS;
}


SIGNAL_HANDLER int app_mike_view_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;
    const char *wnd = GUI_GetFocusWindow();

    APP_CHECK_P(usrdata, EVENT_TRANSFER_STOP);
    event = (GUI_Event *)usrdata;
    switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
    {
        case VK_BOOK_TRIGGER:
            GUI_EndDialog("after wnd_full_screen");
            break;
        case APPK_BACK:
        case APPK_MENU:
            _mike_volume_bal_timeout(NULL);
            if(0 == strcasecmp(WND_FULL_SCREEN, wnd))
            {
                GUI_SendEvent(wnd, event);
            }
            return EVENT_TRANSFER_STOP;
        case APPK_9:
            mike_popmsg_mute_ctrl();
            return EVENT_TRANSFER_STOP;
        case APPK_UP:
            _microphone_up_keypress();
            break;
        case APPK_DOWN:
            _microphone_down_keypress();
            break;
        case APPK_VOLUP:
        case APPK_VOLDOWN:
        case APPK_LEFT:
        case APPK_RIGHT:
            {
                _microphone_left_right_key_press(find_virtualkey_ex(event->key.scancode, event->key.sym));
            }
            break;
        default:
            _mike_volume_bal_timeout(NULL);
            GUI_SendEvent(GUI_GetFocusWindow(), event);
            break;
    }

    return EVENT_TRANSFER_STOP;
}

void app_create_mike_menu(void)
{
    int pos_y = 40;
    popdlg_destroy();
    GUI_CreateDialog(WND_MIKE_VIEW);
    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO){
        GUI_SetProperty(WND_MIKE_VIEW, "move_window_y", &pos_y);
    }
}
void app_mircophone_voice_start(void)
{
    int ret = -1;
    PopDlg pop;
    ret = app_microphone_audio_enable();
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    if(GXCORE_SUCCESS == ret){
        pop.str = "Successfully turned on microphone";
    }
    else{
        if(ret == -1){
            pop.str = "The microphone is already turned on";
        }
        else if(app_get_mic_uac_device_hotplug_status() == -1){
            pop.str = "No device detected";
        }else{
            pop.str = "Opening microphone failed";
        }
    }
    pop.timeout_sec = 3;
    pop.mode = POP_MODE_UNBLOCK;
    popdlg_create(&pop);
}
void app_mircophone_voice_stop(void)
{
    int ret = 0;
    PopDlg pop;

    ret = app_microphone_pause();
    if(GXCORE_SUCCESS == ret){
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        if(app_get_mic_uac_device_hotplug_status() == -1){
            pop.str = "No device detected";
        }else{
            pop.str = "Turn off microphone";
        }
        pop.timeout_sec = 3;
        pop.mode = POP_MODE_UNBLOCK;
        popdlg_create(&pop);
    }
}

#endif
