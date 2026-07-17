#include "app.h"

#if (DEMOD_DTMB > 0)
#include "app_windows.h"
#include "app_wnd_system_setting_opt.h"

typedef enum {
    ITEM_DTMB_AUTO = 0,
    ITEM_DTMB_MANUAL,
    ITEM_DTMB_FULL,
    ITEM_DTMB_SETFRE,
    ITEM_DTMB_TOTAL
} ItemDtmbSel;

static SystemSettingOpt s_dtmb_opt;
static SystemSettingItem s_dtmb_item[ITEM_DTMB_TOTAL];
static int _dtmb_auto_button_press_cb(unsigned short key)
{
    if (key == STBK_OK)
    {
        extern void app_dtmb_auto_search(void);
        app_dtmb_auto_search();
    }
    return EVENT_TRANSFER_KEEPON;
}

static int _dtmb_manual_button_press_cb(unsigned short key)
{
    if (key == STBK_OK)
    {
        extern void app_dtmb_manual_search_menu_exec(void);
        app_dtmb_manual_search_menu_exec();
    }
    return EVENT_TRANSFER_KEEPON;
}


static int _dtmb_full_button_press_cb(unsigned short key)
{
    if (key == STBK_OK)
    {
        extern void app_dtmb_all_search_menu_exec(void);
        app_dtmb_all_search_menu_exec();
    }
    return EVENT_TRANSFER_KEEPON;
}

static int _dtmb_setfre_button_press_cb(unsigned short key)
{
    if (key == STBK_OK)
    {
        extern void app_dtmb_main_freq_menu_exec(void);
        app_dtmb_main_freq_menu_exec();
    }
    return EVENT_TRANSFER_KEEPON;
}

static char *s_dtmb_title[ITEM_DTMB_TOTAL] = {
    STR_ID_AUTO_SEARCH,
    STR_ID_MANUAL_SEARCH,
    STR_ID_FULL_SEARCH,
    STR_ID_MAIN_FREQ
};

typedef int (*dtmb_press_cb)(unsigned short key);
static dtmb_press_cb s_dtmb_press_cb[ITEM_DTMB_TOTAL] = {
    _dtmb_auto_button_press_cb,
    _dtmb_manual_button_press_cb,
    _dtmb_full_button_press_cb,
    _dtmb_setfre_button_press_cb
};

static void app_dtmb_item_init(ItemDtmbSel sel)
{
    s_dtmb_item[sel].itemTitle = s_dtmb_title[sel];
    s_dtmb_item[sel].itemType = ITEM_PUSH;
    s_dtmb_item[sel].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    s_dtmb_item[sel].itemCallback.btnCallback.BtnPress = s_dtmb_press_cb[sel];
    s_dtmb_item[sel].itemStatus = ITEM_NORMAL;
}

static int app_dtmb_setting_exit_callback(ExitType exit_type)
{
    return EXIT_RET_DEFAULT;
}

void app_dtmb_setting(void)
{
    int i = 0;

    for (i = 0; i < ITEM_DTMB_TOTAL; i++)
        app_dtmb_item_init(i);

    memset(&s_dtmb_opt, 0, sizeof(s_dtmb_opt));

    s_dtmb_opt.menuTitle = STR_ID_SEARCH_SETTING;
    s_dtmb_opt.titleImage = "s_title_left_utility";
    s_dtmb_opt.itemNum = ITEM_DTMB_TOTAL;
    s_dtmb_opt.timeDisplay = TIP_HIDE;
    s_dtmb_opt.item = s_dtmb_item;
    s_dtmb_opt.exit = app_dtmb_setting_exit_callback;

    app_system_set_create(&s_dtmb_opt);

    return;
}

#endif//DEMOD_DVB_C
