#include "app.h"
#include "app_module.h"
#if THEME_GRID_PURPLE
#include "full_screen.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_windows.h"
#include "app_main_menu_style.h"
#include "app_main_menu.h"
#include "channel_common.h"
#include "app_common_2nd_menu.h"
#include "app_main_menu_exapi.h"

extern int MainMenuSubSize_SizeGet(void);
extern char* MainMenuSubTitle_Get(int sel);
extern ItemEntry* MainMenuEntryItem_Get(int sel);
extern MainmenuStyleItem* MainMenuStyleItem_Get(int sel);
extern int MainMenuDefaultFocus_Get(void);

static uint32_t s_grid_sel = 0;
static event_list *sp_MainMenuTimer = NULL;

static void _grid_setfocus_button(uint32_t index)
{
    uint32_t count = MainMenuSubSize_SizeGet();
    char focus_wgt[40] = {0};
    if(index < 0 || index >= count)
        return;
    sprintf(focus_wgt,"btn_main_menu_opt%d",  index+1);
    GUI_SetFocusWidget(focus_wgt);
    return;
}

static void _grid_ui_init(void)
{
    int i = 0;
    int total = 0;
    MainmenuStyleItem* style_item = NULL;
    total = MainMenuSubSize_SizeGet();
    char focus_wgt[40] = {0};
    for(i = 0; i < total; i++)
    {
        style_item = MainMenuStyleItem_Get(i);
        sprintf(focus_wgt,"btn_main_menu_opt%d",  i+1);
        GUI_SetProperty(focus_wgt, "focus_img", (void*)style_item->focus_image);
        GUI_SetProperty(focus_wgt, "unfocus_img", (void*)style_item->unfocus_image);
        if(style_item->state == true)
            GUI_SetProperty(focus_wgt, "string", (void*)MainMenuSubTitle_Get(i));
    }
}

static int _main_menu_timeout_cb(void* usrdata)
{
    char time_str[24] = {0};
    char time_str_temp[6] = {0};

    gxapp_exapi_timer_str_get(time_str, sizeof(time_str));

    // time
    if(strlen(time_str) > 24)
    {
        app_log_error("\nlenth error\n");
        return 1;
    }
    memcpy(time_str_temp,&time_str[strlen(time_str)-5],5);// 00:00
    GUI_SetProperty("text_main_menu_time", "string", time_str_temp);

    // ymd
    time_str[strlen(time_str)-5] = '\0';
    GUI_SetProperty("text_main_menu_ymd", "string", time_str);

    return 0;
}

static void _grid_main_menu_timer_start(void)
{
    _main_menu_timeout_cb(NULL);
    if (reset_timer(sp_MainMenuTimer) != 0)
    {
        sp_MainMenuTimer = create_timer(_main_menu_timeout_cb,10*SEC_TO_MILLISEC, NULL, TIMER_REPEAT);
    }
}

static void _grid_main_menu_timer_stop(void)
{
    remove_timer(sp_MainMenuTimer);
    sp_MainMenuTimer = NULL;
}

static int _grid_style_create(void)
{
    uint32_t index = 0;
    _grid_ui_init();
    index = MainMenuDefaultFocus_Get();
    if(index == 0xff)
    {
        _grid_setfocus_button(s_grid_sel);
    }
    else
    {
        s_grid_sel = index;
        _grid_setfocus_button(index);
    }
    _grid_main_menu_timer_start();
    return EVENT_TRANSFER_STOP;
}

static int _grid_style_destroy(void)
{
    _grid_main_menu_timer_stop();
    gxapp_exapi_mainmenu_destory();
    return EVENT_TRANSFER_STOP;
}

static int _grid_style_got_focus(void)
{
    _grid_main_menu_timer_start();
    return EVENT_TRANSFER_STOP;
}

static int _grid_style_lost_focus(void)
{
    _grid_main_menu_timer_stop();
    return EVENT_TRANSFER_STOP;
}

static int _grid_style_keypress(int key)
{
    MainmenuStyleItem* style_item = NULL;
    ItemEntry* item_entry = NULL;
    int total = 0;
    total = MainMenuSubSize_SizeGet();

    switch(key)
    {
        case VK_BOOK_TRIGGER:
            GUI_EndDialog("after wnd_full_screen");
            break;
        case STBK_OK:
            item_entry = MainMenuEntryItem_Get(s_grid_sel);
            gxapp_exapi_set_entry_uiid(item_entry->appui_id);
            app_submenu_enter(item_entry->attr,item_entry->create);
            break;
        case STBK_UP:
            {
                style_item = MainMenuStyleItem_Get(s_grid_sel);
                if(style_item->up == 0xff)
                {
                    if(s_grid_sel == 0)
                    {
                        s_grid_sel = total-1;
                    }
                    else
                    {
                        s_grid_sel--;
                    }
                }
                else
                {
                    s_grid_sel = style_item->up;
                }
                _grid_setfocus_button(s_grid_sel);
            }
            break;
        case STBK_DOWN:
            {
                style_item = MainMenuStyleItem_Get(s_grid_sel);
                if(style_item->down == 0xff)
                {
                    if(s_grid_sel == (total-1))
                    {
                        s_grid_sel = 0;
                    }
                    else
                    {
                        s_grid_sel++;
                    }
                }
                else
                {
                    s_grid_sel = style_item->down;
                }
                _grid_setfocus_button(s_grid_sel);
            }
            break;
        case STBK_LEFT:
            {
                style_item = MainMenuStyleItem_Get(s_grid_sel);
                if(style_item->left == 0xff)
                {
                    if(s_grid_sel == 0)
                    {
                        s_grid_sel = total-1;
                    }
                    else
                    {
                        s_grid_sel--;
                    }
                }
                else
                {
                    s_grid_sel = style_item->left;
                }
                _grid_setfocus_button(s_grid_sel);
            }
            break;
        case STBK_RIGHT:
            {
                style_item = MainMenuStyleItem_Get(s_grid_sel);
                if(style_item->right == 0xff)
                {
                    if(s_grid_sel == (total-1))
                    {
                        s_grid_sel = 0;
                    }
                    else
                    {
                        s_grid_sel++;
                    }
                }
                else
                {
                    s_grid_sel = style_item->right;
                }
                _grid_setfocus_button(s_grid_sel);
            }
            break;
        case STBK_EXIT:
        case STBK_MENU:
            {
                if(gxapp_exapi_mainmenu_check_end())
                    break;
                GUI_EndDialog(WND_MAIN_MENU);
                gxapp_exapi_mainmenu_exit();
            }
            break;
    }
    return EVENT_TRANSFER_STOP;
}

static int _classic_media_entry(void)
{
    gxapp_exapi_mainmenu_media_entry();
    return 0;
}

MainmenuStyleOps gridops = {
    .create = _grid_style_create,
    .destroy = _grid_style_destroy,
    .keypress = _grid_style_keypress,
    .got_focus = _grid_style_got_focus,
    .lost_focus = _grid_style_lost_focus,
    .media_entry = _classic_media_entry,
};
#endif
