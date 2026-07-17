#include "app.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_windows.h"
#include "app_main_menu_style.h"

static MainmenuStyleOps s_MainmenuStyle = {0};

void app_mainmenu_style_register(MainmenuStyleOps* style)
{
    memset(&s_MainmenuStyle,0,sizeof(MainmenuStyleOps));
    memcpy(&s_MainmenuStyle,style,sizeof(MainmenuStyleOps));
}

int app_mainmenu_style_create(void)
{
    MainMenuStyleItem_Init();
    if (s_MainmenuStyle.create)
        return s_MainmenuStyle.create();
    return EVENT_TRANSFER_STOP;
}

int app_mainmenu_style_destroy(void)
{
    if (s_MainmenuStyle.destroy)
        return s_MainmenuStyle.destroy();
    return EVENT_TRANSFER_STOP;
}

int app_mainmenu_style_got_focus(void)
{
    if (s_MainmenuStyle.got_focus)
        return s_MainmenuStyle.got_focus();
    return EVENT_TRANSFER_STOP;
}

int app_mainmenu_style_lost_focus(void)
{
    if (s_MainmenuStyle.lost_focus)
        return s_MainmenuStyle.lost_focus();
    return EVENT_TRANSFER_STOP;
}

int app_mainmenu_style_keypress(int key)
{
    if (s_MainmenuStyle.keypress)
        return s_MainmenuStyle.keypress(key);
    return EVENT_TRANSFER_STOP;
}

#ifdef DLL_OS_LINUX_SUPPORT
__attribute__((visibility("default"))) int app_mainmenu_style_media_entry(void);
#endif

int app_mainmenu_style_media_entry(void)
{
    if (!s_MainmenuStyle.media_entry){
        MainMenuStyleItem_Init();
    }
    if (s_MainmenuStyle.media_entry){
        return s_MainmenuStyle.media_entry();
    }
    return EVENT_TRANSFER_STOP;
}
