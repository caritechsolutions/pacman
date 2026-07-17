#include "app.h"
#include "app_config.h"
#include "app_windows.h"
#include "module/app_gpio.h"
#include "app_dvbs_sat_opt.h"

#if LNB_SHORT_DETECT_SUPPORT

#if DOUBLE_S2_SUPPORT
#define DETECT_LNB_CNT (2)
#else
#define DETECT_LNB_CNT (1)
#endif

typedef struct {
    event_list *timer;
}LNBShortDetectCtrl;

static LNBShortDetectCtrl s_lsd_ctrl = {
    .timer = NULL,
};

static PopDlg s_ShortTipPopDlg = {0};

static int _lnb_check_exit(PopDlgRet Ret)
{
    memset(&s_ShortTipPopDlg, 0, sizeof(PopDlg));
    return 0;
}

static void _modify_all_sat_to_lnb_off(int tuner)
{
    int i = 0;
    bool need_sync = false;
    int sat_num = GxBus_PmSatNumGet();
    GxBusPmDataSat *sat_array = NULL;

    if(0 == sat_num)
        return;

    if(NULL == (sat_array = GxCore_Calloc(sat_num, sizeof(GxBusPmDataSat))))
    {
        app_log_error("calloc failed\n");
        return;
    }

    if(sat_num != GxBus_PmSatsGetByPos(0, sat_num, sat_array))
    {
        app_log_error("get %d sat failed\n", sat_num);
        goto end;
    }

    for(i = 0; i < sat_num; i++)
    {
        if(sat_array[i].tuner != tuner || sat_array[i].type != GXBUS_PM_SAT_S || GXBUS_PM_SAT_LNB_POWER_OFF == sat_array[i].sat_s.lnb_power)
            continue;

        need_sync = true;
        sat_array[i].sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_OFF;
        GxBus_PmSatModify(&sat_array[i]);
    }

    if(need_sync)
        GxBus_PmSync(GXBUS_PM_SYNC_SAT);
end:
    GxCore_Free(sat_array);
    return;
}

static void _change_globa_variable_sat_dat_to_lnb_off(void)
{
    GxBusPmDataSat *sat = &s_dvbs_sat_data.cur_sat.sat_data;

    if(GXBUS_PM_SAT_S == sat->type)
        sat->sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_OFF;
}

static int _lnb_short_detect_timer(void *usrdata)
{
#define TIP_STRINGS_LEN (30)
    int i = 0, short_cnt = 0;
    static char tip_strings[TIP_STRINGS_LEN] = {0};
    char temp[TIP_STRINGS_LEN] = {0};
    int shorted[DETECT_LNB_CNT] = {0};

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_UPGRADE_PROCESS))
        return 0;

    for(i = 0; i < DETECT_LNB_CNT; i++)
    {
        if(0 == app_gpio_lnb_power_get(i) || 0 == app_gpio_lnb_short_get(i))
            continue;

        _modify_all_sat_to_lnb_off(i);
        _change_globa_variable_sat_dat_to_lnb_off();
        GxFrontend_SetVoltage(i, SEC_VOLTAGE_OFF);
        shorted[i] = 1;
        short_cnt++;
    }

    if(0 == short_cnt)
        return 0;

    if(1 == DETECT_LNB_CNT)
    {
        strcat(temp, "LNB Short Circuit");
    }
    else
    {
        for(i = 0; i < DETECT_LNB_CNT; i++)
        {
            if(0 == shorted[i])
                continue;

            if(0 == strlen(temp))
                snprintf(temp, sizeof(temp), "LNB%d", i+1);
            else
                snprintf(temp, sizeof(temp), " LNB%d", i+1);
        }
        strcat(temp, " Short Circuit");
    }

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH))
    {
        GUI_Event event = {0};

        event.type = GUI_KEYDOWN;
        event.key.sym = STBK_EXIT;
        GUI_SendEvent(WND_SEARCH, &event);
    }

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP) && s_ShortTipPopDlg.str && strstr(s_ShortTipPopDlg.str, "Short Circuit"))
    {
        strncpy(tip_strings, temp, sizeof(temp));
        app_pop_set_string(tip_strings);
    }
    else
    {
        strncpy(tip_strings, temp, sizeof(temp));
        popdlg_destroy();

        memset(&s_ShortTipPopDlg, 0, sizeof(PopDlg));
        s_ShortTipPopDlg.type = POP_TYPE_OK;
        s_ShortTipPopDlg.format = POP_FORMAT_DLG;
        s_ShortTipPopDlg.str = tip_strings;
        s_ShortTipPopDlg.mode = POP_MODE_UNBLOCK;
        s_ShortTipPopDlg.exit_cb = _lnb_check_exit;
        popdlg_create(&s_ShortTipPopDlg);
    }

    app_update_pop_dlg(WND_POP_BOOK);
    return 0;
}

int app_dvbs_lnb_detect_init(void)
{
    LNBShortDetectCtrl *ctrl = &s_lsd_ctrl;

    APP_TIMER_ADD(ctrl->timer, _lnb_short_detect_timer, 30, TIMER_REPEAT);
    return 0;
}
#endif
