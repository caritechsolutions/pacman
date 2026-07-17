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

#if (DEMOD_DTMB > 0)

typedef struct
{
    uint32_t start_fre;
    uint32_t end_fre;
}dtmb_all_search_param;

enum DTMB_ALL_SEARCH_ITEM
{
    ITEM_DTMB_ALL_SEARCH_START_FRE = 0,
    ITEM_DTMB_ALL_SEARCH_END_FRE,
	ITEM_DTMB_ALL_SEARCH_BANDWIDTH,
    ITEM_DTMB_ALL_SEARCH_START,
    ITEM_DTMB_ALL_SEARCH_TOTAL,
};

static CSTOpt thiz_DtmbAllSearchOpt = {0};
static CSTItem thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_TOTAL];
static char thiz_DtmbStartFreq[7] = {0};
static char thiz_DtmbEndFreq[7] = {0};
static dtmb_all_search_param sDtmbAllSearchParam = {0};
static int32_t sgMainBandWidthInAllSearch = 0;


static int _dtmb_all_search_start_freq_edit_press_end(void)
{
    uint32_t fre = 0;
    float edit_fre = 0;
    CSTItemData freq_data = {0};

    app_cst_item_data_get(ITEM_DTMB_ALL_SEARCH_START_FRE, &freq_data);
    edit_fre = app_float_edit_str_to_value(freq_data.string);
    fre = 1000* edit_fre;
    if(FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DTMB))
    {
        memset(thiz_DtmbStartFreq, 0, sizeof(thiz_DtmbStartFreq));
        sprintf(thiz_DtmbStartFreq, "%03d.%d", sDtmbAllSearchParam.start_fre / 1000, (sDtmbAllSearchParam.start_fre % 1000) / 100);
        app_cst_item_data_update(ITEM_DTMB_ALL_SEARCH_START_FRE);
        return 0;
    }

    if(fre != sDtmbAllSearchParam.start_fre)
        sDtmbAllSearchParam.start_fre = fre;

    return 0;
}

static int _dtmb_all_search_end_freq_edit_press_end(void)
{
    uint32_t fre = 0;
    float edit_fre = 0;
    CSTItemData freq_data = {0};

    app_cst_item_data_get(ITEM_DTMB_ALL_SEARCH_END_FRE, &freq_data);
    edit_fre = app_float_edit_str_to_value(freq_data.string);
    fre = 1000* edit_fre;
    if(FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DTMB))
    {
        memset(thiz_DtmbEndFreq, 0, sizeof(thiz_DtmbEndFreq));
        sprintf(thiz_DtmbEndFreq, "%03d.%d", sDtmbAllSearchParam.end_fre / 1000, (sDtmbAllSearchParam.end_fre % 1000) / 100);
        app_cst_item_data_update(ITEM_DTMB_ALL_SEARCH_END_FRE);
        return 0;
    }

    if(fre != sDtmbAllSearchParam.end_fre)
        sDtmbAllSearchParam.end_fre = fre;
    return 0;
}

static int _dtmb_all_search_bandwidth_change(int sel)
{
	return GxBus_ConfigSetInt(MAIN_FREQ_BANDWIDTH_KEY, sel);
}

static int _dtmb_all_search_popdlg_exit_cb(PopDlgRet ret)
{
    uint32_t sat_id = 0;

    sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_DTMB);

    if ( ret == POP_VAL_OK )
    {
        ret = POP_VAL_NONE ;
        app_all_channels_del_by_sat_id(sat_id);
    }

    if ( ret == POP_VAL_NONE )
    {
        app_all_tp_del_by_sat_id(sat_id);
        app_search_set_search_type(SEARCH_DTMB);
        GUI_CreateDialog(WND_SEARCH);
        GUI_SetInterface("flush", NULL);
        app_search_scan_all_mode();
    }

    return 0 ;
}

static int _dtmb_all_search_press_cb(void)
{
    PopDlg pop;
    uint32_t tv_num = 0;
    uint32_t radio_num = 0;
    uint32_t sat_id = 0;

    if(FALSE == app_search_dtmb_all_search_fre_info_get(sDtmbAllSearchParam.start_fre, sDtmbAllSearchParam.end_fre, 0, 0))
        return 0;

    sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_DTMB);
    app_channel_num_check_by_sat_id(sat_id, &tv_num, &radio_num);
    if((tv_num != 0) || (radio_num != 0))
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_DEL_ALL_CHANNEL;
        pop.format = POP_FORMAT_DLG;
        pop.exit_cb = _dtmb_all_search_popdlg_exit_cb;
        popdlg_create(&pop);
        return 0 ;
    }

    _dtmb_all_search_popdlg_exit_cb(POP_VAL_NONE);
    return 0;
}

static int _dtmb_all_search_exit_cb(CSTExitType type)
{
	int32_t band_width_value = 0;

	GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &band_width_value, MAIN_FREQ_BANDWIDTH_VALUE);
	if(sgMainBandWidthInAllSearch != band_width_value)
		GxBus_ConfigSetInt(MAIN_FREQ_BANDWIDTH_KEY, sgMainBandWidthInAllSearch);

    return 0;
}


static void _dtmb_all_search_start_freq_item_init(void)
{
    int32_t freq_value = DTMB_FRE_BEGIN_LOW;

    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemTitle = STR_ID_START_FREQ;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemType = CST_ITEM_EDIT;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemProperty.itemPropertyEdit.format = "edit_float";
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemProperty.itemPropertyEdit.maxlen = "5";
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemProperty.itemPropertyEdit.intaglio = NULL;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    sprintf(thiz_DtmbStartFreq, "%03d.%d", freq_value / 1000, (freq_value % 1000) / 100);
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemProperty.itemPropertyEdit.string = thiz_DtmbStartFreq;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemCb.editCb.EditPressEnd = _dtmb_all_search_start_freq_edit_press_end;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemCb.editCb.EditPress = NULL;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START_FRE].itemCb.editCb.EditReachEnd = NULL;
    sDtmbAllSearchParam.start_fre = freq_value;
}

static void _dtmb_all_search_end_freq_item_init(void)
{
    int32_t freq_value = DTMB_FRE_BEGIN_HIGH;

    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemTitle = STR_ID_END_FREQ;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemType = CST_ITEM_EDIT;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemProperty.itemPropertyEdit.format = "edit_float";
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemProperty.itemPropertyEdit.maxlen = "5";
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemProperty.itemPropertyEdit.intaglio = NULL;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    sprintf(thiz_DtmbEndFreq, "%03d.%d", freq_value / 1000, (freq_value % 1000) / 100);
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemProperty.itemPropertyEdit.string = thiz_DtmbEndFreq;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemCb.editCb.EditPressEnd = _dtmb_all_search_end_freq_edit_press_end;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemCb.editCb.EditPress = NULL;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_END_FRE].itemCb.editCb.EditReachEnd = NULL;
    sDtmbAllSearchParam.end_fre = freq_value;
}

static void _dtmb_all_search_bandwidth_item_init(void)
{
	int32_t band_width_value = 0;

	thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_BANDWIDTH].itemTitle = STR_ID_BANDWIDTH;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_BANDWIDTH].itemType = CST_ITEM_CHOICE;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_BANDWIDTH].itemProperty.itemPropertyCmb.content="[8,7,6]";
    GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &band_width_value, MAIN_FREQ_BANDWIDTH_VALUE);
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_BANDWIDTH].itemProperty.itemPropertyCmb.sel = band_width_value;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_BANDWIDTH].itemCb.cmbCb.CmbChange = _dtmb_all_search_bandwidth_change;
	sgMainBandWidthInAllSearch = band_width_value;
    return;
}


static void _dtmb_all_search_start_item_init(void)
{
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START].itemTitle = STR_ID_START_SEARCH;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START].itemType = CST_ITEM_PUSH;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    thiz_DtmbAllSearchItem[ITEM_DTMB_ALL_SEARCH_START].itemCb.btnCb.BtnPress = _dtmb_all_search_press_cb;
}

void app_dtmb_all_search_menu_exec(void)
{
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif
    memset(&thiz_DtmbAllSearchOpt, 0, sizeof(CSTOpt));

    thiz_DtmbAllSearchOpt.menuTitle = STR_ID_FULL_SEARCH;
    thiz_DtmbAllSearchOpt.itemNum = ITEM_DTMB_ALL_SEARCH_TOTAL;
    thiz_DtmbAllSearchOpt.item = thiz_DtmbAllSearchItem;
    thiz_DtmbAllSearchOpt.signal_show = false;
    thiz_DtmbAllSearchOpt.signal.exit_cb = _dtmb_all_search_exit_cb;

    _dtmb_all_search_start_freq_item_init();
    _dtmb_all_search_end_freq_item_init();
	_dtmb_all_search_bandwidth_item_init();
    _dtmb_all_search_start_item_init();
    app_cst_dialog_create(&thiz_DtmbAllSearchOpt);
    return;
}
#endif

