#include "app_default_params.h"
#include "app_pop.h"
#include "gui_core.h"
#include "app_main_menu_style.h"
#include "app_main_menu.h"
#include "app_main_menu_exapi.h"
#include "app_main_menu_theme_type.h"

#define WIDGET_NAME_OPT	        "wnd_main_menu_item_listview"

static int _submenu_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
    CreateFunc create = (CreateFunc)usr_data;

    if(ret->result == PASSWD_OK)
    {
        if(create)
            create();
    }
    else if(ret->result == PASSWD_ERROR)
    {
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg.usr_data = usr_data;
        passwd_dlg.passwd_exit_cb = _submenu_passwd_dlg_cb;
        passwd_dlg.pos.x = gxapp_exapi_passwd_dlg_offset_x_get();
        passwd_dlg.pos.y = gxapp_exapi_passwd_dlg_offset_y_get();
        gxapp_exapi_passwd_dlg_create((unsigned int)(&passwd_dlg));
    }

    return 0;
}

int app_submenu_enter(unsigned int attr, CreateFunc create)
{
    bool passwd_dlg_flag = false;

    if(attr & MENU_NETWORK_CHECK)
    {
        if(gxapp_exapi_mainmenu_net_connect_check() == GXCORE_ERROR)
        {
            return EVENT_TRANSFER_STOP;
        }
    }

    if(attr & MENU_LOCK_FORCE)
    {
        passwd_dlg_flag = true;
    }
    else if(attr & MENU_LOCK_CHECK)
    {
        int init_value = 0;
        gxapp_exapi_config_get_int(MENU_LOCK_KEY, &init_value, MENU_LOCK);
        if (init_value == 1)
            passwd_dlg_flag = true;
    }
    if(passwd_dlg_flag)
    {
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.passwd_exit_cb = _submenu_passwd_dlg_cb;
        passwd_dlg.usr_data = (void*)create;
        passwd_dlg.pos.x = gxapp_exapi_passwd_dlg_offset_x_get();
        passwd_dlg.pos.y = gxapp_exapi_passwd_dlg_offset_y_get();
        gxapp_exapi_passwd_dlg_create((unsigned int)(&passwd_dlg));
    }
    else
    {
        if(create)
            create();
    }

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_main_menu_create(const char *widget, void *usrdata)
{
    return app_mainmenu_style_create();
}

SIGNAL_HANDLER int app_main_menu_destroy(const char *widget, void *usrdata)
{
    return app_mainmenu_style_destroy();
}

SIGNAL_HANDLER int app_main_menu_keypress(const char *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    if(event->type != GUI_KEYDOWN)
        return EVENT_TRANSFER_STOP;
    return app_mainmenu_style_keypress(event->key.sym);
}

SIGNAL_HANDLER int app_main_menu_lost_focus(const char *widget, void *usrdata)
{
    return app_mainmenu_style_lost_focus();
}

SIGNAL_HANDLER int app_main_menu_got_focus(const char *widget, void *usrdata)
{
    return app_mainmenu_style_got_focus();
}

void app_main_menu_listview_update(void)
{
    GUI_SetProperty(WIDGET_NAME_OPT, "update_all", NULL);
    return;
}

void app_main_menu_media_item_create(void)
{
    app_mainmenu_style_media_entry();
    return;
}
