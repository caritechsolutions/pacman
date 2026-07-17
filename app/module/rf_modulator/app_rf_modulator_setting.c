#include "app.h"
#if (RF_MODULATOR_SUPPORT > 0)
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#if (RF_RT500 > 0)
#include "RT500.h"
#endif
#if (RF_RT534 > 0)
#include "RT534.h"
#endif

#if (RF_RT500 > 0)
typedef enum
{
    ITEM_MODULATOR_CH_STEP = 0,
    ITEM_MODULATOR_CH_NUMER,
    ITEM_MODULATOR_SOUND_MOD,
    ITEM_MODULATOR_RF_MAX
} ITEM_RT500_MODELATOR;

typedef enum
{
    CH_STEP_7M = 0,
    CH_STEP_8M,
    CH_STEP_MAX,
} CHANNEL_STEP;

typedef struct
{
    uint32_t channel_step;
    uint32_t channel_numer;
    uint32_t sound_mode;
} Rt500ModulatorSettingPara;

Rt500ModulatorSettingPara rt500_current_setting_para = {8, 22, 1};
Rt500ModulatorSettingPara rt500_backup_setting_para = {8, 22, 1};
static char *s_rt500_modulator_channel_buffer = NULL;
static SystemSettingOpt  s_Rt500ModulatorOpt;
static SystemSettingItem s_RfModulatorItem[ITEM_MODULATOR_RF_MAX];
static char *s_rf_modulator_setting_item_title[ITEM_MODULATOR_RF_MAX] =
{
    "Band Width",
    "Channel NO",
    "Sound Mode"
};
static const uint8_t s_rf_modulator_step[CH_STEP_MAX] = {7, 8};

#define		SOUND_MODE_CONTENT		"[M,BG,I,DK,L]"
#define		MAX_CHANNEL_PARA_LEN	10
#define		BANDWIDTH_CONTENT		"[7,8]"
#define		RFMODU_MIN_FREQ			471
#define		START_CH_NUMER			21
#define		END_CH_NUMER			69

static int app_sel_to_bandwidth_step(int sel)
{
    if (sel < 0 || sel >= CH_STEP_MAX)
        return 0;
    return s_rf_modulator_step[sel];
}

static int app_bandwith_step_to_sel(int step)
{
    uint8_t i = 0;

    for (i = 0; i < CH_STEP_MAX; i++)
    {
        if (s_rf_modulator_step[i] == step)
            return i;
    }
    return 1;
}

static uint32_t app_modulator_get_modulator_fre(uint32_t channel_numer, uint32_t channel_step)
{
    if (channel_numer < START_CH_NUMER || channel_numer > END_CH_NUMER)
        return 0;
    return ((channel_numer - START_CH_NUMER) * channel_step) + RFMODU_MIN_FREQ;
}

static status_t app_rf_modulator_channel_numer_content_build(void)
{
    char buffer[MAX_CHANNEL_PARA_LEN + 1] = {0};
    uint8_t i = 0;
    uint8_t channel_num = END_CH_NUMER - START_CH_NUMER;

    APP_FREE(s_rt500_modulator_channel_buffer);
    s_rt500_modulator_channel_buffer = GxCore_Calloc(channel_num, (MAX_CHANNEL_PARA_LEN + 1) + 2);
    if (NULL == s_rt500_modulator_channel_buffer)
    {
        app_log_error("\033[31mNo enough memory\033[0m\n");
        return GXCORE_ERROR;
    }

    strcat(s_rt500_modulator_channel_buffer, "[");
    sprintf(buffer, "%d (%dMHz)", START_CH_NUMER, app_modulator_get_modulator_fre(START_CH_NUMER, rt500_current_setting_para.channel_step));
    strcat(s_rt500_modulator_channel_buffer, buffer);
    for (i = START_CH_NUMER + 1; i < END_CH_NUMER; i++)
    {
        memset(buffer, 0, MAX_CHANNEL_PARA_LEN + 1);
        sprintf(buffer, ",%d (%dMHz)", i, app_modulator_get_modulator_fre(i, rt500_current_setting_para.channel_step));
        strcat(s_rt500_modulator_channel_buffer, buffer);
    }
    strcat(s_rt500_modulator_channel_buffer, "]");
    return GXCORE_SUCCESS;
}

static void app_rt500_setup(Rt500ModulatorSettingPara rf_para)
{
    uint32_t rf_channel = 0;
    uint32_t fre = 0;

    fre = app_modulator_get_modulator_fre(rf_para.channel_numer, rf_para.channel_step);
    rf_channel = 1000 * fre;
    RT500_Setup(rf_channel, rf_para.sound_mode);
}

static int app_rf_modulator_bandwidth_change(int sel)
{
    rt500_current_setting_para.channel_step = app_sel_to_bandwidth_step(sel);
    app_rf_modulator_channel_numer_content_build();
    s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemProperty.itemPropertyCmb.content = s_rt500_modulator_channel_buffer;
    s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemProperty.itemPropertyCmb.sel = rt500_current_setting_para.channel_numer - START_CH_NUMER;
    app_system_set_item_property(ITEM_MODULATOR_CH_NUMER, &s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemProperty);
    return EVENT_TRANSFER_KEEPON;
}

static int app_rf_modulator_channel_numer_change(int sel)
{
    rt500_current_setting_para.channel_numer = sel + START_CH_NUMER;
    app_rt500_setup(rt500_current_setting_para);
    GxBus_ConfigSetInt(RF_CH_NUMER_KEY, rt500_current_setting_para.channel_numer);
    return EVENT_TRANSFER_KEEPON;
}

static int app_rf_modulator_sound_mode_change(int sel)
{
    rt500_current_setting_para.sound_mode = sel;
    app_rt500_setup(rt500_current_setting_para);
    GxBus_ConfigSetInt(RF_SOUND_MODE_KEY, rt500_current_setting_para.sound_mode);
    return EVENT_TRANSFER_KEEPON;
}

static void app_rt500_modulator_band_width_item_init(void)
{
    s_RfModulatorItem[ITEM_MODULATOR_CH_STEP].itemTitle = s_rf_modulator_setting_item_title[ITEM_MODULATOR_CH_STEP];
    s_RfModulatorItem[ITEM_MODULATOR_CH_STEP].itemType = ITEM_CHOICE;
    s_RfModulatorItem[ITEM_MODULATOR_CH_STEP].itemProperty.itemPropertyCmb.content = BANDWIDTH_CONTENT;
    s_RfModulatorItem[ITEM_MODULATOR_CH_STEP].itemProperty.itemPropertyCmb.sel = app_bandwith_step_to_sel(rt500_current_setting_para.channel_step);
    s_RfModulatorItem[ITEM_MODULATOR_CH_STEP].itemCallback.cmbCallback.CmbChange = app_rf_modulator_bandwidth_change;
    s_RfModulatorItem[ITEM_MODULATOR_CH_STEP].itemStatus = ITEM_NORMAL;
}

static void app_rt500_modulator_channel_no_item_init(void)
{
    if (app_rf_modulator_channel_numer_content_build() == GXCORE_ERROR)
        return;
    s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemTitle = s_rf_modulator_setting_item_title[ITEM_MODULATOR_CH_NUMER];
    s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemType = ITEM_CHOICE;
    s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemProperty.itemPropertyCmb.content = s_rt500_modulator_channel_buffer;
    s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemProperty.itemPropertyCmb.sel = rt500_current_setting_para.channel_numer - START_CH_NUMER;
    s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemCallback.cmbCallback.CmbChange = app_rf_modulator_channel_numer_change;
    s_RfModulatorItem[ITEM_MODULATOR_CH_NUMER].itemStatus = ITEM_NORMAL;
}

static void app_rt500_modulator_sound_mode_item_init(void)
{
    s_RfModulatorItem[ITEM_MODULATOR_SOUND_MOD].itemTitle = s_rf_modulator_setting_item_title[ITEM_MODULATOR_SOUND_MOD];
    s_RfModulatorItem[ITEM_MODULATOR_SOUND_MOD].itemType = ITEM_CHOICE;
    s_RfModulatorItem[ITEM_MODULATOR_SOUND_MOD].itemProperty.itemPropertyCmb.content = SOUND_MODE_CONTENT;
    s_RfModulatorItem[ITEM_MODULATOR_SOUND_MOD].itemProperty.itemPropertyCmb.sel = rt500_current_setting_para.sound_mode;
    s_RfModulatorItem[ITEM_MODULATOR_SOUND_MOD].itemCallback.cmbCallback.CmbChange = app_rf_modulator_sound_mode_change;
    s_RfModulatorItem[ITEM_MODULATOR_SOUND_MOD].itemStatus = ITEM_NORMAL;

}

static bool app_rt500_modulator_setting_check_cb(void)
{
    if (rt500_current_setting_para.channel_step != rt500_backup_setting_para.channel_step)
        return true;
    if (rt500_current_setting_para.channel_numer != rt500_backup_setting_para.channel_numer)
        return true;
    if (rt500_current_setting_para.sound_mode != rt500_backup_setting_para.sound_mode)
        return true;
    return false;
}

static int app_rt500_modulator_setting_pop_cb(PopDlgRet ret)
{
    if (ret == POP_VAL_OK)
    {
        memcpy(&rt500_backup_setting_para, &rt500_current_setting_para, sizeof(Rt500ModulatorSettingPara));
    }
    else if (ret == POP_VAL_CANCEL)
    {
        app_rt500_setup(rt500_backup_setting_para);
        GxBus_ConfigSetInt(RF_CH_NUMER_KEY, rt500_backup_setting_para.channel_numer);
        GxBus_ConfigSetInt(RF_SOUND_MODE_KEY, rt500_backup_setting_para.sound_mode);
        memcpy(&rt500_current_setting_para, &rt500_backup_setting_para, sizeof(Rt500ModulatorSettingPara));
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING, "draw_now", NULL);
    return 0;

}

static int app_rt500__modulator_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if (EXIT_ABANDON == exit_type)
    {
    }
    else
    {
        if (false == app_system_set_callback_handler(app_rt500_modulator_setting_check_cb, app_rt500_modulator_setting_pop_cb))
        {
        }
        ret = EXIT_RET_POP;
    }

    APP_FREE(s_rt500_modulator_channel_buffer);
    return ret;
}

static void app_rf_modulator_get_para(void)
{
    int value = 0;
    GxBus_ConfigGetInt(RF_SOUND_MODE_KEY, &value, RF_SOUND_MODE);
    rt500_current_setting_para.sound_mode = value;
    GxBus_ConfigGetInt(RF_CH_NUMER_KEY, &value, RF_CH_NUMER);
    rt500_current_setting_para.channel_numer = value;
}

void app_rf_rt500_setting_menu(void)
{
    memset(&s_Rt500ModulatorOpt, 0, sizeof(SystemSettingOpt));
    app_rf_modulator_get_para();
    s_Rt500ModulatorOpt.menuTitle = STR_ID_RF_SEARCH;
    s_Rt500ModulatorOpt.itemNum = ITEM_MODULATOR_RF_MAX;
    s_Rt500ModulatorOpt.item = s_RfModulatorItem;
    s_Rt500ModulatorOpt.timeDisplay = TIP_HIDE;
    s_Rt500ModulatorOpt.exit = app_rt500__modulator_exit_callback;
    app_rt500_modulator_band_width_item_init();
    app_rt500_modulator_channel_no_item_init();
    app_rt500_modulator_sound_mode_item_init();
    memcpy(&rt500_backup_setting_para, &rt500_current_setting_para, sizeof(Rt500ModulatorSettingPara));
    app_system_set_create(&s_Rt500ModulatorOpt);
}

#endif

#if (RF_RT534 > 0)
typedef enum
{
    ITEM_RF_MODULATOR_POWER = 0,
    ITEM_RF_MODULATOR_MODE,
    ITEM_RF_MODULATOR_RT534_MAX,
} RF_ITEM;

typedef enum
{
    RF_ON = 0,
    RF_OFF,
    RF_POWER_TOTAL
} RF_POWER;

typedef enum
{
    RF_CH3 = 0,
    RF_CH4,
    RF_MODE_TOTAL
} RF_CH;

typedef struct
{
    uint8_t rf_power;
    uint8_t rf_mode;
} Rt534ModulatorSettingPara;
static Rt534ModulatorSettingPara rt534_current_setting_para = {0};
static Rt534ModulatorSettingPara rt534_backup_setting_para = {0};
static SystemSettingOpt  s_Rt534ModulatorOpt;
static SystemSettingItem s_Rt534ModulatorItem[ITEM_RF_MODULATOR_RT534_MAX];

static char *s_rt534_item_content[ITEM_RF_MODULATOR_RT534_MAX] =
{
    "[On,Off]",
    "[CH3,CH4]"
};

void app_rt534_modulator_get_para(void)
{
    int value = 0;

    GxBus_ConfigGetInt(RF_ON_OFF_KEY, &value, RF_POWER_DEFAULT);
    rt534_current_setting_para.rf_power = value;
    GxBus_ConfigGetInt(RF_MODE_KEY, &value, RF_MODE_DEFAULT);
    rt534_current_setting_para.rf_mode = value;
}

static int app_rt534_modulator_power_change(int sel)
{
    rt534_current_setting_para.rf_power = sel;
    RT534_RF_ON_OFF(sel);
    GxBus_ConfigSetInt(RF_ON_OFF_KEY, sel);
    if (sel == RF_OFF)
    {
        s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemStatus = ITEM_DISABLE;
        app_system_set_item_state_update(ITEM_RF_MODULATOR_MODE, s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemStatus);
    }
    else if (sel == RF_ON)
    {
        s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemStatus = ITEM_NORMAL;
        app_system_set_item_state_update(ITEM_RF_MODULATOR_MODE, s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemStatus);
    }
    return EVENT_TRANSFER_KEEPON;
}

static int app_rt534_modulator_mode_change(int sel)
{
    rt534_current_setting_para.rf_mode = sel;
    RT534_RF_CH3_CH4(sel);
    GxBus_ConfigSetInt(RF_MODE_KEY, sel);
    return EVENT_TRANSFER_KEEPON;
}

static void app_rt534_modulator_power_item_init(void)
{
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_POWER].itemTitle = STR_ID_RF_ON_OFF;
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_POWER].itemType = ITEM_CHOICE;
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_POWER].itemProperty.itemPropertyCmb.content = s_rt534_item_content[ITEM_RF_MODULATOR_POWER];
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_POWER].itemProperty.itemPropertyCmb.sel = rt534_current_setting_para.rf_power;
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_POWER].itemCallback.cmbCallback.CmbChange = app_rt534_modulator_power_change;
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_POWER].itemStatus = ITEM_NORMAL;
}

static void app_rt534_modulator_mode_item_init(void)
{
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemTitle = STR_ID_RF_CH3_CH4;
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemType = ITEM_CHOICE;
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemProperty.itemPropertyCmb.content = s_rt534_item_content[ITEM_RF_MODULATOR_MODE];
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemProperty.itemPropertyCmb.sel = rt534_current_setting_para.rf_mode;
    s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemCallback.cmbCallback.CmbChange = app_rt534_modulator_mode_change;
    if (rt534_current_setting_para.rf_power == RF_ON)
        s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemStatus = ITEM_NORMAL;
    else if (rt534_current_setting_para.rf_power == RF_OFF)
        s_Rt534ModulatorItem[ITEM_RF_MODULATOR_MODE].itemStatus = ITEM_DISABLE;
}

static bool app_rt534_modulator_setting_check_cb(void)
{
    if (rt534_current_setting_para.rf_power != rt534_backup_setting_para.rf_power)
        return true;
    if (rt534_current_setting_para.rf_mode != rt534_backup_setting_para.rf_mode)
        return true;
    return false;
}

static int app_rt534_modulator_setting_pop_cb(PopDlgRet ret)
{
    if (ret == POP_VAL_OK)
    {
        memcpy(&rt534_backup_setting_para, &rt534_current_setting_para, sizeof(Rt534ModulatorSettingPara));
    }
    else if (ret == POP_VAL_CANCEL)
    {
        RT534_RF_ON_OFF(rt534_backup_setting_para.rf_power);
        GxBus_ConfigSetInt(RF_ON_OFF_KEY, rt534_backup_setting_para.rf_power);
        RT534_RF_CH3_CH4(rt534_backup_setting_para.rf_mode);
        GxBus_ConfigSetInt(RF_MODE_KEY, rt534_backup_setting_para.rf_mode);
        memcpy(&rt534_current_setting_para, &rt534_backup_setting_para, sizeof(Rt534ModulatorSettingPara));
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING, "draw_now", NULL);
    return 0;
}

static int app_rt534_modulator_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if (EXIT_ABANDON == exit_type)
    {

    }
    else
    {
        if (false == app_system_set_callback_handler(app_rt534_modulator_setting_check_cb, app_rt534_modulator_setting_pop_cb))
        {

        }
        ret = EXIT_RET_POP;
    }

    return ret;
}

void app_rf_rt534_setting_menu(void)
{
    memset(&s_Rt534ModulatorOpt, 0, sizeof(SystemSettingOpt));
    app_rt534_modulator_get_para();
    s_Rt534ModulatorOpt.menuTitle = STR_ID_RF_SEARCH;
    s_Rt534ModulatorOpt.itemNum = ITEM_RF_MODULATOR_RT534_MAX;
    s_Rt534ModulatorOpt.item = s_Rt534ModulatorItem;
    s_Rt534ModulatorOpt.timeDisplay = TIP_HIDE;
    s_Rt534ModulatorOpt.exit = app_rt534_modulator_exit_callback;
    app_rt534_modulator_power_item_init();
    app_rt534_modulator_mode_item_init();
    memcpy(&rt534_backup_setting_para, &rt534_current_setting_para, sizeof(Rt534ModulatorSettingPara));
    app_system_set_create(&s_Rt534ModulatorOpt);
}
#endif

void app_rf_modulator_setting_menu_exec(void)
{
#if (RF_RT500 > 0)
    app_rf_rt500_setting_menu();
#endif
#if (RF_RT534 > 0)
    app_rf_rt534_setting_menu();
#endif
}

#endif
