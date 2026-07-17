#include "app.h"
#include "app_module.h"
#if (DEMOD_DVB_C > 0)
#include "app_wnd_search.h"
#include "app_windows.h"
#include "app_common_search_setting.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

extern void app_dvbc_search_set_main_tp(void);

typedef struct
{
    uint32_t start_fre;
    uint32_t end_fre;
    uint32_t symbol_rate;
    uint32_t qam;
}dvbc_all_search_param;

static dvbc_all_search_param sDvbcAllSearchParam = {0};

static char g_App_startFre[7] = {0};
static char g_App_endFre[7]   = {0};
static char g_sApp_Sym[5]     = {0};

enum ITEM_ALL_SEARCH
{
    ITEM_START_FREQ = 0,
    ITEM_END_FREQ,
    ITEM_SYMBOL_RATE,
    ITEM_QAM,
    ITEM_ALL_SEARCH_EXEC,
    ITEM_ALL_SEARCH_TOTAL
};

static CSTOpt s_DvbcAllSearchOpt;
static CSTItem s_DvbcAllSearchItem[ITEM_ALL_SEARCH_TOTAL];

static int _dvbc_all_search_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        uint32_t sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_C);
        app_all_channels_del_by_sat_id(sat_id);
        app_all_tp_del_by_sat_id(sat_id);
        app_search_set_search_type(SEARCH_CABLE);
        GUI_CreateDialog(WND_SEARCH);
        GUI_SetInterface("flush", NULL);
        app_search_scan_all_mode();
    }
    return 0;
}

static int _all_search_press_ok_callback(void)
{
    float fre = 0;
    uint32_t lowfre = 0;
    uint32_t highfre = 0;
    uint32_t symbol_rate = 0;
    uint32_t SearchTvNum = 0;
    uint32_t SearchRadioNum = 0;
    CSTItemData start_freq = {0};
    CSTItemData end_freq = {0};
    CSTItemData qam_data = {0};

    app_cst_item_data_get(ITEM_START_FREQ, &start_freq);
    app_cst_item_data_get(ITEM_END_FREQ, &end_freq);

#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
	symbol_rate = MAIN_FREQ_SYM_VALUE;
    qam_data.value = MAIN_FREQ_QAM_VALUE ;
#else
    CSTItemData symbol_data = {0};

    app_cst_item_data_get(ITEM_SYMBOL_RATE, &symbol_data);
    app_cst_item_data_get(ITEM_QAM, &qam_data);
#endif
    fre = app_float_edit_str_to_value(start_freq.string);
    lowfre = 1000 * fre;

    fre = app_float_edit_str_to_value(end_freq.string);
    highfre = 1000 * fre;
#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
#else
    symbol_rate = atoi(symbol_data.string);
#endif
    if(FALSE == app_search_check_fre_range_valid(lowfre, highfre,DEMOD_TYPE_DVBC)
            || FALSE == app_search_dvbc_all_search_fre_info_get(lowfre, highfre, symbol_rate, qam_data.value))
    {
        sDvbcAllSearchParam.start_fre = DVBC_FRE_BEGIN_LOW;
        sDvbcAllSearchParam.end_fre = DVBC_FRE_BEGIN_HIGH;
        app_cst_item_data_update(ITEM_START_FREQ);
        app_cst_item_data_update(ITEM_END_FREQ);
        app_cst_item_data_update(ITEM_SYMBOL_RATE);
        return EVENT_TRANSFER_KEEPON;
    }

    app_dvbc_search_set_main_tp();
    uint32_t sat_id = 0;
    sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_C);
    app_channel_num_check_by_sat_id(sat_id, &SearchTvNum, &SearchRadioNum);

    if((SearchTvNum != 0) || (SearchRadioNum != 0))
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = _dvbc_all_search_pop_cb;
        pop.str = STR_ID_TO_FULL_SEARCH;
        pop.pos.x = APP_POP_DLG_POS_X;
        pop.pos.y = APP_POP_DLG_POS_Y;

        if(popdlg_create(&pop) != POP_VAL_OK)
            return EVENT_TRANSFER_KEEPON;

    }
    else
    {
        app_all_tp_del_by_sat_id(sat_id);
        app_search_set_search_type(SEARCH_CABLE);
        GUI_CreateDialog(WND_SEARCH);
        GUI_SetInterface("flush", NULL);
        app_search_scan_all_mode();
    }

    return EVENT_TRANSFER_KEEPON;
}

static int _all_search_start_freq_edit_press_end(void)
{
    uint32_t fre = 0;
    float edit_fre = 0;
    CSTItemData data = {0};

    app_cst_item_data_get(ITEM_START_FREQ, &data);
    edit_fre = app_float_edit_str_to_value(data.string);
    fre = 1000 * edit_fre;
    if(FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DVBC))
    {
        memset(g_App_startFre, 0, sizeof(g_App_startFre));
        sprintf(g_App_startFre, "%03d.%d", sDvbcAllSearchParam.start_fre / 1000, (sDvbcAllSearchParam.start_fre % 1000) / 100);
        app_cst_item_data_update(ITEM_START_FREQ);
        return 0;
    }

    if(fre != sDvbcAllSearchParam.start_fre)
        sDvbcAllSearchParam.start_fre = fre;

    return 0;
}

static int _all_search_end_freq_edit_press_end(void)
{
    uint32_t fre = 0;
    float edit_fre = 0;
    CSTItemData data = {0};

    app_cst_item_data_get(ITEM_END_FREQ, &data);
    edit_fre = app_float_edit_str_to_value(data.string);
    fre = 1000 * edit_fre;
    if(FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DVBC))
    {
        memset(g_App_endFre, 0, sizeof(g_App_endFre));
        sprintf(g_App_endFre, "%03d.%d", sDvbcAllSearchParam.end_fre / 1000, (sDvbcAllSearchParam.end_fre % 1000) / 100);
        app_cst_item_data_update(ITEM_END_FREQ);
        return 0;
    }

    if(fre != sDvbcAllSearchParam.end_fre)
        sDvbcAllSearchParam.end_fre = fre;

    return 0;
}

#if 0 == DVBC_TUNE_AUTO_SUPPORT
static int _all_search_sym_edit_press_end(void)
{
    uint32_t symbol_rate = 0;
    CSTItemData data = {0};

    app_cst_item_data_get(ITEM_SYMBOL_RATE, &data);
    symbol_rate = atoi(data.string);
    if(FALSE == app_search_check_sym_valid(symbol_rate))
    {
        memset(g_sApp_Sym, 0, sizeof(g_sApp_Sym));
        sprintf(g_sApp_Sym, "%04d", sDvbcAllSearchParam.symbol_rate);
        app_cst_item_data_update(ITEM_SYMBOL_RATE);
        return 0;
    }

    if(symbol_rate != sDvbcAllSearchParam.symbol_rate)
        sDvbcAllSearchParam.symbol_rate = symbol_rate;

    return 0;
}
#endif

static void _start_frequency_item_init(void)
{
    uint32_t lowfre = DVBC_FRE_BEGIN_LOW;

    sDvbcAllSearchParam.start_fre = lowfre;
    memset(g_App_startFre, 0, sizeof(g_App_startFre));

    s_DvbcAllSearchItem[ITEM_START_FREQ].itemTitle = STR_ID_START_FREQ;
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemType = CST_ITEM_EDIT;
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemProperty.itemPropertyEdit.format = "edit_float";
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemProperty.itemPropertyEdit.maxlen = "5";
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    sprintf(g_App_startFre, "%03d.%d", lowfre / 1000, (lowfre % 1000) / 100);
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemProperty.itemPropertyEdit.string = g_App_startFre;

    s_DvbcAllSearchItem[ITEM_START_FREQ].itemCb.editCb.EditPressEnd = _all_search_start_freq_edit_press_end;
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemCb.editCb.EditReachEnd = NULL;
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemCb.editCb.EditPress = NULL;
}

static void _end_frequency_item_init(void)
{
    uint32_t highfre = DVBC_FRE_BEGIN_HIGH;

    sDvbcAllSearchParam.end_fre = highfre;
    memset(g_App_endFre, 0, sizeof(g_App_endFre));

    s_DvbcAllSearchItem[ITEM_END_FREQ].itemTitle = STR_ID_END_FREQ;
    s_DvbcAllSearchItem[ITEM_END_FREQ].itemType = CST_ITEM_EDIT;
    s_DvbcAllSearchItem[ITEM_END_FREQ].itemProperty.itemPropertyEdit.format = "edit_float";
    s_DvbcAllSearchItem[ITEM_END_FREQ].itemProperty.itemPropertyEdit.maxlen = "5";
    s_DvbcAllSearchItem[ITEM_END_FREQ].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_DvbcAllSearchItem[ITEM_END_FREQ].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    sprintf(g_App_endFre, "%03d.%d", highfre / 1000, (highfre % 1000) / 100);
    s_DvbcAllSearchItem[ITEM_END_FREQ].itemProperty.itemPropertyEdit.string = g_App_endFre;

    s_DvbcAllSearchItem[ITEM_END_FREQ].itemCb.editCb.EditPressEnd = _all_search_end_freq_edit_press_end;
    s_DvbcAllSearchItem[ITEM_END_FREQ].itemCb.editCb.EditReachEnd = NULL;
    s_DvbcAllSearchItem[ITEM_START_FREQ].itemCb.editCb.EditPress = NULL;
}

static void _symbol_rate_item_init(void)
{
    memset(g_sApp_Sym, 0, sizeof(g_sApp_Sym));

    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemTitle = STR_ID_SYMBOL_RATE;
#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemType = CST_ITEM_PUSH;
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyBtn.string = "Auto";
#else
    int32_t sym_value = 0;

    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemType = CST_ITEM_EDIT;
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.format = "edit_digit";
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.maxlen = "4";
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &sym_value, MAIN_FREQ_SYM_VALUE);
    sDvbcAllSearchParam.symbol_rate = sym_value;
    sprintf(g_sApp_Sym, "%04d", sym_value);
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.string = g_sApp_Sym;

    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemCb.editCb.EditPressEnd = _all_search_sym_edit_press_end;
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemCb.editCb.EditReachEnd = NULL;
    s_DvbcAllSearchItem[ITEM_SYMBOL_RATE].itemCb.editCb.EditReachEnd = NULL;
#endif
}

static void _qam_item_init(void)
{
    s_DvbcAllSearchItem[ITEM_QAM].itemTitle = STR_ID_QAM;
#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
    s_DvbcAllSearchItem[ITEM_QAM].itemType = CST_ITEM_PUSH;
    s_DvbcAllSearchItem[ITEM_QAM].itemProperty.itemPropertyBtn.string = "Auto";
#else
    int32_t qam_value = 0;

    s_DvbcAllSearchItem[ITEM_QAM].itemType = CST_ITEM_CHOICE;
    s_DvbcAllSearchItem[ITEM_QAM].itemProperty.itemPropertyCmb.content="[16QAM,32QAM,64QAM,128QAM,256QAM]";
    GxBus_ConfigGetInt(MAIN_FREQ_QAM_KEY, &qam_value, MAIN_FREQ_QAM_VALUE);
    s_DvbcAllSearchItem[ITEM_QAM].itemProperty.itemPropertyCmb.sel = qam_value;
    s_DvbcAllSearchItem[ITEM_QAM].itemCb.cmbCb.CmbChange = NULL;
#endif
}

static void _all_search_exec_item_init(void)
{
    s_DvbcAllSearchItem[ITEM_ALL_SEARCH_EXEC].itemTitle = STR_ID_START;
    s_DvbcAllSearchItem[ITEM_ALL_SEARCH_EXEC].itemType = CST_ITEM_PUSH;
    s_DvbcAllSearchItem[ITEM_ALL_SEARCH_EXEC].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    s_DvbcAllSearchItem[ITEM_ALL_SEARCH_EXEC].itemCb.btnCb.BtnPress = _all_search_press_ok_callback;
}

void app_dvbc_all_search_menu_exec(void)
{
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif
    memset(&s_DvbcAllSearchOpt, 0 ,sizeof(CSTOpt));

    s_DvbcAllSearchOpt.menuTitle = STR_ID_FULL_SEARCH;
    s_DvbcAllSearchOpt.itemNum = ITEM_ALL_SEARCH_TOTAL;
    s_DvbcAllSearchOpt.item = s_DvbcAllSearchItem;
    s_DvbcAllSearchOpt.signal_show = false;

    _start_frequency_item_init();
    _end_frequency_item_init();
    _symbol_rate_item_init();
    _qam_item_init();
    _all_search_exec_item_init();

    app_cst_dialog_create(&s_DvbcAllSearchOpt);
}

int app_dvbc_all_search_menu_check(void)
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
