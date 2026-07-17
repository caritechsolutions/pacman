
#include "app_config.h"
#include <string.h>
#include <stdio.h>
#include "app_module.h"
#include "app_main_menu_theme_type.h"
#include "app_main_menu_theme_config.h"

#define APP_MAIN_MENU_OBJ_VERSION "1.0.0"

extern int app_main_menu_create(const char* widgetname, void *usrdata);
extern int app_main_menu_destroy(const char* widgetname, void *usrdata);
extern int app_main_menu_keypress(const char* widgetname, void *usrdata);
extern int app_main_menu_lost_focus(const char* widgetname, void *usrdata);
extern int app_main_menu_got_focus(const char* widgetname, void *usrdata);

extern int app_main_item_canvas_show(const char* widgetname, void *usrdata);
extern int app_main_menu_item_create(const char* widgetname, void *usrdata);
extern int app_main_menu_item_got_focus(const char* widgetname, void *usrdata);
extern int app_main_menu_item_keypress(const char* widgetname, void *usrdata);
extern int listview_main_menu_opt_change(const char* widgetname, void *usrdata);
extern int listview_main_menu_opt_get_data(const char* widgetname, void *usrdata);
extern int listview_main_menu_opt_get_total(const char* widgetname, void *usrdata);
extern int listview_main_menu_opt_keypress(const char* widgetname, void *usrdata);

static int _gxobj_mainmenu_set_signal_connect(void)
{
	int ret  = 0;

	printf("gxobj signal set \n");

	ret |= GUI_SignalConnect("app_main_menu_create",app_main_menu_create);
	ret |= GUI_SignalConnect("app_main_menu_destroy",app_main_menu_destroy);
	ret |= GUI_SignalConnect("app_main_menu_keypress",app_main_menu_keypress);
	ret |= GUI_SignalConnect("app_main_menu_lost_focus",app_main_menu_lost_focus);
	ret |= GUI_SignalConnect("app_main_menu_got_focus",app_main_menu_got_focus);
	
#if THEME_CLASSIC_DARK || THEME_CLASSIC_BLUE ||  THEME_CLASSIC_PURPLE || THEME_SKY_BLUE

	ret |= GUI_SignalConnect("app_main_item_canvas_show",app_main_item_canvas_show);

	ret |= GUI_SignalConnect("app_main_menu_item_create",app_main_menu_item_create);
	ret |= GUI_SignalConnect("app_main_menu_item_got_focus",app_main_menu_item_got_focus);
	ret |= GUI_SignalConnect("app_main_menu_item_keypress",app_main_menu_item_keypress);
	ret |= GUI_SignalConnect("listview_main_menu_opt_get_total",listview_main_menu_opt_get_total);
	ret |= GUI_SignalConnect("listview_main_menu_opt_get_data",listview_main_menu_opt_get_data);
	ret |= GUI_SignalConnect("listview_main_menu_opt_keypress",listview_main_menu_opt_keypress);
	ret |= GUI_SignalConnect("listview_main_menu_opt_change",listview_main_menu_opt_change);
#else
	printf("gxobj signal grid \n");
#endif

	printf("gxobj signal end ret = %d \n",ret);

	return ret ;
}

static int gxobj_inside_mainmenu_signal_event_set(void)
{
	int ret = 0 ;

	ret = _gxobj_mainmenu_set_signal_connect();

	return ret ;
}


#ifdef DLL_OS_LINUX_SUPPORT
__attribute__((visibility("default"))) int gxobj_api_mainmenu_signal_init(void);
__attribute__((visibility("default"))) int gxobj_api_mainmenu_sys_init(AppThemeConfig *pConfig);
#endif

int gxobj_api_mainmenu_signal_init(void)
{
	gxobj_inside_mainmenu_signal_event_set();
	return 0 ;
}

int gxobj_api_mainmenu_sys_init(AppThemeConfig *pConfig)
{
	if ( pConfig == NULL )
	{
		printf("gxobj pconfig para err \n");
		return -1 ;
	}

#ifdef DLL_ARCH_CSKY_SUPPORT
	printf("gxobj arch is csky \n");
#endif
#ifdef DLL_ARCH_ARM_SUPPORT
	printf("gxobj arch is arm \n");
#endif
#ifdef DLL_OS_ECOS_SUPPORT
	printf("gxobj os is ecos \n" );
#endif
#ifdef DLL_OS_LINUX_SUPPORT
	printf("gxobj os is linux \n" );
#endif

	printf("gxobj ver = %s \n",APP_MAIN_MENU_OBJ_VERSION);
	printf("gxobj build = %s %s \n",__DATE__,__TIME__);

	gxapp_main_menu_theme_set_config(pConfig);

	printf("gxobj theme type = %d \n",pConfig->type);
	printf("gxobj theme width = %d \n",pConfig->width);
	printf("gxobj theme width = %d \n",pConfig->height);

	return 0 ;
}