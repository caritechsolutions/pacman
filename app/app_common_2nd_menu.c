#include "app.h"
#include "app_windows.h"
#include "app_common_2nd_menu.h"
#include "app_main_menu.h"

#define S_ARROW_L               "s_arrow_l"
#define S_ARROW_R               "s_arrow_r"

extern int app_submenu_enter(unsigned int attr, CreateFunc create);
extern void app_adult_video_control(int key);

Common2ndEntry g_Common2ndEntry[COMMON_2ND_MAX_COUNT];
int g_Common2ndEntryCount = 0;

int _lock_passwd_cb(PopPwdRet *ret, void *usr_data)
{
    int select = *(int*)usr_data;

    if(ret->result == PASSWD_OK)
    {
        if(g_Common2ndEntry[select].create)
            g_Common2ndEntry[select].create();
    }
    else if (ret->result == PASSWD_ERROR)
    {
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg.usr_data = usr_data;
        passwd_dlg.passwd_key_msg = KEY_MSG_STOP;
        passwd_dlg.passwd_exit_cb = _lock_passwd_cb;
        passwd_dlg.pos.x = APP_POP_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_POP_PASSWD_POS_Y;
        passwd_dlg_create(&passwd_dlg);
    }

    return 0;
}

SIGNAL_HANDLER int listview_common_2nd_menu_keypress(const char* widget, void *usrdata)
{
    static int sel = 0;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;

    if(event->type != GUI_KEYDOWN)
        return EVENT_TRANSFER_KEEPON;

    GUI_GetProperty(LISTVIEW_COMMON2ND_MENU, "select", &sel);

    switch(event->key.sym)
    {
        case STBK_RIGHT:
        case STBK_OK:
            if(sel < g_Common2ndEntryCount)
            {
                app_submenu_enter(g_Common2ndEntry[sel].attr, g_Common2ndEntry[sel].create);
            }
            break;

        default:
            break;
    }

    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int listview_common_2nd_menu_get_total(const char *widget, void *usrdata)
{
    return g_Common2ndEntryCount;
}

SIGNAL_HANDLER int app_common_2nd_menu_create(const char *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_2nd_menu_destroy(const char *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_2nd_menu_keypress(const char *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;

    if(event->type != GUI_KEYDOWN)
        return EVENT_TRANSFER_KEEPON;

    switch(event->key.sym)
    {
        case VK_BOOK_TRIGGER:
            GUI_EndDialog("after wnd_full_screen");
            break;

        case STBK_EXIT:
        case STBK_MENU:
            GUI_EndDialog(WND_COMMON_2ND_MENU);
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
            app_adult_video_control(event->key.sym);
            break;

        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int listview_common_2nd_menu_get_data(const char *widget, void *usrdata)
{
    int sel = 0;
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;

    GUI_GetProperty(LISTVIEW_COMMON2ND_MENU, "select", &sel);

    if(item->sel >= g_Common2ndEntryCount)
        return GXCORE_ERROR;

    //col-0
    item->x_offset = 0;
    item->image = NULL;
    item->string = g_Common2ndEntry[item->sel].title;

    //col-1
    item = item->next;
    item->string = NULL;
    item->image = NULL;

    if(sel == item->sel && 0 == strcasecmp(LISTVIEW_COMMON2ND_MENU, GUI_GetFocusWidget()))
        item->image = S_ARROW_R;

    //col-2
    item = item->next;
    item->image = NULL;
    item->string = NULL;

    //col-3
    item = item->next;
    item->image = NULL;
    item->string = NULL;
    return GXCORE_SUCCESS;
}
