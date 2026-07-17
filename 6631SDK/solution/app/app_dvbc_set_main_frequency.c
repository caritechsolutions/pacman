#include "app.h"
#include "app_module.h"
#if (DEMOD_DVB_C > 0)
#include "app_wnd_search.h"
#include "app_windows.h"
#include "app_common_search_setting.h"

enum ITEM_MAIN_FREQ_SET
{
    ITEM_MAIN_FREQ = 0,
    ITEM_SYMBOL_RATE,
    ITEM_QAM,
    ITEM_MAIN_FREQ_SET_TOTAL
};

static CSTOpt s_MainFreqSetOpt;
static CSTItem s_MainFreqSetItem[ITEM_MAIN_FREQ_SET_TOTAL];
static char App_Fre[7] = {0};
static char sApp_Sym[5] = {0};

static int _main_freq_edit_press_end(void)
{
    uint32_t fre = 0;
    int32_t value = 0;
    float edit_fre = 0;
    CSTItemData data = {0};

    app_cst_item_data_get(ITEM_MAIN_FREQ, &data);
    edit_fre = app_float_edit_str_to_value(data.string);
    fre = 1000 * edit_fre;
    if (FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DVBC))
    {
        GxBus_ConfigGetInt(MAIN_FREQ_KEY, &value, MAIN_FREQ_VALUE);
        memset(App_Fre, 0, sizeof(App_Fre));
        sprintf(App_Fre, "%03d.%d", value / 1000, (value % 1000) / 100);
        app_cst_item_data_update(ITEM_MAIN_FREQ);
    }

    return 0;
}

#if 0 == DVBC_TUNE_AUTO_SUPPORT
static int _sym_edit_press_end(void)
{
    int32_t value = 0;
    uint32_t symbol_rate = 0;
    CSTItemData data = {0};

    app_cst_item_data_get(ITEM_SYMBOL_RATE, &data);
    symbol_rate = atoi(data.string);
    if(FALSE == app_search_check_sym_valid(symbol_rate))
    {
        GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &value, MAIN_FREQ_SYM_VALUE);
        memset(sApp_Sym, 0, sizeof(sApp_Sym));
        sprintf(sApp_Sym, "%04d", value);
        app_cst_item_data_update(ITEM_SYMBOL_RATE);
    }

    return 0;
}
#endif

static void _main_frequency_item_init(void)
{
    int32_t freq_value = 0;

    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemTitle = STR_ID_MAIN_FREQUENCY;
    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemType = CST_ITEM_EDIT;
    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemProperty.itemPropertyEdit.format = "edit_float";
    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemProperty.itemPropertyEdit.maxlen = "5";
    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
    sprintf(App_Fre, "%03d.%d", freq_value / 1000, (freq_value % 1000) / 100);
    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemProperty.itemPropertyEdit.string = App_Fre;

    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemCb.editCb.EditPressEnd = _main_freq_edit_press_end;
    s_MainFreqSetItem[ITEM_MAIN_FREQ].itemCb.editCb.EditReachEnd = NULL;
}

static void _symbol_rate_item_init(void)
{
    int32_t sym_value = 0;

    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemTitle = STR_ID_SYMBOL_RATE;

    GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &sym_value, MAIN_FREQ_SYM_VALUE);
    sprintf(sApp_Sym, "%04d", sym_value);
#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemType = CST_ITEM_PUSH;
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyBtn.string= "Auto";
#else
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemType = CST_ITEM_EDIT;
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.format = "edit_digit";
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.maxlen = "4";
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemProperty.itemPropertyEdit.string = sApp_Sym;

    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemCb.editCb.EditPressEnd = _sym_edit_press_end;
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemCb.editCb.EditReachEnd = NULL;
    s_MainFreqSetItem[ITEM_SYMBOL_RATE].itemCb.editCb.EditPress = NULL;
#endif
}

static void _qam_item_init(void)
{
    s_MainFreqSetItem[ITEM_QAM].itemTitle = STR_ID_QAM;
#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
    s_MainFreqSetItem[ITEM_QAM].itemType = CST_ITEM_PUSH;
    s_MainFreqSetItem[ITEM_QAM].itemProperty.itemPropertyBtn.string= "Auto";
#else
    int32_t qam_value = 0;

    s_MainFreqSetItem[ITEM_QAM].itemType = CST_ITEM_CHOICE;
    s_MainFreqSetItem[ITEM_QAM].itemProperty.itemPropertyCmb.content="[16QAM,32QAM,64QAM,128QAM,256QAM]";
    GxBus_ConfigGetInt(MAIN_FREQ_QAM_KEY, &qam_value, MAIN_FREQ_QAM_VALUE);
    s_MainFreqSetItem[ITEM_QAM].itemProperty.itemPropertyCmb.sel = qam_value;
    s_MainFreqSetItem[ITEM_QAM].itemCb.cmbCb.CmbChange = NULL;
#endif
}

static bool _main_freq_check_cb(void)
{
    int32_t fre = 0;
    int32_t freq_value = 0;
    int32_t sym_value = 0;
    int32_t qam_value = 0;
    float edit_fre = 0;
    bool ret = false;
    CSTItemData freq_data = {0};
    CSTItemData symbol_data = {0};
    CSTItemData qam_data = {0};

    app_cst_item_data_get(ITEM_MAIN_FREQ, &freq_data);
    app_cst_item_data_get(ITEM_SYMBOL_RATE, &symbol_data);
    app_cst_item_data_get(ITEM_QAM, &qam_data);

    edit_fre = app_float_edit_str_to_value(freq_data.string);
    fre = 1000 * edit_fre;
    if(FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DVBC))
        return ret;

#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
#else
    int32_t qam = 0;
    int32_t symbol_rate = 0;

    symbol_rate = atoi(symbol_data.string);
    if(FALSE == app_search_check_sym_valid(symbol_rate))
        return ret;

    qam = qam_data.value;
#endif
    GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
    GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &sym_value, MAIN_FREQ_SYM_VALUE);
    GxBus_ConfigGetInt(MAIN_FREQ_QAM_KEY, &qam_value, MAIN_FREQ_QAM_VALUE);

    if(fre != freq_value)
        ret = true;

#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
#else
    if(symbol_rate != sym_value)
        ret = true;

    if(qam != qam_value)
        ret = true;
#endif
    return ret;
}

static int _main_freq_save_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        int32_t fre = 0;
        int32_t freq_value = 0;
        int32_t sym_value = 0;
        int32_t qam_value = 0;
        float edit_fre = 0;
        CSTItemData freq_data = {0};
        CSTItemData symbol_data = {0};
        CSTItemData qam_data = {0};

        app_cst_item_data_get(ITEM_MAIN_FREQ, &freq_data);
        app_cst_item_data_get(ITEM_SYMBOL_RATE, &symbol_data);
        app_cst_item_data_get(ITEM_QAM, &qam_data);

        edit_fre = app_float_edit_str_to_value(freq_data.string);
        fre = 1000 * edit_fre;
        if(FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DVBC))
            goto end;

#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
#else
        int32_t qam = 0;
        int32_t symbol_rate = 0;

        symbol_rate = atoi(symbol_data.string);
        if(FALSE == app_search_check_sym_valid(symbol_rate))
            goto end;

        qam = qam_data.value;
#endif
        GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
        GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &sym_value, MAIN_FREQ_SYM_VALUE);
        GxBus_ConfigGetInt(MAIN_FREQ_QAM_KEY, &qam_value, MAIN_FREQ_QAM_VALUE);

        GxBus_ConfigSetInt(MAIN_FREQ_KEY, fre);

#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
#else
        GxBus_ConfigSetInt(MAIN_FREQ_SYM_KEY, symbol_rate);

        GxBus_ConfigSetInt(MAIN_FREQ_QAM_KEY, qam);
#endif
    }

end:
    GUI_EndDialog(WND_COMMON_SEARCH_SETTING);
    return 0;
}

static int _dvbc_set_main_frequency_exit_callback(CSTExitType exit_type)
{
    int ret = CST_EXIT_RET_DEFAULT;

    if(CST_EXIT_ABANDON != exit_type)
    {
        if(true == app_cst_callback_handler(_main_freq_check_cb, _main_freq_save_pop_cb))
            ret = CST_EXIT_RET_POP;
    }

    return ret;
}

void app_dvbc_set_main_frequency_menu_exec(void)
{
    memset(&s_MainFreqSetOpt, 0, sizeof(CSTOpt));

    s_MainFreqSetOpt.menuTitle = STR_ID_MAIN_FREQ;
    s_MainFreqSetOpt.itemNum = ITEM_MAIN_FREQ_SET_TOTAL;
    s_MainFreqSetOpt.item = s_MainFreqSetItem;

    _main_frequency_item_init();
    _symbol_rate_item_init();
    _qam_item_init();
    s_MainFreqSetOpt.signal_show = false;
    s_MainFreqSetOpt.signal.exit_cb = _dvbc_set_main_frequency_exit_callback;

    app_cst_dialog_create(&s_MainFreqSetOpt);
}

int app_dvbc_set_main_frequency_menu_check(void)
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
