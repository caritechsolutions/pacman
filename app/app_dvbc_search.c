#include "app_wnd_search.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"
#include "app_common_search_setting.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#if (DEMOD_DVB_C > 0)
#include "module/app_frontend_board_config.h"
/* Private variable------------------------------------------------------- */
#define QAM_CONTENT	"[16QAM,32QAM,64QAM,128QAM,256QAM]"

typedef struct
{
    uint32_t fre;
    uint32_t symbol_rate;
    uint32_t qam;
}search_dvbc_param;

enum DVBC_SEARCH_ITEM
{
    ITEM_C_MANUAL_SEARCH_FRE = 0,
    ITEM_C_MANUAL_SEARCH_SYM,
    ITEM_C_MANUAL_SEARCH_QAM,
#if ((DEMOD_DVB_COMBO > 0)&&(DEMOD_DVB_T > 0))
    ITEM_C_MANUAL_SEARCH_NIT,
#endif
    ITEM_C_MANUAL_SEARCH_START,
    ITEM_C_MANUAL_SEARCH_TOTAL,
};

static CSTOpt thiz_DvbcSearchOpt = {0};
static CSTItem thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_TOTAL];
static char thiz_DvbcFreq[7] = {0};
static char thiz_DvbcSymbol[5] = {0};
static search_dvbc_param sManualSearchParam = {0};

static void app_dvbc_search_set_tp(uint32_t fre, uint32_t symb, fe_modulation_t modulation)
{
    AppFrontend_SetTp params = {0};
    GxBusPmDataSat sat = {0};

    app_get_sat_params(&sat, DEMOD_TYPE_DVBC);

    params.type = FRONTEND_DVB_C;
    params.fre = fre;
    params.symb = symb;
    params.qam = modulation + 3;
    params.tune_mode = DVBC_TUNE_MODE;
    app_log_debug("\n----fre=%d,---\n",  params.fre);
    app_set_tp(sat.tuner, &params, SET_TP_FORCE);
}

void app_dvbc_search_set_main_tp(void)
{
    int32_t symbol_rate = 0;
    int32_t qam = 0;
    int32_t fre = 0;

    GxBus_ConfigGetInt(MAIN_FREQ_KEY, &fre, MAIN_FREQ_VALUE);
    GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &symbol_rate, MAIN_FREQ_SYM_VALUE);
    GxBus_ConfigGetInt(MAIN_FREQ_QAM_KEY, &qam, MAIN_FREQ_QAM_VALUE);

    // auto search / full search must call app_s2_t2_pin_switch() first in combo project
    // app_s2_t2_pin_switch() is called by app_dvbc_search_set_tp()
    app_dvbc_search_set_tp(fre, symbol_rate, qam);
}

static bool _main_tp_lock_status_check(void)
{
#if ((DEMOD_DVB_COMBO > 0 )&&(DEMOD_DVB_T > 0))
    return true;
#else
    unsigned long temp = 0;
    AppFrontend_LockState lock = FRONTEND_UNLOCK;
    GxBusPmDataSat sat = {0};

    app_get_sat_params(&sat, DEMOD_TYPE_DVBC);
    app_dvbc_search_set_main_tp();

    temp = GxCore_TickStart(2000);
    do
    {
        GxCore_ThreadDelay(500);

        app_ioctl(sat.tuner, FRONTEND_LOCK_STATE_GET, &lock);
        if (lock == FRONTEND_LOCKED)
            break;
    }
    while(!GxCore_TickEnd(temp));

    if(FRONTEND_UNLOCK == lock)
    {
        PopDlg pop = {0};
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_LOCK_FAILED;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return false;
    }
    return true;
#endif
}

static void _dvbc_auto_search_start(void)
{
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_INSTALLATION_GUIDE))
        GUI_EndDialog(WND_INSTALLATION_GUIDE);

    app_send_msg_exec(GXMSG_BOOK_RESET, NULL);
    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, true);
    app_search_set_search_type(SEARCH_CABLE);
    app_search_set_search_mode(AUTO_SEARCH);
    GxBusPmDataSat sat = {0};
    app_get_sat_params(&sat, DEMOD_TYPE_DVBC);
    app_tuner_cur_tuner_set(sat.tuner);

#if NDEMOD_WITH_1TUNER && (APP_FRONTEND_PIN_SWITCH_COUNT || APP_FRONTEND_DEMOD_ADAPT_PIN_SWITCH_COUNT)
    extern void app_pin_func_switch(int tuner);
    app_pin_func_switch(sat.tuner);
#endif
#if ((DEMOD_DVB_COMBO > 0 )&&(DEMOD_DVB_T > 0))
    extern void app_search_scan_preset_mode(void);
    app_search_scan_preset_mode();
#else
    app_search_scan_nit_mode();
#endif
    GUI_CreateDialog(WND_SEARCH);
    GUI_SetInterface("flush", NULL);
    return;
}

static int _dvbc_prog_delete_cb(PopDlgRet ret)
{
    int32_t sat_id = 0;

    if(POP_VAL_OK == ret)
    {
        sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_C);
        app_all_channels_del_by_sat_id(sat_id);
        app_all_tp_del_by_sat_id(sat_id);
        _dvbc_auto_search_start();
    }
    return 0;
}

static void _dvbc_auto_search_with_prog_delete(void)
{
    PopDlg pop = {0};
    uint32_t tv_num = 0;
    uint32_t radio_num = 0;

    uint32_t sat_id = 0;
    sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_C);
    app_channel_num_check_by_sat_id(sat_id, &tv_num, &radio_num);
    if(0 != tv_num || 0 != radio_num)
    {
        memset(&pop, 0, sizeof(PopDlg));

        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_DEL_ALL_CHANNEL;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = _dvbc_prog_delete_cb;
        popdlg_create(&pop);
    }
    else
    {
        app_all_tp_del_by_sat_id(sat_id);
        _dvbc_auto_search_start();
    }
}

void app_dvbc_auto_search(bool delete_prog)
{
    if(false == _main_tp_lock_status_check())
        return;

    if(true == delete_prog)
        _dvbc_auto_search_with_prog_delete();
    else
        _dvbc_auto_search_start();
}

void app_dvbc_auto_search_entry(void)
{
    app_dvbc_auto_search(true);
}

static int _dvbc_manual_search_freq_edit_press_end(void)
{
    uint32_t fre = 0;
    float edit_fre = 0;
    CSTItemData data = {0};

    app_cst_item_data_get(ITEM_C_MANUAL_SEARCH_FRE, &data);
    edit_fre = app_float_edit_str_to_value(data.string);
    fre = 1000 * edit_fre;
    if (FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DVBC))
    {
        memset(thiz_DvbcFreq, 0, sizeof(thiz_DvbcFreq));
        sprintf(thiz_DvbcFreq, "%03d.%d", sManualSearchParam.fre / 1000, (sManualSearchParam.fre % 1000) / 100);
        app_cst_item_data_update(ITEM_C_MANUAL_SEARCH_FRE);
        return 0;
    }

    if (fre != sManualSearchParam.fre)
    {
        sManualSearchParam.fre = fre;
        app_dvbc_search_set_tp(sManualSearchParam.fre, sManualSearchParam.symbol_rate, sManualSearchParam.qam);
    }

    return 0;
}

#if 0 == DVBC_TUNE_AUTO_SUPPORT
static int _dvbc_manual_search_sym_edit_press_end(void)
{
    uint32_t symbol_rate = 0;
    CSTItemData data = {0};

    app_cst_item_data_get(ITEM_C_MANUAL_SEARCH_SYM, &data);
    symbol_rate = atoi(data.string);
    if(FALSE == app_search_check_sym_valid(symbol_rate))
    {
        memset(thiz_DvbcSymbol, 0, sizeof(thiz_DvbcSymbol));
        sprintf(thiz_DvbcSymbol, "%04d", sManualSearchParam.symbol_rate);
        app_cst_item_data_update(ITEM_C_MANUAL_SEARCH_SYM);
        return 0;
    }

    if(symbol_rate != sManualSearchParam.symbol_rate)
    {
        sManualSearchParam.symbol_rate = symbol_rate;
        app_dvbc_search_set_tp(sManualSearchParam.fre, sManualSearchParam.symbol_rate, sManualSearchParam.qam);
    }

    return 0;
}

static int _dvbc_manual_search_qam_cmb_change(int sel)
{
    CSTItemData qam_data = {0};

    app_cst_item_data_get(ITEM_C_MANUAL_SEARCH_QAM, &qam_data);
    if (qam_data.value != sManualSearchParam.qam)
    {
        sManualSearchParam.qam = qam_data.value;
        app_dvbc_search_set_tp(sManualSearchParam.fre, sManualSearchParam.symbol_rate, sManualSearchParam.qam);
    }

    return 0;
}
#endif

static int _dvbc_manual_search_press_cb(void)
{
    AppFrontend_LockState lock = FRONTEND_UNLOCK;
    GxBusPmDataSat sat = {0};

    app_get_sat_params(&sat, DEMOD_TYPE_DVBC);
    app_ioctl(sat.tuner, FRONTEND_LOCK_STATE_GET, &lock);
    if(FRONTEND_UNLOCK == lock)
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_LOCK_FAILED;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return 0;
    }

    app_search_set_search_type(SEARCH_CABLE);
    app_search_set_search_mode(MANUAL_SEARCH);
    app_del_noprog_tp_by_sat_id(app_sat_id_get_by_type(GXBUS_PM_SAT_C));
    GUI_CreateDialog(WND_SEARCH);
    GUI_SetInterface("flush", NULL);
#if ((DEMOD_DVB_COMBO > 0)&&(DEMOD_DVB_T > 0))
    CSTItemData nit_data = {0};
    app_cst_item_data_get(ITEM_C_MANUAL_SEARCH_NIT, &nit_data);
    if(nit_data.value == 1)
    {
        GxBus_ConfigSetInt(MAIN_FREQ_KEY, sManualSearchParam.fre);
        GxBus_ConfigSetInt(MAIN_FREQ_SYM_KEY, sManualSearchParam.symbol_rate);
        GxBus_ConfigSetInt(MAIN_FREQ_QAM_KEY, sManualSearchParam.qam);
        app_search_scan_nit_mode();
    }
    else
#endif
    app_search_scan_manual_mode(sManualSearchParam.fre, sManualSearchParam.symbol_rate, sManualSearchParam.qam);
    return 0;
}

static int _dvbc_manual_search_create_cb(void)
{
    app_panel_show_prog_num(PANEL_UNDISPLAY_TIME);
    app_dvbc_search_set_tp(sManualSearchParam.fre, sManualSearchParam.symbol_rate, sManualSearchParam.qam);
    return 0;
}

static int _dvbc_manual_search_exit_cb(CSTExitType exit_type)
{
    app_panel_show_prog_num(PANEL_DISPLAY_TIME | PANEL_CLEAN_PROG_NUM);
    return CST_EXIT_RET_DEFAULT;
}


static void _dvbc_frequency_item_init(void)
{
    int32_t freq_value = 0;

    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemTitle = STR_ID_FREQUENCY;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemType = CST_ITEM_EDIT;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemProperty.itemPropertyEdit.format = "edit_float";
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemProperty.itemPropertyEdit.maxlen = "5";
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemProperty.itemPropertyEdit.intaglio = NULL;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
    sprintf(thiz_DvbcFreq, "%03d.%d", freq_value / 1000, (freq_value % 1000) / 100);
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemProperty.itemPropertyEdit.string = thiz_DvbcFreq;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemCb.editCb.EditPressEnd = _dvbc_manual_search_freq_edit_press_end;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemCb.editCb.EditPress = NULL;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_FRE].itemCb.editCb.EditReachEnd = NULL;
    sManualSearchParam.fre = freq_value;
}

static void _dvbc_symbol_item_init(void)
{
    int32_t sym_value = 0;

    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemTitle = STR_ID_SYMBOL_RATE;

#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemType = CST_ITEM_PUSH;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemProperty.itemPropertyBtn.string = "Auto";
    GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &sym_value, MAIN_FREQ_SYM_VALUE);
    sprintf(thiz_DvbcSymbol, "%04d", sym_value);
#else
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemType = CST_ITEM_EDIT;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemProperty.itemPropertyEdit.format = "edit_digit";
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemProperty.itemPropertyEdit.maxlen = "4";
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemProperty.itemPropertyEdit.intaglio = NULL;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &sym_value, MAIN_FREQ_SYM_VALUE);
    sprintf(thiz_DvbcSymbol, "%04d", sym_value);
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemProperty.itemPropertyEdit.string = thiz_DvbcSymbol;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemCb.editCb.EditPressEnd = _dvbc_manual_search_sym_edit_press_end;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemCb.editCb.EditPress = NULL;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_SYM].itemCb.editCb.EditReachEnd = NULL;
#endif
    sManualSearchParam.symbol_rate = sym_value;
}

static void _dvbc_qam_item_init(void)
{
    int32_t qam_value = 0;

    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_QAM].itemTitle = STR_ID_QAM;
#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_QAM].itemType = CST_ITEM_PUSH;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_QAM].itemProperty.itemPropertyBtn.string = "Auto";
#else
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_QAM].itemType = CST_ITEM_CHOICE;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_QAM].itemProperty.itemPropertyCmb.content = QAM_CONTENT;
    GxBus_ConfigGetInt(MAIN_FREQ_QAM_KEY, &qam_value, MAIN_FREQ_QAM_VALUE);
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_QAM].itemProperty.itemPropertyCmb.sel = qam_value;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_QAM].itemCb.cmbCb.CmbChange = _dvbc_manual_search_qam_cmb_change;
#endif
    sManualSearchParam.qam = qam_value;
}
#if ((DEMOD_DVB_COMBO > 0)&&(DEMOD_DVB_T > 0))
static void _dvbc_nit_item_init(void)
{
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_NIT].itemTitle = STR_ID_NIT_SEARCH;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_NIT].itemType = CST_ITEM_CHOICE;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_NIT].itemProperty.itemPropertyCmb.content = "[Off,On]";
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_NIT].itemProperty.itemPropertyCmb.sel = 0;
}

#endif
static void _dvbc_search_start_item_init(void)
{
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_START].itemTitle = STR_ID_START;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_START].itemType = CST_ITEM_PUSH;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_START].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    thiz_DvbcSearchItem[ITEM_C_MANUAL_SEARCH_START].itemCb.btnCb.BtnPress = _dvbc_manual_search_press_cb;
}

void app_dvbc_manual_search_menu_exec(void)
{
    GxBusPmDataSat sat = {0};

#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif
    app_get_sat_params(&sat, DEMOD_TYPE_DVBC);
    memset(&thiz_DvbcSearchOpt, 0 ,sizeof(CSTOpt));

    thiz_DvbcSearchOpt.menuTitle = STR_ID_MANUAL_SEARCH;
    thiz_DvbcSearchOpt.itemNum = ITEM_C_MANUAL_SEARCH_TOTAL;
    thiz_DvbcSearchOpt.item = thiz_DvbcSearchItem;
    thiz_DvbcSearchOpt.signal_show = true;
    thiz_DvbcSearchOpt.cur_tuner = sat.tuner;
    thiz_DvbcSearchOpt.signal.create_cb = _dvbc_manual_search_create_cb;
    thiz_DvbcSearchOpt.signal.exit_cb = _dvbc_manual_search_exit_cb;

    _dvbc_frequency_item_init();
    _dvbc_symbol_item_init();
    _dvbc_qam_item_init();
#if ((DEMOD_DVB_COMBO > 0)&&(DEMOD_DVB_T > 0))
    _dvbc_nit_item_init();
#endif
    _dvbc_search_start_item_init();

    app_cst_dialog_create(&thiz_DvbcSearchOpt);
}

int app_dvbc_auto_search_check(void)
{
#if DEMOD_DVB_T && DEMOD_DVB_C && 0 == DEMOD_DVB_S && NDEMOD_WITH_1TUNER
    int dvb_search_mode = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(0 == dvb_search_mode)
        return 1;
#elif DEMOD_DVB_C
    return 1;
#endif
    return 0;
}

int app_dvbc_manual_search_check(void)
{
#if DEMOD_DVB_T && DEMOD_DVB_C && 0 == DEMOD_DVB_S && NDEMOD_WITH_1TUNER
    int dvb_search_mode = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(0 == dvb_search_mode)
        return 1;
#elif DEMOD_DVB_C
    return 1;
#endif
    return 0;
}
#endif

