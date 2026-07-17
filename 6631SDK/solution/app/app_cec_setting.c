#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include "app.h"
#include "app_wnd_system_setting_opt.h"
#include "hdmi_cec/app_cec_config.h"

#define CEC_SWITCH_CONTENT "[Off, On]"
#define CEC_CONTROL_CONTENT "[No,Yes]"
#define CEC_TV_ACTIVITY_CONTENT "[Do Nothing,STB On]"
#define CEC_STB_ACTIVITY_CONTENT "[Do Nothing,TV Standby]"

typedef enum {
    ITEM_CEC_SWITCH = 0,
    ITEM_CEC_USE_TV_REMOTE_CONTROL,
    ITEM_CEC_TV_ON_STB_ACTIVITY,
    ITEM_CEC_STB_STANDBY_TV_ACTIVITY,
    ITEM_CEC_SYSTEM_AUDIO,
    ITEM_CEC_TOTAL
}ITEM_CEC;

typedef enum {
    CEC_SWITCH_OFF = 0,
    CEC_SWITCH_ON,
}CECSwitchSel;

typedef struct
{
    int enable;
    int tv_rc;
    int tv_on;
    int stb_off;
    int sys_audio;
}SysCecPara;

static SystemSettingOpt  s_CecSettingOpt;
static SystemSettingItem s_CecSettingItem[ITEM_CEC_TOTAL];
static SysCecPara s_sys_cec_para;

extern int app_cec_enable(int enable);

static bool _cec_check_cb(void)
{
    bool ret = false;
    int i = 0;

    for(i=0;i<ITEM_CEC_TOTAL;i++)
        app_system_set_item_data_update(i);

    if(s_CecSettingItem[ITEM_CEC_SWITCH].itemProperty.itemPropertyCmb.sel != s_sys_cec_para.enable)
    {
        ret = true;
    }
    if(s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemProperty.itemPropertyCmb.sel != s_sys_cec_para.tv_rc)
    {
        ret = true;
    }
    if(s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemProperty.itemPropertyCmb.sel != s_sys_cec_para.tv_on)
    {
        ret = true;
    }
    if(s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemProperty.itemPropertyCmb.sel != s_sys_cec_para.stb_off)
    {
        ret = true;
    }
    if(s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemProperty.itemPropertyCmb.sel != s_sys_cec_para.sys_audio)
    {
        ret = true;
    }
    return ret;
}

static int _cec_save_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        int32_t cec_config = s_CecSettingItem[ITEM_CEC_SWITCH].itemProperty.itemPropertyCmb.sel;
        GxBus_ConfigSetInt(CEC_CONF_KEY, cec_config);
        app_cec_enable(cec_config);
        if(cec_config == 1){
            int32_t cec_config_value = s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(CEC_USE_TV_REMOTE_CONTROL_KEY, cec_config_value);
            app_cec_set_use_tv_remote_control_enable(cec_config_value);

            cec_config_value = s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(CEC_TV_ON_STB_ACTIVITY_KEY, cec_config_value);
            app_cec_set_tv_on_stb_activity_enable(cec_config_value);

            cec_config_value = s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(CEC_STB_STANDBY_TV_ACTIVITY_KEY, cec_config_value);
            app_cec_set_stb_standby_tv_ativity_enable(cec_config_value);

            cec_config_value = s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemProperty.itemPropertyCmb.sel;
            GxBus_ConfigSetInt(CEC_SYSTEM_AUDIO_KEY, cec_config_value);
            app_cec_set_system_audio_mode_status(cec_config_value);
        }
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    return 0;
}

static int _cec_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
        //do nothing
    }
    else
    {
        if(false == app_system_set_callback_handler(_cec_check_cb, _cec_save_pop_cb))
        {
            //do nothing
        }
        ret = EXIT_RET_POP;
    }
    return ret;
}

static void app_cec_system_para_get(void)
{
    int32_t cec_config = 0;

    GxBus_ConfigGetInt(CEC_CONF_KEY, &cec_config, CEC_CONFIG_VALUE);
    s_sys_cec_para.enable = cec_config;
    GxBus_ConfigGetInt(CEC_USE_TV_REMOTE_CONTROL_KEY, &cec_config, CEC_USE_TV_REMOTE_CONTROL_VALUE);
    s_sys_cec_para.tv_rc = cec_config;
    GxBus_ConfigGetInt(CEC_TV_ON_STB_ACTIVITY_KEY, &cec_config, CEC_TV_ON_STB_ACTIVITY_VALUE);
    s_sys_cec_para.tv_on = cec_config;
    GxBus_ConfigGetInt(CEC_STB_STANDBY_TV_ACTIVITY_KEY, &cec_config, CEC_STB_STANDBY_TV_ACTIVITY_VALUE);
    s_sys_cec_para.stb_off = cec_config;
    GxBus_ConfigGetInt(CEC_SYSTEM_AUDIO_KEY, &cec_config, CEC_SYSTEM_AUDIO_VALUE);
    s_sys_cec_para.sys_audio = cec_config;
}

static int app_cec_switch_change_callback(int sel)
{
    if(sel == CEC_SWITCH_OFF)
    {
        app_system_set_item_state_update(ITEM_CEC_USE_TV_REMOTE_CONTROL,   ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_CEC_TV_ON_STB_ACTIVITY,      ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_CEC_STB_STANDBY_TV_ACTIVITY, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_CEC_SYSTEM_AUDIO,            ITEM_DISABLE);
    }
    else
    {
        app_system_set_item_state_update(ITEM_CEC_USE_TV_REMOTE_CONTROL,   ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_CEC_TV_ON_STB_ACTIVITY,      ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_CEC_STB_STANDBY_TV_ACTIVITY, ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_CEC_SYSTEM_AUDIO,            ITEM_NORMAL);
    }
    return EVENT_TRANSFER_KEEPON;
}

static void _cec_switch_item_init(void)
{
    s_CecSettingItem[ITEM_CEC_SWITCH].itemTitle = STR_ID_HDMI_CEC;
    s_CecSettingItem[ITEM_CEC_SWITCH].itemType = ITEM_CHOICE;
    s_CecSettingItem[ITEM_CEC_SWITCH].itemProperty.itemPropertyCmb.content = CEC_SWITCH_CONTENT;
    s_CecSettingItem[ITEM_CEC_SWITCH].itemProperty.itemPropertyCmb.sel = s_sys_cec_para.enable;
    s_CecSettingItem[ITEM_CEC_SWITCH].itemCallback.cmbCallback.CmbChange = app_cec_switch_change_callback;
    s_CecSettingItem[ITEM_CEC_SWITCH].itemStatus = ITEM_NORMAL;
}

static void _cec_use_tv_remote_control_item_init(void)
{
    s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemTitle = STR_ID_USE_TV_REMOTE_CONTROL;
    s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemType = ITEM_CHOICE;
    s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemProperty.itemPropertyCmb.content = CEC_CONTROL_CONTENT;
    s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemProperty.itemPropertyCmb.sel = s_sys_cec_para.tv_rc;
    s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemCallback.cmbCallback.CmbChange = NULL;
    if(s_sys_cec_para.enable == 0)
        s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemStatus = ITEM_DISABLE;
    else
        s_CecSettingItem[ITEM_CEC_USE_TV_REMOTE_CONTROL].itemStatus = ITEM_NORMAL;
}

static void _cec_tv_on_stb_activity_item_init(void)
{
    s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemTitle = STR_ID_TV_ON_STB_ACTIVITY;
    s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemType = ITEM_CHOICE;
    s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemProperty.itemPropertyCmb.content = CEC_TV_ACTIVITY_CONTENT;
    s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemProperty.itemPropertyCmb.sel = s_sys_cec_para.tv_on;
    s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemCallback.cmbCallback.CmbChange = NULL;
    if(s_sys_cec_para.enable == 0)
        s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemStatus = ITEM_DISABLE;
    else
        s_CecSettingItem[ITEM_CEC_TV_ON_STB_ACTIVITY].itemStatus = ITEM_NORMAL;
}

static void _cec_stb_standby_tv_ativity_item_init(void)
{
    s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemTitle = STR_ID_STB_STANDBY_TV_ACTIVITY;
    s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemType = ITEM_CHOICE;
    s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemProperty.itemPropertyCmb.content = CEC_STB_ACTIVITY_CONTENT;
    s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemProperty.itemPropertyCmb.sel = s_sys_cec_para.stb_off;
    s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemCallback.cmbCallback.CmbChange = NULL;
    if(s_sys_cec_para.enable == 0)
        s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemStatus = ITEM_DISABLE;
    else
        s_CecSettingItem[ITEM_CEC_STB_STANDBY_TV_ACTIVITY].itemStatus = ITEM_NORMAL;
}

static void _cec_system_audio_item_init(void)
{
    s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemTitle = STR_ID_CEC_SYSTEM_AUDIO;
    s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemType = ITEM_CHOICE;
    s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemProperty.itemPropertyCmb.content = CEC_SWITCH_CONTENT;
    s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemProperty.itemPropertyCmb.sel = s_sys_cec_para.stb_off;
    s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemCallback.cmbCallback.CmbChange = NULL;
    if(s_sys_cec_para.enable == 0)
        s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemStatus = ITEM_DISABLE;
    else
        s_CecSettingItem[ITEM_CEC_SYSTEM_AUDIO].itemStatus = ITEM_NORMAL;
}

void app_cec_setting_menu_exec(void)
{
    memset(&s_CecSettingOpt, 0 ,sizeof(SystemSettingOpt));
    s_CecSettingOpt.menuTitle = STR_ID_HDMI_CEC;
    s_CecSettingOpt.itemNum = ITEM_CEC_TOTAL;
    s_CecSettingOpt.item = s_CecSettingItem;
    s_CecSettingOpt.timeDisplay = TIP_HIDE;
    s_CecSettingOpt.exit = _cec_exit_callback;
    app_cec_system_para_get();

    _cec_switch_item_init();
    _cec_use_tv_remote_control_item_init();
    _cec_tv_on_stb_activity_item_init();
    _cec_stb_standby_tv_ativity_item_init();
    _cec_system_audio_item_init();

    app_system_set_create(&s_CecSettingOpt);
}
#endif
