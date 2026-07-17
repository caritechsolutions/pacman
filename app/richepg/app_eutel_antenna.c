#include "app.h"
#if RICHEPG_SUPPORT
#include "app_module.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_wnd_search.h"
#include "richepg_store.h"
#include "richepg_sections.h"
#include "richepg_json.h"
#include "richepg_msg.h"
#include "richepg.h"
#include "richepg_common.h"
#include "app_eutel_database.h"
#include "app_eutel_common.h"

#define FREQ_BUF_LEN                6
#define SYMBOL_BUF_LEN              6
#define IMG_BAR_LONG_FOCUS          "s_bar_long_focus"
#define IMG_BAR_LONG_UNFOCUS        "null"

#define BOX_ANTENNA_OPTIONS         "box_eutel_antenna_options"
#define EDIT_ANTENNA_TP_FREQ        "edit_eutel_antenna_tp_freq"
#define TXT_ANTENNA_TP_FREQ         "text_eutel_antenna_tp_freq2"
#define EDIT_ANTENNA_TP_SYMBOL      "edit_eutel_antenna_tp_symbol"
#define TXT_ANTENNA_TP_SYMBOL       "text_eutel_antenna_tp_symbol2"

#define UNIVERSAL_NUM (3)
#define LNB_POWER_CONTENT           "[Off,On,13V,18V]"
#define LNB_FREQ_CONTENT            "[Universal1(9750/10600),Universal2(9750/10700),Universal3(9750/10750),OCS1(5150/5750),OCS2(5750/5150),5150,5750,5950,9750,10000,10600,10700,10750,11250,11300]"
#define ANTENNA_22K_CONTENT         "[Off,On]"
#define ANTENNA_22K_AUTO_CONTENT    "[Auto]"
#define DISEQC10_CONTENT            "[Off,Port 1,Port 2,Port 3,Port 4]"
#define DISEQC11_CONTENT            "[Off,Port 1,Port 2,Port 3,Port 4,Port 5,Port 6,Port 7,Port 8,Port 9,Port 10,Port 11,Port 12,Port 13,Port 14,Port 15,Port 16]"
#define POLAR_CONTENT               "[H,V]"

static char* s_EutelLnbPower[4] = {STR_ID_OFF,STR_ID_ON,"13V","18V"};
static char *s_EutelLnbFreq[15] = {
    "9750/10600","9750/10700",
    "9750/10750","5150/5750","5750/5150",//OCS2
    "5150","5750","5950","9750","10000","10600","10700","10750","11250","11300" //Universal1
};
static char *s_Eutel22K[2] = {STR_ID_OFF,STR_ID_ON};
static char *s_EutelDiseqc10[5] = {STR_ID_OFF, "Port 1", "Port 2", "Port 3", "Port 4"};
static char *s_EutelDiseqc11[17] = {
    STR_ID_OFF, "Port 1", "Port 2", "Port 3", "Port 4", "Port 5","Port 6", "Port 7", "Port 8",
    "Port 9", "Port 10", "Port 11", "Port 12", "Port 13", "Port 14", "Port 15", "Port 16"
};
static char *s_EutelPolar[2] = {"H","V"};

static char *s_EutelAntBoxitem[] = {
    "boxitem_eutel_antenna_sat",
    "boxitem_eutel_antenna_lnb_power",
    "boxitem_eutel_antenna_lnb_freq",
    "boxitem_eutel_antenna_22k",
    "boxitem_eutel_antenna_diseqc10",
    "boxitem_eutel_antenna_diseqc11",
    "boxitem_eutel_antenna_tp_freq",
    "boxitem_eutel_antenna_tp_polar",
    "boxitem_eutel_antenna_tp_symbol",
    "boxitem_eutel_antenna_search"
};

static char *s_EutelAntCombox[] = {
    "cmbox_eutel_antenna_sat",
    "cmbox_eutel_antenna_lnb_power",
    "cmbox_eutel_antenna_lnb_freq",
    "cmbox_eutel_antenna_22k",
    "cmbox_eutel_antenna_diseqc10",
    "cmbox_eutel_antenna_diseqc11",
    STR_ID_BLANK,
    "cmbox_eutel_antenna_tp_polar"
};

enum BOX_ITEM_NAME {
    BOX_ITEM_SAT = 0,
    BOX_ITEM_LNB_POWER,
    BOX_ITEM_LNB_FREQ,
    BOX_ITEM_22K,
    BOX_ITEM_DISEQC10,
    BOX_ITEM_DISEQC11,
    BOX_ITEM_TP_FREQ,
    BOX_ITEM_TP_POLAR,
    BOX_ITEM_TP_SYMBOL,
    BOX_ITEM_SEARCH,
    BOX_ITEM_TOTAL,
};

typedef struct {
    event_list *lock_timer;
    event_list *signal_timer;
    char *sat_name_content;
    char **sat_name_array;
    uint32_t s22k_bak;
    RichepgTsTP tp_bak;
    char freq_buf[FREQ_BUF_LEN];
    char symbol_buf[SYMBOL_BUF_LEN];
    int edit_sel;
    bool edit_fin;
    bool edit_flag;
    bool tp_mod_en;
} EutelAntCtrl;

static EutelAntCtrl s_EutelAntCtrl;

extern void app_eutel_install_maint_exec(RichepgTsTP *ts);
extern int app_factory_reset(void);

/**** 启用界面修改TP参数功能 ****/
#define TP_MOD_EN_KEY_TOTAL     6
static unsigned int s_tp_mod_en_key_array[] = {STBK_EPG, STBK_INFO, STBK_8, STBK_7, STBK_8, STBK_9};
static int s_tp_mod_en_key_cnt = 0;

static void app_eutel_tp_mod_en_change(bool flag)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    char *setstr = flag ? "enable" : "disable";

    ctrl->tp_mod_en = flag;
    GUI_SetProperty(s_EutelAntBoxitem[BOX_ITEM_TP_FREQ], setstr, NULL);
    GUI_SetProperty(s_EutelAntBoxitem[BOX_ITEM_TP_SYMBOL], setstr, NULL);
    GUI_SetProperty(s_EutelAntBoxitem[BOX_ITEM_TP_POLAR], setstr, NULL);
}

static bool app_eutel_tp_mod_en_set_check(unsigned key)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;

    if (ctrl->tp_mod_en)
        return false;

    if (key != s_tp_mod_en_key_array[s_tp_mod_en_key_cnt])
    {
        s_tp_mod_en_key_cnt = 0;
        return false;
    }
    if (++s_tp_mod_en_key_cnt == TP_MOD_EN_KEY_TOTAL)
    {
        s_tp_mod_en_key_cnt = 0;
        app_eutel_tp_mod_en_change(true);
        return true;
    }
    return false;
}

/**** 显示/隐藏认证卫星功能 ****/
#define CERTIFICATION_KEY_TOTAL     6
static unsigned int s_certification_key_array[] = {STBK_EPG, STBK_INFO, STBK_8, STBK_3, STBK_7, STBK_8};
static int s_certification_key_cnt = 0;

static int32_t _certification_pop_cb(PopDlgRet ret)
{
    if (ret == POP_VAL_OK)
    {
        app_factory_reset();
    }
    return 0;
}

bool app_eutel_certification_pop_check(unsigned key)
{
    if (key != s_certification_key_array[s_certification_key_cnt])
    {
        s_certification_key_cnt = 0;
        return false;
    }
    if (++s_certification_key_cnt == CERTIFICATION_KEY_TOTAL)
    {
        s_certification_key_cnt = 0;

        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = _certification_pop_cb;
        pop.str = app_richepg_certification_access_get() ? STR_ID_DISABLE_CERTIFICATION : STR_ID_ENABLE_CERTIFICATION;
        popdlg_create(&pop);
        return true;
    }
    return false;
}

/**** 从U盘导入预置卫星功能 ****/
#define IMPORT_TS_KEY_TOTAL     6
#define IMPORT_TS_FILE_NAME     "richepg_ts.json"

static unsigned int s_import_ts_key_array[] = {STBK_EPG, STBK_INFO, STBK_8, STBK_3, STBK_6, STBK_9};
static int s_import_ts_key_cnt = 0;
static char s_import_ts_file_path[64] = {0};

static int32_t _import_ts_pop_cb(PopDlgRet ret)
{
    if (ret == POP_VAL_OK)
    {
        if (app_eutel_import_ts(s_import_ts_file_path) == 0)
        {
            app_factory_reset();
        }
        else
        {
            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.timeout_sec = 3;
            pop.exit_cb = NULL;
            pop.format = POP_FORMAT_DLG;
            pop.mode = POP_MODE_UNBLOCK;
            pop.str = "Copy richepg_ts.json failed!";
            popdlg_create(&pop);
        }
    }
    return 0;
}

static bool app_eutel_import_ts_pop_check(unsigned key)
{
    if (key != s_import_ts_key_array[s_import_ts_key_cnt])
    {
        s_import_ts_key_cnt = 0;
        return false;
    }
    if (++s_import_ts_key_cnt == IMPORT_TS_KEY_TOTAL)
    {
        s_import_ts_key_cnt = 0;

        int i = 0;
        char *tip = NULL;
        bool ok_flag = false;
        HotplugPartitionList *list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);

        if (list && list->partition_num > 0) {
            for (i = 0; i < list->partition_num; i++) {
                snprintf(s_import_ts_file_path, sizeof(s_import_ts_file_path), "%s/%s", list->partition[i].partition_entry, IMPORT_TS_FILE_NAME);
                if (GxCore_FileExists(s_import_ts_file_path) == GXCORE_FILE_EXIST)
                {
                    ok_flag = true;
                    tip = "Are you sure to factory reset to import richepg_ts.json?";
                    break;
                }
                if (!ok_flag)
                {
                    tip = "Can't find richepg_ts.json?";
                }
            }
        }
        else
        {
            tip = STR_ID_INSERT_USB;
        }

        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        if (ok_flag)
        {
            pop.type = POP_TYPE_YES_NO;
            pop.exit_cb = _import_ts_pop_cb;
        }
        else
        {
            pop.type = POP_TYPE_NO_BTN;
            pop.timeout_sec = 3;
            pop.exit_cb = NULL;
        }
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = tip;
        popdlg_create(&pop);
        return true;
    }
    return false;
}

/**** FTA EPG的锁频接口 ****/

void app_eutel_set_diseqc(PositionVal *sat_position, RichepgTsTP *ts)
{
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    AppFrontend_DiseqcParameters diseqc10 = {0};
    AppFrontend_DiseqcParameters diseqc11 = {0};
    GxBusPmDataTP tp = {0};

    tp.frequency = ts->frequency;
    tp.tp_s.symbol_rate = ts->symbol_rate;
    tp.tp_s.polar = ts->polar;

    diseqc10.type = DiSEQC10;
    diseqc10.u.params_10.chDiseqc = sat->sat_s.diseqc10;
    diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(sat);
    if (diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
        diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&tp);
    diseqc10.u.params_10.b22k = app_show_to_sat_22k(sat->sat_s.switch_22K, &tp);

    // for polar
    AppFrontend_PolarState Polar = diseqc10.u.params_10.bPolar;
    app_ioctl(sat->tuner, FRONTEND_POLAR_SET, (void*)&Polar);

    // for 22k
    AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
    app_ioctl(sat->tuner, FRONTEND_22K_SET, (void*)&_22K);

    app_set_diseqc_10(sat->tuner,&diseqc10, true);
    diseqc11.type = DiSEQC11;
    diseqc11.u.params_11.chUncom = sat->sat_s.diseqc11;
    diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
    diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
    diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
    app_set_diseqc_11(sat->tuner, &diseqc11, true);

    int32_t motor_state = app_get_motor_state_in_fullscreen();
    MotorState tip = DISH_NONE;
    if ((sat->sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2) {
        tip = app_diseqc12_goto_position(sat->id, sat->tuner,
                sat->sat_s.diseqc12_pos, false, motor_state);
        app_create_position_tip(tip, motor_state);
    } else if((sat->sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS) {
        LongitudeInfo local_longitude = {0};
        local_longitude.longitude = sat->sat_s.longitude;
        local_longitude.longitude_direct = sat->sat_s.longitude_direct;
        tip = app_usals_goto_position(sat->id, sat->tuner,
                &local_longitude, sat_position, false, motor_state);
        app_create_position_tip(tip, motor_state);
    }
    // POST SEM
    GxFrontendOccur(sat->tuner);
}

void app_eutel_set_tp(SetTpMode set_mode, RichepgTsTP *ts)
{
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    AppFrontend_SetTp params = {0};
    GxBusPmDataTP tp = {0};

    tp.frequency = ts->frequency;
    tp.tp_s.symbol_rate = ts->symbol_rate;
    tp.tp_s.polar = ts->polar;

    params.fre = app_show_to_tp_freq(sat, &tp);
    params.symb = tp.tp_s.symbol_rate;
    params.pls_n = tp.tp_s.pls_n;
    params.stream_size = tp.tp_s.stream_size;
    params.polar = app_show_to_lnb_polar(sat);

    if (params.polar == SEC_VOLTAGE_ALL)
        params.polar = app_show_to_tp_polar(&tp);
    params.sat22k = app_show_to_sat_22k(sat->sat_s.switch_22K, &tp);
    params.tuner = sat->tuner;

#if UNICABLE_SUPPORT
    uint32_t centre_frequency = 0;//实际中频
    uint32_t lnb_index = 0;
    uint32_t set_tp_req = 0; //用户下行频率
    lnb_index = app_calculate_set_freq_and_initfreq(sat, &tp, &set_tp_req, &centre_frequency);
    if((0x80 == (lnb_index&0x80)) || (sat->sat_s.unicable_para.lnb_fre_index < 0x20)) {
        params.fre = set_tp_req;
        params.lnb_index = lnb_index;
    } else {
        params.lnb_index = 0x20;
    }

    params.centre_fre = centre_frequency;
    params.if_channel_index = sat->sat_s.unicable_para.if_channel;
    params.if_fre = sat->sat_s.unicable_para.centre_fre[params.if_channel_index];
#else
    params.lnb_index = 0x20;
    params.centre_fre = 0;
    params.if_channel_index = 0;
    params.if_fre = 0;
#endif

    params.type = FRONTEND_DVB_S;
    app_tuner_cur_tuner_set(sat->tuner);
    app_set_tp(sat->tuner, &params, set_mode);
}

/**** FTA EPG的卫星设置界面 ****/

static int _eutel_ant_lock_timer_cb(void *userdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    app_eutel_set_diseqc(NULL, &ctrl->tp_bak);
    app_eutel_set_tp(SET_TP_FORCE, &ctrl->tp_bak);
    ctrl->lock_timer = NULL;
    return 0;
}

static int _eutel_ant_signal_timer_cb(void *userdata)
{
#define PROGBAR_ANTENNA_STRENGTH    "progbar_eutel_antenna_strength"
#define PROGBAR_ANTENNA_QUALITY     "progbar_eutel_antenna_quality"
#define TXT_ANTENNA_STRENGTH        "text_eutel_antenna_strength_value"
#define TXT_ANTENNA_QUALITY         "text_eutel_antenna_quality_value"
    uint32_t value = 0;
    char buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    int tuner = sat->tuner;

    // progbar strength
    app_ioctl(tuner, FRONTEND_STRENGTH_GET, &value);
    if(value > 99) value = 100;
    siganl_bar.strength_bar = PROGBAR_ANTENNA_STRENGTH;
    siganl_val.strength_val = value;

    // strength value
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d%%", value);
    GUI_SetProperty(TXT_ANTENNA_STRENGTH, "string", buffer);

    // progbar quality
    app_ioctl(tuner, FRONTEND_QUALITY_GET, &value);
    if(value > 99) value = 100;
    siganl_bar.quality_bar = PROGBAR_ANTENNA_QUALITY;
    siganl_val.quality_val = value;

    // quality value
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d%%", value);
    GUI_SetProperty(TXT_ANTENNA_QUALITY, "string", buffer);

    app_signal_progbar_update(tuner, &siganl_bar, &siganl_val);
    app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);

    return 0;
}

static void _ant_tp_show_update(void)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int32_t box_sel = 0;
    bool freq_edit_show = false;
    bool symbol_edit_show = false;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    if (ctrl->edit_flag) {
        if (box_sel == BOX_ITEM_TP_FREQ)
            freq_edit_show = true;
        else if (box_sel == BOX_ITEM_TP_SYMBOL)
            symbol_edit_show = true;
    }

    if (freq_edit_show) {
        GUI_SetProperty(TXT_ANTENNA_TP_FREQ, "state", "hide");
        GUI_SetProperty(EDIT_ANTENNA_TP_FREQ, "state", "show");
        app_set_widget_string(EDIT_ANTENNA_TP_FREQ, ctrl->freq_buf);
    } else {
        GUI_SetProperty(EDIT_ANTENNA_TP_FREQ, "state", "hide");
        GUI_SetProperty(TXT_ANTENNA_TP_FREQ, "state", "show");
        app_set_widget_string(TXT_ANTENNA_TP_FREQ, ctrl->freq_buf);
    }

    if (symbol_edit_show) {
        GUI_SetProperty(TXT_ANTENNA_TP_SYMBOL, "state", "hide");
        GUI_SetProperty(EDIT_ANTENNA_TP_SYMBOL, "state", "show");
        app_set_widget_string(EDIT_ANTENNA_TP_SYMBOL, ctrl->symbol_buf);
    } else {
        GUI_SetProperty(EDIT_ANTENNA_TP_SYMBOL, "state", "hide");
        GUI_SetProperty(TXT_ANTENNA_TP_SYMBOL, "state", "show");
        app_set_widget_string(TXT_ANTENNA_TP_SYMBOL, ctrl->symbol_buf);
    }
}

static void _ant_tp_edit_num(char num)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int32_t box_sel = 0;
    char *buf = NULL;
    int len = 0;
    char *widget = NULL;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    switch (box_sel) {
        case BOX_ITEM_TP_FREQ:
            buf = ctrl->freq_buf;
            len = FREQ_BUF_LEN-1;
            widget = EDIT_ANTENNA_TP_FREQ;
            break;
        case BOX_ITEM_TP_SYMBOL:
            buf = ctrl->symbol_buf;
            len = SYMBOL_BUF_LEN-1;
            widget = EDIT_ANTENNA_TP_SYMBOL;
            break;
        default:
            return;
    }

    if (!ctrl->edit_flag) {
        ctrl->edit_flag = true;
        _ant_tp_show_update();
    }

    ctrl->edit_fin = false;
    if (ctrl->edit_sel == len) {
        ctrl->edit_sel = 0;
    }
    if (ctrl->edit_sel == 0) {
        GUI_SetProperty(widget, "clear", NULL);
        memset(buf, 0, len+1);
    }
    buf[ctrl->edit_sel++] = num;
    GUI_SetProperty(widget, "string", buf);
    GUI_SetProperty(widget, "select", &ctrl->edit_sel);
    if (ctrl->edit_sel == len) {
        ctrl->edit_fin = true;
    }
}

static status_t _ant_tp_left_key(void)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int32_t box_sel = 0;
    char *buf = NULL;
    int len = 0;
    char *widget = NULL;
    char *widget2 = NULL;
    int value = 0;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    switch (box_sel) {
        case BOX_ITEM_TP_FREQ:
            buf = ctrl->freq_buf;
            len = FREQ_BUF_LEN;
            widget = EDIT_ANTENNA_TP_FREQ;
            widget2 = TXT_ANTENNA_TP_FREQ;
            break;
        case BOX_ITEM_TP_SYMBOL:
            buf = ctrl->symbol_buf;
            len = SYMBOL_BUF_LEN;
            widget = EDIT_ANTENNA_TP_SYMBOL;
            widget2 = TXT_ANTENNA_TP_SYMBOL;
            break;
        default:
            return EVENT_TRANSFER_KEEPON;
    }

    if (!ctrl->edit_flag) {
        value = atoi(buf);
        if (value > 0)
            --value;
        snprintf(buf, len, "%d", value);
        GUI_SetProperty(widget2, "string", buf);
    } else {
        if (ctrl->edit_sel > 0) {
            buf[--ctrl->edit_sel] = 0;
            GUI_SetProperty(widget, "string", buf);
            GUI_SetProperty(widget, "select", &ctrl->edit_sel);
        }
    }
    return EVENT_TRANSFER_STOP;
}

static status_t _ant_tp_right_key(void)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int32_t box_sel = 0;
    char *buf = NULL;
    int len = 0;
    char *widget = NULL;
    int value = 0;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    switch (box_sel) {
        case BOX_ITEM_TP_FREQ:
            buf = ctrl->freq_buf;
            len = FREQ_BUF_LEN;
            widget = TXT_ANTENNA_TP_FREQ;
            break;
        case BOX_ITEM_TP_SYMBOL:
            buf = ctrl->symbol_buf;
            len = SYMBOL_BUF_LEN;
            widget = TXT_ANTENNA_TP_SYMBOL;
            break;
        default:
            return EVENT_TRANSFER_KEEPON;
    }

    if (!ctrl->edit_flag) {
        value = atoi(buf);
        if (value < 100000)
            ++value;
        snprintf(buf, len, "%d", value);
        GUI_SetProperty(widget, "string", buf);
    }
    return EVENT_TRANSFER_STOP;
}

static bool _ant_tp_check_valid(RichepgTsTP *tp_para)
{
    if ((tp_para->symbol_rate >= 800) && (tp_para->symbol_rate <= 60000)) {
        GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
        uint32_t tp_frequency = 0;
        GxBusPmDataTP temp_tp = {0};
        temp_tp.frequency= tp_para->frequency;
        temp_tp.tp_s.polar = tp_para->polar;
        tp_frequency = app_show_to_tp_freq(sat, &temp_tp);
        if((tp_frequency >= 915) && (tp_frequency <= 2185)) {
            return true;
        }
    }

    return false;
}

static void _ant_tp_param_save(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    RichepgTsTP *tp = &mgr->sat_array[mgr->sat_sel].tp_array[0];
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int32_t cur_sel = 0;
    int32_t box_sel = 0;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    if (box_sel != BOX_ITEM_TP_FREQ && box_sel != BOX_ITEM_TP_SYMBOL)
        return;

    ctrl->edit_sel = 0;
    ctrl->edit_flag = false;
    ctrl->edit_fin = false;
    ctrl->tp_bak.frequency = atoi(ctrl->freq_buf);
    ctrl->tp_bak.symbol_rate = atoi(ctrl->symbol_buf);
    if (!_ant_tp_check_valid(&ctrl->tp_bak)) {
        memcpy(&ctrl->tp_bak, tp, sizeof(RichepgTsTP));
        snprintf(ctrl->freq_buf, FREQ_BUF_LEN, "%u", tp->frequency);
        snprintf(ctrl->symbol_buf, SYMBOL_BUF_LEN, "%u", tp->symbol_rate);
        cur_sel = tp->polar;
        GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_TP_POLAR], "select", &cur_sel);
    }
    _ant_tp_show_update();
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);
}

static int _eutel_ant_display_update(void)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    RichepgTsTP *tp = &mgr->sat_array[mgr->sat_sel].tp_array[0];
    uint32_t cur_sel = 0;
    uint32_t freq_sel = 0;

    cur_sel = sat->sat_s.lnb_power;
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_LNB_POWER], "select", &cur_sel);
    app_lnb_freq_data_to_sel(sat, &freq_sel);
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_LNB_FREQ], "select", &freq_sel);
    if(freq_sel < UNIVERSAL_NUM && freq_sel >= 0) {
        GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_22K], "content", ANTENNA_22K_AUTO_CONTENT);
        GUI_SetProperty(s_EutelAntBoxitem[BOX_ITEM_22K], "disable", NULL);
        sat->sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
    } else {
        GUI_SetProperty(s_EutelAntBoxitem[BOX_ITEM_22K], "enable", NULL);
        GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_22K], "content", ANTENNA_22K_CONTENT);
        cur_sel = sat->sat_s.switch_22K;
        GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_22K], "select", &cur_sel);
    }

    ctrl->edit_sel = 0;
    ctrl->edit_flag = false;
    ctrl->edit_fin = false;
    cur_sel = sat->sat_s.diseqc10;
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_DISEQC10], "select", &cur_sel);
    cur_sel = sat->sat_s.diseqc11;
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_DISEQC11], "select", &cur_sel);

    memcpy(&ctrl->tp_bak, tp, sizeof(RichepgTsTP));
    snprintf(ctrl->freq_buf, FREQ_BUF_LEN, "%u", tp->frequency);
    snprintf(ctrl->symbol_buf, SYMBOL_BUF_LEN, "%u", tp->symbol_rate);
    cur_sel = tp->polar;
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_TP_POLAR], "select", &cur_sel);
    _ant_tp_show_update();

    return 0;
}

static int _ant_pop(int box_sel, int cmb_sel)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int ret = -1;
    PopList pop_list;
    memset(&pop_list, 0, sizeof(PopList));
    pop_list.sel = cmb_sel;
    pop_list.show_num = false;

    switch(box_sel)
    {
        case BOX_ITEM_SAT:
            pop_list.title = STR_ID_SATELLITE;
            pop_list.item_num = mgr->sat_num;
            pop_list.item_content = ctrl->sat_name_array;
            break;
        case BOX_ITEM_LNB_POWER:
            pop_list.title = STR_ID_LNB_POWER;
            pop_list.item_num = sizeof(s_EutelLnbPower)/sizeof(*s_EutelLnbPower);
            pop_list.item_content = s_EutelLnbPower;
            break;
        case BOX_ITEM_LNB_FREQ:
            pop_list.title = STR_ID_LNB_FREQ;
            pop_list.item_num = sizeof(s_EutelLnbFreq)/sizeof(*s_EutelLnbFreq);
            pop_list.item_content = s_EutelLnbFreq;
            break;
        case BOX_ITEM_22K:
            pop_list.title = "22K";
            pop_list.item_num = sizeof(s_Eutel22K)/sizeof(*s_Eutel22K);
            pop_list.item_content = s_Eutel22K;
            break;
        case BOX_ITEM_DISEQC10:
            pop_list.title = "DiSEqC 1.0";
            pop_list.item_num = sizeof(s_EutelDiseqc10)/sizeof(*s_EutelDiseqc10);
            pop_list.item_content = s_EutelDiseqc10;
            break;
        case BOX_ITEM_DISEQC11:
            pop_list.title = "DiSEqC 1.1";
            pop_list.item_num = sizeof(s_EutelDiseqc11)/sizeof(*s_EutelDiseqc11);
            pop_list.item_content = s_EutelDiseqc11;
            break;
        case BOX_ITEM_TP_POLAR:
            pop_list.title = "Polar";
            pop_list.item_num = sizeof(s_EutelPolar)/sizeof(*s_EutelPolar);
            pop_list.item_content = s_EutelPolar;
            break;
        default:
            return ret;
    }

    ret = poplist_create(&pop_list);
    return ret;
}

static void _ant_ok_keypress(void)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int32_t box_sel = 0;
    int32_t cmb_sel = 0;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    switch (box_sel) {
        case BOX_ITEM_SAT:
        case BOX_ITEM_LNB_POWER:
        case BOX_ITEM_LNB_FREQ:
        case BOX_ITEM_22K:
        case BOX_ITEM_DISEQC10:
        case BOX_ITEM_DISEQC11:
        case BOX_ITEM_TP_POLAR:
            GUI_GetProperty(s_EutelAntCombox[box_sel], "select", &cmb_sel);
            cmb_sel = _ant_pop(box_sel, cmb_sel);
            GUI_SetProperty(s_EutelAntCombox[box_sel], "select", &cmb_sel);
            break;
        case BOX_ITEM_TP_FREQ:
        case BOX_ITEM_TP_SYMBOL:
            ctrl->edit_fin = true;
            break;
        case BOX_ITEM_SEARCH:
            _ant_tp_param_save();
            GUI_EndDialog(WND_EUTEL_ANTENNA);
            app_eutel_install_maint_exec(&ctrl->tp_bak);
            break;
        default:
            break;
    }
}

static void app_eutel_antenna_save_params(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    GxBusPmDataSat sat;
    GxBusPmDataSat *psat = NULL;

    bool change_flag = false;
    int i = 0;

    for (i = 0; i < mgr->sat_num; i++) {
        psat = app_eutel_sat_data_get(i);
        GxBus_PmSatGetById(psat->id, &sat);
        if ((sat.sat_s.lnb_power == psat->sat_s.lnb_power)
                && (sat.sat_s.lnb1 == psat->sat_s.lnb1)
                && (sat.sat_s.lnb2 == psat->sat_s.lnb2)
                && (sat.sat_s.switch_22K == psat->sat_s.switch_22K)
                && (sat.sat_s.diseqc10 == psat->sat_s.diseqc10)
                && (sat.sat_s.diseqc11 == psat->sat_s.diseqc11)) {
            continue;
        }
        GxBus_PmSatModify(psat);
        change_flag = true;
    }
    if (change_flag)
        GxBus_PmSync(GXBUS_PM_SYNC_SAT);
}

static bool app_eutel_antenna_end_dialog(bool exit_to_set_en)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    if (GUI_CheckDialog(WND_EUTEL_ANTENNA) == GXCORE_SUCCESS) {
        if (GUI_CheckDialog(WND_FULL_SCREEN) == GXCORE_SUCCESS) {
            app_eutel_sat_sel_set(mgr->sat_sel_bak, false);
            GUI_EndDialog(WND_EUTEL_ANTENNA);
            return true;
        } else if (exit_to_set_en) {
            app_eutel_sat_sel_set(mgr->sat_sel_bak, false);
            GUI_EndDialog(WND_EUTEL_ANTENNA);
            //app_eutel_enduser_setting_menu_exec();
            return true;
        } else {
            return false;
        }
    }
    return true;
}

bool app_eutel_antenna_check_end_dialog(void)
{
    return app_eutel_antenna_end_dialog(false);
}

SIGNAL_HANDLER int app_eutel_ant_cmb_sat_change(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    uint32_t cmb_sel = 0;

    GUI_GetProperty(s_EutelAntCombox[BOX_ITEM_SAT], "select", &cmb_sel);
    app_eutel_sat_sel_set(cmb_sel, false);
    _eutel_ant_display_update();
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_cmb_lnb_power_change(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    uint32_t cmb_sel = 0;

    GUI_GetProperty(s_EutelAntCombox[BOX_ITEM_LNB_POWER], "select", &cmb_sel);
    sat->sat_s.lnb_power = cmb_sel;
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_cmb_lnb_freq_change(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    uint32_t freq_sel = 0;
    uint32_t s22k_sel = 0;

    GUI_GetProperty(s_EutelAntCombox[BOX_ITEM_LNB_FREQ], "select", &freq_sel);
    app_lnb_freq_sel_to_data(freq_sel, sat);
    if(freq_sel < UNIVERSAL_NUM && freq_sel >= 0) {
        GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_22K], "content", ANTENNA_22K_AUTO_CONTENT);
        GUI_SetProperty(s_EutelAntBoxitem[BOX_ITEM_22K], "disable", NULL);
        sat->sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
    } else {
        GUI_SetProperty(s_EutelAntBoxitem[BOX_ITEM_22K], "enable", NULL);
        GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_22K], "content", ANTENNA_22K_CONTENT);
        if (sat->sat_s.switch_22K == GXBUS_PM_SAT_22K_AUTO)
            sat->sat_s.switch_22K = ctrl->s22k_bak;
        s22k_sel = sat->sat_s.switch_22K;
        GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_22K], "select", &s22k_sel);
    }
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_cmb_22k_change(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    uint32_t sel_lnb = 0;
    uint32_t sel_22k = 0;

    GUI_GetProperty(s_EutelAntCombox[BOX_ITEM_LNB_FREQ], "select", &sel_lnb);
    if(sel_lnb >= UNIVERSAL_NUM) {
        GUI_GetProperty(s_EutelAntCombox[BOX_ITEM_22K], "select", &sel_22k);
        sat->sat_s.switch_22K = sel_22k;
        ctrl->s22k_bak = sel_22k;
    }
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_cmb_diseqc10_change(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    uint32_t cmb_sel = 0;

    GUI_GetProperty(s_EutelAntCombox[BOX_ITEM_DISEQC10], "select", &cmb_sel);
    sat->sat_s.diseqc10 = cmb_sel;
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_cmb_diseqc11_change(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    uint32_t cmb_sel = 0;

    GUI_GetProperty(s_EutelAntCombox[BOX_ITEM_DISEQC11], "select", &cmb_sel);
    sat->sat_s.diseqc11 = cmb_sel;
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_cmb_tp_polar_change(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    uint32_t cmb_sel = 0;

    GUI_GetProperty(s_EutelAntCombox[BOX_ITEM_TP_POLAR], "select", &cmb_sel);
    ctrl->tp_bak.polar = cmb_sel;
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_box_keypress(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    char num = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            if (app_eutel_tp_mod_en_set_check(event->key.sym))
            {
                return EVENT_TRANSFER_STOP;
            }
            if (app_eutel_certification_pop_check(event->key.sym))
            {
                return EVENT_TRANSFER_STOP;
            }
            if (app_eutel_import_ts_pop_check(event->key.sym))
            {
                return EVENT_TRANSFER_STOP;
            }
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_UP:
                    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                    if(box_sel == BOX_ITEM_TP_FREQ ||  box_sel == BOX_ITEM_TP_SYMBOL)
                        ctrl->edit_fin = true;
                    if(BOX_ITEM_SAT == box_sel) {
                        uint32_t sel = BOX_ITEM_SEARCH;
                        GUI_SetProperty(BOX_ANTENNA_OPTIONS, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    } else {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_DOWN:
                    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                    if(box_sel == BOX_ITEM_TP_FREQ ||  box_sel == BOX_ITEM_TP_SYMBOL)
                        ctrl->edit_fin = true;
                    if(BOX_ITEM_SEARCH == box_sel) {
                        uint32_t sel = BOX_ITEM_SAT;
                        GUI_SetProperty(BOX_ANTENNA_OPTIONS, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    } else {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_LEFT:
                    ret = _ant_tp_left_key();
                    break;
                case STBK_RIGHT:
                    ret = _ant_tp_right_key();
                    break;
                case STBK_0: num = '0'; break;
                case STBK_1: num = '1'; break;
                case STBK_2: num = '2'; break;
                case STBK_3: num = '3'; break;
                case STBK_4: num = '4'; break;
                case STBK_5: num = '5'; break;
                case STBK_6: num = '6'; break;
                case STBK_7: num = '7'; break;
                case STBK_8: num = '8'; break;
                case STBK_9: num = '9'; break;
                case STBK_OK:
                    _ant_ok_keypress();
                    break;
                case STBK_MENU:
                case STBK_EXIT:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    if (num) {
        _ant_tp_edit_num(num);
    }
    if (ctrl->edit_fin) {
        _ant_tp_param_save();
    }
    return ret;
}

SIGNAL_HANDLER int app_eutel_ant_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;
        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_EXIT:
                case STBK_MENU:
                    app_eutel_antenna_end_dialog(true);
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_create(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    GxBusPmDataSat *sat = NULL;
    char buffer[32] = {0};
    int i = 0;
    int len = 0;

    ctrl->sat_name_array = GxCore_Calloc(mgr->sat_num, sizeof(char*));
    for (i = 0; i < mgr->sat_num; i++) {
        sat = app_eutel_sat_data_get(i);
        sprintf(buffer, "(%d/%d) ", i+1, mgr->sat_num);
        GxBus_PmSatGetById(sat->id, sat);
        ctrl->sat_name_array[i] = (char*)sat->sat_s.sat_name;
        len += strlen(ctrl->sat_name_array[i]) + strlen(buffer) + 1;
    }
    len += 2;
    ctrl->sat_name_content = GxCore_Calloc(1, len);
    ctrl->sat_name_content[0] = '[';
    for (i = 0; i < mgr->sat_num; i++) {
        sprintf(buffer, "(%d/%d) ", i+1, mgr->sat_num);
        strcat(ctrl->sat_name_content, buffer);
        strcat(ctrl->sat_name_content, ctrl->sat_name_array[i]);
        strcat(ctrl->sat_name_content, ",");
    }
    ctrl->sat_name_content[len-2] = ']';

    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_SAT], "content", ctrl->sat_name_content);
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_LNB_POWER], "content", LNB_POWER_CONTENT);
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_LNB_FREQ], "content", LNB_FREQ_CONTENT);
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_22K], "content", ANTENNA_22K_CONTENT);
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_DISEQC10], "content", DISEQC10_CONTENT);
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_DISEQC11], "content", DISEQC11_CONTENT);
    GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_TP_POLAR], "content", POLAR_CONTENT);
    if (mgr->sat_sel == 0)
        _eutel_ant_display_update();
    else
        GUI_SetProperty(s_EutelAntCombox[BOX_ITEM_SAT], "select", &mgr->sat_sel);
    app_eutel_tp_mod_en_change(false);

    APP_TIMER_ADD(ctrl->signal_timer, _eutel_ant_signal_timer_cb, 100, TIMER_REPEAT);
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_destroy(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;

    APP_FREE(ctrl->sat_name_array);
    APP_FREE(ctrl->sat_name_content);
    APP_TIMER_REMOVE(ctrl->signal_timer);
    APP_TIMER_REMOVE(ctrl->lock_timer);
    app_eutel_antenna_save_params();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_got_focus(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int32_t box_sel = 0;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    GUI_SetProperty(s_EutelAntBoxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
    APP_TIMER_ADD(ctrl->signal_timer, _eutel_ant_signal_timer_cb, 100, TIMER_REPEAT);
    APP_TIMER_ADD(ctrl->lock_timer, _eutel_ant_lock_timer_cb, FRONTEND_SET_DELAY_MS, TIMER_ONCE);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_ant_lost_focus(GuiWidget *widget, void *usrdata)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    int32_t box_sel = 0;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    GUI_SetProperty(s_EutelAntBoxitem[box_sel], "unfocus_image", IMG_BAR_LONG_FOCUS);
    APP_TIMER_REMOVE(ctrl->signal_timer);
    return EVENT_TRANSFER_STOP;
}

void app_eutel_ant_menu_exec(void)
{
    if (GUI_CheckDialog(WND_EUTEL_ANTENNA) != GXCORE_SUCCESS)
        GUI_CreateDialog(WND_EUTEL_ANTENNA);
}

void app_eutel_ant_install_exec(int sel)
{
    EutelAntCtrl *ctrl = &s_EutelAntCtrl;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    RichepgTsTP *tp = NULL ;

    app_eutel_sat_sel_set(sel, false);

    tp = &mgr->sat_array[mgr->sat_sel].tp_array[0];

    memcpy(&ctrl->tp_bak, tp, sizeof(RichepgTsTP));

    app_eutel_install_maint_exec(&ctrl->tp_bak);
}

#endif
