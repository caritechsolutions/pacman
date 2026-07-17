/*****************************************************************************
 * 			  CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2009-2014, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	app_dtmb_search.c
 * Author    : 	baiyingshan
 * Project   :	dvbt2 Foundation
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 1.0  	2014.06	          		 Dugan Du    	Create
 *****************************************************************************/
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

enum DTMB_SEARCH_ITEM
{
    ITEM_DTMB_SEARCH_FRE = 0,
    ITEM_DTMB_BANDWIDTH,
    ITEM_DTMB_SEARCH_START,
    ITEM_DTMB_SEARCH_TOTAL,
};

static CSTOpt thiz_DtmbSearchOpt = {0};
static CSTItem thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_TOTAL];
static char thiz_DtmbFreq[7] = {0};
static uint32_t sgPreFreq = 0;
static int32_t sgMainBandWidthInSearch = 0;

static void app_dtmb_search_set_tp(uint32_t fre)
{
    AppFrontend_SetTp params = {0};
    int32_t bandwidth = 0;
    GxBusPmDataSat sat = {0};

    app_get_sat_params(&sat, DEMOD_TYPE_DTMB);
    GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &bandwidth, MAIN_FREQ_BANDWIDTH_VALUE);

    params.type = FRONTEND_DTMB;
    params.fre = fre;
    params.bandwidth = bandwidth;

    app_set_tp(sat.tuner, &params, SET_TP_FORCE);
}

static int _dtmb_auto_search_popdlg_exit_cb(PopDlgRet ret)
{
    if ( ret == POP_VAL_OK )
    {
        ret = POP_VAL_NONE ;
        _delete_all_channels();
    }

    if ( ret == POP_VAL_NONE )
    {
        app_all_tp_del_by_sat_id(app_sat_id_get_by_type(GXBUS_PM_SAT_DTMB));
        app_send_msg_exec(GXMSG_BOOK_RESET, NULL);

        app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, true);
        app_search_set_search_type(SEARCH_DTMB);
        GUI_CreateDialog(WND_SEARCH);
        GUI_SetInterface("flush", NULL);
        app_search_scan_nit_mode();
    }

    return 0 ;
}

void app_dtmb_auto_search(void)
{
    int32_t fre = 0;
    PopDlg pop = {0};
    unsigned long temp = 0;
    uint32_t tv_num = 0;
    uint32_t radio_num = 0;
    AppFrontend_LockState lock = FRONTEND_UNLOCK;
    GxBusPmDataSat sat = {0};

#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif
    app_get_sat_params(&sat, DEMOD_TYPE_DTMB);
    GxBus_ConfigGetInt(MAIN_FREQ_KEY, &fre, MAIN_FREQ_VALUE);

    app_dtmb_search_set_tp(fre);

    temp = GxCore_TickStart(2000);
    do
    {
        GxCore_ThreadDelay(500);

        app_ioctl(sat.tuner, FRONTEND_LOCK_STATE_GET, &lock);
        if (lock == FRONTEND_LOCKED)
            break;
    }
    while (!GxCore_TickEnd(temp));

    if (lock == FRONTEND_UNLOCK)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_LOCK_FAILED;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return;
    }

    tv_num = app_check_channel_num(GXBUS_PM_PROG_TV);
    radio_num = app_check_channel_num(GXBUS_PM_PROG_RADIO);
    if ((tv_num != 0) || (radio_num != 0))
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_DEL_ALL_CHANNEL;
        pop.format = POP_FORMAT_DLG;
        pop.exit_cb = _dtmb_auto_search_popdlg_exit_cb;
        popdlg_create(&pop);
        return ;
    }

    _dtmb_auto_search_popdlg_exit_cb(POP_VAL_NONE);
    return;
}

static int _dtmb_freq_edit_press_end(void)
{
    uint32_t fre = 0;
    float edit_fre = 0;
    CSTItemData freq_data = {0};

    app_cst_item_data_get(ITEM_DTMB_SEARCH_FRE, &freq_data);
    edit_fre = app_float_edit_str_to_value(freq_data.string);
    fre = 1000* edit_fre;
    if(FALSE == app_search_check_fre_valid(fre,DEMOD_TYPE_DTMB))
    {
        memset(thiz_DtmbFreq, 0, sizeof(thiz_DtmbFreq));
        sprintf(thiz_DtmbFreq, "%03d.%d", sgPreFreq / 1000, (sgPreFreq % 1000) / 100);
        app_cst_item_data_update(ITEM_DTMB_SEARCH_FRE);
        return 0;
    }

    if(fre != sgPreFreq)
    {
        sgPreFreq = fre;
        app_dtmb_search_set_tp(sgPreFreq);
    }
    return 0;
}

static int _dtmb_search_press_cb(void)
{
    PopDlg pop = {0};
    GxBusPmDataSat sat = {0};
    AppFrontend_LockState lock = FRONTEND_UNLOCK;

    app_get_sat_params(&sat, DEMOD_TYPE_DTMB);
    app_ioctl(sat.tuner, FRONTEND_LOCK_STATE_GET, &lock);
    if(FRONTEND_UNLOCK == lock)
    {
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
    app_del_noprog_tp_by_sat_id(app_sat_id_get_by_type(GXBUS_PM_SAT_DTMB));
    app_search_set_search_type(SEARCH_DTMB);
    GUI_CreateDialog(WND_SEARCH);
    GUI_SetInterface("flush", NULL);
    app_search_scan_manual_mode(sgPreFreq, 0, 0);
    return 0;
}

static int _dtmb_search_create_cb(void)
{
    app_dtmb_search_set_tp(sgPreFreq);
    return 0;
}

static int _dtmb_search_exit_cb(CSTExitType type)
{
	int32_t band_width_value = 0;

	GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &band_width_value, MAIN_FREQ_BANDWIDTH_VALUE);
	if(sgMainBandWidthInSearch != band_width_value)
		GxBus_ConfigSetInt(MAIN_FREQ_BANDWIDTH_KEY, sgMainBandWidthInSearch);

    return 0;
}

static int _dtmb_search_bandwidth_change(int sel)
{
	GxBus_ConfigSetInt(MAIN_FREQ_BANDWIDTH_KEY, sel);
    app_dtmb_search_set_tp(sgPreFreq);
    return 0;
}

static void _dtmb_freq_item_init(void)
{
    int32_t freq_value = 0;

    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemTitle = STR_ID_FREQUENCY;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemType = CST_ITEM_EDIT;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemProperty.itemPropertyEdit.format = "edit_float";
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemProperty.itemPropertyEdit.maxlen = "5";
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemProperty.itemPropertyEdit.intaglio = NULL;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
    sprintf(thiz_DtmbFreq, "%03d.%d", freq_value / 1000, (freq_value % 1000) / 100);
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemProperty.itemPropertyEdit.string = thiz_DtmbFreq;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemCb.editCb.EditPressEnd = _dtmb_freq_edit_press_end;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemCb.editCb.EditPress = NULL;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_FRE].itemCb.editCb.EditReachEnd = NULL;
    sgPreFreq = freq_value;
    return;
}

static void _dtmb_bandwidth_item_init(void)
{
	int32_t band_width_value = 0;

	thiz_DtmbSearchItem[ITEM_DTMB_BANDWIDTH].itemTitle = STR_ID_BANDWIDTH;
    thiz_DtmbSearchItem[ITEM_DTMB_BANDWIDTH].itemType = CST_ITEM_CHOICE;
    thiz_DtmbSearchItem[ITEM_DTMB_BANDWIDTH].itemProperty.itemPropertyCmb.content="[8,7,6]";
    GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &band_width_value, MAIN_FREQ_BANDWIDTH_VALUE);
    thiz_DtmbSearchItem[ITEM_DTMB_BANDWIDTH].itemProperty.itemPropertyCmb.sel = band_width_value;
    thiz_DtmbSearchItem[ITEM_DTMB_BANDWIDTH].itemCb.cmbCb.CmbChange = _dtmb_search_bandwidth_change;
	sgMainBandWidthInSearch = band_width_value;
    return;
}


static void _dtmb_search_start_item_init(void)
{
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_START].itemTitle = STR_ID_START_SEARCH;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_START].itemType = CST_ITEM_PUSH;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_START].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    thiz_DtmbSearchItem[ITEM_DTMB_SEARCH_START].itemCb.btnCb.BtnPress = _dtmb_search_press_cb;
    return;
}

void app_dtmb_manual_search_menu_exec(void)
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
    app_get_sat_params(&sat, DEMOD_TYPE_DTMB);
    memset(&thiz_DtmbSearchOpt, 0 ,sizeof(CSTOpt));

    thiz_DtmbSearchOpt.menuTitle = STR_ID_MANUAL_SEARCH;
    thiz_DtmbSearchOpt.itemNum = ITEM_DTMB_SEARCH_TOTAL;
    thiz_DtmbSearchOpt.item = thiz_DtmbSearchItem;
    thiz_DtmbSearchOpt.signal_show = true;
    thiz_DtmbSearchOpt.cur_tuner = sat.tuner;
    thiz_DtmbSearchOpt.signal.create_cb = _dtmb_search_create_cb;
    thiz_DtmbSearchOpt.signal.exit_cb = _dtmb_search_exit_cb;

    _dtmb_freq_item_init();
	_dtmb_bandwidth_item_init();
    _dtmb_search_start_item_init();
    app_cst_dialog_create(&thiz_DtmbSearchOpt);
    return;
}
#endif
