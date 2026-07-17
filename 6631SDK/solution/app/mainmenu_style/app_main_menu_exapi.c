#include "app.h"
#include "app_module.h"
#include "module/channel_edit.h"
#include "full_screen.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#include "netapps/app_netapps_pop.h"
#include "app_main_menu_exapi.h"
#include "app_main_menu.h"

#if APP_MAIN_MENU_OBJ_SUPPORT
static unsigned int s_mainmenu_entry_uiid = 0 ;
static unsigned int s_mainmenu_proc_uiid = 0 ;
#endif

int gxapp_exapi_mainmenu_destory(void)
{
	g_AppChOps.update_prog_num();
	return 0 ;
}

int gxapp_exapi_mainmenu_check_end(void)
{
	if(app_ott_config_get() == true)
		return 1 ;
	return 0 ;
}

int gxapp_exapi_mainmenu_exit(void)
{
    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
    if(g_AppChOps.get_list_total() != 0)
    {
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_resume_normal_play())
#endif
        {
            g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(g_AppPlayOps.normal_play.play_count);
            app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
            g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

            if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
                g_AppFullArb.tip = FULL_STATE_LOCKED;
            else
                g_AppFullArb.tip = FULL_STATE_DUMMY;

#if BOUQUET_SUPPORT
            g_AppBat.start(&g_AppBat);
#endif
        }
    }

    return 0 ;
}

int gxapp_exapi_mainmenu_event_key_check(int key)
{
#if NETAPPS_SUPPORT
	extern int app_netapps_event_key_check(int key);
	app_netapps_event_key_check(key);
#endif
	return 0 ;
}

int gxapp_exapi_mainmenu_net_connect_check(void)
{
	return app_pop_net_connect_check();
}

int gxapp_exapi_mainmenu_media_entry(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
	gxapp_exapi_mainmenu_sub_create(APPUI_ENTRY_MEDIA_FILE);
#else
	extern void app_media_file_menu_exec(void);
    app_media_file_menu_exec();
#endif
    return 0 ;
}

int   gxapp_exapi_passwd_dlg_create(unsigned int handle)
{
	PasswdDlg *pwdlg = NULL;
    pwdlg = (PasswdDlg *)handle;
    if(pwdlg)
    {
        if(passwd_dlg_create(pwdlg) == GXCORE_SUCCESS)
        {
            return 0;
        }
    }
    return 1 ;
}

int   gxapp_exapi_passwd_dlg_offset_x_get(void)
{
	return APP_POP_PASSWD_POS_X ;
}

int   gxapp_exapi_passwd_dlg_offset_y_get(void)
{
	return APP_POP_PASSWD_POS_Y ;
}

int*  gxapp_exapi_config_get_int(const char *key, int *buf, int dvalue)
{
    return GxBus_ConfigGetInt(key, buf, dvalue);
}

void  gxapp_exapi_timer_str_get(char *tm_str, unsigned int str_len)
{
	g_AppTime.time_get(&g_AppTime, tm_str, str_len);
}

void gxapp_exapi_set_entry_uiid(int id)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
    s_mainmenu_entry_uiid = id ;
#endif
}

int gxapp_exapi_get_entry_uiid(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
    return s_mainmenu_entry_uiid ;
#else
	return 0 ;
#endif
}

void  gxapp_exapi_set_proc_uiid(int id)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
    s_mainmenu_proc_uiid = id ;
#endif
}

int   gxapp_exapi_get_proc_uiid(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
    return s_mainmenu_proc_uiid ;
#else
	return 0 ;
#endif
}
