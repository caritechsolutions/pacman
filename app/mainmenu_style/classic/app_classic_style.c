#include "app.h"
#if THEME_CLASSIC_PURPLE || THEME_SKY_BLUE || THEME_CLASSIC_DARK || THEME_CLASSIC_BLUE
#include "app_windows.h"
#include "app_main_menu.h"
#include "app_main_menu_style.h"
#include "app_main_menu_exapi.h"

#define WIDGET_NAME_OPT	        "wnd_main_menu_item_listview"
#define CANVAS_MAIN_ITEM	    "canvas_main_item"
#define WIDGET_NAME_TITLE	    "text_main_menu_title"
#define WIDGET_ITEM_NAME_DOWN   "img_main_item_menu_down"
#define S_ARROW_L               "s_arrow_l"
#define S_ARROW_R               "s_arrow_r"

static char* s_MenuTitle[] =
{
    "btn_main_menu_title1",
    "btn_main_menu_title2",
    "btn_main_menu_title3",
    "btn_main_menu_title4",
    "btn_main_menu_title5",
    "btn_main_menu_title6",
    "btn_main_menu_title7"
};

extern MainMenuStyle* MainMenuStyle_DataGet(void);
extern int* ClassificationChange_DataGet(int sel);
extern int ClassificationChange_SizeGet(void);
extern char** MainMenuMainTitle_DataGet(int sel);
extern int MainMenuMainTitle_SizeGet(void);
extern OperationLadder* OperationLadder_DataGet(int sel);
extern int OperationLadder_SizeGet(void);
extern unsigned int* MainMenuSubSize_DataGet(int sel);
extern int MainMenuSubSize_SizeGet(void);
extern char** MainMenuSubTitle_DataGet(int sel);
extern int MainMenuSubSize_SizeGet(void);
extern MainMenuSetting* MainMenuSetting_DataGet(int sel);
extern int MainMenuSetting_SizeGet(void);


static unsigned int s_CurMenu = 0;

static int _setting_item_keypress(unsigned int subsel, int key)
{
    ItemSetting *setting = NULL;
    int ret = EVENT_TRANSFER_KEEPON;
    MainMenuSetting *main_menu_setting = MainMenuSetting_DataGet(subsel);

    if(MAIN_MENU_SETTING == main_menu_setting->type)
    {
        setting = &main_menu_setting->u.item_s;
        if(setting->keypress)
        {
            gxapp_exapi_set_proc_uiid(setting->appui_id);
            ret = setting->keypress(key);
        }
    }

    return ret;
}

static bool _check_key(int key, int *key_arry, int arry_size)
{
    int i = 0;

    if(0 == arry_size || NULL == key_arry)
        return false;

    for(i = 0; i < arry_size; i++)
    {
        if(key == key_arry[i])
            return true;
    }

    return false;
}

static void _main_menu_hide_fun(int key)
{
    gxapp_exapi_mainmenu_event_key_check(key);

    GUI_SetProperty(WIDGET_NAME_OPT,"update_all",NULL);
}

static void _main_item_menu_create(void)
{
    int pos_x = 0;
    MainMenuStyle *main_menu_style = MainMenuStyle_DataGet();
    const char *focus_widget = GUI_GetFocusWidget();
    OperationLadder *ladder = OperationLadder_DataGet(0);

    if(GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS)
    {
        if(GXCORE_ERROR == GUI_CheckDialog(WND_MAIN_MENU_ITEM))
            GUI_CreateDialog(WND_MAIN_MENU_ITEM);

        if(true == main_menu_style->item_move)
        {
            pos_x = mainmenu_item_offset_get() + s_CurMenu*mainmenu_item_length_get();
            GUI_SetProperty(WND_MAIN_MENU_ITEM, "move_window_x", &pos_x);
        }

        if(ladder->focus_widget && focus_widget && strcmp(ladder->focus_widget, focus_widget))
            GUI_SetFocusWidget(ladder->focus_widget);

        if(mainmenu_item_arrow_exist_get())
            GUI_SetProperty(WIDGET_ITEM_NAME_DOWN,"state","hide");
    }
}

static int _classic_style_create(void)
{
    int value = 0;
    MainMenuStyle *main_menu_style = MainMenuStyle_DataGet();

    if(false == main_menu_style->default_last_sub)
        s_CurMenu = main_menu_style->default_sub;

    GUI_SetProperty(s_MenuTitle[s_CurMenu],"state","disable");
    GUI_SetProperty(WIDGET_NAME_TITLE, "string", *MainMenuMainTitle_DataGet(s_CurMenu));

	GUI_SetProperty(WND_MAIN_MENU, "back_sync", "true");
#if THEME_CLASSIC_DARK      //270469
	GUI_SetInterface("flush",NULL);
    _main_item_menu_create();
#else
    _main_item_menu_create();
	GUI_SetInterface("flush",NULL);
#endif

    GUI_SetProperty(WIDGET_NAME_OPT, "update_all", NULL);

    GUI_SetProperty(WIDGET_NAME_OPT, "select", &value);
    GUI_SetInterface("video_off",NULL);
    return EVENT_TRANSFER_STOP;
}

static int _classic_style_destroy(void)
{
    MainMenuStyle *main_menu_style = MainMenuStyle_DataGet();

    gxapp_exapi_mainmenu_destory();
    if(false == main_menu_style->default_last_sub)
        s_CurMenu = main_menu_style->default_sub;
    return EVENT_TRANSFER_STOP;
}

static int _classic_style_keypress(int key)
{
    int ret = EVENT_TRANSFER_STOP;
    OperationLadder *ladder = OperationLadder_DataGet(0);

    if(VK_BOOK_TRIGGER == key)
    {
        GUI_EndDialog(WND_MAIN_MENU_ITEM);
        GUI_EndDialog(WND_MAIN_MENU);
        GUI_SetProperty(WND_MAIN_MENU, "draw_now", NULL);
    }
    else if(true == _check_key(key, ladder->back_ladder_key, ladder->back_key_num))
    {
        GUI_EndDialog(WND_MAIN_MENU_ITEM);
        GUI_EndDialog(WND_MAIN_MENU);

        gxapp_exapi_mainmenu_exit();
    }

    return ret;
}

static int _classic_style_media_entry(void)
{
#if THEME_CLASSIC_DARK
    unsigned int i = 0;
    unsigned int num = MainMenuSubSize_SizeGet();
    for (i = 0; i < num; i++)
    {
        if(strcmp(STR_ID_MEDIA_CENTER, *MainMenuMainTitle_DataGet(i)) == 0)
            break;
    }
    s_CurMenu = i;

    if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
    {
        GUI_CreateDialog(WND_MAIN_MENU);
    }
#elif THEME_SKY_BLUE || THEME_CLASSIC_PURPLE
    gxapp_exapi_mainmenu_media_entry();
#endif

    return 0;
}

static uint32_t _main_menu_get_subsize(void)
{
    unsigned int mainsel = s_CurMenu;
    unsigned int subsel = 0, i = 0, num = 0;
    unsigned int total = *MainMenuSubSize_DataGet(mainsel);

    while(mainsel > 0)
    {
        mainsel--;
        subsel += *MainMenuSubSize_DataGet(mainsel);
    }

    num = *MainMenuSubSize_DataGet(s_CurMenu);
    for(i = subsel; i < subsel + num; i++)
    {
        CheckFunc check = NULL;
        MainMenuSetting *main_menu_setting = MainMenuSetting_DataGet(i);

        if(MAIN_MENU_ENTRY == main_menu_setting->type)
        {
            check = main_menu_setting->u.item_e.check;
            gxapp_exapi_set_proc_uiid(main_menu_setting->u.item_e.appui_id);
        }
        else
        {
            check = main_menu_setting->u.item_s.check;
            gxapp_exapi_set_proc_uiid(main_menu_setting->u.item_s.appui_id);
        }

        if(check && 0 == check())
            total--;
    }

    return total;
}

static void _main_menu_arrow_set(void)
{
    int item_sel = 0;
    uint32_t item_total = _main_menu_get_subsize();

    if(!mainmenu_item_arrow_exist_get())
        return;
    GUI_GetProperty(WIDGET_NAME_OPT, "select", &item_sel);
    if((item_sel == (item_total - 1)) || (item_total <= mainmenu_item_listview_visual_get()))
    {
        GUI_SetProperty(WIDGET_ITEM_NAME_DOWN,"state","hide");
    }
    else
    {
        GUI_SetProperty(WIDGET_ITEM_NAME_DOWN,"state","show");
    }
}

static int _main_menu_item_move(int key)
{
    int pos_x = 0;
    int value = 0;
    int before_menu = 0;
    int index = 0;
    int need_to_move = 0;
    int move_level = mainmenu_item_move_level_get();
    int move_step = mainmenu_item_move_step_get();
    int move_length = mainmenu_item_length_get();
    int move_offset = mainmenu_item_offset_get();
    MainMenuStyle *main_menu_style = MainMenuStyle_DataGet();
    int mainmenu_total = MainMenuSubSize_SizeGet();
    int key_left = *ClassificationChange_DataGet(0);
    int key_right = *ClassificationChange_DataGet(1);
    const char *focus_widget = GUI_GetFocusWidget();
    OperationLadder *ladder = OperationLadder_DataGet(0);

    if(NULL == focus_widget || NULL == ladder->focus_widget
            || (key != key_left && key != key_right)
            || strcmp(focus_widget, ladder->focus_widget))
        return EVENT_TRANSFER_KEEPON;

    GUI_SetProperty(s_MenuTitle[s_CurMenu], "state", "enable");
    if(key_right == key)
    {
        if((mainmenu_total - 1) == s_CurMenu)
        {
            s_CurMenu = 0;
        }
        else
        {
            s_CurMenu++;
            need_to_move = 1;
        }
        before_menu = (s_CurMenu == 0) ? (mainmenu_total - 1) : (s_CurMenu - 1);
    }
    else
    {
        if(0 == s_CurMenu)
        {
            s_CurMenu = mainmenu_total - 1;
        }
        else
        {
            s_CurMenu--;
            need_to_move = 1;
        }
        before_menu = (s_CurMenu == (mainmenu_total - 1)) ? 0 : (s_CurMenu + 1);
    }

    GUI_SetProperty(s_MenuTitle[s_CurMenu], "state", "disable");
    GUI_SetProperty(WIDGET_NAME_TITLE, "string", *MainMenuMainTitle_DataGet(s_CurMenu));

    if(true == main_menu_style->item_move)
    {
        if(need_to_move)
        {
            for(index = 0; index < move_level; index++)
            {
                unsigned int start = move_offset + before_menu * move_length;
                pos_x = (STBK_RIGHT == key) ? (start + move_step*(index+1)) : (start - (move_step*(index+1)));
                GUI_SetProperty(WND_MAIN_MENU_ITEM, "move_window_x", &pos_x);
                GUI_SetInterface("flush",NULL);
            }
        }
        else
        {
            pos_x = move_offset+((s_CurMenu)*move_length);
            GUI_SetProperty(WND_MAIN_MENU_ITEM, "move_window_x", &pos_x);
            GUI_SetInterface("flush",NULL);
        }
    }

    _main_menu_arrow_set();

    GUI_SetProperty(WIDGET_NAME_OPT, "select", &value);
    GUI_SetProperty(WIDGET_NAME_OPT, "update_all", NULL);
    if(ladder->focus_widget)
        GUI_SetFocusWidget(ladder->focus_widget);
    return EVENT_TRANSFER_STOP;
}

static unsigned int _convert_to_submenu_sel(unsigned int sel)
{
    unsigned int subsel = 0, i = 0, show = 0, total = 0;
    unsigned int mainsel = s_CurMenu;

    while(mainsel > 0)
    {
        mainsel--;
        subsel += *MainMenuSubSize_DataGet(mainsel);
    }

    total = *MainMenuSubSize_DataGet(s_CurMenu);
    for(i = subsel; i < subsel + total; i++)
    {
        CheckFunc check = NULL;
        MainMenuSetting *main_menu_setting = MainMenuSetting_DataGet(i);

        if(MAIN_MENU_ENTRY == main_menu_setting->type)
        {
            check = main_menu_setting->u.item_e.check;
            gxapp_exapi_set_proc_uiid(main_menu_setting->u.item_e.appui_id);
        }
        else
        {
            check = main_menu_setting->u.item_s.check;
            gxapp_exapi_set_proc_uiid(main_menu_setting->u.item_s.appui_id);
        }

        if(!check || (check && check() == 1))
            show++;

        if(show == sel + 1)
            break;
    }

    return i;
}

SIGNAL_HANDLER int app_main_menu_item_create(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_main_menu_item_got_focus(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_main_item_canvas_show(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_main_menu_item_keypress(const char* widgetname, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    OperationLadder *cur_ladder = NULL;
    OperationLadder *next_ladder = NULL;

    event = (GUI_Event *)usrdata;
    if(event->type != GUI_KEYDOWN)
        return ret;

    cur_ladder = OperationLadder_DataGet(0);

    if(OperationLadder_SizeGet() > 1)
        next_ladder = OperationLadder_DataGet(1);

    if(true == _check_key(event->key.sym, cur_ladder->next_ladder_key, cur_ladder->next_key_num))
    {
        int sel = 0;
        if(next_ladder && next_ladder->focus_widget && strcmp(next_ladder->focus_widget, GUI_GetFocusWidget()))
        {
            GUI_SetFocusWidget(next_ladder->focus_widget);
            GUI_SetProperty(WIDGET_NAME_OPT, "select", &sel);
        }
    }
    else if(true == _check_key(event->key.sym, cur_ladder->back_ladder_key, cur_ladder->back_key_num)
            || VK_BOOK_TRIGGER == event->key.sym)
    {
        GUI_SendEvent(WND_MAIN_MENU, event);
    }
    else
    {
        _main_menu_item_move(event->key.sym);
    }

    return ret;
}

SIGNAL_HANDLER int listview_main_menu_opt_get_total(const char* widgetname, void *usrdata)
{
    return _main_menu_get_subsize();
}

SIGNAL_HANDLER int listview_main_menu_opt_get_data(const char* widgetname, void *usrdata)
{
    int list_value = 0;
    ListItemPara* item = NULL;
    unsigned int subsel = 0;
    ItemSetting *setting = NULL;
    const char *focus_widget = NULL;
    MainMenuSetting *main_menu_setting = NULL;

    focus_widget = GUI_GetFocusWidget();
    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;

    //col-0 subMenuTitle
    GUI_GetProperty(WIDGET_NAME_OPT, "select", &list_value);
    subsel = _convert_to_submenu_sel(item->sel);
    item->string = *MainMenuSubTitle_DataGet(subsel);
    item->image = NULL;

    //col-1 arrow
    item = item->next;
    if(item)
    {
        item->image = NULL;
        item->string = NULL;

        if(focus_widget && 0 == strcmp(focus_widget, WIDGET_NAME_OPT))
        {
            main_menu_setting = MainMenuSetting_DataGet(subsel);
            if(MAIN_MENU_ENTRY == main_menu_setting->type && list_value == item->sel)
                item->image = S_ARROW_R;
            else if(MAIN_MENU_SETTING == main_menu_setting->type && list_value == item->sel)
                item->image = S_ARROW_L;
        }
    }
    else
    {
        return GXCORE_SUCCESS;
    }

    //col-2
    item = item->next;
    if(item)
    {
        item->string = NULL;
        item->image = NULL;

        main_menu_setting = MainMenuSetting_DataGet(subsel);
        if(MAIN_MENU_SETTING == main_menu_setting->type)
        {
            setting = &main_menu_setting->u.item_s;
            if (setting->content)
            {
                gxapp_exapi_set_proc_uiid(setting->appui_id);
                item->string = setting->content();
            }
        }
    }
    else
    {
        return GXCORE_SUCCESS;
    }

    //col-3
    item = item->next;
    if(item)
    {
        item->image = NULL;
        item->string = NULL;

        if(focus_widget && 0 == strcmp(focus_widget, WIDGET_NAME_OPT))
        {
            main_menu_setting = MainMenuSetting_DataGet(subsel);
            if(MAIN_MENU_SETTING == main_menu_setting->type && list_value == item->sel)
                item->image = S_ARROW_R;
        }
    }

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int listview_main_menu_opt_keypress(const char* widgetname, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    static unsigned int subsel;
    GUI_Event *event = NULL;
    int i = 0, ladder_size = 0;
    int item_sel = 0;
    OperationLadder *cur_ladder = NULL;
    OperationLadder *pre_ladder = NULL;
    MainMenuSetting *main_menu_setting = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN != event->type)
        return ret;

    GUI_GetProperty(WIDGET_NAME_OPT, "select", &item_sel);
    subsel = _convert_to_submenu_sel(item_sel);

    ladder_size = OperationLadder_SizeGet();

    for(i = 0; i < ladder_size; i++)
    {
        if(0 == strcmp(OperationLadder_DataGet(i)->focus_widget, WIDGET_NAME_OPT))
        {
            cur_ladder = OperationLadder_DataGet(i);
            break;
        }
    }

    if(NULL == cur_ladder)
        return EVENT_TRANSFER_KEEPON;

    if(i > 0)
        pre_ladder = OperationLadder_DataGet(i-1);

    _main_menu_hide_fun(find_virtualkey_ex(event->key.scancode, event->key.sym));
    ret = _setting_item_keypress(subsel, event->key.sym);

    main_menu_setting = MainMenuSetting_DataGet(subsel);
    if(_check_key(event->key.sym, cur_ladder->next_ladder_key, cur_ladder->next_key_num)
            && MAIN_MENU_ENTRY == main_menu_setting->type)
    {
        ItemEntry *entry = &main_menu_setting->u.item_e;

        gxapp_exapi_set_entry_uiid(entry->appui_id);
        ret = app_submenu_enter(entry->attr, entry->create);
    }
    else if(_check_key(event->key.sym, cur_ladder->back_ladder_key, cur_ladder->back_key_num))
    {
        if(pre_ladder && pre_ladder->focus_widget)
            GUI_SetFocusWidget(pre_ladder->focus_widget);
        else
            GUI_SendEvent(WND_MAIN_MENU, event);

        ret = EVENT_TRANSFER_STOP;
    }

    return ret;
}

SIGNAL_HANDLER int listview_main_menu_opt_change(const char* widgetname, void *usrdata)
{
    _main_menu_arrow_set();

    return EVENT_TRANSFER_STOP;
}


MainmenuStyleOps classicops = {
    .create = _classic_style_create,
    .destroy = _classic_style_destroy,
    .keypress = _classic_style_keypress,
    .media_entry = _classic_style_media_entry,
};
#endif
