/*****************************************************************************
 * 			  CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2009-2014, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	app_dvbt2_search.c
 * Author    : 	baiyingshan
 * Project   :	dvbt2 Foundation
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 1.0  	2014.06	          		 Dugan Du    	Create
 *****************************************************************************/
//public header files
#include "app.h"

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "full_screen.h"
#include "app_book.h"
#include "app_windows.h"
#include "app_dvbt2_search.h"
#include "app_volume_value.h"
#include "app_common_search_setting.h"
#include "module/app_log.h"
#include "module/app_frontend_board_config.h"
#if LCN_SUPPORT
#include "app_lcn.h"
#endif
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif

#define DEBUG_DVBT2_SEARCH
#ifdef DEBUG_DVBT2_SEARCH
#define dvbt2_search_printf		app_log_debug
#else
#define dvbt2_search_printf
#endif

#define DVBT_FRE_BEGIN_LOW (50000)
#define DVBT_FRE_BEGIN_HIGH (866000)

#define MAX_NIT_SECTION_NUM				(32)
#define TP_MAX_NUM						(200)

#define MAIN_FREQ_DEFAULT_VALUE			(850000)
#define MAIN_BANDWIDTH_DEFAULT_VALUE	(0)

#if DEMOD_DVB_T
#define EDIT_FRE_LENMAX		"5"
#endif
#if DEMOD_ISDBT
#define EDIT_FRE_LENMAX		"7"
#endif

#define MAX_CH_FRE_LEN 8

enum ITEM_DVBT_MANUAL_SEARCH
{
    ITEM_DVBT_CHANNEL = 0,
    ITEM_DVBT_FREQ,
    ITEM_DVBT_BANDWIDTH,
    ITEM_DVBT_SEARCH_START,
    ITEM_DVBT_SEARCH_TOTAL
};

#ifdef DVB_T2_NIT_SUPPORT //nit search need test
typedef struct
{
    uint32_t app_fre_array[TP_MAX_NUM];
    uint32_t app_band_array[TP_MAX_NUM];
    uint32_t app_fre_tsid[TP_MAX_NUM];
    uint16_t num;
    uint8_t  nit_recv_num;
    uint8_t  *nit_subtable_flag;
    uint8_t  nit_flag;
} classDvbt2SearchNit;

static classDvbt2SearchNit thizDvbt2NitResult ;
static volatile uint8_t thizNitParseFlag = 0;
#endif

static classDefaultFreqCh *thizFreqChannel = NULL;
static event_list *sp_Dvbt2LockTimer = NULL;
static SetTpMode s_set_tp_mode = SET_TP_AUTO;
static uint32_t thizDvbt2SatId = 0;
static uint32_t thizDvbt2TpidArray[TP_MAX_NUM];
static uint32_t thizDvbt2TsidArray[TP_MAX_NUM] ;
static CSTOpt thiz_DvbtSearchOpt;
static CSTItem thiz_DvbtSearchItem[ITEM_DVBT_SEARCH_TOTAL];
static char *thiz_ChannelBuffer = NULL;
static char thiz_DvbtFreq[MAX_CH_FRE_LEN] = {0};
extern status_t _modify_prog_callback(GxBusPmDataProg* prog);

#define DVBT_BANDWIDTH_CONTENT "[8,7,6]"


#ifdef DVB_T2_NIT_SUPPORT
static void dvbt2_nit_scan_tp(GxMsgProperty_ScanDvbt2Start *p_msg_dvbt2_scan);
static uint32_t app_t2_nit_auto_search_get_tp(uint32_t sat_id, uint32_t *tp_array);
#endif

int app_t2_search_check_fre_valid(uint32_t fre)
{
#define DVBT2_CHECK_INFO_BUF_SIZE 100
    char buf[DVBT2_CHECK_INFO_BUF_SIZE] = {0};
    PopDlg pop;
    char *i18n_str = NULL;
    if (fre < DVBT_FRE_BEGIN_LOW)
    {
        i18n_str =(char*)GXGDI_I18N_String(STR_ID_FREQ_LESS_THAN);
        memset(buf,0,DVBT2_CHECK_INFO_BUF_SIZE);
        sprintf((void *)buf, "%s %.1fMHz", i18n_str,(float)(DVBT_FRE_BEGIN_LOW) / 1000);
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = buf;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }

    if (fre > DVBT_FRE_BEGIN_HIGH)
    {
        i18n_str =(char*)GXGDI_I18N_String(STR_ID_FREQ_LARGER_THAN);
        memset(buf,0,DVBT2_CHECK_INFO_BUF_SIZE);
        sprintf((void *)buf, "%s %.1fMHz", i18n_str,(float)(DVBT_FRE_BEGIN_HIGH) / 1000);
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = buf;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }

    return TRUE;
}

#define APP_T2_MAX_FRE_LEN 7
static unsigned int app_t2_get_fre_atof_multiply1000(char *fre_in)//first atof then multiply 1000
{
    int i = 0, j = 0;
    unsigned int fre_out = 0;
    char temp_fre[APP_T2_MAX_FRE_LEN + 1] = {0};
    for (i = 0; i < APP_T2_MAX_FRE_LEN; i++)
    {
        if (fre_in[i] >= '0' && fre_in[i] <= '9')
        {
            temp_fre[j++] = fre_in[i];
        }
        else if (fre_in[i] == '.')
        {
            temp_fre[j++] = fre_in[++i];
            temp_fre[j++] = '0';
            temp_fre[j++] = '0';
            temp_fre[j++] = '\0';
            break;
        }
        else
        {
            return 0;
        }
    }
    fre_out = atoi(temp_fre);
    return fre_out;
}

static void app_t2_manual_search_set_tp(int tuner)
{
    AppFrontend_SetTp params = {0};
    char freq[APP_T2_MAX_FRE_LEN] = {0};
    CSTItemData freq_data = {0};
    CSTItemData bandwidth_data = {0};

    app_cst_item_data_get(ITEM_DVBT_FREQ, &freq_data);
#if DEMOD_DVB_T
    app_cst_item_data_get(ITEM_DVBT_BANDWIDTH, &bandwidth_data);
#elif DEMOD_ISDBT
    bandwidth_data.value = 2;	//6M
#endif

    memcpy(freq, freq_data.string, APP_T2_MAX_FRE_LEN);
    params.fre = app_t2_get_fre_atof_multiply1000(freq);
    params.type = FRONTEND_DVB_T2;
    params.bandwidth = bandwidth_data.value;
    params.data_plp_id = 0xff;
    params.common_plp_exist = 0xff;
    params.common_plp_id = 0xff;
    params.set_tp_mode = SET_AUTO_PLP;

    params.type_1501 = DVBT_AUTO_MODE;
    app_set_tp(tuner, &params, SET_TP_FORCE);
}

static int32_t app_t2_manual_search_get_tp(uint32_t sat_id, uint32_t *tp_array)
{
    int list_num = 0;
    int32_t  ret = 0;
    GxBusPmDataTP tp = {0};
    char freq[APP_T2_MAX_FRE_LEN] = {0};
    uint32_t freq_tmp = 0;
    uint32_t tpid_temp = 0;
    CSTItemData freq_data = {0};
    CSTItemData bandwidth_data = {0};

    list_num = 1;

    app_cst_item_data_get(ITEM_DVBT_FREQ, &freq_data);
    memcpy(freq, freq_data.string, APP_T2_MAX_FRE_LEN);
    freq_tmp = app_t2_get_fre_atof_multiply1000(freq);
#if DEMOD_DVB_T
    app_cst_item_data_get(ITEM_DVBT_BANDWIDTH, &bandwidth_data);
#elif DEMOD_ISDBT
    bandwidth_data.value = 2;	//6M
#endif

    //GxBus_PmTpExistChek can't use because of param freq is uint16_t
    ret = GxBus_PmTpExistChek((uint16_t)sat_id, (uint32_t) freq_tmp, bandwidth_data.value, 0, 0, (uint16_t *)&tpid_temp);

    if (ret == 0)
    {
        tp.sat_id = sat_id;
        tp.frequency = freq_tmp;
        tp.tp_dvbt2.bandwidth = bandwidth_data.value;
        ret = GxBus_PmTpAdd(&tp);
        if (GXCORE_SUCCESS != ret)
        {
            //tp加入不成功
            app_log_info("can't add Tp\n");
            //diag error datebase error
            return -1;
            //not finish
        }
        tp_array[0] = tp.id;
    }
    else if (ret > 0)
    {
        tp_array[0]  = tpid_temp;
        if (GXCORE_SUCCESS == GxBus_PmTpGetById(tpid_temp, &tp))
        {
            if (tp.tp_dvbt2.bandwidth != bandwidth_data.value)
            {
                tp.tp_dvbt2.bandwidth = bandwidth_data.value;
                GxBus_PmTpModify(&tp);
            }
        }
    }
    else
    {
        dvbt2_search_printf("[DVBT2 Search]can't add Tp\n");
        return -1;
    }

    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    return list_num;
}

static uint32_t app_t2_default_auto_search_get_tp(uint32_t sat_id, uint32_t *tp_array)
{
    int list_num = 0;
    int i = 0;
    int32_t  ret = 0;
    GxBusPmDataTP tp = {0};
    uint16_t tpid_temp;

    list_num = AppDvbt2GetFreqList(&thizFreqChannel);
    if (TP_MAX_NUM < list_num)
    {
        list_num = TP_MAX_NUM;
    }
    for (i = 0; i < list_num; i++)
    {
        ret = GxBus_PmTpExistChek((uint16_t)sat_id, (uint32_t)1000 * thizFreqChannel[i].freq,
                (uint32_t)thizFreqChannel[i].band, 0, 0, (uint16_t *)&tpid_temp);
        if (ret == 0)
        {
            tp.sat_id = sat_id;
            tp.frequency = 1000 * thizFreqChannel[i].freq;
            {
                tp.tp_dvbt2.bandwidth  = thizFreqChannel[i].band;
            }
            ret = GxBus_PmTpAdd(&tp);
            if (GXCORE_SUCCESS != ret)
            {
                //tp加入不成功
                app_log_error("can't add Tp\n");
                //diag error datebase error
                break;
                //not finish
            }
            tp_array[i] = tp.id;
        }
        else if (ret > 0)
        {
            tp_array[i] = tpid_temp;
        }
        else
        {
            dvbt2_search_printf("[DVBT2 Search]can't add Tp\n");
            return -1;
        }
    }

    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    return list_num;
}

static int app_t2_manual_search_start(void)
{
    GxMsgProperty_ScanDvbt2Start msg_dvbt2_scan = {0,};
    int32_t tp_max_num = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    AppFrontend_Config cfg = {0};
    int i = 0;
    GxBusPmDataSat sat = {0};

    memset(thizDvbt2TpidArray, 0, sizeof(uint32_t)*TP_MAX_NUM);
    memset(thizDvbt2TsidArray, 0, sizeof(uint32_t)*TP_MAX_NUM);

    if (0 ==  thizDvbt2SatId || GxBus_PmSatGetById(thizDvbt2SatId, &sat) == GXCORE_ERROR)
    {
        thizDvbt2SatId = app_sat_id_get_by_type(GXBUS_PM_SAT_DVBT2);
        if(0 == thizDvbt2SatId)
        {
            //msg
        }
    }
    app_del_noprog_tp_by_sat_id(thizDvbt2SatId);
    tp_max_num = app_t2_manual_search_get_tp(thizDvbt2SatId, thizDvbt2TpidArray);
    if (tp_max_num <= 0)
        return -1;

    node.node_type = NODE_SAT;
    node.id = thizDvbt2SatId;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
    for (i = 0; i < tp_max_num; i++)
    {
        thizDvbt2TsidArray[i] = cfg.ts_src;
    }

    //dvbt2_search_printf("ts_id = %d\n",thizDvbt2TsidArray[0]);

    msg_dvbt2_scan.scan_type = GX_SEARCH_MANUAL;
    msg_dvbt2_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    int32_t fta_cas_mode;
    GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &fta_cas_mode, SEARCH_FTA_CAS_VALUE);
    msg_dvbt2_scan.fta_cas = fta_cas_mode;//mode
    msg_dvbt2_scan.nit_switch = GX_SEARCH_NIT_DISABLE;

    msg_dvbt2_scan.params_t.sat_id = thizDvbt2SatId ;
    msg_dvbt2_scan.params_t.max_num = tp_max_num;
    msg_dvbt2_scan.params_t.array = thizDvbt2TpidArray;
    msg_dvbt2_scan.params_t.ts = thizDvbt2TsidArray;
    // needn't ext
    msg_dvbt2_scan.ext = NULL;
    msg_dvbt2_scan.ext_num = 0;

    msg_dvbt2_scan.time_out.pat = TIMEOUT_T_PAT;
    msg_dvbt2_scan.time_out.sdt = TIMEOUT_T_SDT;
    msg_dvbt2_scan.time_out.nit = TIMEOUT_T_NIT;
    msg_dvbt2_scan.time_out.pmt = TIMEOUT_T_PMT;

    msg_dvbt2_scan.modify_prog = _modify_prog_callback;

    //display search window right now
    app_search_set_search_type(SEARCH_DVBT2);
    app_search_set_search_mode(MANUAL_SEARCH);
    GUI_CreateDialog(WND_SEARCH);
    GUI_SetInterface("flush", NULL);
    APP_TIMER_REMOVE(sp_Dvbt2LockTimer);
    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, false);

#if LCN_SUPPORT
	static GxSearchExtend SearchExtParse[SEARCH_EXTEND_MAX];

    if(app_search_add_extend_callback != NULL)
        app_search_add_extend_callback(&g_SearchExtendList);
    if(g_SearchExtendList.extendnum > 0)
    {
		memset(SearchExtParse,0,SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        msg_dvbt2_scan.ext_num = g_SearchExtendList.extendnum;
        memcpy(&SearchExtParse[0], &g_SearchExtendList.searchExtend[0], SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        msg_dvbt2_scan.ext = &SearchExtParse[0];
    }
    app_lcn_list_init(&thizDvbt2SatId, 1);
#endif
    app_send_msg_exec(GXMSG_SEARCH_SCAN_DVBT2_START, (void *)&msg_dvbt2_scan);

    return 0;
}

static void _dvbt2_auto_search_start(void)
{
    GxMsgProperty_ScanDvbt2Start msg_dvbt2_scan = {0,};
    GxMsgProperty_NodeByIdGet node = {0};
    AppFrontend_Config cfg = {0};
    uint8_t tp_max_num = 0;
    int32_t i = 0, mode = 0;
    uint32_t sat_id = 0;

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

    sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_DVBT2);

    GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &mode, SEARCH_FTA_CAS_VALUE);

    memset(thizDvbt2TpidArray, 0, sizeof(uint32_t)*TP_MAX_NUM);
    memset(thizDvbt2TsidArray, 0, sizeof(uint32_t)*TP_MAX_NUM);

    app_search_set_search_type(SEARCH_DVBT2);
    app_search_set_search_mode(AUTO_SEARCH);
    node.node_type = NODE_SAT;
    node.id = sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
    app_tuner_cur_tuner_set(node.sat_data.tuner);

#if NDEMOD_WITH_1TUNER && (APP_FRONTEND_PIN_SWITCH_COUNT || APP_FRONTEND_DEMOD_ADAPT_PIN_SWITCH_COUNT)
    extern void app_pin_func_switch(int tuner);
    app_pin_func_switch(node.sat_data.tuner);
#endif
    app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
    for (i = 0; i < TP_MAX_NUM; i++)
    {
        thizDvbt2TsidArray[i] = cfg.ts_src;
    }

    msg_dvbt2_scan.scan_type = GX_SEARCH_MANUAL;
    msg_dvbt2_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    msg_dvbt2_scan.fta_cas = mode;
    msg_dvbt2_scan.nit_switch = GX_SEARCH_NIT_DISABLE;//nit enable for auto scan

    msg_dvbt2_scan.params_t.sat_id = sat_id;
    msg_dvbt2_scan.params_t.ts = thizDvbt2TsidArray;
    // needn't ext
    msg_dvbt2_scan.ext = NULL;
    msg_dvbt2_scan.ext_num = 0;

    msg_dvbt2_scan.time_out.pat = TIMEOUT_T_PAT;
    msg_dvbt2_scan.time_out.sdt = TIMEOUT_T_SDT;
    msg_dvbt2_scan.time_out.nit = TIMEOUT_T_NIT;
    msg_dvbt2_scan.time_out.pmt = TIMEOUT_T_PMT;

    msg_dvbt2_scan.modify_prog = _modify_prog_callback;

#ifdef DVB_T2_NIT_SUPPORT //nit search need test
    memset(&thizDvbt2NitResult, 0, sizeof(thizDvbt2NitResult));
    thizNitParseFlag = 0;

    params.fre = MAIN_FREQ_DEFAULT_VALUE;
    params.type = FRONTEND_DVB_T2;
    params.bandwidth = MAIN_BANDWIDTH_DEFAULT_VALUE ;
    params.data_plp_id = 0xff;
    params.common_plp_exist = 0xff;
    params.common_plp_id = 0xff;

    params.type_1501 = DVBT_AUTO_MODE;
    app_set_tp(node.sat_data.tuner, &params, SET_TP_FORCE);

    GxCore_ThreadDelay(100);

    AppFrontend_LockState lock;
    app_ioctl(node.sat_data.tuner, FRONTEND_LOCK_STATE_GET, &lock);

    if (FRONTEND_LOCKED == lock)
        dvbt2_nit_scan_tp(&msg_dvbt2_scan);

    if (thizDvbt2NitResult.num > 0)
        tp_max_num = app_t2_nit_auto_search_get_tp(sat_id, thizDvbt2TpidArray);
    else
#endif
        tp_max_num = app_t2_default_auto_search_get_tp(sat_id, thizDvbt2TpidArray);

    if(tp_max_num <= 0)
    {
        dvbt2_search_printf("tp_max_num  = %d !!error\n", tp_max_num);
        return;
    }
    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, true);
    GxFrontend_CleanRecordPara(node.sat_data.tuner);

    GUI_CreateDialog(WND_SEARCH);

    msg_dvbt2_scan.params_t.max_num = tp_max_num;
    msg_dvbt2_scan.params_t.array = thizDvbt2TpidArray;

#if LCN_SUPPORT
	static GxSearchExtend SearchExtParse[SEARCH_EXTEND_MAX];

    if(app_search_add_extend_callback != NULL)
        app_search_add_extend_callback(&g_SearchExtendList);

    if(g_SearchExtendList.extendnum > 0)
    {
		memset(SearchExtParse,0,SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        msg_dvbt2_scan.ext_num = g_SearchExtendList.extendnum;
        memcpy(&SearchExtParse[0], &g_SearchExtendList.searchExtend[0], SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        msg_dvbt2_scan.ext = &SearchExtParse[0];
    }
    app_lcn_list_init(&thizDvbt2SatId, 1);
#endif
    app_send_msg_exec(GXMSG_SEARCH_SCAN_DVBT2_START, (void *)&msg_dvbt2_scan);
    return;
}

static int _dvbt2_prog_delete_cb(PopDlgRet ret)
{
    uint32_t sat_id = 0;

    if(POP_VAL_OK == ret)
    {
        sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_DVBT2);
        app_all_tp_del_by_sat_id(sat_id);
        app_all_channels_del_by_sat_id(sat_id);
        _dvbt2_auto_search_start();
    }
    return 0;
}

static void _dvbt2_auto_search_with_delete_prog(void)
{
    PopDlg pop = {0};
    uint32_t tv_num = 0;
    uint32_t radio_num = 0;
    uint32_t sat_id = 0;

    sat_id = app_sat_id_get_by_type(GXBUS_PM_SAT_DVBT2);
    app_channel_num_check_by_sat_id(sat_id, &tv_num, &radio_num);
    if(0 != tv_num || 0 != radio_num)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_DEL_ALL_CHANNEL;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = _dvbt2_prog_delete_cb;
        popdlg_create(&pop);
    }
    else
    {
        app_all_tp_del_by_sat_id(sat_id);
        _dvbt2_auto_search_start();
    }
}

void app_dvbt2_auto_search(bool delete_prog)
{
    if(true == delete_prog)
        _dvbt2_auto_search_with_delete_prog();
    else
        _dvbt2_auto_search_start();
}

void app_dvbt2_auto_search_entry(void)
{
    app_dvbt2_auto_search(true);
}

#ifdef DVB_T2_NIT_SUPPORT //nit search need test
static uint32_t app_t2_nit_auto_search_get_tp(uint32_t sat_id, uint32_t *tp_array)
{
    int list_num = 0;
    int i = 0;
    int32_t  ret = 0;
    GxBusPmDataTP tp = {0};
    uint32_t tpid_temp;

    list_num = thizDvbt2NitResult.num;
    if (TP_MAX_NUM < list_num)
    {
        list_num = TP_MAX_NUM;
    }
    for (i = 0; i < list_num; i++)
    {
        ret = GxBus_PmTpExistChek((uint16_t)sat_id, (uint32_t)thizDvbt2NitResult.app_fre_array[i],
                (uint32_t)thizDvbt2NitResult.app_band_array[i], 0, 0, (uint16_t *)&tpid_temp);
        if (ret == 0)
        {
            tp.sat_id = sat_id;
            tp.frequency = thizDvbt2NitResult.app_fre_array[i];
            {
                tp.tp_dvbt2.bandwidth  = thizDvbt2NitResult.app_band_array[i];
            }
            ret = GxBus_PmTpAdd(&tp);
            if (GXCORE_SUCCESS != ret)
            {
                //tp加入不成功
                app_log_error("can't add Tp\n");
                //diag error datebase error
                break;
                //not finish
            }
            tp_array[i] = tp.id;
        }
        else if (ret > 0)
        {
            tp_array[i]  = tpid_temp;
        }
        else
        {
            dvbt2_search_printf("[DVBT2 Search]can't add Tp\n");
            return -1;
        }
    }

    GxBus_PmSync(GXBUS_PM_SYNC_TP);

    return list_num;
}

static int dvbt2_search_nit_section_parse(uint8_t *p_section_data, size_t section_size, classDvbt2SearchNit *nit_result)
{
    uint8_t *section_data = NULL, *section_data1 = NULL;
    int16_t len1 = 0;
    int16_t len2 = 0;
    int16_t len3 = 0;
    int16_t len4 = 0;
    int16_t i = 0;
    uint32_t freq = 0;
    uint32_t bandwidth = 0;
    int32_t  nitVersion = 0;
    uint16_t ts_id = 0;
    uint8_t  current_section_num = 0;
    uint8_t  last_section_num = 0;

    if (NULL == p_section_data || NULL == nit_result)
    {
        return -1;
    }

    section_data = p_section_data;
    current_section_num = section_data[6];
    last_section_num = section_data[7];
    dvbt2_search_printf("dvbt2_search_nit_section_parse size=%d len=%d\n",
            section_size, ((section_data[1] & 0x0f) << 8) + section_data[2] + 3);
    dvbt2_search_printf("dvbt2_search_nit_section_parse section_data[7]=%d section_data[6]=%d\n",
            section_data[7], section_data[6]);

    if (NULL == nit_result->nit_subtable_flag)
    {
        nit_result->nit_subtable_flag = GxCore_Malloc(MAX_NIT_SECTION_NUM);
        if (NULL == nit_result->nit_subtable_flag)
        {
            dvbt2_search_printf("[APP dvbt2 Search] malloc error : %s\n", __func__);
            return -1;
        }

        dvbt2_search_printf("[APP dvbt2 Search] Malloc nit_subtable_flag\n");
        memset(nit_result->nit_subtable_flag, 0, (MAX_NIT_SECTION_NUM));
    }
    // NIT table counter
    nit_result->nit_recv_num++;

    if (section_data[0] == NIT_ACTUAL_NETWORK_TID)
    {
        for (i = 0; i <= last_section_num; i++)
        {
            if (nit_result->nit_subtable_flag[i] != 1)
            {
                break;
            }
        }
        dvbt2_search_printf("[APP dvbt2 Search] First empty Nit flag index: %d\n", i);
        // check all sections received
        if (i == (last_section_num + 1))
        {
            // parsing all nit sections complete
            return 0;
        }

        // check whether this section is duplicated
        if (nit_result->nit_subtable_flag[current_section_num] == 1)
        {
            // this nit section is the same before
            return -2;
        }

        // then parse this nit section
        nit_result->nit_subtable_flag[current_section_num] = 1;
        dvbt2_search_printf("[APP dvbt2 Search] Get New Nit section \n");
        {
            nitVersion = (section_data[5] & 0x3E) >> 1;
            //app_flash_save_config_center_nit_fre_version(nitVersion);
            freq = nit_result->app_fre_array[0];
            dvbt2_search_printf("MAIN_FREQ_NIT=%d\n", freq);
            dvbt2_search_printf("MAIN_FREQ_NITVERSION=%d\n", nitVersion);
            freq = 0;
        }
        len1 = ((section_data[8] & 0xf) << 8) + section_data[9];
        len2 = ((section_data[9 + len1 + 1] & 0xf) << 8) + section_data[9 + len1 + 2];
        section_data += (9 + 1 + len1 + 2);

        while (len2 > 0)
        {
            ts_id = ((section_data[0] << 8) & 0xff00) + section_data[1];
            len4 = len3 = ((section_data[4] & 0xf) << 8) + section_data[5];
            section_data1 = &section_data[6];
            while (len3 > 0)
            {
                switch (section_data1[0])
                {
                    case 0x5a://SI_PARSE_LIB_TER_DELIVERY_SYSTEM_DES://terrestrial_delivery_system_descriptor:
                        {
                            // -t
                            freq = ((section_data1[2] & 0xf0) >> 4) * 10000000 + (section_data1[2] & 0xf) * 1000000
                                + ((section_data1[3] & 0xf0) >> 4) * 100000 + (section_data1[3] & 0xf) * 10000
                                + ((section_data1[4] & 0xf0) >> 4) * 1000 + (section_data1[4] & 0xf) * 100
                                + ((section_data1[5] & 0xf0) >> 4) * 10 + (section_data1[5] & 0xf);

                            freq = freq * 10; // hz
                            bandwidth = ((section_data1[6] >> 5) & 0x07);
                            dvbt2_search_printf("dvbt2_search_nit_section_parse dvbt2 freq=%d\n", freq);
                            for (i = 0; i < nit_result->num; i++)
                            {
                                if (freq == nit_result->app_fre_array[i])
                                {
                                    nit_result->app_fre_tsid[i] = ts_id;
                                    break;
                                }
                            }
                            if (i == nit_result->num)
                            {
                                if (freq > 0)
                                {
                                    nit_result->app_fre_array[nit_result->num] = freq;
                                    nit_result->app_band_array[nit_result->num] = bandwidth;
                                    nit_result->app_fre_tsid[nit_result->num] = ts_id;
                                    nit_result->num++;
                                }
                            }
                            break;
                        }
                    default:
                        break;
                }
                len3 -= (section_data1[1] + 2);
                section_data1 += (section_data1[1] + 2);
            }
            section_data += (6 + len4);
            len2 -= (6 + len4);
        }
    }
    // parsing current nit section complete
    return 1;
}

static private_parse_status dvbt2_search_nit_section_process_callback(uint8_t *p_section_data, size_t section_size)
{
    int32_t  parse_result = -1;
    thizNitParseFlag = 1;
    //copy data
    parse_result = dvbt2_search_nit_section_parse(p_section_data, section_size, &thizDvbt2NitResult);
    dvbt2_search_printf("\n\n[SI Cabllback] Recv Nit counter: %d\n\n\n", thizDvbt2NitResult.nit_recv_num);
    if (parse_result == 0 || thizDvbt2NitResult.nit_recv_num > MAX_NIT_SECTION_NUM)
    {
        return PRIVATE_SUBTABLE_OK;
    }

    return PRIVATE_SECTION_OK;
}

static void dvbt2_nit_scan_tp(GxMsgProperty_ScanDvbt2Start *p_msg_dvbt2_scan)
{
    GxSubTableDetail subtable_detail = {0};
    GxMsgProperty_SiCreate	msg_si_create = {0};
    GxMsgProperty_SiStart msg_si_start = {0};
    GxTime start_time = {0};
    GxTime local_time = {0};

    //set filter config
    subtable_detail.ts_src = (p_msg_dvbt2_scan->params_t.ts[0]);
    subtable_detail.demux_id = 0;
    subtable_detail.time_out = NIT_FILTER_TIMEOUT;
    subtable_detail.si_filter.pid = NIT_PID;
    subtable_detail.si_filter.match_depth = 1/*5*/;
    subtable_detail.si_filter.eq_or_neq = EQ_MATCH;
    subtable_detail.si_filter.match[0] = NIT_ACTUAL_NETWORK_TID;
    subtable_detail.si_filter.mask[0] = 0xff;
    subtable_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
    subtable_detail.table_parse_cfg.table_parse_fun = dvbt2_search_nit_section_process_callback;
    msg_si_create = &subtable_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void *)&msg_si_create);

    msg_si_start = subtable_detail.si_subtable_id;

    app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void *)&msg_si_start);

    GxCore_GetTickTime(&start_time);
    GxCore_GetTickTime(&local_time);

    while (thizNitParseFlag == 0  && (start_time.seconds + (NIT_FILTER_TIMEOUT / 1000) > local_time.seconds))
    {
        dvbt2_search_printf("nit wait\n");
        GxCore_GetTickTime(&local_time);
        GxCore_ThreadDelay(100);
    }

    app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void *)&msg_si_start);
    if (NULL != thizDvbt2NitResult.nit_subtable_flag)
    {
        GxCore_Free(thizDvbt2NitResult.nit_subtable_flag);
        thizDvbt2NitResult.nit_subtable_flag = NULL;
        thizDvbt2NitResult.nit_recv_num = 0;
    }
}
#endif

int app_dvbt2_clean_signal(void)
{
    GxBusPmDataSat sat = {0};
    uint32_t value = 0;
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};
    char buffer[10] = {0};

    // progbar strength
    siganl_bar.strength_bar = "progbar_t2_search_strength";
    siganl_val.strength_val = value;

    // strength value
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d%%", value);
    GUI_SetProperty("text_t2_search_strength_value", "string", buffer);

    // progbar quality
    siganl_bar.quality_bar = "progbar_t2_search_quality";
    siganl_val.quality_val = value;

    // quality value
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d%%", value);
    GUI_SetProperty("text_t2_search_quality_value", "string", buffer);

    app_get_sat_params(&sat, DEMOD_TYPE_DVBT);
    app_signal_progbar_update(sat.tuner, &siganl_bar, &siganl_val);

    app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);
    return 0;
}

static int timer_dvbt2_set_frontend(void *userdata)
{
    SetTpMode force = 0;
    GxBusPmDataSat sat = {0};
    sp_Dvbt2LockTimer = NULL;

    if (userdata != NULL)
    {
        force = *(SetTpMode *)userdata;
        if (force == SET_TP_FORCE)
        {
            GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
        }
    }
    {
#ifndef DOUBLE_S2_SUPPORT
        GxMsgProperty_NodeByIdGet node = {0};
        node.node_type = NODE_SAT;
        node.id = thizDvbt2SatId;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
#endif
    }
    app_get_sat_params(&sat, DEMOD_TYPE_DVBT);
    app_t2_manual_search_set_tp(sat.tuner);
    return 0;
}

static void timer_dvbt2_set_frontend_create(void)
{
    s_set_tp_mode = SET_TP_FORCE;
    if (reset_timer(sp_Dvbt2LockTimer) != 0)
    {
        sp_Dvbt2LockTimer = create_timer(timer_dvbt2_set_frontend, 500, (void *)&s_set_tp_mode, TIMER_ONCE);
    }
}

static int _dvbt_search_channel_cmb_change(int sel)
{
#if DEMOD_DVB_T
    if(thizFreqChannel[sel].freq < 100)
        sprintf(thiz_DvbtFreq, "0%4.1f", thizFreqChannel[sel].freq);
    else
        sprintf(thiz_DvbtFreq, "%4.1f", thizFreqChannel[sel].freq);
#endif
#if DEMOD_ISDBT
    sprintf(thiz_DvbtFreq, "%3.3f", thizFreqChannel[sel].freq);
#endif
    app_cst_choice_item_sel_set(ITEM_DVBT_BANDWIDTH, thizFreqChannel[sel].band);
    app_cst_item_data_update(ITEM_DVBT_FREQ);
    app_cst_item_data_update(ITEM_DVBT_BANDWIDTH);

    timer_dvbt2_set_frontend_create();
    return 0;
}

static int _app_search_channel_poplist_cb(int32_t ret_sel, unsigned short key)
{
    int sel_set = -1;
    CSTItemData channel_data = {0};

    sel_set = ret_sel;
    app_cst_item_data_get(ITEM_DVBT_CHANNEL, &channel_data);
    if(sel_set != channel_data.value)
    {
        app_cst_choice_item_sel_set(ITEM_DVBT_CHANNEL, sel_set);
        app_cst_item_data_update(ITEM_DVBT_CHANNEL);
        _dvbt_search_channel_cmb_change(sel_set);
        app_update_pop_dlg(WND_POP_BOOK);
    }
    else
    {
        char temp_buffer[MAX_CH_FRE_LEN] = {0};
        CSTItemData bandwidth_data = {0};
        CSTItemData freq_data = {0};

        app_cst_item_data_get(ITEM_DVBT_FREQ, &freq_data);
        app_cst_item_data_get(ITEM_DVBT_BANDWIDTH, &bandwidth_data);
#if DEMOD_DVB_T
        sprintf(temp_buffer, "%3.1f", thizFreqChannel[sel_set].freq);
#endif
#if DEMOD_ISDBT
        sprintf(temp_buffer, "%3.3f", thizFreqChannel[sel_set].freq);
#endif
        if((strcmp(freq_data.string, temp_buffer) != 0) || thizFreqChannel[sel_set].band != bandwidth_data.value)
        {
            _dvbt_search_channel_cmb_change(sel_set);
            app_update_pop_dlg(WND_POP_BOOK);
        }
    }

    poplist_destory_cb_free_item_content();
    return 0;
}

static int _dvbt_search_channel_cmb_press(void)
{
    char *bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M", "6M"};
    int list_num = 0;
    int i;
    char **channel_name = NULL;
    CSTItemData channel_data = {0};

    list_num = AppDvbt2GetFreqList(&thizFreqChannel);
    app_cst_item_data_get(ITEM_DVBT_CHANNEL, &channel_data);

    channel_name = (char **)GxCore_Malloc(list_num * sizeof(char *));
    if (channel_name == NULL)
    {
        app_log_error("\nt2_search_channel_pop_failed, malloc failed...%s,%d\n", __FILE__, __LINE__);
        return -1;
    }
    else
    {
        for (i = 0; i < list_num; i++)
        {
            channel_name[i] = (char *)GxCore_Malloc(MAX_CH_NAME_LEN + MAX_CH_FRE_LEN + 10);
            if (channel_name[i] != NULL)
            {
                memset(channel_name[i], 0, MAX_CH_NAME_LEN + MAX_CH_FRE_LEN + 10);
#if DEMOD_DVB_T
                sprintf(channel_name[i], "%s / %3.1f / %s",
                        thizFreqChannel[i].channel,
                        thizFreqChannel[i].freq,
                        bandwidth_str[thizFreqChannel[i].band]);
#endif
#if DEMOD_ISDBT
                sprintf(channel_name[i], "%s / %3.3f / %s",
                        thizFreqChannel[i].channel,
                        thizFreqChannel[i].freq,
                        bandwidth_str[thizFreqChannel[i].band]);
#endif
            }
        }
    }

    PopList pop_list;
    memset(&pop_list, 0, sizeof(PopList));
    pop_list.title = STR_ID_CHANNEL;
    pop_list.item_num = list_num;
    pop_list.item_content = channel_name;
    pop_list.sel = channel_data.value;
    pop_list.show_num = true;
    pop_list.pos.x = APP_WND_FREQ_POP_LIST_POS_X;
    pop_list.pos.y = APP_WND_FREQ_POP_LIST_POS_Y;
    pop_list.mode = POP_MODE_UNBLOCK;
    pop_list.exit_cb = _app_search_channel_poplist_cb;
    poplist_create(&pop_list);
    return 0;
}

static int _dvbt_search_freq_press_end(void)
{
    uint32_t fre = 0;
    float edit_fre = 0;
    CSTItemData freq_data = {0};
    CSTItemData channel_data = {0};

    app_cst_item_data_get(ITEM_DVBT_FREQ, &freq_data);
    edit_fre = app_float_edit_str_to_value(freq_data.string);
    fre = 1000 * edit_fre;
    if(FALSE == app_t2_search_check_fre_valid(fre))
    {
        app_cst_item_data_get(ITEM_DVBT_CHANNEL, &channel_data);
        if(thizFreqChannel[channel_data.value].freq < 100)
            sprintf(thiz_DvbtFreq, "0%4.1f", thizFreqChannel[channel_data.value].freq);
        else
            sprintf(thiz_DvbtFreq, "%4.1f", thizFreqChannel[channel_data.value].freq);
        app_cst_item_data_update(ITEM_DVBT_FREQ);
        return 0;
    }

    timer_dvbt2_set_frontend_create();
    return 0;
}

#if DEMOD_DVB_T
static int _dvbt_search_bandwidth_cmb_change(int sel)
{
    timer_dvbt2_set_frontend_create();
    return 0;
}
#endif

static int _dvbt_search_start_press_cb(void)
{
    app_t2_manual_search_start();
    return 0;
}

static int _dvbt_search_create_cb(void)
{
    GxBusPmDataSat sat = {0};

    app_panel_show_prog_num(PANEL_UNDISPLAY_TIME);
    app_get_sat_params(&sat, DEMOD_TYPE_DVBT);
    app_t2_manual_search_set_tp(sat.tuner);
    return 0;
}

static int _dvbt_search_exit_cb(CSTExitType type)
{
    APP_TIMER_REMOVE(sp_Dvbt2LockTimer);
    APP_FREE(thiz_ChannelBuffer);
    app_panel_show_prog_num(PANEL_DISPLAY_TIME | PANEL_CLEAN_PROG_NUM);
    return CST_EXIT_RET_DEFAULT;
}

static int _dvbt_search_got_focus(void)
{
    timer_dvbt2_set_frontend_create();
    return 0;
}

static void _dvbt_search_channel_item_init(void)
{
    int list_num = 0;
    int i = 0;
    char buffer[MAX_CH_NAME_LEN + 1] = {0};

    list_num = AppDvbt2GetFreqList(&thizFreqChannel);

    APP_FREE(thiz_ChannelBuffer);
    thiz_ChannelBuffer = GxCore_Calloc(list_num, (MAX_CH_NAME_LEN + 1) + 2);
    if(NULL == thiz_ChannelBuffer)
    {
        app_log_error("\033[31mNo enough memory\033[0m\n");
        return;
    }

    //first
    sprintf(thiz_ChannelBuffer, "[%s", thizFreqChannel[0].channel);
    for (i = 1; i < list_num; i++)
    {
        memset(buffer, 0, MAX_CH_NAME_LEN + 1);
        sprintf(buffer, ",%s", thizFreqChannel[i].channel);
        strcat(thiz_ChannelBuffer, buffer);
    }
    strcat(thiz_ChannelBuffer, "]");

    thiz_DvbtSearchItem[ITEM_DVBT_CHANNEL].itemTitle = STR_ID_CHANNEL;
    thiz_DvbtSearchItem[ITEM_DVBT_CHANNEL].itemType = CST_ITEM_POP_CHOICE;
    thiz_DvbtSearchItem[ITEM_DVBT_CHANNEL].itemProperty.itemPropertyCmb.content = thiz_ChannelBuffer;
    thiz_DvbtSearchItem[ITEM_DVBT_CHANNEL].itemProperty.itemPropertyCmb.sel = 0;
    thiz_DvbtSearchItem[ITEM_DVBT_CHANNEL].itemCb.cmbCb.CmbPress = _dvbt_search_channel_cmb_press;
    thiz_DvbtSearchItem[ITEM_DVBT_CHANNEL].itemCb.cmbCb.CmbChange = _dvbt_search_channel_cmb_change;

    return;
}

static void _dvbt_search_freq_item_init(void)
{
#if DEMOD_DVB_T
    if(thizFreqChannel[0].freq < 100)
        sprintf(thiz_DvbtFreq, "0%4.1f", thizFreqChannel[0].freq);
    else
        sprintf(thiz_DvbtFreq, "%4.1f", thizFreqChannel[0].freq);
#endif
#if DEMOD_ISDBT
    sprintf(thiz_DvbtFreq, "%3.3f", thizFreqChannel[0].freq);
#endif

    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemTitle = STR_ID_FREQUENCY;
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemType = CST_ITEM_EDIT;
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemProperty.itemPropertyEdit.format = "edit_float";
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemProperty.itemPropertyEdit.maxlen = EDIT_FRE_LENMAX;
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemProperty.itemPropertyEdit.intaglio = NULL;
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemProperty.itemPropertyEdit.string = thiz_DvbtFreq;
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemCb.editCb.EditPressEnd = _dvbt_search_freq_press_end;
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemCb.editCb.EditReachEnd = NULL;
    thiz_DvbtSearchItem[ITEM_DVBT_FREQ].itemCb.editCb.EditPress = NULL;

}

static void _dvbt_search_bandwidth_item_init(void)
{
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemTitle = STR_ID_BANDWIDTH;
#if DEMOD_DVB_T
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemType = CST_ITEM_CHOICE;
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemProperty.itemPropertyCmb.content = DVBT_BANDWIDTH_CONTENT;
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemProperty.itemPropertyCmb.sel = thizFreqChannel[0].band;
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemCb.cmbCb.CmbChange = _dvbt_search_bandwidth_cmb_change;
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemCb.cmbCb.CmbPress = NULL;
#elif DEMOD_ISDBT
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemType = CST_ITEM_PUSH;
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemStatus = CST_ITEM_DISABLE;
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemProperty.itemPropertyBtn.string = "6";
    thiz_DvbtSearchItem[ITEM_DVBT_BANDWIDTH].itemCb.btnCb.BtnPress = NULL;
#endif

    return;
}

static void _dvbt_search_start_item_init(void)
{
    thiz_DvbtSearchItem[ITEM_DVBT_SEARCH_START].itemTitle = STR_ID_START;
    thiz_DvbtSearchItem[ITEM_DVBT_SEARCH_START].itemType = CST_ITEM_PUSH;
    thiz_DvbtSearchItem[ITEM_DVBT_SEARCH_START].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    thiz_DvbtSearchItem[ITEM_DVBT_SEARCH_START].itemCb.btnCb.BtnPress = _dvbt_search_start_press_cb;
    return;
}


void app_dvbt_manual_search_menu_exec(void)
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
    app_get_sat_params(&sat, DEMOD_TYPE_DVBT);
    memset(&thiz_DvbtSearchOpt, 0, sizeof(CSTOpt));

    thiz_DvbtSearchOpt.menuTitle = STR_ID_MANUAL_SEARCH;
    thiz_DvbtSearchOpt.itemNum = ITEM_DVBT_SEARCH_TOTAL;
    thiz_DvbtSearchOpt.item = thiz_DvbtSearchItem;
    thiz_DvbtSearchOpt.signal_show = true;
    thiz_DvbtSearchOpt.cur_tuner = sat.tuner;
    thiz_DvbtSearchOpt.signal.create_cb = _dvbt_search_create_cb;
    thiz_DvbtSearchOpt.signal.exit_cb = _dvbt_search_exit_cb;
    thiz_DvbtSearchOpt.signal.got_focus = _dvbt_search_got_focus;

    _dvbt_search_channel_item_init();
    _dvbt_search_freq_item_init();
    _dvbt_search_bandwidth_item_init();
    _dvbt_search_start_item_init();
    app_cst_dialog_create(&thiz_DvbtSearchOpt);
}

int app_dvbt_auto_search_check(void)
{
#if DEMOD_DVB_T && DEMOD_DVB_C && 0 == DEMOD_DVB_S && NDEMOD_WITH_1TUNER
    int dvb_search_mode = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(1 == dvb_search_mode)
        return 1;
#elif DEMOD_DVB_T
    return 1;
#endif
    return 0;
}

int app_dvbt_manual_search_check(void)
{
#if DEMOD_DVB_T && DEMOD_DVB_C && 0 == DEMOD_DVB_S && NDEMOD_WITH_1TUNER
    int dvb_search_mode = 0;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &dvb_search_mode, SEARCH_MODE_TYPE_VALUE);
    if(1 == dvb_search_mode)
        return 1;
#elif DEMOD_DVB_T
    return 1;
#endif

    return 0;
}
#endif//DEMOD_DVB_T
