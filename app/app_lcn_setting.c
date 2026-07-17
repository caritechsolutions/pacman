#include "app.h"
#include "app_config.h"
#include "app_wnd_system_setting_opt.h"

#if LCN_SUPPORT

typedef enum {
    ITEM_LCN_SWITCH = 0,
    ITEM_LCN_TOTAL
}ITEM_LCN;

#define LCN_SWITCH_CONTENT "[Off, On]"

static SystemSettingOpt  s_LcnSettingOpt;
static SystemSettingItem s_LcnSettingItem[ITEM_LCN_TOTAL];

static bool _lcn_check_cb(void)
{
    bool ret = false;
    int32_t lcn_config;

    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);

    app_system_set_item_data_update(ITEM_LCN_SWITCH);
    if(s_LcnSettingItem[ITEM_LCN_SWITCH].itemProperty.itemPropertyCmb.sel != lcn_config)
        ret = true;

    return ret;
}

static int _lcn_save_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        int32_t lcn_config = s_LcnSettingItem[ITEM_LCN_SWITCH].itemProperty.itemPropertyCmb.sel;
        GxBus_ConfigSetInt(LCN_KEY, lcn_config);
        if(0 == lcn_config)
            GxBus_ConfigSetInt(PROGRAM_LCN_SORT_KEY, lcn_config);
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    return 0;
}

static int _lcn_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
        //do nothing
    }
    else
    {
        if(false == app_system_set_callback_handler(_lcn_check_cb, _lcn_save_pop_cb))
        {
            //do nothing
        }
        ret = EXIT_RET_POP;
    }

    return ret;
}

static void _lcn_switch_item_init(void)
{
    int32_t lcn_config;

    GxBus_ConfigGetInt(LCN_KEY, &lcn_config, LCN_VALUE);

    s_LcnSettingItem[ITEM_LCN_SWITCH].itemTitle = STR_ID_LCN;
    s_LcnSettingItem[ITEM_LCN_SWITCH].itemType = ITEM_CHOICE;
    s_LcnSettingItem[ITEM_LCN_SWITCH].itemProperty.itemPropertyCmb.content = LCN_SWITCH_CONTENT;
    s_LcnSettingItem[ITEM_LCN_SWITCH].itemProperty.itemPropertyCmb.sel = lcn_config;
    s_LcnSettingItem[ITEM_LCN_SWITCH].itemCallback.cmbCallback.CmbChange = NULL;
    s_LcnSettingItem[ITEM_LCN_SWITCH].itemStatus = ITEM_NORMAL;
}

void app_lcn_setting_menu_exec(void)
{
    memset(&s_LcnSettingOpt, 0 ,sizeof(SystemSettingOpt));
    s_LcnSettingOpt.menuTitle = STR_ID_LCN_SETTING;
    s_LcnSettingOpt.itemNum = ITEM_LCN_TOTAL;
    s_LcnSettingOpt.item = s_LcnSettingItem;
    s_LcnSettingOpt.timeDisplay = TIP_HIDE;
    s_LcnSettingOpt.exit = _lcn_exit_callback;

    _lcn_switch_item_init();
    app_system_set_create(&s_LcnSettingOpt);
}
#endif
