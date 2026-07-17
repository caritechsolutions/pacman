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
#include "app_module.h"
#include "full_screen.h"
#include "app_common_select.h"
#include "app_fm.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

typedef enum{
    STATE_NORMAL =0,
    STATE_DEL
}FmListState;

static FmListState s_fm_list_state = STATE_NORMAL;
FmListData* sp_fm_list_data = NULL;

static bool app_fm_freq_list_check_del_flag(uint32_t sel)
{
    if(sp_fm_list_data == NULL)
        return false;

    if(sp_fm_list_data[sel].del_flag == 0)
    {
        return false;
    }
    return true;
}

static bool app_fm_freq_list_check_all_del_flag(void)
{
    int num = 0;
    int i = 0;
    int total = 0;

    if(sp_fm_list_data == NULL)
        return false;

    num = app_fm_freq_num_get();
    for(i = 0; i < num; i++)
    {
        if(sp_fm_list_data[i].del_flag == 1)
            total++;
    }
    if(total == num)
        return true;
    else
        return false;
}

static void app_fm_freq_list_set_del_flag(uint32_t sel)
{
    if(sp_fm_list_data == NULL)
        return;

    if(sp_fm_list_data[sel].del_flag == 0)
        sp_fm_list_data[sel].del_flag = 1;
    else
        sp_fm_list_data[sel].del_flag = 0;
}

static void app_fm_freq_list_set_all_del_flag(void)
{
    int num = 0;
    int i = 0;

    if(sp_fm_list_data == NULL)
        return;

    num = app_fm_freq_num_get();
    if(app_fm_freq_list_check_all_del_flag() == true)
    {
        for(i = 0; i < num; i++)
        {
            sp_fm_list_data[i].del_flag = 0;
        }
    }
    else
    {
        for(i = 0; i < num; i++)
        {
            sp_fm_list_data[i].del_flag = 1;
        }
    }
}

static bool app_fm_freq_list_check_del_exit(void)
{
    int num = 0;
    int i = 0;

    if(sp_fm_list_data == NULL)
        return false;

    num = app_fm_freq_num_get();
    for(i = 0; i < num; i++)
    {
        if(sp_fm_list_data[i].del_flag == 1)
            return true;
    }
    return false;
}

SIGNAL_HANDLER int app_fm_freq_list_create(GuiWidget *widget, void *usrdata)
{
    int sel = 0, num = 0;

    s_fm_list_state = STATE_NORMAL;
    num = app_fm_freq_num_get();
    sp_fm_list_data = (FmListData*)GxCore_Malloc(num * sizeof(FmListData));
    if(sp_fm_list_data)
    {
        memset(sp_fm_list_data, 0, num * sizeof(FmListData));
        app_fm_freq_list_get(sp_fm_list_data);
        sel = app_fm_play_get_index();
        GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
        GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_freq_list_destroy(GuiWidget *widget, void *usrdata)
{
    s_fm_list_state = STATE_NORMAL;
    GXCORE_FREE(sp_fm_list_data);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_freq_list_got_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_freq_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_fm_freq_list_list_get_data(GuiWidget *widget, void *usrdata)
{

    ListItemPara *item = NULL;
    static char buffer1[10];
    static char buffer2[20+FM_FREQ_NAME_LEN];

    item = (ListItemPara *)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    //col-0: num
    item->x_offset = 2;
    item->image = NULL;
    memset(buffer1, 0, 10);
    sprintf(buffer1, " %d", (item->sel + 1));
    item->string = buffer1;

    //col-1: fm freq
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer2, 0, 20+FM_FREQ_NAME_LEN);
#if FM_FREQ_NAME_SUPPORT
    sprintf(buffer2, " %.1f MHz %s", sp_fm_list_data[item->sel].freq/1000.0,sp_fm_list_data[item->sel].name);
#else
    sprintf(buffer2, " %.1f MHz", sp_fm_list_data[item->sel].freq/1000.0);
#endif
    item->string = buffer2;

    //col-2: del
    item = item->next;
    item->x_offset = 0;
    if(app_fm_freq_list_check_del_flag(item->sel) == true)
    {
        item->image = "s_icon_del";
    }
    else
    {
        item->image = NULL;
    }
    item->string = NULL;

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_freq_list_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_fm_freq_list_list_get_total(GuiWidget *widget, void *usrdata)
{
    return app_fm_freq_num_get();
}

static void app_fm_freq_list_play(void)
{
    int num = 0;
    int sel = 0;

    num = app_fm_freq_num_get();
    if(num > 0)
    {
        GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
#if DVB2IP_SERVER_SUPPORT
    if(app_dvb2ip_fm_stop_rec_tip_by_freq(sp_fm_list_data[sel].freq)<0)
        return;
#endif
        GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
        app_fm_play_set_index(sel);
        app_fm_play_prog(sp_fm_list_data[sel].freq);
    }
    return;
}

static void app_fm_freq_list_tips_update(void)
{
    if(s_fm_list_state == STATE_DEL)
    {
        GUI_SetProperty(g_CommonSelect.widget.tips.text_green, "string", STR_ID_DEL_ALL);
        GUI_SetProperty(g_CommonSelect.widget.tips.text_green, "state", "show");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_green, "state", "show");
        GUI_SetProperty(g_CommonSelect.widget.tips.text_ok, "string", STR_ID_SELECT);
        GUI_SetProperty(g_CommonSelect.widget.tips.text_ok, "state", "show");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_ok, "state", "show");
        GUI_SetProperty(g_CommonSelect.widget.tips.text_exit, "string", STR_ID_FM_EXIT_DEL);
        GUI_SetProperty(g_CommonSelect.widget.tips.text_exit, "state", "show");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_exit, "state", "show");
        GUI_SetProperty(g_CommonSelect.widget.tips.text_red, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_red, "state", "hide");
    }
    else
    {
        GUI_SetProperty(g_CommonSelect.widget.tips.text_red, "string", STR_ID_DELETE);
        GUI_SetProperty(g_CommonSelect.widget.tips.text_red, "state", "show");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_red, "state", "show");
#if FM_FREQ_NAME_SUPPORT
        GUI_SetProperty(g_CommonSelect.widget.tips.text_green, "string", STR_ID_RENAME);
        GUI_SetProperty(g_CommonSelect.widget.tips.img_green, "state", "show");
#else
        GUI_SetProperty(g_CommonSelect.widget.tips.text_green, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_green, "state", "hide");
#endif
        GUI_SetProperty(g_CommonSelect.widget.tips.text_ok, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_ok, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.text_exit, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_exit, "state", "hide");
    }
}

static void app_fm_freq_list_del_freq(void)
{
    int sel1 = 0;
    int num = 0;
    int index = 0;
    int i = 0;
    int cur_freq = 0;
    int replay = 0;

    GUI_GetProperty(g_CommonSelect.widget.list, "active", &sel1);
    app_fm_freq_del(sp_fm_list_data);
    GUI_SetProperty(g_CommonSelect.widget.list, "update_all", NULL);

    num = app_fm_freq_num_get();
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_prog_num_update();
#endif
    if(num > 0)
    {
        if(sp_fm_list_data[sel1].del_flag == true)
            replay = 1;
        cur_freq = sp_fm_list_data[sel1].freq;
        sp_fm_list_data = (FmListData*)GxCore_Realloc(sp_fm_list_data, num * sizeof(FmListData));
        if(sp_fm_list_data)
        {
            memset(sp_fm_list_data, 0, num * sizeof(FmListData));
            app_fm_freq_list_get(sp_fm_list_data);
            if(replay == 1)
            {
                index = 0;
                app_fm_play_prog(sp_fm_list_data[index].freq);
                app_fm_play_set_index(index);
            }
            else
            {
                for(i=0; i<num; i++)
                {
                    if(sp_fm_list_data[i].freq == cur_freq)
                        break;
                }
                index = i;
            }
            app_fm_play_set_index(index);
            GUI_SetProperty(g_CommonSelect.widget.list, "select", &index);
            GUI_SetProperty(g_CommonSelect.widget.list, "active", &index);
            s_fm_list_state = STATE_NORMAL;
            app_fm_freq_list_tips_update();
        }
    }
    else
    {
        app_fm_freq_del_file();
        app_fm_play_set_index(0);
        GxPlayer_MediaStop(PLAYER_FOR_FM);
        GUI_EndDialog(g_CommonSelect.widget.wnd);
        GUI_CreateDialog(WND_FM_PLAY_INFO);
    }

    return;
}
#if FM_FREQ_NAME_SUPPORT
static void fm_list_keyboard_release_cb(PopKeyboard *data)
{
    int sel=0;

    if(data->in_ret == POP_VAL_CANCEL)
        return;

    GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
    memset(sp_fm_list_data[sel].name,0,FM_FREQ_NAME_LEN);
    strncpy(sp_fm_list_data[sel].name,data->out_name,FM_FREQ_NAME_LEN-1);
    app_fm_update_freq_name(&sp_fm_list_data[sel]);

    GUI_SetProperty(g_CommonSelect.widget.list, "update_row", &sel);

}
#endif

static int app_fm_freq_list_del_pop_cb(PopDlgRet ret)
{
    int num = 0;
    int i = 0;
    int sel = 0;

    if(POP_VAL_OK == ret)
    {
        app_fm_freq_list_del_freq();
    }
    else
    {
        num = app_fm_freq_num_get();
        for(i = 0; i < num; i++)
        {
            sp_fm_list_data[i].del_flag = 0;
        }
        GUI_GetProperty(g_CommonSelect.widget.list, "active", &sel);
        GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
        s_fm_list_state = STATE_NORMAL;
        app_fm_freq_list_tips_update();
    }

    return 0;
}

SIGNAL_HANDLER int app_fm_freq_list_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_DOWN:
            case STBK_UP:
                ret = EVENT_TRANSFER_KEEPON;
                break;
            case STBK_PAGE_UP:
                {
                    uint32_t sel;
                    GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                    if (sel == 0)
                    {
                        sel = app_fm_freq_num_get()-1;
                        GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_UP;
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                }
                break;
            case STBK_PAGE_DOWN:
                {
                    uint32_t sel;
                    GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                    if (sel == app_fm_freq_num_get()-1)
                    {
                        sel = 0;
                        GUI_SetProperty(g_CommonSelect.widget.list, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_DOWN;
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                }
                break;
            case STBK_EXIT:
            case STBK_MENU:
                if(s_fm_list_state == STATE_DEL)
                {
                    if(app_fm_freq_list_check_del_exit() == true)
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_YES_NO;
                        pop.format = POP_FORMAT_DLG;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.exit_cb = app_fm_freq_list_del_pop_cb;
                        pop.str = STR_ID_SURE_DELETE;
                        popdlg_create(&pop);
                    }
                    else
                    {
                        s_fm_list_state = STATE_NORMAL;
                        app_fm_freq_list_tips_update();
                    }
                }
                else
                {
                    GUI_EndDialog(g_CommonSelect.widget.wnd);
                    GUI_CreateDialog(WND_FM_PLAY_INFO);
                }
                break;
            case STBK_RED:
#if DVB2IP_SERVER_SUPPORT
                if(app_dvb2ip_fm_stop_rec_tip_by_freq(0)<0)
                    return EVENT_TRANSFER_STOP;
#endif
                if(s_fm_list_state == STATE_NORMAL)
                {
                    s_fm_list_state = STATE_DEL;
                    app_fm_freq_list_tips_update();
                }
                break;
            case STBK_GREEN:
                if(s_fm_list_state == STATE_DEL)
                {
                    int sel = 0;
                    GUI_GetProperty(g_CommonSelect.widget.list, "active", &sel);
                    app_fm_freq_list_set_all_del_flag();
                    GUI_SetProperty(g_CommonSelect.widget.list, "update_all", NULL);
                    GUI_SetProperty(g_CommonSelect.widget.list, "active", &sel);
                }
#if FM_FREQ_NAME_SUPPORT
                else
                {
                    static PopKeyboard keyboard;
                    int sel=0;

                    GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                    memset(&keyboard, 0, sizeof(PopKeyboard));
                    keyboard.in_name    = sp_fm_list_data[sel].name;
                    keyboard.max_num    = FM_FREQ_NAME_LEN - 1;
                    keyboard.out_name   = NULL;
                    keyboard.change_cb  = NULL;
                    keyboard.release_cb = fm_list_keyboard_release_cb;
                    keyboard.usr_data   = NULL;
                    keyboard.pos.x      = APP_WND_FIND_KB_POS_X;
                    keyboard.pos.y      = APP_WND_FIND_KB_POS_Y;
                    multi_language_keyboard_create(&keyboard);
                }
#endif
                break;
            case STBK_OK:
                if(s_fm_list_state == STATE_NORMAL)
                    app_fm_freq_list_play();
                else
                {
                    int sel = 0;
                    GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
                    app_fm_freq_list_set_del_flag(sel);
                    GUI_SetProperty(g_CommonSelect.widget.list, "update_row", &sel);
                }
                break;
        }
    }
    return ret;
}

void app_create_fm_freq_list(void)
{
    CommonSelectParas paras = {0};

    GUI_EndDialog(g_CommonSelect.widget.wnd);
    paras.choice = COMMON_SELECT_TITLE_ONLY;
    paras.title = "FM List";
    paras.tip_strs.str_red = STR_ID_DELETE;
#if FM_FREQ_NAME_SUPPORT
    paras.tip_strs.str_green = STR_ID_RENAME;
#endif
    paras.signal.create = app_fm_freq_list_create;
    paras.signal.destroy = app_fm_freq_list_destroy;
    paras.signal.keypress = app_fm_freq_list_keypress;
    paras.signal.got_focus = app_fm_freq_list_got_focus;
    paras.signal.list_change = app_fm_freq_list_list_change;
    paras.signal.list_get_total = app_fm_freq_list_list_get_total;
    paras.signal.list_get_data = app_fm_freq_list_list_get_data;
    paras.signal.list_keypress = app_fm_freq_list_list_keypress;
    app_common_select_create_dialog(&paras);

    return;
}

#endif
