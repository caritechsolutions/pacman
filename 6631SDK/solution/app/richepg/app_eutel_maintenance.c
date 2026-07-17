#include "app.h"
#if RICHEPG_SUPPORT
#include "app_windows.h"
#include "app_book.h"
#include "richepg_msg.h"
#include "richepg_file_splice.h"
#include "richepg.h"
#include "module/app_time.h"
#include "full_screen.h"
#include "app_eutel_common.h"
#include "app_eutel_database.h"
#include <assert.h>
#include "app_channel_info.h"
#include "module/channel_edit.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#include "module/app_ioctl.h"

#define BLIND_SCAN_MAINT_SUPPORT    1
#define ECL_ELLD_CH_NUM_SHOW        0

#define TEXT_MT_TITLE               "text_eutel_mt_title"
#define TEXT_MT_SAT                 "text_eutel_mt_sat"
#define PROGBAR_MT_PROCESS          "progbar_eutel_mt_process"
#define TEXT_MT_PROCESS             "text_eutel_mt_process_value"
#define TEXT_MT_INFO1               "text_eutel_mt_info1"
#define TEXT_MT_INFO2               "text_eutel_mt_info2"
#define TEXT_MT_INFO3               "text_eutel_mt_info3"
#define TEXT_MT_INFO4               "text_eutel_mt_info4"
#define TEXT_MT_INFO5               "text_eutel_mt_info5"
#define TEXT_MT_INFO6               "text_eutel_mt_info6"
#define IMG_MT_TIP                  "img_eutel_mt_tip"
#define TEXT_MT_TIP                 "text_eutel_mt_tip"
#define IMG_CH_TIP                  "img_eutel_mt_ch_tip"
#define TEXT_CH_TITLE1              "text_eutel_mt_ch_tip_title1"
#define TEXT_CH_VALUE1              "text_eutel_mt_ch_tip_value1"
#define TEXT_CH_TITLE2              "text_eutel_mt_ch_tip_title2"
#define TEXT_CH_VALUE2              "text_eutel_mt_ch_tip_value2"

#define PROGBAR_MAINTENANCE_STRENGTH          "progbar_eutel_mt_strength"
#define PROGBAR_MAINTENANCE_QUALITY           "progbar_eutel_mt_quality"
#define TEXT_MAINTENANCE_STRENGTH_VALUE       "text_eutel_mt_strength_value"
#define TEXT_MAINTENANCE_QUALITY_VALUE        "text_eutel_mt_quality_value"
#define TEXT_MAINTENANCE_QUALITY_TITLE        "text_eutel_mt_strength_title"
#define TEXT_MAINTENANCE_STRENGTH_TITLE       "text_eutel_mt_quality_title"

typedef struct {
    time_t pop_time;
    event_list *timer;
} EutelMaintCheck;

static EutelMaintCheck s_eutel_maint_check;

typedef enum {
    MAINT_SEGMENT_NONE = 0,
    MAINT_SEGMENT_DOING,
    MAINT_SEGMENT_DONE
} EutelMaintSegment;

typedef enum {
    MAINT_USER_TS = 0,
    MAINT_HOME_TS,
    MAINT_MASTER_TS,
    MAINT_BACKUP_TS,
    MAINT_MIGRATING_TS,
    MAINT_SCANNED_TS
} EutelMaintTsType;

typedef enum {
    MAINT_PROCESS_NONE = 0,
    MAINT_IDENTIFY_TS,
    MAINT_UPDATE_CAROUSEL,
    MAINT_PROCESS_LINEUP,
    MAINT_PROCESS_CHANNEL,
    MAINT_PROCESS_EPG,
    MAINT_MIGRATE_TS,
    MAINT_BLIND_SCAN,
    MAINT_LINEUP_SELECT
} EutelMaintProcess;

typedef enum {
    NO_MAINT = 0,
    INSTALL_MAINT,
    NORMAL_MAINT,
    FORCE_MAINT,
    LINEUP_MAINT,
    USBUP_MAINT
} EutelMaintType;

typedef struct {
    uint8_t data_flag;
    uint8_t update_flag;
    uint16_t mig_tsid;
    uint16_t mig_onid;
    uint16_t sid;
    uint8_t ver_bat;
    uint8_t ver_nit;
    time_t table_time;
    bool   rich_mode_changed;
#if ECL_ELLD_CH_NUM_SHOW
    int ecl_ch_num;
    int elld_ch_num;
#endif
    time_t start_time;
} EutelMaintInfo;

typedef struct {
    EutelMaintProcess process;
    EutelMaintSegment migration;
    EutelMaintSegment blindscan;
    EutelMaintType maintype;

    EutelMaintTsType cur_ts_type;
    int cur_ts_sel;
    EutelMaintTsType last_ts_type;
    int last_ts_sel;

    RichepgTsTP cur_ts;
    RichepgTsTP user_ts;
    RichepgTsTP mig_ts;
    RichepgTsTP *scan_ts_array;
    int scan_ts_num;
    event_list *timer;
    event_list *mt_signal_timer;

    int non_stop_flag;
    int non_update_ts;
    EutelMaintInfo info;
} EutelMaintCtrl;

typedef struct {
    int sat_sel;
    uint8_t data_flag;
    time_t sys_time;
    uint32_t sys_tick;
} EutelMaintBak;

typedef struct {
    uint8_t epg_rebuild;
    uint8_t ret_value;
    unsigned short timer_count;
} EutelMaintConfig;

/* 待机醒来后开始维护，由于某些情况并没有维护，这个时候，等待多少时间再次进入待机
 * 最大值unsigned short，最小值建议用默认的，1800秒
 */
#define APP_EUTEL_WAKE_NOT_MAINT_TIMER_OUT_VALUE (1800)

static EutelMaintConfig s_eutel_maint_config = {0,0,0};

static EutelMaintCtrl s_eutel_maint_ctrl;
static EutelMaintBak s_eutel_maint_bak;

extern bool app_simple_lowpower_status_get(void);
extern void app_eutel_set_diseqc(PositionVal *sat_position, RichepgTsTP *ts);
extern void app_eutel_set_tp(SetTpMode set_mode, RichepgTsTP *ts);
static bool _switch_to_next_sat(void);

#define CUR_SAT_NAME ((char *)app_eutel_cur_sat_data_get()->sat_s.sat_name)

#define APP_EUTEL_MAINTENANCE_SIGNAL_UPDATE_AUTO   (0)
#define APP_EUTEL_MAINTENANCE_SIGNAL_UPDATE_MANUAL (1)

#define APP_EUTEL_MAINTENANCE_SIGNAL_WIDGET_HIDE   (0)
#define APP_EUTEL_MAINTENANCE_SIGNAL_WIDGET_SHOW   (1)

static int _app_eutel_maintenance_signal_update(int is_manual)
{
    uint32_t value = 0;
    char buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    int tuner = sat->tuner;

    // progbar strength
    if ( is_manual ){
        value = 0 ;
    }
    else{
        app_ioctl(tuner, FRONTEND_STRENGTH_GET, &value);
    }
    if(value > 99) value = 100;
    siganl_bar.strength_bar = PROGBAR_MAINTENANCE_STRENGTH;
    siganl_val.strength_val = value;

    // strength value
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d%%", value);
    GUI_SetProperty(TEXT_MAINTENANCE_STRENGTH_VALUE, "string", buffer);

    // progbar quality
    if ( is_manual ){
        value = 0 ;
    }
    else{
        app_ioctl(tuner, FRONTEND_QUALITY_GET, &value);
    }
    if(value > 99) value = 100;
    siganl_bar.quality_bar = PROGBAR_MAINTENANCE_QUALITY;
    siganl_val.quality_val = value;

    // quality value
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d%%", value);
    GUI_SetProperty(TEXT_MAINTENANCE_QUALITY_VALUE, "string", buffer);

    app_signal_progbar_update(tuner, &siganl_bar, &siganl_val);
    app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);

    return 0;
}

static int _app_eutel_maintenance_signal_widget_state(int is_show)
{
    if ( is_show ){
        GUI_SetProperty(PROGBAR_MAINTENANCE_STRENGTH, "state", "show");
        GUI_SetProperty(PROGBAR_MAINTENANCE_QUALITY, "state", "show");
        GUI_SetProperty(TEXT_MAINTENANCE_STRENGTH_VALUE, "state", "show");
        GUI_SetProperty(TEXT_MAINTENANCE_QUALITY_VALUE, "state", "show");
        GUI_SetProperty(TEXT_MAINTENANCE_QUALITY_TITLE, "state", "show");
        GUI_SetProperty(TEXT_MAINTENANCE_STRENGTH_TITLE, "state", "show");
    }
    else{
        GUI_SetProperty(PROGBAR_MAINTENANCE_STRENGTH, "state", "hide");
        GUI_SetProperty(PROGBAR_MAINTENANCE_QUALITY, "state", "hide");
        GUI_SetProperty(TEXT_MAINTENANCE_STRENGTH_VALUE, "state", "hide");
        GUI_SetProperty(TEXT_MAINTENANCE_QUALITY_VALUE, "state", "hide");
        GUI_SetProperty(TEXT_MAINTENANCE_QUALITY_TITLE, "state", "hide");
        GUI_SetProperty(TEXT_MAINTENANCE_STRENGTH_TITLE, "state", "hide");
    }

    return 0 ;
}

static int _app_eutel_maintenance_signal_timer_cb(void *userdata)
{
    if (GUI_CheckDialog(WND_EUTEL_MAINTENANCE) == GXCORE_ERROR)
        return -1 ;

    _app_eutel_maintenance_signal_update(APP_EUTEL_MAINTENANCE_SIGNAL_UPDATE_AUTO);
    return 0;
}

static int _app_eutel_maintenance_signal_timer_start(void)
{
    APP_TIMER_ADD(s_eutel_maint_ctrl.mt_signal_timer, _app_eutel_maintenance_signal_timer_cb, 1000, TIMER_REPEAT);
    return 0 ;
}

static int _app_eutel_maintenance_signal_timer_stop(void)
{
    APP_TIMER_REMOVE(s_eutel_maint_ctrl.mt_signal_timer);
    return 0 ;
}

static int _app_eutel_maintenance_signal_work_reset(void)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    if (ctrl->maintype == LINEUP_MAINT )
        return -1;

    _app_eutel_maintenance_signal_update(APP_EUTEL_MAINTENANCE_SIGNAL_UPDATE_MANUAL);
    _app_eutel_maintenance_signal_timer_start();

    return 0;
}

bool app_eutel_install_collect_check(void)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    if (ctrl->maintype == INSTALL_MAINT)
        return true;
    return false;
}

bool app_eutel_maint_collect_check(void)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    if (ctrl->maintype != LINEUP_MAINT && ctrl->maintype != INSTALL_MAINT)
        return true;
    return false;
}

void app_eutel_time_update(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    RichepgTsTP *ts = NULL;
    struct richepg_msg msg = {0};
    struct richepg_homets *homets = NULL;
    int cnt = 0;

    if (mgr->sat_array[mgr->sat_sel].lineup_id) {
        ts = &mgr->sat_array[mgr->sat_sel].tp_array[cnt++];
    } else {
        return;
    }

    app_eutel_set_diseqc(NULL, ts);
    app_eutel_set_tp(SET_TP_FORCE, ts);
    richepg_start_time_update(0, 0);
    do {
        GxCore_ThreadDelay(50);
        if(-1 == richepg_get_msg(&msg))
            continue;
        if(RICHEPG_GUI_STATUS != msg.type)
            continue;

        if(RICHEPG_UPDATE_TIME_SUCCESS == msg.msg_id) {
            if(0 == richepg_get_homets(&homets)) {
                app_time_check_mode_sync(&g_AppTime, homets->tdt.utc_time);
                richepg_free_homets();
            }
            break;
        } else if(RICHEPG_UPDATE_TIME_FAILED == msg.msg_id) {
            if (cnt < mgr->sat_array[mgr->sat_sel].tp_num) {
                ts = &mgr->sat_array[mgr->sat_sel].tp_array[cnt++];
                app_eutel_set_diseqc(NULL, ts);
                app_eutel_set_tp(SET_TP_FORCE, ts);
                richepg_start_time_update(0, 0);
            } else {
                break;
            }
        }
    } while(1);
}

static bool _check_usb_valid(void)
{
    HotplugPartitionList *list = NULL;

    list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (!list || list->partition_num == 0) {
        return false;
    }
    return true;
}

static void _update_show_title(EutelMaintType type, char *specified_title)
{
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    char *sat_name = NULL;
    char *title = NULL;

    sat_name = (char *)sat->sat_s.sat_name;
    switch (type)
    {
        case INSTALL_MAINT: title = STR_ID_INSTALLATION; break;
        case NORMAL_MAINT:  title = STR_ID_MANUAL_MAINT; break;
        case FORCE_MAINT:   title = STR_ID_FORCE_MAINT; break;
        case LINEUP_MAINT:  title = STR_ID_LINEUP_SELECT; break;
        case USBUP_MAINT:   title = STR_ID_USB_MAINT; break;
        default: assert(0); return;
    }

    if(specified_title)
        GUI_SetProperty(TEXT_MT_TITLE, "string", specified_title);
    else
        GUI_SetProperty(TEXT_MT_TITLE, "string", title);

    if (type != LINEUP_MAINT)
        GUI_SetProperty(TEXT_MT_SAT, "string", sat_name);
    else
        GUI_SetProperty(TEXT_MT_SAT, "string", STR_ID_BLANK);

    EUTEL_SINFO("\033[34m%s, maint type: %s\033[0m\n", __func__, specified_title? specified_title:title);

}

static void _update_show_percent(int percent)
{
    char buffer[16] = {0};

    GUI_SetProperty(PROGBAR_MT_PROCESS, "value", &percent);
    sprintf(buffer, "%d%%", (percent > 100) ? 100 : percent);
    GUI_SetProperty(TEXT_MT_PROCESS, "string", buffer);
}

static void _update_show_tip(const char *str)
{
    app_set_widget_string(TEXT_MT_INFO1, (void *)str);
}

static void _update_show_ts(void)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    char buffer[64] = {0};

    switch (ctrl->cur_ts_type)
    {
        case MAINT_USER_TS:      sprintf(buffer, "%s", "UserTS");    break;
        case MAINT_HOME_TS:      sprintf(buffer, "%s", "HomeTS");    break;
        case MAINT_MASTER_TS:    sprintf(buffer, "%s", "MasterMainTS");  break;
        case MAINT_BACKUP_TS:    sprintf(buffer, "%s", "MasterBackupTS");  break;
        case MAINT_MIGRATING_TS: sprintf(buffer, "%s", "MigrateTS"); break;
        case MAINT_SCANNED_TS:   sprintf(buffer, "%s(%d/%d)", "ScannedTS", \
                             ctrl->cur_ts_sel+1, ctrl->scan_ts_num); break;
        default: assert(0); return;
    }
    GUI_SetProperty(TEXT_MT_INFO4, "string", buffer);
    sprintf(buffer, "%d / %c / %d", ctrl->cur_ts.frequency,
            (ctrl->cur_ts.polar == RICHEPG_TS_POLAR_H) ? 'H' : 'V',
            ctrl->cur_ts.symbol_rate);
    GUI_SetProperty(TEXT_MT_INFO5, "string", buffer);
}

static void _update_clean_ts(void)
{
    GUI_SetProperty(TEXT_MT_INFO4, "string", STR_ID_BLANK);
    GUI_SetProperty(TEXT_MT_INFO5, "string", STR_ID_BLANK);
}

#if ECL_ELLD_CH_NUM_SHOW
static void _update_show_channel(bool show_flag, int ecl_ch_num, int elld_ch_num)
{
    if (show_flag) {
        char buffer[8] = {0};
        GUI_SetProperty(IMG_CH_TIP, "state", "show");
        GUI_SetProperty(TEXT_CH_TITLE1, "state", "show");
        GUI_SetProperty(TEXT_CH_VALUE1, "state", "show");
        GUI_SetProperty(TEXT_CH_TITLE2, "state", "show");
        GUI_SetProperty(TEXT_CH_VALUE2, "state", "show");
        snprintf(buffer, sizeof(buffer), "%d", ecl_ch_num);
        GUI_SetProperty(TEXT_CH_VALUE1, "string", buffer);
        snprintf(buffer, sizeof(buffer), "%d", elld_ch_num);
        GUI_SetProperty(TEXT_CH_VALUE2, "string", buffer);
    } else {
        GUI_SetProperty(TEXT_CH_TITLE1, "state", "hide");
        GUI_SetProperty(TEXT_CH_VALUE1, "state", "hide");
        GUI_SetProperty(TEXT_CH_TITLE2, "state", "hide");
        GUI_SetProperty(TEXT_CH_VALUE2, "state", "hide");
        GUI_SetProperty(IMG_CH_TIP, "state", "hide");
    }
}
#endif

static void _update_show_data(uint32_t data_flag)
{
#define INSERT_INTER()  do { if (flag) strcat(buffer, ", "); else flag = true; } while(0)
    char buffer[256] = {0};
    bool flag = false;

    if (data_flag == 0) {
        GUI_SetProperty(TEXT_MT_INFO6, "string", STR_ID_BLANK);
        return;
    }
    strcat(buffer, app_richepg_translate_str("Got"));
    strcat(buffer, ": ");
    if (data_flag & RICHEPG_MASTERINDEX){ INSERT_INTER(); strcat(buffer, "MasterIndex"); }
    if (data_flag & RICHEPG_ECF)        { INSERT_INTER(); strcat(buffer, "ECF"); }
    if (data_flag & RICHEPG_ECL)        { INSERT_INTER(); strcat(buffer, "ECL"); }
    if (data_flag & RICHEPG_ELLI)       { INSERT_INTER(); strcat(buffer, "ELL-i"); }
    if (data_flag & RICHEPG_ELLD)       { INSERT_INTER(); strcat(buffer, "ELL-d"); }
    if (data_flag & RICHEPG_EPG)        { INSERT_INTER(); strcat(buffer, "EPG"); }
    if (data_flag & RICHEPG_EAR)        { INSERT_INTER(); strcat(buffer, "EAR"); }
    GUI_SetProperty(TEXT_MT_INFO6, "string", buffer);
}

void _update_show_process(EutelMaintProcess process)
{
#define TOTAL_PROCESS  5
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    char buffer[16] = {0};
    char *tmpbuf = NULL;

    ctrl->process = process;
    switch (process)
    {
        case MAINT_PROCESS_NONE:
            strcat(buffer, "-/-");
            tmpbuf = STR_ID_BLANK;
            break;
        case MAINT_IDENTIFY_TS:
            if (ctrl->maintype != LINEUP_MAINT) {
                sprintf(buffer, "%d/%d", MAINT_IDENTIFY_TS, TOTAL_PROCESS);
            } else {
                sprintf(buffer, "%d/%d", 1, 2);
            }
            tmpbuf = STR_ID_MAINT_IDENTIFY_TS;
            break;
        case MAINT_UPDATE_CAROUSEL:
            sprintf(buffer, "%d/%d", MAINT_UPDATE_CAROUSEL, TOTAL_PROCESS);
            tmpbuf = STR_ID_MAINT_UPDATE_CAROUSEL;
            break;
        case MAINT_PROCESS_LINEUP:
            sprintf(buffer, "%d/%d", MAINT_PROCESS_LINEUP, TOTAL_PROCESS);
            tmpbuf = STR_ID_MAINT_PROCESS_LINEUP;
            break;
        case MAINT_PROCESS_CHANNEL:
            if (ctrl->maintype == USBUP_MAINT) {
                sprintf(buffer, "%d/%d", TOTAL_PROCESS, TOTAL_PROCESS);
            } if (ctrl->maintype != LINEUP_MAINT) {
                sprintf(buffer, "%d/%d", MAINT_PROCESS_CHANNEL, TOTAL_PROCESS);
            } else {
#if FAST_SWITCH_PM_SUPPORT
                sprintf(buffer, "%d/%d", 1, 1);
#else
                sprintf(buffer, "%d/%d", 2, 2);
#endif
            }
            tmpbuf = STR_ID_MAINT_PROCESS_CHANNEL;
            break;
        case MAINT_PROCESS_EPG:
            sprintf(buffer, "%d/%d", MAINT_PROCESS_EPG, TOTAL_PROCESS);
            tmpbuf = STR_ID_MAINT_PROCESS_EPG;
            break;
        case MAINT_MIGRATE_TS:
            strcat(buffer, "-/-");
            tmpbuf = STR_ID_MAINT_MIGRATE_TS;
            break;
        case MAINT_BLIND_SCAN:
            strcat(buffer, "-/-");
            tmpbuf = STR_ID_MAINT_BLIND_SCAN;
            break;
        case MAINT_LINEUP_SELECT:
            strcat(buffer, "-/-");
            tmpbuf = STR_ID_MAINT_LINEUP_SELECT;
            break;
        default:
            tmpbuf = STR_ID_BLANK;
            break;
    }

    GUI_SetProperty(TEXT_MT_INFO2, "string", buffer);
    GUI_SetProperty(TEXT_MT_INFO3, "string", tmpbuf);
}

static void _maint_change_ts(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    re_homets_bids bids;
    uint16_t nid;
    int i = 0;

    nid = (uint16_t)mgr->sat_array[mgr->sat_sel].nid;
    if (mgr->sat_array[mgr->sat_sel].bid_num < MAX_BIDS) {
        for (i = 0; i < mgr->sat_array[mgr->sat_sel].bid_num; i++)
            bids[i] = (uint16_t)mgr->sat_array[mgr->sat_sel].bid_array[i];
        for (i= mgr->sat_array[mgr->sat_sel].bid_num; i < MAX_BIDS; i++)
            bids[i] = 0;
    } else {
        for (i = 0; i < MAX_BIDS; i++)
            bids[i] = (uint16_t)mgr->sat_array[mgr->sat_sel].bid_array[i];
    }

    _app_eutel_maintenance_signal_work_reset();
    app_eutel_set_diseqc(NULL, &ctrl->cur_ts);
    app_eutel_set_tp(SET_TP_FORCE, &ctrl->cur_ts);
    richepg_start_homets_identifying(0, 0, nid, &bids);

    _update_show_process(MAINT_IDENTIFY_TS);
    _update_show_ts();
}

int app_eutel_maint_release(void)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;

    if (ctrl->scan_ts_array)
        GxCore_Free(ctrl->scan_ts_array);
    APP_TIMER_REMOVE(ctrl->timer);
    _app_eutel_maintenance_signal_timer_stop();
    memset(ctrl, 0, sizeof(EutelMaintCtrl));
    return 0;
}

static int  _app_eutel_get_pragram_total(int *free_total,int *eutel_total)
{
    int i = 0;
    GxMsgProperty_NodeByPosGet node_get = {0};
    int cnt1 = 0;
    int cnt2 = 0 ;
    int num = 0;
    GxBusPmViewInfo view_info;
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
    view_info.group_mode = 0;
    view_info.stream_type = GXBUS_PM_PROG_ALL;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;

    app_fuse_spec_del_prog_check_callback();

    GxBus_PmViewInfoMemSet(&view_info);

    // get total prog
    GxMsgProperty_NodeNumGet prog_num = {0};
    prog_num.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&prog_num));

    num = prog_num.node_num;

    for (i = 0; i < num; i++)
    {
        node_get.node_type = NODE_PROG;
        node_get.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

        extern bool app_check_prog_visible(uint32_t prog_id);
        if(app_check_prog_visible(node_get.prog_data.id) == false)
            continue;

        if (app_eutel_sat_type_check(node_get.prog_data.sat_id))
        {
            cnt2++ ;
            continue;
        }
        cnt1++;
    }
    app_fuse_spec_set_prog_check_callback();
    GxBus_PmViewInfoMemClear();
    if (free_total)
        *free_total = cnt1 ;
    if (eutel_total)
        *eutel_total = cnt2 ;

    return 0;
}

SIGNAL_HANDLER int app_eutel_maintenance_create(GuiWidget *widget, void *usrdata)
{
    app_richepg_mode_set(1);
    if(memcmp(&g_AppPlayOps.current_play.view_info, &g_AppPlayOps.normal_play.view_info, sizeof(GxBusPmViewInfo)))
        memcpy(&g_AppPlayOps.current_play.view_info,&g_AppPlayOps.normal_play.view_info, sizeof(GxBusPmViewInfo));
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif
    app_eutel_maint_check_stop();
    GUI_SetProperty(IMG_MT_TIP, "state", "show");
    GUI_SetProperty(TEXT_MT_TIP, "state", "show");
    if (_check_usb_valid())
        GUI_SetProperty(TEXT_MT_TIP, "string", STR_ID_ENHANCED_MAINT_TIP2);
    else
        GUI_SetProperty(TEXT_MT_TIP, "string", STR_ID_ENHANCED_MAINT_TIP1);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_maintenance_destroy(GuiWidget *widget, void *usrdata)
{
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    EutelMaintBak *bak = &s_eutel_maint_bak;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    EutelMaintType maintype = ctrl->maintype;
    EutelMaintProcess process = ctrl->process;
    uint8_t data_flag = ctrl->info.data_flag;
    bool rich_mode_changed = ctrl->info.rich_mode_changed;

    if (maintype != LINEUP_MAINT) {
        richepg_stop();
    } else {
        richepg_delete_msg_queue();
        richepg_parse_release();
    }

    if (maintype == USBUP_MAINT) {
        app_eutel_sat_sel_set(bak->sat_sel, true);
        mgr->sat_sel_bak = bak->sat_sel;
        data_flag = bak->data_flag;
        app_time_check_mode_sync(&g_AppTime, bak->sys_time + app_tick_time_sec_get() - bak->sys_tick);
    }

    app_eutel_epg2_bak_timer_stop();
    app_eutel_maint_release();

    app_eutel_proglist_skip_set(false);

    if (richepg_store_getint(RICHEPG_ECL_STATE)
            && richepg_get_profile_type(USB_ECL_TIMESTAMP) == RICHEPG_FULL_PROFILE)
        richepg_files_splice_index_json_parse_all();

    eutel_lineup_logo_key_add();
    if ((data_flag & RICHEPG_ECL)
            || (maintype == LINEUP_MAINT && process == MAINT_PROCESS_CHANNEL)
            || (maintype == USBUP_MAINT)) {
        eutel_channel_ctrl_build();
        eutel_channel_group_all_build();
        richepg_epg_build_enable_set(true, true);
        richepg_epg_build(true);
    } else {
        if (data_flag & RICHEPG_EAR) {
            eutel_channel_ear_logo_update();
        }
        if ((data_flag & RICHEPG_EPG) || rich_mode_changed) {
            richepg_epg_build_enable_set(true, true);
            richepg_epg_build(true);
        } else {
            richepg_epg_build_enable_set(true, false);
        }
    }

    int change_mode = 0 ;
    if ((g_AppPlayOps.current_play.view_info.group_mode == GROUP_MODE_SAT) &&
	    app_eutel_sat_type_check(g_AppPlayOps.current_play.view_info.sat_id) )
    {
        change_mode =  1 ;
    }

    int free_total  = 0 ;
    int eutel_total = 0 ;
    _app_eutel_get_pragram_total(&free_total,&eutel_total);

    EUTEL_SINFO("mantenance destory mode=%d,total=%d,%d \n",change_mode,free_total,eutel_total);

    if (0 == eutel_total){
        EUTEL_SINFO("mantenance eutel total is empty \n");
        app_richepg_mode_set(0);
        if ( change_mode == 1 ){
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
        }
        else{
            memcpy(&g_AppPlayOps.normal_play.view_info,&g_AppPlayOps.current_play.view_info,sizeof(GxBusPmViewInfo));
        }
    }
    else if (0 == free_total){
        g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
        g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_SAT;
        g_AppPlayOps.normal_play.view_info.sat_id = app_eutel_cur_sat_id_get();
        EUTEL_SINFO("mantenance free total is empty, sat id = %d \n",g_AppPlayOps.normal_play.view_info.sat_id);
    }
    else{
        if ( change_mode == 0 ){
            app_richepg_mode_set(0);
            memcpy(&g_AppPlayOps.normal_play.view_info,&g_AppPlayOps.current_play.view_info,sizeof(GxBusPmViewInfo));
        }
    }

    if (GUI_CheckDialog(WND_FULL_SCREEN) == GXCORE_SUCCESS) {
        if (GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS) {
            g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
            if (maintype != LINEUP_MAINT) {
                if (data_flag & RICHEPG_ECL) {
                    int lcn = -1, channel_id = 0;
                    channel_id = richepg_store_getint(RICHEPG_LINEUP_WAKEUP_CHANNEL);
                    lcn = eutel_channel_find_lcn_by_channel_id(channel_id, NULL);
                    if(lcn >= 0)
                        eutel_channel_set_play_lcn(lcn);
                }
            }
        } else {
            g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
            g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
            g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);

            if (g_AppPlayOps.normal_play.play_total != 0) {
                //if received other data except masterindex mean user reinstall lineup, should play wakeup ch
                if ( app_richepg_mode_get() ){
                    if (data_flag & RICHEPG_ECL)
                        eutel_channel_play_first();
                    else
                        eutel_channel_play_current(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
                }
                else{
                    g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(g_AppPlayOps.normal_play.play_count);
                    app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
                }
            }
        }
    }

    app_fuse_spec_wake_from_reboot_flag_keypress(-1);
    app_eutel_maint_check_start();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_maintenance_got_focus(GuiWidget *widget, void *usrdata)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    if ( LINEUP_MAINT != ctrl->maintype){
        _app_eutel_maintenance_signal_timer_start();
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_maintenance_lost_focus(GuiWidget *widget, void *usrdata)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    if ( LINEUP_MAINT != ctrl->maintype){
        _app_eutel_maintenance_signal_timer_stop();
    }

    return EVENT_TRANSFER_STOP;
}

static bool _eutel_maintenance_check_end_dialog(bool exit_to_sat_en)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    EutelMaintType maintype = ctrl->maintype;

    if (GUI_CheckDialog(WND_EUTEL_MAINTENANCE) == GXCORE_SUCCESS) {
        if (ctrl->non_stop_flag)
            return false;
#if BLIND_SCAN_MAINT_SUPPORT
        if (ctrl->blindscan == MAINT_SEGMENT_DOING) {
            app_eutel_blind_scan_stop();
        }
#endif
        if (maintype == INSTALL_MAINT) {
            if (richepg_check_stop() < 0)
                return false;
            mgr->sat_array[mgr->sat_sel].lineup_id = mgr->sat_array[mgr->sat_sel].lineup_id_bak;
            app_eutel_sat_sel_set(mgr->sat_sel_bak, true);
        } else if (maintype == LINEUP_MAINT) {
            mgr->sat_array[mgr->sat_sel].lineup_id = mgr->sat_array[mgr->sat_sel].lineup_id_bak;
            app_eutel_sat_sel_set(mgr->sat_sel_bak, true);
        } else {
            if (richepg_check_stop() < 0)
                return false;
        }
        richepg_stop_sections_request();
        if (GUI_CheckDialog(WND_EUTEL_LINEUP) == GXCORE_SUCCESS)
            GUI_EndDialog(WND_EUTEL_LINEUP);
        GUI_EndDialog(WND_EUTEL_MAINTENANCE);
        if (exit_to_sat_en && maintype == INSTALL_MAINT) {
            //extern void app_eutel_ant_menu_exec(void);
            //app_eutel_ant_menu_exec();
        }
        GUI_SetInterface("flush", NULL);
        return true;
    }
    return true;
}

bool app_eutel_maintenance_check_end_dialog(void)
{
    return _eutel_maintenance_check_end_dialog(true);
}

bool app_eutel_process_check_all_no_stop(void)
{
    if (app_eutel_antenna_check_end_dialog() && _eutel_maintenance_check_end_dialog(false))
    {
        return false;
    }
    return true;
}

SIGNAL_HANDLER int app_eutel_maintenance_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
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
                    app_eutel_maintenance_check_end_dialog();
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

static void _maint_popup(char *tip, ExitCb cb, uint32_t timeout_sec)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = cb;
    pop.str = tip;
    if (timeout_sec > 0) {
        pop.timeout_sec = timeout_sec;
        pop.show_time = true;
    }
    popdlg_create(&pop);
}

static void _process_popup(char *tip, uint32_t timeout_sec, ExitCb cb)
{
    if (GUI_CheckDialog(WND_EUTEL_LINEUP) == GXCORE_SUCCESS)
        GUI_EndDialog(WND_EUTEL_LINEUP);

    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_WAIT;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = cb;
    pop.str = tip;
    pop.timeout_sec = timeout_sec;
    popdlg_create(&pop);
}

static int _popup_ok_cb(PopDlgRet ret)
{
    bool collect_flag = (app_eutel_install_collect_check() || app_eutel_maint_collect_check())
            && !app_simple_lowpower_status_get();

    GUI_EndDialog(WND_EUTEL_MAINTENANCE);
    popdlg_destroy();

    app_eutel_collect_finish_index_record();
    app_eutel_collect_save();
    if (collect_flag) {
        app_eutel_collect_create_dialog();
    }
    return 0;
}

static int _popup_fail_cb(PopDlgRet ret)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    EutelMaintType maintype = ctrl->maintype;
    GxTime sys_time;

    bool collect_flag = app_eutel_maint_collect_check() && !app_simple_lowpower_status_get();

    if (app_eutel_maint_collect_check()) {
        GxCore_GetLocalTime(&sys_time);
        app_eutel_collect_add_item_fail_maint(CUR_SAT_NAME, sys_time.seconds);
    }

    if (maintype == INSTALL_MAINT) {
        mgr->sat_array[mgr->sat_sel].lineup_id = mgr->sat_array[mgr->sat_sel].lineup_id_bak;
        app_eutel_sat_sel_set(mgr->sat_sel_bak, true);
        //GUI_CreateDialog(WND_EUTEL_ANTENNA);
    } else if (maintype == LINEUP_MAINT) {
        mgr->sat_array[mgr->sat_sel].lineup_id = mgr->sat_array[mgr->sat_sel].lineup_id_bak;
        app_eutel_sat_sel_set(mgr->sat_sel_bak, true);
    }
    GUI_EndDialog(WND_EUTEL_MAINTENANCE);

    app_eutel_collect_finish_index_record();
    app_eutel_collect_save();
    if (collect_flag) {
        app_eutel_collect_create_dialog();
    }
    GUI_SetInterface("flush", NULL);
    return 0;
}

static int app_eutel_next_ts_start(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    EutelMaintTsType ts_type_bak = MAINT_USER_TS;
    int ts_sel_bak = 0;
    bool next_flag = false;
    bool scan_flag = false;

#if 0 // ts迁移失败时: 0, 原来的ts继续; 1, 原来的ts的下一个ts继续.
    if (ctrl->cur_ts_type == MAINT_MIGRATING_TS) {
        ctrl->cur_ts_type = ctrl->last_ts_type;
        ctrl->cur_ts_sel = ctrl->last_ts_sel;
    }
#else
    if (ctrl->process == MAINT_MIGRATE_TS) {
        ts_type_bak = ctrl->cur_ts_type;
        ts_sel_bak = ctrl->cur_ts_sel;
        next_flag = true;
    }
    else if (ctrl->cur_ts_type == MAINT_MIGRATING_TS)
    {
        ctrl->cur_ts_type = ctrl->last_ts_type;
        ctrl->cur_ts_sel = ctrl->last_ts_sel;
        ts_type_bak = ctrl->cur_ts_type;
        ts_sel_bak = ctrl->cur_ts_sel;
        switch (ctrl->cur_ts_type)
        {
            case MAINT_USER_TS:
                memcpy(&ctrl->cur_ts, &ctrl->user_ts, sizeof(RichepgTsTP));
                break;
            case MAINT_HOME_TS:
                memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[0], sizeof(RichepgTsTP));
                break;
            case MAINT_MASTER_TS:
                memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[1], sizeof(RichepgTsTP));
                break;
            case MAINT_BACKUP_TS:
                memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[2+ctrl->cur_ts_sel], sizeof(RichepgTsTP));
                break;
            case MAINT_SCANNED_TS:
                memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[ctrl->cur_ts_sel], sizeof(RichepgTsTP));
                break;
            default:
                assert(0);
                break;
        }
        next_flag = true;
    }
    else
#endif
    {
        ts_type_bak = ctrl->cur_ts_type;
        ts_sel_bak = ctrl->cur_ts_sel;
        switch (ctrl->cur_ts_type)
        {
            case MAINT_USER_TS:
                ctrl->cur_ts_type = MAINT_HOME_TS;
                ctrl->cur_ts_sel = 0;
                memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[0], sizeof(RichepgTsTP));
                next_flag = true;
                break;
            case MAINT_HOME_TS:
                if (mgr->sat_array[mgr->sat_sel].tp_num > 1) {
                    ctrl->cur_ts_type = MAINT_MASTER_TS;
                    ctrl->cur_ts_sel = 0;
                    memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[1], sizeof(RichepgTsTP));
                    next_flag = true;
                } else {
                    scan_flag = true;
                }
                break;
            case MAINT_MASTER_TS:
                if (mgr->sat_array[mgr->sat_sel].tp_num > 2) {
                    ctrl->cur_ts_type = MAINT_BACKUP_TS;
                    ctrl->cur_ts_sel = 0;
                    memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[2], sizeof(RichepgTsTP));
                    next_flag = true;
                } else {
                    scan_flag = true;
                }
                break;
            case MAINT_BACKUP_TS:
                if (mgr->sat_array[mgr->sat_sel].tp_num > 3 + ctrl->cur_ts_sel) {
                    ctrl->cur_ts_type = MAINT_BACKUP_TS;
                    ++ctrl->cur_ts_sel;
                    memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[2+ctrl->cur_ts_sel], sizeof(RichepgTsTP));
                    next_flag = true;
                } else {
                    scan_flag = true;
                }
                break;
            case MAINT_SCANNED_TS:
                if (ctrl->scan_ts_num > 1 + ctrl->cur_ts_sel) {
                    ctrl->cur_ts_type = MAINT_SCANNED_TS;
                    ++ctrl->cur_ts_sel;
                    memcpy(&ctrl->cur_ts, &ctrl->scan_ts_array[ctrl->cur_ts_sel], sizeof(RichepgTsTP));
                    next_flag = true;
                }
                break;
            default:
                break;
        }
    }

    if (scan_flag) {
        if (ctrl->blindscan == MAINT_SEGMENT_NONE) {
#if BLIND_SCAN_MAINT_SUPPORT
            ctrl->blindscan = MAINT_SEGMENT_DOING;
            uint32_t sat_id = app_eutel_cur_sat_id_get();
            EutelBlindScan scan = {&sat_id, 1};
            _update_show_process(MAINT_BLIND_SCAN);
            _update_clean_ts();
            app_eutel_blind_scan_start(&scan);

            _app_eutel_maintenance_signal_timer_stop();
            _app_eutel_maintenance_signal_update(APP_EUTEL_MAINTENANCE_SIGNAL_UPDATE_MANUAL);

            return 0;
#else
            _process_popup(STR_ID_HOMETS_FAIL, 3, _popup_fail_cb);
            return -1;
#endif
        } else {
            if (ctrl->scan_ts_num > 0) {
                ctrl->cur_ts_type = MAINT_SCANNED_TS;
                ctrl->cur_ts_sel = 0;
                memcpy(&ctrl->cur_ts, &ctrl->scan_ts_array[0], sizeof(RichepgTsTP));
                next_flag = true;
            }
        }
    }

    if (next_flag) {
        ctrl->non_update_ts = 0;
        ctrl->last_ts_type = ts_type_bak;
        ctrl->last_ts_sel = ts_sel_bak;
        if (ctrl->cur_ts_type == MAINT_SCANNED_TS) {
            _update_show_percent(100 * (ctrl->cur_ts_sel+1)/ctrl->scan_ts_num);
        } else {
            _update_show_percent(0);
        }
        _maint_change_ts();
        return 0;
    }
    _process_popup(STR_ID_HOMETS_FAIL, 3, _popup_fail_cb);
    return -1;

}

static int app_eutel_transport_check(void)
{
#define MAX_ONIDS_NUM   255
    int i = 0, j = 0, k = 0;
    uint16_t onid = 0;
    uint16_t *onid_array = NULL;
    uint8_t onid_num = 0;
    struct eutelsat_channel *channel = NULL;
    struct eutelsat_channel_list *ecl = NULL;
#if !FAST_SWITCH_PM_SUPPORT
    struct eutelsat_lineup_data *elld = NULL;
    richepg_get_elld(&elld);
#endif
    richepg_get_ecl(&ecl);

    if ((onid_array = GxCore_Calloc(MAX_ONIDS_NUM, sizeof(uint16_t))) == NULL) {
        EUTEL_ERR("malloc failed!\n");
        return -1;
    }

#if FAST_SWITCH_PM_SUPPORT
    for (i = 0; i < ecl->channels_num; i++) {
        onid = 0;
        channel = &ecl->channels[i];
        for (k = 0; k < channel->tech_num; k++) {
            if (channel->techs[k].delivery == EUTELSAT_DELIVERY_DVB) {
                onid = channel->techs[k].trans.dvb.onid;
                break;
            }
        }
#else
    for (i = 0; i < elld->channel_num; i++) {
        onid = 0;
        for (j = 0; j < ecl->channels_num; j++) {
            if(elld->channels[i].id == ecl->channels[j].id) {
                channel = &ecl->channels[j];
                for (k = 0; k < channel->tech_num; k++) {
                    if (channel->techs[k].delivery == EUTELSAT_DELIVERY_DVB) {
                        onid = channel->techs[k].trans.dvb.onid;
                        break;
                    }
                }
                break;
            }
        }
#endif
        if (onid == 0)
            continue;

        for (j = 0; j < onid_num; j++) {
            if (onid == onid_array[j])
                break;
        }
        if (j == onid_num)
            onid_array[onid_num++] = onid;
        if (onid_num >= MAX_ONIDS_NUM)
            break;
    }

    _update_show_process(MAINT_PROCESS_CHANNEL);
    _update_show_percent(0);
    richepg_start_update_tsinfo(0, 0, onid_array, onid_num);
    GxCore_Free(onid_array);
    return 0;
}

static int app_eutel_homets_next_step(uint16_t service_id)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;

    if (ctrl->maintype != LINEUP_MAINT) {
        _update_show_process(MAINT_UPDATE_CAROUSEL);
        richepg_start_update_carouselsinfo(0, 0, service_id);
    } else {
        app_eutel_transport_check();
    }
    return 0;
}

static int app_eutel_homets_update(void)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    EutelMaintBak *bak = &s_eutel_maint_bak;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    RichepgTsSat *cur_sat = &mgr->sat_array[mgr->sat_sel];
    RichepgTsTP *tp_array = cur_sat->tp_array;
    RichepgTsTP *tmp_array = NULL;
    int tmp_num = 0;
    int get_sel = -1;
    int i = 0, cnt = 0;
    bool update_flag = false;
    int sat_sel = 0;

    if (ctrl->non_update_ts == 0) {
        for (i = 0; i < cur_sat->tp_num; i++) {
            if (app_eutel_same_ts_check(&tp_array[i], &ctrl->cur_ts)) {
                get_sel = i;
                break;
            }
        }
        if (get_sel != 0) {
            tmp_num = (get_sel > 0) ? (cur_sat->tp_num) : (cur_sat->tp_num + 1);
            if ((tmp_array = GxCore_Calloc(tmp_num, sizeof(RichepgTsTP))) == NULL) {
                EUTEL_ERR("malloc failed!\n");
                return -1;
            }
            if (get_sel > 0) {
                memcpy(&tmp_array[cnt++], &tp_array[get_sel], sizeof(RichepgTsTP));
                for (i = 0; i < cur_sat->tp_num; i++) {
                    if (i != get_sel)
                        memcpy(&tmp_array[cnt++], &tp_array[i], sizeof(RichepgTsTP));
                }
            } else {
                memcpy(&tmp_array[cnt++], &ctrl->cur_ts, sizeof(RichepgTsTP));
                for (i = 0; i < cur_sat->tp_num; i++) {
                    memcpy(&tmp_array[cnt++], &tp_array[i], sizeof(RichepgTsTP));
                }
            }

            GxCore_Free(cur_sat->tp_array);
            cur_sat->tp_num = tmp_num;
            cur_sat->tp_array = tmp_array;
            update_flag = true;
        }
    }

    sat_sel = (ctrl->maintype != USBUP_MAINT) ? mgr->sat_sel : bak->sat_sel;
    if (ctrl->maintype == INSTALL_MAINT || ctrl->maintype == LINEUP_MAINT || update_flag
            || mgr->sat_array[mgr->sat_sel].lineup_id_bak != mgr->sat_array[mgr->sat_sel].lineup_id) {
        if (richepg_ts_mgr_save(sat_sel) < 0) {
            EUTEL_ERR("ts config file save failed!\n");
            return -1;
        }
        mgr->sat_array[mgr->sat_sel].lineup_id_bak = mgr->sat_array[mgr->sat_sel].lineup_id;
        if (ctrl->maintype != USBUP_MAINT)
            mgr->sat_sel_bak = mgr->sat_sel;
    }
    return 0;
}

static int app_eutel_homets_migrate(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    struct richepg_homets *homets = NULL;
    struct richepg_nit *nit = NULL;
    struct richepg_nit_ts *ts = NULL;
    int i = 0, j = 0;

    richepg_get_homets(&homets);
    for(i = 0; i < homets->nit_count; i++) {
        if(ctrl->info.mig_onid == homets->nit[i].network_id) {
            nit = &homets->nit[i];
            for (j = 0; j < nit->ts_count; j++) {
                if(ctrl->info.mig_tsid == nit->ts[j].transport_stream_id) {
                    ts = &nit->ts[j];
                    break;
                }
            }
            break;
        }
    }
    if (!nit || !ts) {
        EUTEL_SINFO("\033[31m%s onid(%d) & tsid(%d) not find!\033[0m\n", __func__, ctrl->info.mig_onid, ctrl->info.mig_tsid);
        richepg_free_homets();
        return -1;
    }
    ctrl->mig_ts.frequency = ts->frequency;
    ctrl->mig_ts.symbol_rate = ts->symbol_rate;
    ctrl->mig_ts.polar = ts->polar % 2 == 0 ? RICHEPG_TS_POLAR_H : RICHEPG_TS_POLAR_V;
    richepg_free_homets();

    if (app_eutel_same_ts_check(&ctrl->mig_ts, &ctrl->cur_ts)) {
        app_eutel_homets_next_step(ctrl->info.sid);
    } else if (app_eutel_same_ts_check(&ctrl->mig_ts, &mgr->sat_array[mgr->sat_sel].tp_array[0])) {
        ctrl->non_update_ts = 1;
        app_eutel_homets_next_step(ctrl->info.sid);
    } else {
        ctrl->last_ts_sel = ctrl->cur_ts_sel;
        ctrl->last_ts_type = ctrl->cur_ts_type;
        memcpy(&ctrl->cur_ts, &ctrl->mig_ts, sizeof(RichepgTsTP));
        ctrl->cur_ts_type = MAINT_MIGRATING_TS;
        _update_show_percent(0);
        _maint_change_ts();
    }
    return 0;
}

static void _maint_update_bat_nit_info(void)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    richepg_store_setint(RICHEPG_BAT_VERSION, ctrl->info.ver_bat);
    richepg_store_setint(RICHEPG_BAT_LASTUPDATED, ctrl->info.table_time);
    richepg_store_setint(RICHEPG_NIT_VERSION, ctrl->info.ver_nit);
    richepg_store_setint(RICHEPG_NIT_LASTUPDATED, ctrl->info.table_time);
    richepg_store_flush();
}

static int32_t _maint_epg_exec(PopDlgRet ret)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;

    if(POP_VAL_OK == ret) {
        _update_show_process(MAINT_PROCESS_EPG);
        richepg_start(0, 0, RICHEPG_EPG_STAGE, 0);
    } else {
        //when cancel epg maint need to release previous memory epg data in INSTALL_MAINT
        if(INSTALL_MAINT == ctrl->maintype)
            richepg_epg_ctrl_release();
        _popup_ok_cb(POP_VAL_OK);
    }
    return 0;
}

static void _richepg_status(uint32_t msg_id)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    EutelMaintBak *bak = &s_eutel_maint_bak;
    struct richepg_homets *homets = NULL;
    struct richepg_carousels *carousel = NULL;
    GxTime sys_time;
    int i = 0;

    EUTEL_SINFO("\033[34m%s: get status %d\033[0m\n", __func__, msg_id);
    switch (msg_id)
    {
        case RICHEPG_HOMETS_FAILED:
        case RICHEPG_UPDATE_CAROUSEL_FAILED:
            app_eutel_next_ts_start();
            break;
        case RICHEPG_UPDATE_TSINFO_FAILED:
            if(ctrl->migration == MAINT_SEGMENT_DOING) {
                ctrl->migration = MAINT_SEGMENT_DONE;
                app_eutel_next_ts_start();
            } else {
                _process_popup(STR_ID_MAINT_FAIL, 3, _popup_fail_cb);
            }
            break;
        case RICHEPG_ECL_FAILED:
            if (GUI_CheckDialog(WND_EUTEL_LINEUP) == GXCORE_SUCCESS)
                GUI_EndDialog(WND_EUTEL_LINEUP);
            _process_popup(STR_ID_MAINT_FAIL, 3, _popup_fail_cb);
            break;
        case RICHEPG_EPG_FAILED:
            _process_popup(STR_ID_MAINT_FAIL, 3, _popup_fail_cb);
            break;

        case RICHEPG_HOMETS_SUCCESS:
            richepg_get_homets(&homets);
            i = app_time_check_mode_sync(&g_AppTime, homets->tdt.utc_time);
            if (ctrl->maintype != USBUP_MAINT) {
                if (i>0){
                    bak->sys_time = homets->tdt.utc_time;
                    bak->sys_tick = app_tick_time_sec_get();
                }
            }
            ctrl->info.ver_bat = homets->bat.version;
            ctrl->info.ver_nit = homets->nit[0].version;
            ctrl->info.table_time = time(NULL);
            ctrl->info.sid = homets->bat.service_id;

            if (homets->bat.migration.need == 1 && ctrl->migration == MAINT_SEGMENT_NONE) {
                ctrl->migration = MAINT_SEGMENT_DOING;
                ctrl->info.mig_tsid = homets->bat.migration.ts_id;
                ctrl->info.mig_onid = homets->bat.migration.on_id;

                richepg_free_homets();
                _update_show_process(MAINT_MIGRATE_TS);
                richepg_start_update_tsinfo(0, 0, &ctrl->info.mig_onid, 1);
            } else {
                richepg_free_homets();
                app_eutel_homets_next_step(ctrl->info.sid);
            }
            break;

        case RICHEPG_UPDATE_CAROUSEL_SUCCESS:
            richepg_get_carousels(&carousel);
            for (i = 0; i < carousel->pmt.carousel_num; i++) {
                EUTEL_SINFO("Carousel:%d pid:%d tag:%d\n",
                        carousel->pmt.carousel[i].carousel_id,
                        carousel->pmt.carousel[i].pid,
                        carousel->pmt.carousel[i].component_tag);
            }
            richepg_add_carousels(&carousel->pmt);
            richepg_free_carousels();
            if (ctrl->maintype != USBUP_MAINT) {
                _update_show_process(MAINT_PROCESS_LINEUP);
                richepg_start(0, 0, RICHEPG_LINEUP_STAGE, (ctrl->maintype == INSTALL_MAINT) ? 0 : mgr->sat_array[mgr->sat_sel].lineup_id);
            } else {
                _update_show_process(MAINT_PROCESS_EPG);
                richepg_start(0, 0, RICHEPG_USBUP_STAGE, mgr->sat_array[mgr->sat_sel].lineup_id);
            }
            break;

        case RICHEPG_UPDATE_TSINFO_SUCCESS:
            if(ctrl->migration == MAINT_SEGMENT_DOING) {
                ctrl->migration = MAINT_SEGMENT_DONE;
                app_eutel_homets_migrate();
            } else {
                bool store_ch = (ctrl->maintype != USBUP_MAINT);
                bool del_all = (ctrl->maintype == INSTALL_MAINT);
                bool sat_change = (mgr->sat_sel != mgr->sat_sel_bak);

                if (app_eutel_maint_collect_check()) {
                    GxCore_GetLocalTime(&sys_time);
                    app_eutel_collect_add_item_ecl_maint(CUR_SAT_NAME, sys_time.seconds);
                }

                app_eutel_homets_update();
                ctrl->non_stop_flag = 1;
                if (app_eutel_proglist_update(store_ch, del_all) < 0) {
                    richepg_store_setint(RICHEPG_ELLD_LASTUPDATED, 0); // 错误时清除ELLD时间戳以便下次维护重建
                    richepg_store_flush();
                    _process_popup(STR_ID_MAINT_FAIL, 3, _popup_fail_cb);
                    break;
                }
                if (ctrl->maintype == INSTALL_MAINT) {
                    _maint_popup(STR_ID_CONTINUE_EPG_MAINT, _maint_epg_exec, 10);
                } else if (ctrl->maintype == USBUP_MAINT) {
                    _update_show_percent(100);
                    _switch_to_next_sat();
                } else if (ctrl->maintype != LINEUP_MAINT) {
                    _maint_epg_exec(POP_VAL_OK);
                } else {
                    richepg_switch_lineup_step3(sat_change, app_richepg_lineup_mode_get());
                    _update_show_percent(100);
                    _process_popup(STR_ID_LINEUP_SWITCH_OK, 3, _popup_ok_cb);
                }
            }
            break;

        case RICHEPG_INFO_UNCHANGED:
            _maint_update_bat_nit_info();
            app_eutel_homets_update();
            _update_show_percent(100);
            _switch_to_next_sat();

            break;
        case RICHEPG_ECL_SUCCESS:
            if(ctrl->info.start_time)
                app_richepg_start_maint_time_set(ctrl->info.start_time);
            _maint_update_bat_nit_info();
            if ((ctrl->info.data_flag & RICHEPG_ECL) && (ctrl->info.update_flag & RICHEPG_ECL)) {
                app_eutel_transport_check();
            } else {
                app_eutel_homets_update();
                if (ctrl->maintype == INSTALL_MAINT) {
                    _maint_popup(STR_ID_CONTINUE_EPG_MAINT, _maint_epg_exec, 10);
                } else {
                    _maint_epg_exec(POP_VAL_OK);
                }
            }
            break;
        case RICHEPG_EPG_SUCCESS:
            if(ctrl->info.start_time)
                app_richepg_start_maint_time_set(ctrl->info.start_time);
            if (app_eutel_maint_collect_check()) {
                GxCore_GetLocalTime(&sys_time);
                app_eutel_collect_add_item_epg_maint(CUR_SAT_NAME, sys_time.seconds);
            }

            if ((ctrl->maintype == USBUP_MAINT)
                    && (ctrl->info.data_flag & RICHEPG_ECL)) {
                app_eutel_transport_check();
            } else {
                _update_show_percent(100);
                _switch_to_next_sat();
            }
            break;

        default:
            break;
    }
}

static void _richepg_complete(struct richepg_msg *msg)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;

    EUTEL_SINFO("\033[33m%s: get data %d\033[0m\n", __func__, msg->data_type);
    ctrl->info.data_flag |= msg->data_type;
    if (msg->update_flag)
        ctrl->info.update_flag |= msg->data_type;

#if ECL_ELLD_CH_NUM_SHOW
    if (msg->data_type == RICHEPG_ELLD) {
        struct eutelsat_lineup_data *elld = NULL;
        if (richepg_get_elld(&elld) < 0 || !elld) {
            ctrl->info.elld_ch_num = 0;
        } else {
            ctrl->info.elld_ch_num = elld->channel_num;
        }
        if (ctrl->info.data_flag & RICHEPG_ECL) {
            _update_show_channel(true, ctrl->info.ecl_ch_num, ctrl->info.elld_ch_num);
        }
    } else if (msg->data_type == RICHEPG_ECL) {
        struct eutelsat_channel_list *ecl = NULL;
        if (richepg_get_ecl(&ecl) < 0 || !ecl) {
            ctrl->info.ecl_ch_num = 0;
        } else {
            ctrl->info.ecl_ch_num = ecl->channels_num;
        }
        if (ctrl->info.data_flag & RICHEPG_ELLD) {
            _update_show_channel(true, ctrl->info.ecl_ch_num, ctrl->info.elld_ch_num);
        }
    }
#endif
    _update_show_data(ctrl->info.data_flag);

    switch(msg->data_type)
    {
        case RICHEPG_MASTERINDEX:
            break;
        case RICHEPG_ECF:
            break;
        case RICHEPG_ECL:
            break;
        case RICHEPG_ELLI:
            if (richepg_get_lineup() == 0) {
                ctrl->non_stop_flag = 1;
                app_eutel_lineup_create_dialog(true);
            }
            break;
        case RICHEPG_ELLD:
            break;
        case RICHEPG_EPG:
            break;
        case RICHEPG_EAR:
            if (msg->data_type && ctrl->process == MAINT_PROCESS_EPG && ctrl->maintype != USBUP_MAINT)
                app_eutel_proglist_ear_update();
            break;
        default:
            break;
    }
}

static int32_t _process_timer_cb(void *usrdata)
{
    struct richepg_msg msg = {0};
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    char tmpbuf[50] = {0};
    int total = 0, index = 0;

#if BLIND_SCAN_MAINT_SUPPORT
    int percent = 0;
    if (ctrl->blindscan == MAINT_SEGMENT_DOING) {
        if (app_eutel_blind_scan_check(&ctrl->scan_ts_array, &ctrl->scan_ts_num, &percent) < 0) {
            _update_show_percent(percent);
            return -1;
        }
        _update_show_percent(percent);
        ctrl->blindscan = MAINT_SEGMENT_DONE;
        app_eutel_next_ts_start();
        return 0;
    }
#endif
    if(MAINT_PROCESS_EPG == ctrl->process)
    {
        total = richepg_epg_receive_epg_carousel_total_get() * 3;
        index = richepg_epg_cur_receive_epg_carousel_index_get();
        index = index ? (index - 1) * 3: 0;
    }

    while (1) {
        if (richepg_get_msg(&msg) < 0)
            return -1;

        switch (msg.type) {
            case RICHEPG_DATA:
                _richepg_complete(&msg);
                break;
            case RICHEPG_GUI_STATUS:
                _richepg_status(msg.msg_id);
                break;
            case RICHEPG_DATA_PROCESS:
                if(MAINT_PROCESS_EPG == ctrl->process)
                {
                    snprintf(tmpbuf, sizeof(tmpbuf), "%s(%d/%d)", STR_ID_MAINT_PROCESS_EPG, index + 1, total);
                    GUI_SetProperty(TEXT_MT_INFO3, "string", tmpbuf);
                }
                _update_show_tip(STR_ID_RECEIVING_PROCESS);
                _update_show_percent(msg.percent);
                break;
            case RICHEPG_PARSING_PROCESS:
                if(MAINT_PROCESS_EPG == ctrl->process)
                {
                    snprintf(tmpbuf, sizeof(tmpbuf), "%s(%d/%d)", STR_ID_MAINT_PROCESS_EPG, index + 2, total);
                    GUI_SetProperty(TEXT_MT_INFO3, "string", tmpbuf);
                }
                _update_show_tip(STR_ID_PARSING_PROCESS);
                _update_show_percent(msg.percent);
                break;
            case RICHEPG_WRITING_PROCESS:
                if(MAINT_PROCESS_EPG == ctrl->process)
                {
                    snprintf(tmpbuf, sizeof(tmpbuf), "%s(%d/%d)", STR_ID_MAINT_PROCESS_EPG, index + 3, total);
                    GUI_SetProperty(TEXT_MT_INFO3, "string", tmpbuf);
                }
                _update_show_tip(STR_ID_WRITING_PROCESS);
                _update_show_percent(msg.percent);
                break;
            default:
                break;
        }
    }

    return 0;
}

static void _maint_prepare(void)
{
    if (GXCORE_ERROR == GUI_CheckDialog(WND_MAIN_MENU) && GXCORE_SUCCESS == GUI_CheckDialog(WND_FULL_SCREEN)) {
        GUI_EndDialog("after wnd_full_screen");
        GUI_SetProperty("txt_full_state", "state", "hide");
        GUI_SetProperty("img_full_state", "state", "hide");
        g_AppFullArb.timer_stop();
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        g_AppPlayOps.program_stop();

        //the following 4 line codes for #47966
        uint32_t state_mute_bak = g_AppFullArb.state.mute;
        g_AppFullArb.state.mute = STATE_OFF;
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
        g_AppFullArb.state.mute = state_mute_bak;
    } else {
        static GUI_Event event;
        event.type = GUI_KEYDOWN;
        event.key.sym = STBK_EXIT;

        if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
            GUI_SendEvent(WIN_MOVIE_VIEW, &event);
        else if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
            GUI_SendEvent(WIN_MUSIC_VIEW, &event);
        else if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_PIC_VIEW))
            GUI_SendEvent(WIN_PIC_VIEW, &event);
        else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
            GUI_SendEvent(WND_NETVIDEO_PLAY, &event);
    }
}

static bool _check_rich_mode(EutelMaintType type)
{
    bool ret = false;

    if(LINEUP_MAINT == type || USBUP_MAINT == type)
        ret = richepg_rich_mode_get();
    else if(type != NORMAL_MAINT)
        ret = true;

    return ret;
}

static void _maint_exec(EutelMaintType type, RichepgTsTP *ts, char *specified_title)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    GxTime sys_time = {0};

    richepg_set_rich_mode(_check_rich_mode(type));
    if(richepg_store_getint(RICHEPG_RICH_MODE) != richepg_rich_mode_get())
        ctrl->info.rich_mode_changed = true;
    else
        ctrl->info.rich_mode_changed = false;

    if (type != USBUP_MAINT) {
        richepg_epg_build_enable_set(false, false);
        while (richepg_epg_build_working_check())
            GxCore_ThreadDelay(10);
    }

    if (type == INSTALL_MAINT && !app_eutel_same_ts_check(&mgr->sat_array[mgr->sat_sel].tp_array[0], ts)) {
        ctrl->cur_ts_type = MAINT_USER_TS;
        ctrl->cur_ts_sel = 0;
        memcpy(&ctrl->cur_ts, ts, sizeof(RichepgTsTP));
        memcpy(&ctrl->user_ts, ts, sizeof(RichepgTsTP));
    } else {
        ctrl->cur_ts_type = MAINT_HOME_TS;
        ctrl->cur_ts_sel = 0;
        memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[0], sizeof(RichepgTsTP));
    }
    ctrl->last_ts_sel = ctrl->cur_ts_sel;
    ctrl->last_ts_type = ctrl->cur_ts_type;

    // ctrl->non_stop_flag = (type == INSTALL_MAINT || type == FORCE_MAINT) ? 1 : 0;
    ctrl->non_stop_flag = (type != INSTALL_MAINT && GUI_CheckDialog(WND_FULL_SCREEN) != GXCORE_SUCCESS) ? 1 : 0;
    ctrl->maintype = type;

    app_eutel_sat_sel_set(mgr->sat_sel, true);


    if (app_eutel_install_collect_check()) {
        app_eutel_collect_release();
    }

    if (app_eutel_maint_collect_check()) {
        if (!app_simple_lowpower_status_get() && ctrl->maintype != USBUP_MAINT) {
            app_eutel_collect_release();
        }
        GxCore_GetLocalTime(&sys_time);
        app_eutel_collect_add_item_start_maint(CUR_SAT_NAME, sys_time.seconds);
    }

    if ( type != USBUP_MAINT ){
        app_eutel_collect_start_index_record();
    }

    if (GUI_CheckDialog(WND_EUTEL_MAINTENANCE) == GXCORE_ERROR)
        GUI_CreateDialog(WND_EUTEL_MAINTENANCE);

    _app_eutel_maintenance_signal_timer_stop();
    if ( LINEUP_MAINT == type){
        _app_eutel_maintenance_signal_widget_state(APP_EUTEL_MAINTENANCE_SIGNAL_WIDGET_HIDE);
    }
    else{
       _app_eutel_maintenance_signal_widget_state(APP_EUTEL_MAINTENANCE_SIGNAL_WIDGET_SHOW);
       _app_eutel_maintenance_signal_update(APP_EUTEL_MAINTENANCE_SIGNAL_UPDATE_AUTO);
    }

    _update_show_title(type, specified_title);
    _update_show_percent(0);
    _update_show_tip(STR_ID_BLANK);
#if ECL_ELLD_CH_NUM_SHOW
    _update_show_channel(false, 0, 0);
#endif
    _update_show_data(ctrl->info.data_flag);
    if (ctrl->maintype != LINEUP_MAINT) {
        if (type != USBUP_MAINT) {
            GxCore_GetLocalTime(&sys_time);
            ctrl->info.start_time = sys_time.seconds;
        }
        _maint_change_ts();
    }
    APP_TIMER_ADD(ctrl->timer, _process_timer_cb, 50, TIMER_REPEAT);
}

static bool _switch_to_next_sat(void)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    EutelMaintBak *bak = &s_eutel_maint_bak;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    GxTime sys_time = {0};
    int i = 0;
    int next_sat = -1;

    if (app_eutel_maint_collect_check()) {
        GxCore_GetLocalTime(&sys_time);
        app_eutel_collect_add_item_finish_maint(CUR_SAT_NAME, sys_time.seconds);
    }

    if (ctrl->maintype == INSTALL_MAINT || ctrl->maintype == LINEUP_MAINT) {
        _process_popup(STR_ID_MAINT_OK, 6, _popup_ok_cb);
        return false;
    } else if (ctrl->maintype == NORMAL_MAINT || ctrl->maintype == FORCE_MAINT) {
        bak->sat_sel = mgr->sat_sel;
        bak->data_flag = ctrl->info.data_flag;
    } else {
        _maint_update_bat_nit_info();
        app_eutel_homets_update();
    }

    if (!app_richepg_update_mode_get()) {
        _process_popup(STR_ID_MAINT_OK, 6, _popup_ok_cb);
        return false;
    }
    if (!_check_usb_valid()) {
        _process_popup(STR_ID_MAINT_OK, 6, _popup_ok_cb);
        return false;
    }

    if (mgr->sat_sel >= bak->sat_sel) {
        for (i = mgr->sat_sel + 1; i < mgr->sat_num; i++) {
            if (mgr->sat_array[i].lineup_id > 0) {
                next_sat = i;
                break;
            }
        }
        if (next_sat < 0) {
            for (i = 0; i < bak->sat_sel; i++) {
                if (mgr->sat_array[i].lineup_id > 0) {
                    next_sat = i;
                    break;
                }
            }
        }
    } else {
        for (i = mgr->sat_sel + 1; i < bak->sat_sel; i++) {
            if (mgr->sat_array[i].lineup_id > 0) {
                next_sat = i;
                break;
            }
        }
    }
    if (next_sat < 0) {
        _process_popup(STR_ID_MAINT_OK, 6, _popup_ok_cb);
        return false;
    }

    GUI_SetInterface("flush", NULL);//sure ui percent =100%,because next sat ready to maintenance;
    mgr->sat_sel = next_sat;
    richepg_stop();
    app_eutel_maint_release();
    _maint_exec(USBUP_MAINT, NULL, NULL);
    return true;
}

static int32_t _epg_maint_cb(PopDlgRet ret, bool cancel_to_epg)
{
    if(POP_VAL_OK == ret) {
        _maint_prepare();
        _maint_exec(FORCE_MAINT, NULL, NULL);
    } else {
        app_eutel_maint_check_start();
        if (cancel_to_epg) {
            app_eutel_epg_real_create_dialog();
        } else {
            app_eutel_chinfo_create_dialog();
        }
    }
    return 0;
}
static int32_t _epg_maint_cancel_not_to_epg_cb(PopDlgRet ret) { return _epg_maint_cb(ret, false); }
static int32_t _epg_maint_cancel_to_epg_cb(PopDlgRet ret) { return _epg_maint_cb(ret, true); }

void app_eutel_epg_maint_popup(char *tip, bool cancel_to_epg)
{
    app_eutel_maint_check_stop();
    if (cancel_to_epg)
        _maint_popup(tip, _epg_maint_cancel_to_epg_cb, 0);
    else
        _maint_popup(tip, _epg_maint_cancel_not_to_epg_cb, 0);
}

static int32_t _force_maint_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret) {
        _maint_prepare();
        _maint_exec(FORCE_MAINT, NULL, NULL);
    } else {
        app_eutel_maint_check_start();
    }
    return 0;
}

void app_eutel_force_maint_popup(void)
{
    int wait = 0 ;
    app_eutel_maint_check_stop();
    wait = app_fuse_spec_get_wake_from_reboot_memory_flag_value();
    _maint_popup(STR_ID_FORCE_MAINT_TIP, _force_maint_cb, wait);
}

static int32_t _auto_maint_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret) {
        _maint_prepare();
        _maint_exec(NORMAL_MAINT, NULL, STR_ID_AUTO_MAINT);
    } else {
        app_eutel_maint_check_start();
    }
    return 0;
}

void app_eutel_auto_maint_popup(void)
{
    app_eutel_maint_check_stop();
    _maint_popup(STR_ID_AUTO_MAINT_TIP, _auto_maint_cb, 30);
}

void app_eutel_auto_maint_exec(void)
{
    app_eutel_maint_check_stop();
    _maint_prepare();
    _maint_exec(NORMAL_MAINT, NULL, STR_ID_AUTO_MAINT);
}

void app_eutel_install_maint_exec(RichepgTsTP *ts)
{
    app_eutel_maint_check_stop();
    _maint_exec(INSTALL_MAINT, ts, NULL);
}

void app_eutel_manual_maint_menu_exec(void)
{
    app_eutel_maint_check_stop();
    _maint_exec(NORMAL_MAINT, NULL, NULL);
}

int app_eutel_change_lineup(int lineup_id)
{
    EutelMaintCtrl *ctrl = &s_eutel_maint_ctrl;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    GxBusPmDataSat *sat = app_eutel_cur_sat_data_get();
    char *sat_name = NULL;

    if (ctrl->maintype != LINEUP_MAINT) {
        richepg_set_lineup(lineup_id);
        app_eutel_ts_set_lineup(lineup_id);
        GUI_EndDialog(WND_EUTEL_LINEUP);
    } else {
        if (mgr->sat_sel == mgr->sat_sel_bak && mgr->sat_array[mgr->sat_sel].lineup_id == lineup_id)
            return 0;
        if (richepg_switch_lineup_step2(lineup_id) < 0) {
            _process_popup(STR_ID_LINEUP_NOT_FIND, 3, _popup_fail_cb);
            return -1;
        }
        GUI_EndDialog(WND_EUTEL_LINEUP);
        sat_name = (char*)sat->sat_s.sat_name;
        GUI_SetProperty(TEXT_MT_SAT, "string", sat_name);
#if ECL_ELLD_CH_NUM_SHOW
        {
            struct eutelsat_lineup_data *elld = NULL;
            struct eutelsat_channel_list *ecl = NULL;
            if (richepg_get_elld(&elld) == 0 && elld && richepg_get_ecl(&ecl) == 0 && ecl) {
                _update_show_channel(true, ecl->channels_num, elld->channel_num);
            } else {
                _update_show_channel(false, 0, 0);
            }
        }
#endif
        GUI_SetInterface("flush", NULL);

        ctrl->cur_ts_type = MAINT_HOME_TS;
        ctrl->cur_ts_sel = 0;
        memcpy(&ctrl->cur_ts, &mgr->sat_array[mgr->sat_sel].tp_array[0], sizeof(RichepgTsTP));
        ctrl->last_ts_sel = ctrl->cur_ts_sel;
        ctrl->last_ts_type = ctrl->cur_ts_type;
        app_eutel_ts_set_lineup(lineup_id);

#if FAST_SWITCH_PM_SUPPORT
        _update_show_process(MAINT_PROCESS_CHANNEL);
        bool sat_change = (mgr->sat_sel != mgr->sat_sel_bak);
        app_eutel_homets_update();
        ctrl->non_stop_flag = 1;
        if (app_eutel_proglist_update2() < 0) {
            richepg_store_setint(RICHEPG_ELLD_LASTUPDATED, 0); // 错误时清除ELLD时间戳以便下次维护重建
            richepg_store_flush();
            _process_popup(STR_ID_LINEUP_SWITCH_FAIL, 3, _popup_fail_cb);
            return -1;
        }

        richepg_switch_lineup_step3(sat_change, app_richepg_lineup_mode_get());
        _update_show_percent(100);
        _process_popup(STR_ID_LINEUP_SWITCH_OK, 3, _popup_ok_cb);
#else
        _maint_change_ts();
#endif
    }
    return 0;
}

void app_eutel_lineup_select_menu_exec(void)
{
    if (!_check_usb_valid()) {
        _process_popup(STR_ID_INSERT_USB, 3, NULL);
        return;
    }
    app_eutel_maint_check_stop();
    _maint_exec(LINEUP_MAINT, NULL, NULL);
    _update_show_process(MAINT_LINEUP_SELECT);
    app_eutel_lineup_create_dialog(false);
}

int app_eutel_main_menu_check(void)
{
    return app_richepg_mode_get();
}

static int _maint_set_config_ret_value(int ret)
{
	if ( s_eutel_maint_config.ret_value != ret )
	{
		EUTEL_SINFO("maint ret = %d \n",ret);
		s_eutel_maint_config.ret_value = ret ;
		return 1 ;
	}
	return 0 ;
}

static int _app_eutel_wake_to_maint_timer_out_check(void)
{
	int ret = 0 ;
	ret = app_fuse_spec_get_wake_from_reboot_memory_flag_value();
	if ( ret <= 0 )
	{
		return 0 ;
	}
	s_eutel_maint_config.timer_count ++ ;
	if ( s_eutel_maint_config.timer_count >= APP_EUTEL_WAKE_NOT_MAINT_TIMER_OUT_VALUE )
	{
		s_eutel_maint_config.timer_count = 0 ;
		//待机醒来后，维护开始前，正好碰到pvr录制，则超时不处理。
		if ( s_eutel_maint_config.ret_value == 4 )
		{
			EUTEL_SINFO("wake up timer out reset \n");
			return 1;
		}
		app_fuse_spec_wake_from_reboot_flag_keypress(0);
		return 1 ;
	}
	if ( 0 == (s_eutel_maint_config.timer_count % 60) )
	{
		EUTEL_SINFO("wake up timer out = %d \n",s_eutel_maint_config.timer_count);
	}
	return 0 ;
}

static int32_t _check_maint_cb(void *arg)
{
#define PER_HOUR                (3600)
#define PER_DAY                 (24 * PER_HOUR)
#define QUARTER_DAY             (6 * PER_HOUR)
#define PER_WEEK                (7 * PER_DAY)
#define PVR_INTERVAL_SEC        900
#define WAIT_PVR_COUNT          30

    EutelMaintCheck *pcheck = &s_eutel_maint_check;
    GxTime sys_time = {0};
    static int wait_cnt = 0;
    time_t local_zone_sec = 0;
    time_t next_pvr_sec = 0;
    time_t last_update_sec = 0;
    int local_maint_hour = 0;
    int attach_maint_hour = -1;
    int ret = 0 ;

    _app_eutel_wake_to_maint_timer_out_check();

    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH)
           || GXCORE_SUCCESS == GUI_CheckDialog(WND_UPGRADE_PROCESS)) {
        _maint_set_config_ret_value(1);
        return 0;
    }

    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK)) {
        wait_cnt = WAIT_PVR_COUNT;
        _maint_set_config_ret_value(2);
        return 0;
    }
    if (wait_cnt > 0) {
        --wait_cnt;
        _maint_set_config_ret_value(3);
        return 0;
    }
    if (g_AppPvrOps.state != PVR_DUMMY){
        _maint_set_config_ret_value(4);
        return 0;
    }

    GxCore_GetLocalTime(&sys_time);
    local_zone_sec = sys_time.seconds + get_display_time_by_timezone(0);
    last_update_sec = richepg_store_getint(RICHEPG_EPG_TIMESTAMP);
    if (0 == last_update_sec) {
        last_update_sec = richepg_store_getint(RICHEPG_ECL_TIMESTAMP);
        if (0 == last_update_sec){
            _maint_set_config_ret_value(5);
            return 0;
        }
    }

    if ( (local_zone_sec - SYS_DEFAULT_TIME_UTC) < PER_WEEK ){
        if ( s_eutel_maint_config.epg_rebuild == 0 ){
            s_eutel_maint_config.epg_rebuild = 1;
        }
    }
    else {
        if ( s_eutel_maint_config.epg_rebuild ){
            s_eutel_maint_config.epg_rebuild = 0 ;
            if ( app_richepg_mode_get() ){
                app_fuse_spec_epg_stop_work();
                app_fuse_spec_epg_start_work();
                richepg_epg_build(true);
            }
        }
    }

    if (0 == pcheck->pop_time)
        pcheck->pop_time = last_update_sec;

    if (pcheck->pop_time > sys_time.seconds)
        pcheck->pop_time = sys_time.seconds;

    if (sys_time.seconds <= last_update_sec) {
        ret = _maint_set_config_ret_value(7);
        if (ret){
            EUTEL_SINFO("maint timer one = %d,%d \n",sys_time.seconds,last_update_sec);
        }
        return 0;
    }
    if (sys_time.seconds - last_update_sec <= PER_HOUR) {
        ret = _maint_set_config_ret_value(8);
        if (ret){
            EUTEL_SINFO("maint timer two = %d,%d\n",sys_time.seconds,last_update_sec);
        }
        return 0;
    }
    if (sys_time.seconds - pcheck->pop_time <= PER_HOUR) {
        ret = _maint_set_config_ret_value(9);
        if (ret){
            EUTEL_SINFO("maint timer three = %d,%d \n",sys_time.seconds,pcheck->pop_time);
        }
        return 0;
    }

    local_maint_hour = local_zone_sec % PER_DAY / PER_HOUR;
    attach_maint_hour = app_richepg_update_attach_get();

    // FORCE_MAINT
    if (sys_time.seconds - last_update_sec >= PER_WEEK
                && sys_time.seconds - pcheck->pop_time >= QUARTER_DAY) {
        _maint_set_config_ret_value(15);
        EUTEL_SINFO("maint force time = %d %d %d \n",sys_time.seconds,last_update_sec,pcheck->pop_time);
        pcheck->pop_time = sys_time.seconds;
        app_eutel_force_maint_popup();
    } else {
        // NORMAL_MAINT
        if ((local_maint_hour != AUTO_MAINTENANCE_HOUR) && (local_maint_hour != attach_maint_hour)){
            ret = _maint_set_config_ret_value(12);
            if (ret) {
                EUTEL_SINFO("maint hour = %d %d %d \n",local_maint_hour,AUTO_MAINTENANCE_HOUR,attach_maint_hour);
            }
            return 0;
        }
        next_pvr_sec = g_AppBook.get_pvr_time(sys_time.seconds, BOOK_PROGRAM_PVR);
        if (next_pvr_sec > 0 && next_pvr_sec <= PVR_INTERVAL_SEC){
            _maint_set_config_ret_value(13);
            return 0;
        }
        _maint_set_config_ret_value(14);
        EUTEL_SINFO("maint auto timer = %d,%d,%d \n",sys_time.seconds,last_update_sec,pcheck->pop_time);
        pcheck->pop_time = sys_time.seconds;
        app_eutel_auto_maint_popup();
    }

    return 0;
}

int app_eutel_maint_check_start(void)
{
    EutelMaintCheck *pcheck = &s_eutel_maint_check;
    APP_TIMER_ADD(pcheck->timer, _check_maint_cb, 1000, TIMER_REPEAT);
    return 0;
}

int app_eutel_maint_check_stop(void)
{
    EutelMaintCheck *pcheck = &s_eutel_maint_check;
    APP_TIMER_REMOVE(pcheck->timer);
    return 0;
}

static int _richepg_set_mode(bool richepg_mode)
{
    if (richepg_mode) {
        app_eutel_proglist_skip_set(false);
        eutel_channel_ctrl_build();
        eutel_channel_group_all_build();
        eutel_lineup_logo_key_add();
        richepg_epg_build_enable_set(true, true);
        richepg_epg_build(true);
        app_eutel_maint_check_start();
    } else {
        app_eutel_maint_check_stop();
        richepg_epg_build_enable_set(false, true);
        richepg_epg_ctrl_release();
        eutel_channel_ctrl_release();
        app_eutel_proglist_skip_set(true);
    }
    return 0;
}

int app_richepg_switch_mode(void)
{
    int richepg_mode = 0;
    richepg_mode = app_richepg_mode_get();
    richepg_mode = richepg_mode ? 0 : 1;
    app_richepg_mode_set(richepg_mode);
    return _richepg_set_mode(richepg_mode);
}

/**** 切换卫星或切换模式 ****/
static void _switch_popup(char *tip, uint32_t timeout_sec)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = tip;
    pop.create_cb = NULL;
    pop.exit_cb = NULL;
    pop.timeout_sec = timeout_sec;
    popdlg_create(&pop);
    GUI_SetInterface("flush", NULL);
}

static bool _switch_lineup_check(int i)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();

    if (mgr->sat_array[i].lineup_id == 0)
        return false;
    app_eutel_sat_sel_set(i, true);
    if (richepg_get_profile_type(USB_ECL_TIMESTAMP) != RICHEPG_FULL_PROFILE)
        return false;
    if (richepg_switch_lineup_step1() < 0 || richepg_switch_lineup_step2(mgr->sat_array[i].lineup_id) < 0)
    {
        richepg_parse_release();
        return false;
    }

    return true;
}

/*
 * 切换到freescan, 释放sattv卫星 ctrl build的内存，
 * 可以省内存，但是等切回到sattv后，需要时间重建
 */
int app_eutel_get_memory_free_flag(void)
{
	int init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_FREE_MEMORY_KEY,&init_value,RICHEPG_FREE_MEMORY_DEF);
	return init_value;
}

void app_eutel_set_memory_free_flag(int flag)
{
	int value = 0 ;
	value = app_eutel_get_memory_free_flag();
	if (value != flag)
	{
		GxBus_ConfigSetInt(RICHEPG_FREE_MEMORY_KEY, flag);
		EUTEL_SINFO("app eutel memory free_flag set = %d, old = %d \n",flag,value);
	}
}

int app_eutel_core_switch_work_mode(int change_mode,int sat_id,
			int cur_mode,int cur_group,int cur_sat)
{
    RichepgTsMgr *mgr = NULL ;
    int i = 0;
    int sat_sel_bak = 0;
    int cur_id ;
    int build_ok = 0 ;

    if (change_mode == 0)
    {
        EUTEL_SINFO("switch free scan ...\n");
        app_richepg_mode_set(0);

        app_eutel_maint_check_stop();
        richepg_epg_build_enable_set(false, true);
        richepg_epg_ctrl_release();
        if ( app_eutel_get_memory_free_flag() )
        {
            eutel_channel_ctrl_release();
        }
        app_eutel_proglist_skip_set(true);
#ifdef APP_EUTEL_MAINT_CHECK_ANY_MODE
        app_eutel_maint_check_start();
#endif
        return 1 ;
    }

    mgr = richepg_ts_mgr_get();
    if ( mgr == NULL ||  mgr->sat_array == NULL )
        return 0 ;
    sat_sel_bak = mgr->sat_sel;

    cur_id = app_eutel_cur_sat_id_get();
    if ( cur_id == sat_id )
    {
        EUTEL_SINFO("switch sat tv default, id =%d \n",cur_id);
        app_richepg_mode_set(1);

        app_eutel_maint_check_stop();
        richepg_epg_build_enable_set(false, true);
        app_eutel_proglist_skip_set(false);

        build_ok = eutel_channel_ctrl_is_already_build();
        if ( !build_ok)
        {
            _switch_popup(STR_ID_SWITCH_FTA_EPG_TIP, 10);
            eutel_channel_ctrl_build();
            eutel_channel_group_all_build();
            eutel_lineup_logo_key_add();
        }
        richepg_epg_build_enable_set(true, true);
        richepg_epg_build(true);
        app_eutel_maint_check_start();
        if ( !build_ok)
            popdlg_destroy();
        return 1 ;
    }

    for (i = 0; i < mgr->sat_num; i++)
    {
        if ( sat_id == mgr->sat_array[i].sat_id )
            break;
    }
    if (i == mgr->sat_num )
        return 0 ;

    app_richepg_mode_set(1);
    app_eutel_maint_check_stop();
    richepg_epg_build_enable_set(false, true);

    if ( !_switch_lineup_check(i))
    {
        EUTEL_SINFO("back to mode = %d \n",cur_mode);
        app_eutel_sat_sel_set(sat_sel_bak, true);
        if (cur_mode)
        {
            richepg_epg_build_enable_set(true, true);
            richepg_epg_build(true);
			app_eutel_maint_check_start();
        }
        else
        {
            app_richepg_mode_set(0);
            g_AppPlayOps.normal_play.view_info.group_mode = cur_group;
            g_AppPlayOps.normal_play.view_info.sat_id = cur_sat ;
            g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
#ifdef APP_EUTEL_MAINT_CHECK_ANY_MODE
            app_eutel_maint_check_start();
#endif
        }
        return 0;
    }

    EUTEL_SINFO("change sat tv another cur_id = %d \n",cur_id);
    _switch_popup(STR_ID_SWITCH_OTHER_SAT_TIP, 10);

    app_eutel_proglist_skip_set(false);
    richepg_files_splice_ctrl_release();
    richepg_files_splice_index_json_parse_all();//usb parse

    app_eutel_proglist_update2();
    richepg_switch_lineup_step3(true, false);
    richepg_parse_release();
    richepg_ts_mgr_save(i);
    mgr->sat_sel_bak = i;
    richepg_store_flush();

    eutel_channel_ctrl_build();
    eutel_channel_group_all_build();
    eutel_lineup_logo_key_add();
    richepg_epg_build_enable_set(true, true);
    richepg_epg_build(true);
    app_eutel_maint_check_start();

    popdlg_destroy();

    return 1;
}

int app_eutel_core_switch_work_data(int change_mode,int sat_id,int x, int y,int epg_build)
{
    RichepgTsMgr *mgr = NULL ;
    int i = 0;
    int sat_sel_bak = 0;
    int cur_id ;
    int hide_pop = 0 ;
    PopDlg pop;

    if (change_mode == 0)
    {
        EUTEL_SINFO("switch free scan ...\n");
        app_eutel_maint_check_stop();
        richepg_epg_build_enable_set(false, true);
        richepg_epg_ctrl_release();
        if ( app_eutel_get_memory_free_flag() )
        {
            eutel_channel_ctrl_release();
        }
#ifdef APP_EUTEL_MAINT_CHECK_ANY_MODE
        app_eutel_maint_check_start();
#endif
        return 1 ;
    }

    memset(&pop, 0, sizeof(PopDlg));
    if ( x < 0 && y < 0 )
    {
        hide_pop = 1;
    }
    else if ( x > 0 && y > 0 )
    {
        pop.pos.x = x;
        pop.pos.y = y;
    }
    if ( !hide_pop )
    {
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        pop.timeout_sec = 10;
    }

    mgr = richepg_ts_mgr_get();
    if ( mgr == NULL ||  mgr->sat_array == NULL )
        return -1 ;
    sat_sel_bak = mgr->sat_sel;

    cur_id = app_eutel_cur_sat_id_get();
    if ( cur_id == sat_id )
    {
        EUTEL_SINFO("switch sat tv default, id =%d \n",cur_id);

        app_eutel_maint_check_stop();
        richepg_epg_build_enable_set(false, true);
        app_eutel_proglist_skip_set(false);
        if ( !hide_pop )
        {
            pop.str = STR_ID_SWITCH_FTA_EPG_TIP;
            popdlg_create(&pop);
            GUI_SetInterface("flush", NULL);
        }
        eutel_channel_ctrl_build();
        eutel_channel_group_all_build();
        eutel_lineup_logo_key_add();
        if ( epg_build )
        {
            richepg_epg_build_enable_set(true, true);
            richepg_epg_build(true);
        }
        app_eutel_maint_check_start();
        if ( !hide_pop )
            GUI_EndDialog(WND_POP_TIP);
        return 1 ;
    }

    for (i = 0; i < mgr->sat_num; i++)
    {
        if ( sat_id == mgr->sat_array[i].sat_id )
            break;
    }
    if (i == mgr->sat_num )
        return -1 ;

    app_eutel_maint_check_stop();
    richepg_epg_build_enable_set(false, true);

    if ( !_switch_lineup_check(i))
    {
        EUTEL_SINFO("lineup check exit \n");
        app_eutel_sat_sel_set(sat_sel_bak, true);
#ifdef APP_EUTEL_MAINT_CHECK_ANY_MODE
        app_eutel_maint_check_start();
#endif
        return -1;
    }

    EUTEL_SINFO("change sat tv other cur_id = %d \n",cur_id);

    if ( !hide_pop )
    {
        pop.str = STR_ID_SWITCH_OTHER_SAT_TIP ;
        popdlg_create(&pop);
        GUI_SetInterface("flush", NULL);
    }

    app_eutel_proglist_skip_set(false);
    richepg_files_splice_ctrl_release();
    richepg_files_splice_index_json_parse_all();//usb parse

    app_eutel_proglist_update2();
    richepg_switch_lineup_step3(true, false);
    richepg_parse_release();
    richepg_ts_mgr_save(i);
    mgr->sat_sel_bak = i;
    richepg_store_flush();

    eutel_channel_ctrl_build();
    eutel_channel_group_all_build();
    eutel_lineup_logo_key_add();
    if ( epg_build )
    {
        richepg_epg_build_enable_set(true, true);
        richepg_epg_build(true);
    }
    app_eutel_maint_check_start();

    if ( !hide_pop )
        GUI_EndDialog(WND_POP_TIP);

    return 1;
}

#endif

