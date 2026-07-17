/*
 * =====================================================================================
 *
 *       Filename:  app_iptv_audio.c
 *
 *    Description:  select iptv audio language
 *
 *        Version:  1.0
 *        Created:  2020-06-03-11:13
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zzy
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
#include "app_common_select.h"
#include "app_netvideo_play.h"

static int netvideo_set_audio_channel(int index)
{
    GxMessage *msg = NULL;
    GxMsgProperty_PlayerAudioSwitch *audio_switch;
    PlayerProgInfo  *info = (PlayerProgInfo *)app_get_netvideo_playinfo();

    if( info == NULL || index < 0 || index >= info->audio_num || index == info->cur_audio_id)
        return 1;

    msg = GxBus_MessageNew(GXMSG_PLAYER_AUDIO_SWITCH);
    APP_CHECK_P(msg, 1);
    audio_switch = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PlayerAudioSwitch);
    APP_CHECK_P(audio_switch, 1);
    audio_switch->player = PLAYER_FOR_IPTV;
    audio_switch->pid = index;
    GxBus_MessageSendWait(msg);
    GxBus_MessageFree(msg);
    info->cur_audio_id = index;

    return 0;
}

int app_netvideo_audio_list_get_total(GuiWidget *widget, void *usrdata)
{
    PlayerProgInfo  *info = (PlayerProgInfo *)app_get_netvideo_playinfo();
    if ((info == NULL) || (0 >= info->audio_num) || (PLAYER_MAX_TRACK_AUDIO < info->audio_num))
    {
        return 0;
    }

    return info->audio_num;
}

int app_netvideo_audio_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    PlayerProgInfo  *info = (PlayerProgInfo *)app_get_netvideo_playinfo();

    item = (ListItemPara *)usrdata;
    if (NULL == item || info == NULL)
        return GXCORE_ERROR;
    if (0 > item->sel)
        return GXCORE_ERROR;

    // col-0: num
    item->x_offset = 0;
    item->image = NULL;
    item->string = NULL;

    //col-1: audio lang
    item = item->next;
    item->image = NULL;
    if(0 == strcmp(info->audio[item->sel].lang,""))
    {
        sprintf(info->audio[item->sel].lang,"audio%d",item->sel+1);
    }
    item->string = info->audio[item->sel].lang;

    return EVENT_TRANSFER_STOP;
}

int app_netvideo_audio_list_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

int app_netvideo_audio_list_destroy(GuiWidget *widget, void *usrdata)
{

    return EVENT_TRANSFER_STOP;
}

int app_netvideo_audio_track_change(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel = 0;

    GUI_GetProperty(g_CommonSelect.widget.group, "select", &box_sel);
    pmpset_set_int(PMPSET_AUDIO_TRACK, box_sel);

    return EVENT_TRANSFER_STOP;
}

int app_netvideo_audio_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    int ret = EVENT_TRANSFER_KEEPON;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                break;

                case STBK_EXIT:
                case STBK_MENU:
                {
                    GUI_GetProperty(g_CommonSelect.widget.group, "select", &box_sel);
                    pmpset_set_int(PMPSET_AUDIO_TRACK, box_sel);
                    GUI_EndDialog(g_CommonSelect.widget.wnd);
                    GUI_SetInterface("flush", NULL);
                    ret = EVENT_TRANSFER_STOP;
                }
                break;

                case STBK_OK:
                {
                    int sel = 0;

                    GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                    if(0 == netvideo_set_audio_channel(sel))
                        GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
                }
                break;
                case STBK_LEFT:
                case STBK_RIGHT:
                {
                    GUI_SendEvent(g_CommonSelect.widget.group, event);
                    ret = EVENT_TRANSFER_STOP;
                }
                break;
                default:
                    break;
            }
    }

    return ret;
}

int app_netvideo_audio_create(GuiWidget *widget, void *usrdata)
{
    uint32_t value = 0;
    int sel = 0;

    value = pmpset_get_int(PMPSET_AUDIO_TRACK);

    GUI_SetProperty(g_CommonSelect.widget.group, "select", &value);
    PlayerProgInfo  *info = (PlayerProgInfo *)app_get_netvideo_playinfo();
    if(info == NULL)
        return EVENT_TRANSFER_STOP;

    sel = info->cur_audio_id;
    GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
    GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
    return EVENT_TRANSFER_STOP;
}

int app_netvideo_audio_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

int app_netvideo_audio_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
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
int app_netvideo_audio_create_dialog(void)
{
    CommonSelectParas paras = {0};
    int i = 0;

    PlayerProgInfo  *info = (PlayerProgInfo *)app_get_netvideo_playinfo();

    if ((info == NULL) || (0 >= info->audio_num) || (PLAYER_MAX_TRACK_AUDIO < info->audio_num))
    {
        return 0;
    }
    for(i=0; i < info->audio_num; i++)
    {
        if(0 == strcmp(info->audio[i].lang,""))
        {
            sprintf(info->audio[i].lang,"audio%d",i+1);
        }
    }
    paras.choice = COMMON_SELECT_TITLE_GROUP;
    paras.title = "Audio";
    paras.content = "[Mono,Left,Right,Stereo]";

    paras.signal.create = app_netvideo_audio_create;
    paras.signal.destroy = app_netvideo_audio_destroy;
    paras.signal.keypress = app_netvideo_audio_keypress;
    paras.signal.group_change = app_netvideo_audio_track_change;
    paras.signal.list_get_total = app_netvideo_audio_list_get_total;
    paras.signal.list_get_data = app_netvideo_audio_list_get_data;
    paras.signal.list_keypress = app_netvideo_audio_list_keypress;
    app_common_select_create_dialog(&paras);

    return 0;
}
