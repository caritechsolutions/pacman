#include "app_wnd_search.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_common_search_setting.h"

#if (DEMOD_DTMB > 0)

enum DTMB_MAIN_FRE_ITEM
{
    ITEM_DTMB_MAIN_FRE = 0,
    ITEM_DTMB_MAIN_BANDWIDTH,
    ITEM_DTMB_MAIN_FRE_TOTAL,
};

static CSTOpt thiz_DtmbMainFreOpt = {0};
static CSTItem thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE_TOTAL];
static char thiz_DtmbMainFre[7] = {0};

static int _dtmb_main_freq_edit_press_end(void)
{
    uint32_t fre = 0;
    int32_t main_freq = 0;
    float edit_fre = 0;
    CSTItemData freq_data = {0};

    app_cst_item_data_get(ITEM_DTMB_MAIN_FRE, &freq_data);
    edit_fre = app_float_edit_str_to_value(freq_data.string);
    fre = 1000 * edit_fre;
    if(FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DTMB))
    {
        memset(thiz_DtmbMainFre, 0, sizeof(thiz_DtmbMainFre));
        GxBus_ConfigGetInt(MAIN_FREQ_KEY, &main_freq, MAIN_FREQ_VALUE);
        sprintf(thiz_DtmbMainFre, "%03d.%d", main_freq / 1000, (main_freq % 1000) / 100);
        app_cst_item_data_update(ITEM_DTMB_MAIN_FRE);
    }
    return 0;
}

static bool _dtmb_main_freq_check_cb(void)
{
    uint32_t fre = 0;
	int32_t band_width = 0;
	int32_t band_width_value = 0;
    float edit_fre = 0;
    int32_t freq_value = 0;
    CSTItemData freq_data = {0};
	CSTItemData band_width_data = {0};

    app_cst_item_data_get(ITEM_DTMB_MAIN_FRE, &freq_data);
    edit_fre = app_float_edit_str_to_value(freq_data.string);
    fre = 1000 * edit_fre;

    GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);

	app_cst_item_data_get(ITEM_DTMB_MAIN_BANDWIDTH, &band_width_data);
	band_width = band_width_data.value;
	GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &band_width_value, MAIN_FREQ_BANDWIDTH_VALUE);

    if(fre != freq_value || band_width != band_width_value)
        return true;

    return false;
}

static int _dtmb_main_freq_save_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        uint32_t fre = 0;
        int32_t band_width = 0;
		int32_t band_width_value = 0;
        float edit_fre = 0;
        int32_t freq_value = 0;
        CSTItemData freq_data = {0};
        CSTItemData band_width_data = {0};

        app_cst_item_data_get(ITEM_DTMB_MAIN_FRE, &freq_data);
        app_cst_item_data_get(ITEM_DTMB_MAIN_BANDWIDTH, &band_width_data);

        edit_fre = app_float_edit_str_to_value(freq_data.string);
        fre = 1000 * edit_fre;
        GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
        if(fre != freq_value)
            GxBus_ConfigSetInt(MAIN_FREQ_KEY, fre);

		band_width = band_width_data.value;
		GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &band_width_value, MAIN_FREQ_BANDWIDTH_VALUE);
		if(band_width != band_width_value)
	        GxBus_ConfigSetInt(MAIN_FREQ_BANDWIDTH_KEY, band_width);
    }
    GUI_EndDialog(WND_COMMON_SEARCH_SETTING);
    return 0;
}

static int _dtmb_main_freq_exit_cb(CSTExitType exit_type)
{
    int ret = CST_EXIT_RET_DEFAULT;
    if(CST_EXIT_ABANDON != exit_type)
    {
        if(true == app_cst_callback_handler(_dtmb_main_freq_check_cb, _dtmb_main_freq_save_pop_cb))
            ret = CST_EXIT_RET_POP;
    }
    return ret;
}

static void _dtmb_main_freq_item_init(void)
{
    int32_t freq_value = 0;

    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemTitle = STR_ID_MAIN_FREQUENCY;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemType = CST_ITEM_EDIT;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemProperty.itemPropertyEdit.format = "edit_float";
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemProperty.itemPropertyEdit.maxlen = "5";
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemProperty.itemPropertyEdit.intaglio = NULL;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
    sprintf(thiz_DtmbMainFre, "%03d.%d", freq_value / 1000, (freq_value % 1000) / 100);
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemProperty.itemPropertyEdit.string = thiz_DtmbMainFre;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemCb.editCb.EditPressEnd = _dtmb_main_freq_edit_press_end;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemCb.editCb.EditPress = NULL;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_FRE].itemCb.editCb.EditReachEnd = NULL;
}

static void _dtmb_main_freq_bandwidth_item_init(void)
{
	int32_t band_width_value = 0;

	thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_BANDWIDTH].itemTitle = STR_ID_BANDWIDTH;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_BANDWIDTH].itemType = CST_ITEM_CHOICE;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_BANDWIDTH].itemProperty.itemPropertyCmb.content="[8,7,6]";
    GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &band_width_value, MAIN_FREQ_BANDWIDTH_VALUE);
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_BANDWIDTH].itemProperty.itemPropertyCmb.sel = band_width_value;
    thiz_DtmbMainFreItem[ITEM_DTMB_MAIN_BANDWIDTH].itemCb.cmbCb.CmbChange = NULL;
}

void app_dtmb_main_freq_menu_exec(void)
{
    memset(&thiz_DtmbMainFreOpt, 0, sizeof(CSTOpt));

    thiz_DtmbMainFreOpt.menuTitle = STR_ID_MAIN_FREQ;
    thiz_DtmbMainFreOpt.itemNum = ITEM_DTMB_MAIN_FRE_TOTAL;
    thiz_DtmbMainFreOpt.item = thiz_DtmbMainFreItem;
    thiz_DtmbMainFreOpt.signal_show = false;
    thiz_DtmbMainFreOpt.signal.exit_cb = _dtmb_main_freq_exit_cb;

    _dtmb_main_freq_item_init();
    _dtmb_main_freq_bandwidth_item_init();
    app_cst_dialog_create(&thiz_DtmbMainFreOpt);
    return;
}
#endif

