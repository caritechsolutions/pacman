#include "app.h"

#if (DEMOD_DVB_C > 0)
#include "app_windows.h"
#include "app_wnd_system_setting_opt.h"

typedef enum {
    ITEM_DVBC_AUTO = 0,
    ITEM_DVBC_MANUAL,
    ITEM_DVBC_FULL,
    ITEM_DVBC_SETFRE,
    ITEM_DVBC_TOTAL
} ItemDvbcSel;

extern void app_dvbc_auto_search_entry(void);
extern void app_dvbc_manual_search_menu_exec(void);
extern void app_dvbc_set_main_frequency_menu_exec(void);
extern void app_dvbc_all_search_menu_exec(void);

static SystemSettingOpt s_dvbc_opt;
static SystemSettingItem s_dvbc_item[ITEM_DVBC_TOTAL];
static int _dvbc_auto_button_press_cb(unsigned short key)
{
    if(key == STBK_OK)
        app_dvbc_auto_search_entry();
    return EVENT_TRANSFER_KEEPON;
}

static int _dvbc_manual_button_press_cb(unsigned short key)
{
    if(key == STBK_OK)
        app_dvbc_manual_search_menu_exec();
    return EVENT_TRANSFER_KEEPON;
}


static int _dvbc_full_button_press_cb(unsigned short key)
{
    if(key == STBK_OK)
        app_dvbc_all_search_menu_exec();
    return EVENT_TRANSFER_KEEPON;
}

static int _dvbc_setfre_button_press_cb(unsigned short key)
{
    if(key == STBK_OK)
        app_dvbc_set_main_frequency_menu_exec();
    return EVENT_TRANSFER_KEEPON;
}

static char *s_dvbc_title[ITEM_DVBC_TOTAL] = {
    STR_ID_AUTO_SEARCH,
    STR_ID_MANUAL_SEARCH,
    STR_ID_FULL_SEARCH,
    STR_ID_MAIN_FREQ
};

typedef int (*dvbc_press_cb)(unsigned short key);
static dvbc_press_cb s_dvbc_press_cb[ITEM_DVBC_TOTAL] = {
    _dvbc_auto_button_press_cb,
    _dvbc_manual_button_press_cb,
    _dvbc_full_button_press_cb,
    _dvbc_setfre_button_press_cb
};

static void app_dvbc_item_init(ItemDvbcSel sel)
{
    s_dvbc_item[sel].itemTitle = s_dvbc_title[sel];
    s_dvbc_item[sel].itemType = ITEM_PUSH;
    s_dvbc_item[sel].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    s_dvbc_item[sel].itemCallback.btnCallback.BtnPress = s_dvbc_press_cb[sel];
    s_dvbc_item[sel].itemStatus = ITEM_NORMAL;
}

static int app_dvbc_setting_exit_callback(ExitType exit_type)
{
    return EXIT_RET_DEFAULT;
}

void app_dvbc_setting(void)
{
    int i = 0;

    for (i = 0; i < ITEM_DVBC_TOTAL; i++)
        app_dvbc_item_init(i);

    memset(&s_dvbc_opt, 0, sizeof(s_dvbc_opt));

    s_dvbc_opt.menuTitle = STR_ID_SEARCH_SETTING;
    s_dvbc_opt.titleImage = "s_title_left_utility";
    s_dvbc_opt.itemNum = ITEM_DVBC_TOTAL;
    s_dvbc_opt.timeDisplay = TIP_HIDE;
    s_dvbc_opt.item = s_dvbc_item;
    s_dvbc_opt.exit = app_dvbc_setting_exit_callback;

    app_system_set_create(&s_dvbc_opt);

    return;
}

#endif//DEMOD_DVB_C
