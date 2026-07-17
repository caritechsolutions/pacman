#include "app.h"
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"
#include "module/app_dir_module.h"
#if (OTA_SUPPORT > 0)
#include "module/app_frontend.h"
#include "module/app_frontend_init.h"
#include "module/app_frontend_board_config.h"
#include "app_pop.h"
#include "full_screen.h"
#include "downloader/gxdlp_api.h"
#include "downloader/gxota_api.h"

#define BOX_DVBS_OTA_OPTIONS  "box_ota_upgrade_options"
#define EDIT_DVBS_OTA_PID     "edit_ota_pid"
#define COMBOX_DVBS_OTA_SAT   "cmbox_ota_sat"
#define COMBOX_DVBS_OTA_TP    "cmbox_ota_tp"
#define BOX_DVBC_OTA_OPTIONS  "box_dvbc_ota_upgrade_options"
#define EDIT_DVBC_OTA_PID     "edit_dvbc_ota_pid"
#define EDIT_DVBC_OTA_FREQ    "edit_dvbc_ota_freq"
#define EDIT_DVBC_OTA_SYM     "edit_dvbc_ota_sym"
#define COMBOX_DVBC_OTA_QAM   "cmbox_dvbc_ota_qam"
#define BOX_DTMB_OTA_OPTIONS  "box_dtmb_ota_upgrade_options"
#define EDIT_DTMB_OTA_PID     "edit_dtmb_ota_pid"
#define EDIT_DTMB_OTA_FREQ    "edit_dtmb_ota_freq"
#define PROGBAR_OTA_UPGRADE   "progbar_ota_upgrade"
#define TEXT_OTA_UPGRADE      "text_ota_upgrade_value"

#define OTA_SEGMENT_CONTENT	  "[All,Default Program]"
#define SPECIAL_FORE_COLOR    "[window_bg_color,window_bg_color,window_bg_color]"

typedef struct
{
    uint32_t fre;
    uint32_t symbol_rate;
    uint32_t qam;
    uint8_t  flag;
}ota_dvbc_dtmb_param;

//Private
static uint32_t s_nFlagOta;
static uint16_t s_wSatTotal = 0;
static uint16_t s_wTpTotal = 0;
GxMsgProperty_NodeByPosGet s_CurOtaSat = {0};
static GxMsgProperty_NodeByPosGet s_CurOtaTp = {0};
static event_list* sp_OtaTimerSignal = NULL;
static event_list* sp_OtaLockTimer = NULL;
static SetTpMode s_set_tp_mode = SET_TP_AUTO;
static event_list* sp_OtaUpgradeTimer = NULL;
static ota_dvbc_dtmb_param previous_ota_param = {0};
static event_list* sp_OtaGetstateTimer = NULL;
#if OTA_BACKEND_POLL
static event_list* sp_OtaCheckTimer = NULL;
static ota_trigger_info s_ota_info = {0};
#endif

enum BOX_ITEM_NAME
{
    OTA_BOX_ITEM_SAT = 0,
    OTA_BOX_ITEM_TP,
    OTA_BOX_ITEM_PID,
    OTA_BOX_ITEM_OK,
    OTA_BOX_ITEM_MAX,
};

enum BOX_DVBC_ITEM_NAME
{
    OTA_BOX_DVBC_ITEM_FREQ = 0,
    OTA_BOX_DVBC_ITEM_SYM,
    OTA_BOX_DVBC_ITEM_QAM,
    OTA_BOX_DVBC_ITEM_PID,
    OTA_BOX_DVBC_ITEM_OK,
    OTA_BOX_DVBC_ITEM_MAX,
};

enum BOX_DTMB_ITEM_NAME
{
    OTA_BOX_DTMB_ITEM_FREQ = 0,
    OTA_BOX_DTMB_ITEM_PID,
    OTA_BOX_DTMB_ITEM_OK,
    OTA_BOX_DTMB_ITEM_MAX,
};

#define OTA_SEGMENT_NUM 2
#define OTA_STR_LEN  128

extern uint32_t app_tuner_cur_tuner_get(void);
extern void app_reboot(void);
extern void app_close_irr(void);
extern int app_bsp_panel_close(void);
extern void app_copy_font_to_ram(void);
extern void Gui_IrrKeyMute(int flag);

static void _ota_set_diseqc(void);
static void _ota_set_tp(SetTpMode set_mode);
//

static void  _ota_set_cur_tuner(void)
{
        uint32_t tuner = app_tuner_cur_tuner_get();
        struct dev_attach attach;
        uint32_t demod_attach;

        APP_DEMOD_ADAPT_TYPE demod_adapt_flag[] = APP_FRONTEND_DEMOD_CONFIG_TYPE_LIST;
        if(DEMOD_AUTO_ADAPT == demod_adapt_flag[tuner]){
#if DEMOD_AUTO_ADAPT_SUPPORT
                int32_t demod_index = FE_DEMOD_ADAPT_INVALID_INDEX;
                int32_t demod_addr_index = FE_DEMOD_ADAPT_INVALID_INDEX;
                demod_adapt_demod_t demod_adapt_params_list[] = APP_FRONTEND_DEMOD_ADAPT_PARAMETERS;
                extern int app_get_demod_config(uint8_t fe_id, int32_t *demod_index, int32_t *demod_addr_index);
                app_get_demod_config(tuner, &demod_index, &demod_addr_index);
                if(FE_DEMOD_ADAPT_INVALID_INDEX == demod_index){
                        app_log_error("[%s:%d], get demod index invalid\n",__func__,__LINE__);
                        return;
                }
                demod_attach = (uint32_t)demod_adapt_params_list[demod_index].demod_attach_adapt;
#endif
        }else{
#ifdef LINUX_OS
                unsigned int   demod_attach_list[]        = APP_FRONTEND_DEMOD_ATTACH;
#else
                demod_attach_func demod_attach_list[]     = APP_FRONTEND_DEMOD_ATTACH;
#endif
                demod_attach = (uint32_t)demod_attach_list[tuner];
        }

#if TUNER_AUTO_ADAPT_SUPPORT
        int32_t tuner_index = FE_TUNER_ADAPT_INVALID_INDEX, addr_index = FE_TUNER_ADDR_ADAPT_INVALID_INDEX;
        struct app_frontend_tuner_adapt_params adapt_params_list[] = APP_FRONTEND_TUNER_ADAPT_PARAMETERS;
        extern int app_get_tuner_config(uint8_t tuner, int32_t *tuner_index, int32_t *addr_index);
        app_get_tuner_config(tuner, &tuner_index, &addr_index);
        attach.tuner_attach = adapt_params_list[tuner_index].tuner_attach;
#else
        APP_TUNER_TYPE tuner_list[] = APP_FRONTEND_TUNER_LIST;
        extern int app_get_tuner_attach(struct dev_attach* attach, APP_TUNER_TYPE type);
        app_get_tuner_attach(&attach, tuner_list[tuner]);
#endif
        GxDLP_Set_FeTunerTypeByAttach((uint32_t)attach.tuner_attach);
        GxDLP_Set_FeDemodTypeByAttach((uint32_t)demod_attach);
}

void app_ota_process_update(int percent)
{
    char Buffer[20] = {0};
    GUI_SetProperty(PROGBAR_OTA_UPGRADE, "value", &percent);
    memset(Buffer, 0, sizeof(Buffer));
    sprintf(Buffer, "%d%%", percent);
    GUI_SetProperty(TEXT_OTA_UPGRADE, "string", Buffer);
}

void app_ota_clear_process(void)
{
    int value = 0;
    GUI_SetProperty(PROGBAR_OTA_UPGRADE, "value", &value);
    GUI_SetProperty(TEXT_OTA_UPGRADE, "string", "0%");
}

static int _failed_ota_pop_cb(PopDlgRet ret)
{

    GxOTA_Stop_Upgrade();
    s_nFlagOta = 0;
    APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
    app_reboot();
    return 0;
}

static int _exit_ota_pop_cb(PopDlgRet ret)
{

    app_ota_clear_process();
    GxOTA_Stop_Upgrade();
    s_nFlagOta = 0;
    APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
    return 0;

}

bool app_get_ota_upgrade_state(void)
{
    int value = 0;
    if(GUI_CheckDialog(WND_OTA_UPGRADE) == GXCORE_SUCCESS)
    {
        GUI_GetProperty(PROGBAR_OTA_UPGRADE, "value", &value);
        if(value >= 50)
        {
            return true;
        }
    }
    return false;
}

int app_ota_process_update_timer(void* usrdata)
{
    struct GxOTA_Msg ota_msg={0};
    GxOTA_Get_Msg(&ota_msg);
    switch(ota_msg.type)
    {
        case GXOTA_GUI_PROCESS:
            app_ota_process_update(ota_msg.percent);
            break;
        case GXOTA_GUI_STATUS:
            {
                switch(ota_msg.msg_id)
                {
                    case GXOTA_STATUS_INIT:
                        app_log_flow("\nGXOTA_STATUS_INIT:0\n");
                        break;

                    case GXOTA_STATUS_NO_DATA_UPDATE:
                        app_log_flow("\nGXOTA_STATUS_NO_DATA_UPDATE:16\n");
                        app_ota_clear_process();
                        app_pop_set_string(STR_ID_CHECK_FAILED);
                        app_pop_reset_timeout(3);
                        APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
                        break;

                    case GXOTA_STATUS_DMX_START_FILTER:
                        app_log_flow("\nGXOTA_STATUS_DMX_START_FILTER:6\n");
                        app_pop_set_string(STR_ID_RECEIVE_DATA);
                        break;

                    case GXOTA_STATUS_FRONTEND_ERROR:
                        app_log_flow("\nGXOTA_STATUS_FRONTEND_ERROR:1\n");
                        app_pop_set_string(STR_ID_CHECK_FAILED);
                        app_pop_reset_timeout(3);
                        APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
                        break;

                    case GXOTA_STATUS_DMX_ERROR:
                        app_log_flow("\nGXOTA_STATUS_DMX_ERROR:5\n");
                        app_pop_set_string(STR_ID_CHECK_FAILED);
                        app_pop_reset_timeout(3);
                        APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
                        break;

                    case GXOTA_STATUS_DMX_TIMEOUT:
                        app_log_flow("\nGXOTA_STATUS_DMX_TIMEOUT:15\n");
                        app_pop_set_string(STR_ID_CHECK_FAILED);
                        app_pop_reset_timeout(3);
                        APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
                        break;

                    case GXOTA_STATUS_VERSION_ERROR:
                        app_log_flow("\nGXOTA_STATUS_VERSION_ERROR:13\n");
                        app_ota_clear_process();
                        app_pop_set_string(STR_ID_VERSION_INVAILD);
                        app_pop_reset_timeout(3);
                        APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
                        break;

                    case GXOTA_STATUS_DMX_DATA_FINISH:
                        app_log_flow("\nGXOTA_STATUS_DMX_DATA_FINISH:7\n");
                        break;
                    case GXOTA_STATUS_UPDATE_SUCCESS:
                        app_log_flow("\nGXOTA_STATUS_UPDATE_SUCCESS:12\n");
                        GxOTA_Stop_Upgrade();
                        s_nFlagOta = 0;
                        APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
                        app_reboot();
                        return 0;

                    case GXOTA_STATUS_VERSION_OK:
                        {
                            app_log_flow("\nGXOTA_STATUS_VERSION_OK:14\n");
                            uint32_t pid;
                            uint32_t tsid;
                            uint32_t polar;
                            fe_type_t fe_type = FE_DVB_S;
                            Gui_IrrKeyMute(1);
                            app_close_irr();
                            app_bsp_panel_close();
                            // 先预读下后续的提示，防止刷完FLASH后，GUI还去访问FLASH的字库，导致
                            // 异常。目前的GUI机制，即使是ASCII字符，也是开机使用后才会缓存内存
                            // 所以，为了保证升级后不操作FLASH，将刷FLASH后的提示预先读到内存，
                            // 但是如果不真正进行绘制，GUI不会将字符缓存到内存，所以目前使用强制
                            // 刷新，先缓存到内存。---还有一个办法就是，升级前将字库读到内存，该
                            // 部分公版方案可以添加，目前客户方案就如此处理了。
                            app_copy_font_to_ram();
                            app_pop_set_exitcb(_failed_ota_pop_cb);
                            pid = GxDLP_Get_DmxPid();
                            tsid = GxDLP_Get_DmxSource();

                            ota_trigger_info ota_info = {0};
                            ota_info.OTA_Type = GXDLP_OTA_DDTEX_DOWNLOAD;
                            ota_info.ota_u.ddtex_upgrade.dmx_pid = pid;
                            ota_info.ota_u.ddtex_upgrade.dmx_source = tsid;
                            if(s_CurOtaSat.sat_data.type == GXBUS_PM_SAT_S)
                            {
                                fe_type = FE_DVB_S;
                                ota_info.ota_u.ddtex_upgrade.Frequency = app_show_to_tp_freq(&s_CurOtaSat.sat_data, &s_CurOtaTp.tp_data);
                                ota_info.ota_u.ddtex_upgrade.Frequency = ota_info.ota_u.ddtex_upgrade.Frequency*1000;
                                ota_info.ota_u.ddtex_upgrade.Symbolrate = s_CurOtaTp.tp_data.tp_s.symbol_rate;
                                polar = app_show_to_lnb_polar(&s_CurOtaSat.sat_data);
                                if(polar == SEC_VOLTAGE_ALL)
                                {
                                    ota_info.ota_u.ddtex_upgrade.Polarity = app_show_to_tp_polar(&s_CurOtaTp.tp_data);
                                }
                                else
                                {
                                    ota_info.ota_u.ddtex_upgrade.Polarity = polar;
                                }
                                ota_info.ota_u.ddtex_upgrade.Modulation = DVBS_QPSK_12;
                                ota_info.ota_u.ddtex_upgrade.Sat22k = app_show_to_sat_22k(s_CurOtaSat.sat_data.sat_s.switch_22K, &s_CurOtaTp.tp_data);
                                ota_info.ota_u.ddtex_upgrade.Diseqc10_Port=s_CurOtaSat.sat_data.sat_s.diseqc10;
                                ota_info.ota_u.ddtex_upgrade.Diseqc11_Port=s_CurOtaSat.sat_data.sat_s.diseqc11;
                            }
                            else if(s_CurOtaSat.sat_data.type == GXBUS_PM_SAT_C)
                            {
                                fe_type = FE_DVB_C;
                                ota_info.ota_u.ddtex_upgrade.Frequency = previous_ota_param.fre;
                                ota_info.ota_u.ddtex_upgrade.Symbolrate = previous_ota_param.symbol_rate;
                                ota_info.ota_u.ddtex_upgrade.Modulation = previous_ota_param.qam+3;
                            }
                            else if(s_CurOtaSat.sat_data.type == GXBUS_PM_SAT_DTMB || s_CurOtaSat.sat_data.type == GXBUS_PM_SAT_T || s_CurOtaSat.sat_data.type == GXBUS_PM_SAT_DVBT2)
                            {
                                if(s_CurOtaSat.sat_data.type == GXBUS_PM_SAT_DTMB)
                                {
                                    fe_type = FE_DTMB;
                                }
                                else if(s_CurOtaSat.sat_data.type == GXBUS_PM_SAT_T)
                                {
                                    fe_type = FE_DVB_T;
                                }
                                else
                                {
                                    fe_type = FE_DVB_T2;
                                }
                                ota_info.ota_u.ddtex_upgrade.Frequency = previous_ota_param.fre;
                                ota_info.ota_u.ddtex_upgrade.bandwidth = BANDWIDTH_8_MHZ;
                            }
                            GxDLP_Set_FeType(fe_type);
                            _ota_set_cur_tuner();
                            GxOTA_SetTrigger(&ota_info);
                            GxOTA_Start_Write_Flash();
                        }
                        break;

                    case GXOTA_STATUS_WRITE_FLASH_OK:
                        app_log_flow("\nGXOTA_STATUS_WRITE_FLASH_OK:9\n");
                        app_pop_set_string(STR_ID_UPGRADE_SUCCESS);
                        break;

                    case GXOTA_STATUS_WRITE_FLASH_FAILED:
                        app_log_flow("\nGXOTA_STATUS_WRITE_FLASH_FAILED:10\n");
                        app_pop_set_string(STR_ID_OTA_UPDATE_FAILD);
                        app_pop_reset_timeout(5);
                        APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
                        return 0;

                    case GXOTA_STATUS_WRITE_FLASH:
                        app_log_flow("\nGXOTA_STATUS_WRITE_FLASH:8\n");
                        app_pop_set_string(STR_ID_FLUSH_FLASH);
                        break;

                    case GXOTA_STATUS_FRONTEND_SETUP:
                        app_log_flow("\nGXOTA_STATUS_FRONTEND_SETUP:2\n");
                        break;

                    case GXOTA_STATUS_FRONTEND_LOCK:
                        app_log_flow("\nGXOTA_STATUS_FRONTEND_LOCK:4\n");
                        break;

                    case GXOTA_STATUS_UPDATE_FAILED:
                        app_log_flow("\nGXOTA_STATUS_UPDATE_FAILED:5\n");
                        app_pop_set_string(STR_ID_OTA_UPDATE_FAILD);
                        app_pop_reset_timeout(3);
                        APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
                        break;

                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
    return 0;
}

static int _exit_menu_ota_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        uint32_t dmxid;
        uint32_t tsid;
        uint32_t pid;
        dmxid = GxDLP_Get_DmxId();
        tsid = GxDLP_Get_DmxSource();
        pid = GxDLP_Get_DmxPid();
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.pos.x = APP_WND_OTA_POP_POS_X;
        pop.pos.y = APP_WND_OTA_POP_POS_Y;
        if(GxOTA_Start_Upgrade(dmxid,tsid,pid)==0)
        {
            pop.str = STR_ID_CHECK_OTA;
            pop.exit_cb = _exit_ota_pop_cb;
            pop.timeout_sec = 1800;
            popdlg_create(&pop);
            APP_TIMER_ADD(sp_OtaUpgradeTimer,app_ota_process_update_timer,100,TIMER_REPEAT);
        }
        else
        {
            pop.str = STR_ID_OTA_UPDATE_FAILD;
            pop.timeout_sec = 5;
            popdlg_create(&pop);
        }
    }
    else
    {
         s_nFlagOta = 0;
    }
	return 0;
}


static int timer_ota_set_upgrade(void)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = _exit_menu_ota_cb;
    pop.str = STR_ID_UPGRADE_SOFTWARE;
    pop.default_ret=POP_VAL_CANCEL;
    pop.pos.x = APP_WND_OTA_POP_POS_X;
    pop.pos.y = APP_WND_OTA_POP_POS_Y;
    popdlg_create(&pop);
    return 0;
}

#if OTA_NO_OEM_PARTiTION
static void _ota_no_oem_ini_config(char* file_path)
{
    if(file_path == NULL)
        return;
    if (GxCore_FileExists("/home/gx/variable_oem.ini") == GXCORE_FILE_UNEXIST)
    {
        app_copy_file_to_file(file_path, "/home/gx/variable_oem.ini", 1024);
    }
    GxDLP_Init("/home/gx/variable_oem.ini",NULL);
}
#endif

#if OTA_BACKEND_POLL
static int _exit_backend_polling_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
#if 0
        app_log_debug("Frequency=%d\n",s_ota_info.ota_u.ddtex_upgrade.Frequency);
        app_log_debug("Symbolrate=%d\n",s_ota_info.ota_u.ddtex_upgrade.Symbolrate);
        app_log_debug("Polarity=%d\n",s_ota_info.ota_u.ddtex_upgrade.Polarity);
        app_log_debug("Sat22k=%d\n",s_ota_info.ota_u.ddtex_upgrade.Sat22k);
        app_log_debug("Diseqc10=%d\n",s_ota_info.ota_u.ddtex_upgrade.Diseqc10_Port);
        app_log_debug("Diseqc11=%d\n",s_ota_info.ota_u.ddtex_upgrade.Diseqc11_Port);
#endif
        _ota_set_cur_tuner();
        GxOTA_SetTrigger(&s_ota_info);
        app_reboot();
    }
    return 0;
}

static int timer_ota_backend_polling(void *userdata)
{
    char* str = NULL;
    char* i18n_str;
    uint32_t new_ts_sw = 0;
    static uint32_t s_ota_sw = 0;
    PopDlg pop;

    if(GxOTA_isTrigger()== true)
    {
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH) ||
                GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP)||
                GXCORE_SUCCESS == GUI_CheckDialog(WND_UPGRADE_PROCESS)||
                GXCORE_SUCCESS == GUI_CheckDialog(WND_OTA_UPGRADE)||
                GXCORE_SUCCESS == GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS))
            return 0;
        GxOTA_Get_OTA_SW_Version(&new_ts_sw);
        if(s_ota_sw == new_ts_sw)
            return 0;
        s_ota_sw = new_ts_sw;
        if((str = (char*)GxCore_Mallocz(OTA_STR_LEN)) == NULL)
            return -1;
        memset(str,0,OTA_STR_LEN);
        i18n_str =(char*)GXGDI_I18N_String(STR_ID_UPDATE_NEW_VER);
        snprintf(str, OTA_STR_LEN, "%s %d",i18n_str,new_ts_sw);

        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = _exit_backend_polling_cb;
        pop.str = str;
        pop.default_ret=POP_VAL_CANCEL;
        popdlg_create(&pop);
        GxCore_Free(str);
    }
    return 0;
}
#endif

static int timer_get_otaupgrade_state(void *userdata)
{
    APP_TIMER_REMOVE(sp_OtaGetstateTimer);
    GxDLP_Download_Type type;
    GxDLP_Get_UpdateType(&type);
    if(type == GXDLP_OTA_FAILED)
    {
        if(GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_TIP))
        {
            GxDLP_Download_Type type = GXDLP_NONE_DOWNLOAD;
            GxDLP_Set_UpdateType(type);
            GxDLP_Sync();
            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.format = POP_FORMAT_DLG;
            pop.mode = POP_MODE_UNBLOCK;
            pop.exit_cb = NULL;
            pop.str = STR_ID_OTA_UPDATE_FAILD;
            popdlg_create(&pop);
        }
    }
    return 0;
}

void app_ota_dlp_init(void)
{
#if OTA_NO_OEM_PARTiTION
    _ota_no_oem_ini_config(WORK_PATH"theme/variable_oem.ini");
#else
    GxDLP_Init("OEM","BACKOEM");
#endif
}

void app_ota_backend_polling_timer_init(void)
{
    APP_TIMER_ADD(sp_OtaGetstateTimer,timer_get_otaupgrade_state,200,TIMER_REPEAT);
#if OTA_BACKEND_POLL
    APP_TIMER_ADD(sp_OtaCheckTimer,timer_ota_backend_polling,3000,TIMER_REPEAT);
#endif
}

void app_ota_backend_polling_start(AppFrontend_SetTp *params)
{
#if OTA_BACKEND_POLL
    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_OTA_UPGRADE))
    {
        uint32_t dmxid;
        uint32_t tsid;
        uint32_t pid;
        fe_type_t fe_type;
        dmxid = GxDLP_Get_DmxId();
        tsid = GxDLP_Get_DmxSource();
        pid = GxDLP_Get_DmxPid();
        s_ota_info.OTA_Type = GXDLP_OTA_DDTEX_DOWNLOAD;
        s_ota_info.ota_u.ddtex_upgrade.dmx_pid = pid;
        s_ota_info.ota_u.ddtex_upgrade.dmx_source = tsid;
        switch(params->type)
        {
            case FRONTEND_DVB_S:
                fe_type = FE_DVB_S;
                GxDLP_Set_FeType(fe_type);
                s_ota_info.ota_u.ddtex_upgrade.Frequency = params->fre*1000;
                s_ota_info.ota_u.ddtex_upgrade.Symbolrate = params->symb;
                s_ota_info.ota_u.ddtex_upgrade.Polarity = params->polar;
                s_ota_info.ota_u.ddtex_upgrade.Modulation = DVBS_QPSK_12;
                s_ota_info.ota_u.ddtex_upgrade.Sat22k =  params->sat22k;
                break;
            case FRONTEND_DVB_C:
                fe_type = FE_DVB_C;
                GxDLP_Set_FeType(fe_type);
                s_ota_info.ota_u.ddtex_upgrade.Frequency = params->fre;
                s_ota_info.ota_u.ddtex_upgrade.Symbolrate = params->symb;
                s_ota_info.ota_u.ddtex_upgrade.Modulation = params->qam;
                break;
            case FRONTEND_DTMB:
            case FRONTEND_DVB_T:
            case FRONTEND_DVB_T2:
                if(params->type == FRONTEND_DTMB)
                {
                    fe_type = FE_DTMB;
                }
                else if(params->type == FRONTEND_DVB_T)
                {
                    fe_type = FE_DVB_T;
                }
                else
                {
                    fe_type = FE_DVB_T2;
                }
                GxDLP_Set_FeType(fe_type);
                s_ota_info.ota_u.ddtex_upgrade.Frequency = params->fre;
                s_ota_info.ota_u.ddtex_upgrade.bandwidth = params->bandwidth;
                break;
            default:
                break;
        }
        GxOTA_Start_Detect_Version(dmxid,tsid,pid);
    }
#endif
}

void app_ota_backend_polling_set_diseqc10(int port)
{
#if OTA_BACKEND_POLL
    s_ota_info.ota_u.ddtex_upgrade.Diseqc10_Port = port;
#endif
}

void app_ota_backend_polling_set_diseqc11(int port)
{
#if OTA_BACKEND_POLL
    s_ota_info.ota_u.ddtex_upgrade.Diseqc11_Port = port;
#endif
}

void app_ota_backend_polling_stop(void)
{
#if OTA_BACKEND_POLL
    GxOTA_Stop_Detect_Version();
#endif
}

/***************************************************************************
						Timer Function
***************************************************************************/
static int timer_ota_show_signal(void *userdata)
{
	unsigned int value = 0;
	char Buffer[20] = {0};
	SignalBarWidget siganl_bar = {0};
	SignalValue siganl_val = {0};

	// progbar strength
	//g_AppNim.property_get(&g_AppNim, AppFrontendID_Strength,(void*)(&value),4);
    app_ioctl(s_CurOtaSat.sat_data.tuner, FRONTEND_STRENGTH_GET, &value);
	siganl_bar.strength_bar = "progbar_ota_strength";
	siganl_val.strength_val = value;

	// strength value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",value);
	GUI_SetProperty("text_ota_strength_value", "string", Buffer);

	// progbar quality
	//g_AppNim.property_get(&g_AppNim, AppFrontendID_Quality,(void*)(&value),4);
    app_ioctl(s_CurOtaSat.sat_data.tuner, FRONTEND_QUALITY_GET, &value);
	siganl_bar.quality_bar= "progbar_ota_quality";
	siganl_val.quality_val = value;
	// quality value
	memset(Buffer,0,sizeof(Buffer));
	sprintf(Buffer,"%d%%",value);
	GUI_SetProperty("text_ota_quality_value", "string", Buffer);

	app_signal_progbar_update(s_CurOtaSat.sat_data.tuner, &siganl_bar, &siganl_val);

	app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);

	return 0;
}


void timer_ota_show_signal_stop(void)
{
	timer_stop(sp_OtaTimerSignal);
}

void timer_ota_show_signal_reset(void)
{
	reset_timer(sp_OtaTimerSignal);
}

static int timer_ota_set_frontend(void *userdata)
{
	SetTpMode force = 0;

	sp_OtaLockTimer = NULL;

	if(userdata != NULL)
	{
		force = *(SetTpMode*)userdata;
		if(force == SET_TP_FORCE)
		{
			GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
		}
	}

	_ota_set_diseqc();
	_ota_set_tp(force);

	return 0;
}



/***************************************************************************
						SUB Function
***************************************************************************/
static int _ota_get_dvbs_sat_num(int num)
{
    int i;
    int total = 0;
	if(num > 0)
	{
		GxMsgProperty_NodeByPosGet node_sat = {0};
		for(i = 0; i < num; i++)
		{
			node_sat.node_type = NODE_SAT;
			node_sat.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
            if(GXBUS_PM_SAT_S == node_sat.sat_data.type)
                total++;
		}
	}
    return total;
}

static int _ota_get_dvbs_sat_sel(int pos)
{
    int i;
    int sel = 0;
    if(pos == 0)
        return 0;
	if(pos >= 1)
	{
		GxMsgProperty_NodeByPosGet node_sat = {0};
		for(i = 0; i <= pos ; i++)
		{
			node_sat.node_type = NODE_SAT;
			node_sat.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
            if(GXBUS_PM_SAT_S == node_sat.sat_data.type)
                sel++;
		}
	}
    return sel-1;
}

static int _ota_get_dvbs_sat_pos(int sel)
{
    int i;
    int pos = 0;
    if(s_wSatTotal > 1)
    {
        GxMsgProperty_NodeByPosGet node_sat = {0};
        for(i = 0; i < s_wSatTotal; i++)
        {
            node_sat.node_type = NODE_SAT;
            node_sat.pos = i;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
            if(GXBUS_PM_SAT_S == node_sat.sat_data.type)
            {
                if(pos == sel)
                    return i;
                pos++;
            }
        }
    }
    return pos;
}

static void _ota_cmb_sat_rebuild(void)
{
#define BUFFER_LENGTH	(MAX_SAT_NAME+20)
	int i = 0;
    int total = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH] = {0};

	//rebuild sat list in combo box
	if(s_wSatTotal == 0)
	{
		app_log_error("_ota_cmb_sat_rebuild total error =%s ,%d\n",__FILE__,__LINE__);
	}
	buffer_all = GxCore_Malloc(s_wSatTotal * BUFFER_LENGTH);
	if(buffer_all == NULL)
	{
		app_log_error("_ota_cmb_sat_rebuild, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}
	memset(buffer_all, 0, s_wSatTotal * BUFFER_LENGTH);
    total = _ota_get_dvbs_sat_num(s_wSatTotal);
    if(total == 0)
    {
         app_log_error("_ota_cmb_sat_rebuild, total = 0...%s,%d\n",__FILE__,__LINE__);
         return;
    }

	//first one
	i = 0;
    int num = 0;
    int first_flag = false;
    sprintf(buffer_all,"[");
    GxMsgProperty_NodeByPosGet node_sat = {0};
    for(i = 0; i < s_wSatTotal; i++)
    {
        node_sat.node_type = NODE_SAT;
        node_sat.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
        if(GXBUS_PM_SAT_S == node_sat.sat_data.type)
        {
            num++;
            if(first_flag == false)
            {
                memcpy(&s_CurOtaSat, &node_sat, sizeof(GxMsgProperty_NodeByPosGet));
                memset(buffer, 0, BUFFER_LENGTH);
                sprintf(buffer,"(%d/%d) %s", num, total, (char*)node_sat.sat_data.sat_s.sat_name);
                strcat(buffer_all, buffer);
                first_flag = true;
            }
            else
            {
                memset(buffer, 0, BUFFER_LENGTH);
                sprintf(buffer,",(%d/%d) %s", num, total, (char*)node_sat.sat_data.sat_s.sat_name);
                strcat(buffer_all, buffer);
            }
        }
    }
    //only 1 sat or finished
	strcat(buffer_all, "]");
	GUI_SetProperty(COMBOX_DVBS_OTA_SAT,"content",buffer_all);

	GxCore_Free(buffer_all);

	return;
}


void _ota_cmb_tp_rebuild(void)
{
#define BUFFER_LENGTH_TP	(64)
	GxMsgProperty_NodeNumGet node_num = {0};
	int i = 0;
	char *buffer_all = NULL;
	char buffer[BUFFER_LENGTH_TP] = {0};
	int cmb_sel;

	// get tp number
	node_num.node_type = NODE_TP;
	node_num.sat_id = s_CurOtaSat.sat_data.id;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	s_wTpTotal = node_num.node_num;

	if(0 == s_wTpTotal)
	{
		// todo
		memset(&s_CurOtaTp, 0, sizeof(s_CurOtaTp));
		GUI_SetProperty(COMBOX_DVBS_OTA_TP,"content","[NONE]");
	}
	else
	{
		buffer_all = GxCore_Malloc(s_wTpTotal * (BUFFER_LENGTH_TP));
		if(buffer_all == NULL)
		{
			app_log_error("_ota_cmb_tp_rebuild, malloc failed...%s,%d\n",__FILE__,__LINE__);
			return ;
		}
		memset(buffer_all, 0, s_wTpTotal * (BUFFER_LENGTH_TP));

		//first one
		i = 0;
		s_CurOtaTp.node_type = NODE_TP;
		s_CurOtaTp.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurOtaTp);

		memset(buffer, 0, BUFFER_LENGTH_TP);
		if(s_CurOtaTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
		{
			sprintf(buffer,"[(%d/%d) %d / %s / %d", i+1, s_wTpTotal,
				s_CurOtaTp.tp_data.frequency, "V" ,s_CurOtaTp.tp_data.tp_s.symbol_rate);
		}
		else if(s_CurOtaTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
		{
			sprintf(buffer,"[(%d/%d) %d / %s / %d", i+1, s_wTpTotal,
				s_CurOtaTp.tp_data.frequency, "H" ,s_CurOtaTp.tp_data.tp_s.symbol_rate);
		}
		strcat(buffer_all, buffer);

		if(s_wTpTotal > 1)
		{
			GxMsgProperty_NodeByPosGet node_tp = {0};
			for(i = 1; i < s_wTpTotal; i++)
			{
				node_tp.node_type = NODE_TP;
				node_tp.pos = i;
				app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
				memset(buffer, 0, BUFFER_LENGTH_TP);
				if(node_tp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
				{
					sprintf(buffer,",(%d/%d) %d / %s / %d", i+1, s_wTpTotal,
						node_tp.tp_data.frequency, "V" ,node_tp.tp_data.tp_s.symbol_rate);
				}
				else if(node_tp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
				{
					sprintf(buffer,",(%d/%d) %d / %s / %d", i+1, s_wTpTotal,
						node_tp.tp_data.frequency, "H" ,node_tp.tp_data.tp_s.symbol_rate);
				}
				strcat(buffer_all, buffer);
			}
		}
		//only 1 sat or finished
		strcat(buffer_all, "]");
		GUI_SetProperty(COMBOX_DVBS_OTA_TP,"content",buffer_all);
		cmb_sel = 0;
		GUI_SetProperty(COMBOX_DVBS_OTA_TP,"select",&cmb_sel);

		GxCore_Free(buffer_all);
	}
	return;
}

static int _ota_sat_tp_poplist_cb(int32_t ret_sel, unsigned short key)
{
    uint32_t box_sel;
    GUI_GetProperty(BOX_DVBS_OTA_OPTIONS, "select", &box_sel);

    if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
    {
        //43583
    }
    else
    {
        if((OTA_BOX_ITEM_SAT== box_sel) && (s_wSatTotal > 0))
        {
            GUI_SetProperty(COMBOX_DVBS_OTA_SAT, "select", &ret_sel);
        }
        if((OTA_BOX_ITEM_TP== box_sel) && (s_wTpTotal > 0))
        {
            GUI_SetProperty(COMBOX_DVBS_OTA_TP, "select", &ret_sel);
        }

        GUI_SetProperty(BOX_DVBS_OTA_OPTIONS, "update", NULL);
    }
    poplist_destory_cb_free_item_content();
    return 0;
}

static int _ota_sat_pop(int sel)
{
	PopList pop_list;
	int ret =-1;
	int i;
	char **sat_name = NULL;
	GxMsgProperty_NodeByPosGet node;
    int total = 0;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_SATELLITE;

	if(0 == s_wSatTotal)
	{
		app_log_error("_ota_sat_pop, no sat..%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	sat_name = (char**)GxCore_Malloc(s_wSatTotal * sizeof(char*));
	if(sat_name == NULL)
	{
		app_log_error("_ota_sat_pop, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return 0;
	}
	if(sat_name != NULL)
	{
		for(i = 0; i < s_wSatTotal; i++)
		{
			node.node_type = NODE_SAT;
			node.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
            if(GXBUS_PM_SAT_S == node.sat_data.type)
            {
                sat_name[total] = (char*)GxCore_Malloc(MAX_SAT_NAME);
                if(sat_name[total] != NULL)
                {
                    memset(sat_name[total], 0, MAX_SAT_NAME);
                    strcpy(sat_name[total] , (char*)node.sat_data.sat_s.sat_name);
                }
                total++;
            }
        }
	}
	pop_list.item_num = total;
	pop_list.item_content = sat_name;

	pop_list.sel = sel;
	pop_list.show_num= true;

	pop_list.mode = POP_MODE_UNBLOCK;
	pop_list.exit_cb = _ota_sat_tp_poplist_cb;

	ret = poplist_create(&pop_list);
	return ret;
}

static int _ota_tp_pop(int sel)
{
#define BUFFER_LENGTH_TP	(64)

	PopList pop_list;
	int ret =-1;
	int i;
	char **tp_info = NULL;
	GxMsgProperty_NodeByPosGet node;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_TP;
	pop_list.item_num = s_wTpTotal;

	if(0 == s_wTpTotal)
	{
		app_log_info("_ota_sat_pop, no tp..%s,%d\n",__FILE__,__LINE__);
		return 0;
	}

	tp_info = (char**)GxCore_Malloc(s_wTpTotal * sizeof(char*));
	if(tp_info != NULL)
	{
		for(i = 0; i < s_wTpTotal; i++)
		{
			node.node_type = NODE_TP;
			node.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

			tp_info[i] = (char*)GxCore_Malloc(BUFFER_LENGTH_TP);
			if(tp_info[i] != NULL)
			{
				memset(tp_info[i], 0, BUFFER_LENGTH_TP);
				if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
				{
					sprintf(tp_info[i],"%d / %s / %d", node.tp_data.frequency, "V" ,node.tp_data.tp_s.symbol_rate);
				}
				else if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
				{
					sprintf(tp_info[i],"%d / %s / %d", node.tp_data.frequency, "H" ,node.tp_data.tp_s.symbol_rate);
				}
			}
		}
	}
	pop_list.item_content = tp_info;

	pop_list.sel = sel;
	pop_list.show_num= true;

	pop_list.mode = POP_MODE_UNBLOCK;
	pop_list.exit_cb = _ota_sat_tp_poplist_cb;

	ret = poplist_create(&pop_list);
	return ret;
}

static void _check_modify_pid(void)
{
    char *pid = NULL;
    uint32_t old_pid;

    switch(s_CurOtaSat.sat_data.type)
    {
        case GXBUS_PM_SAT_S:
            GUI_GetProperty(EDIT_DVBS_OTA_PID, "string", &pid);
            break;
        case GXBUS_PM_SAT_C:
            GUI_GetProperty(EDIT_DVBC_OTA_PID, "string", &pid);
            break;
        case GXBUS_PM_SAT_DTMB:
        case GXBUS_PM_SAT_T:
        case GXBUS_PM_SAT_DVBT2:
            GUI_GetProperty(EDIT_DTMB_OTA_PID, "string", &pid);
            break;
        default:
            GUI_GetProperty(EDIT_DVBS_OTA_PID, "string", &pid);
            break;
    }

    if(NULL == pid)
    {
        app_log_info("\nWRONG, %s, %d\n", __FUNCTION__, __LINE__);
        return ;
    }
    old_pid = GxDLP_Get_DmxPid();
    if(old_pid != atoi(pid))
    {
        GxDLP_Set_DmxPid(pid);
        GxDLP_Sync();
    }
}


static void _ota_ok_keypress(void)
{
	uint32_t box_sel;
	int cmb_sel;

    switch(s_CurOtaSat.sat_data.type)
    {
        case GXBUS_PM_SAT_S:
            GUI_GetProperty(BOX_DVBS_OTA_OPTIONS, "select", &box_sel);

            if((OTA_BOX_ITEM_SAT== box_sel) && (s_wSatTotal > 0))
            {
                GUI_GetProperty(COMBOX_DVBS_OTA_SAT, "select", &cmb_sel);
                cmb_sel = _ota_sat_pop(cmb_sel);
                return ;
            }

            if((OTA_BOX_ITEM_TP== box_sel) && (s_wTpTotal > 0))
            {
                GUI_GetProperty(COMBOX_DVBS_OTA_TP, "select", &cmb_sel);
                cmb_sel = _ota_tp_pop(cmb_sel);
                return ;
            }
            if((OTA_BOX_ITEM_OK == box_sel) && (0 == s_nFlagOta))
            {
                s_nFlagOta = 1;
                _check_modify_pid();
                timer_ota_set_upgrade();
            }

            GUI_SetProperty(BOX_DVBS_OTA_OPTIONS, "update", NULL);
            break;
       case GXBUS_PM_SAT_C:
            GUI_GetProperty(BOX_DVBC_OTA_OPTIONS, "select", &box_sel);

            if((OTA_BOX_DVBC_ITEM_OK == box_sel) && (0 == s_nFlagOta))
            {
                s_nFlagOta = 1;
                _check_modify_pid();
                timer_ota_set_upgrade();
            }

            GUI_SetProperty(BOX_DVBC_OTA_OPTIONS, "update", NULL);
            break;
       case GXBUS_PM_SAT_DTMB:
            GUI_GetProperty(BOX_DTMB_OTA_OPTIONS, "select", &box_sel);

            if((OTA_BOX_DTMB_ITEM_OK == box_sel) && (0 == s_nFlagOta))
            {
                s_nFlagOta = 1;
                _check_modify_pid();
                timer_ota_set_upgrade();
            }

            GUI_SetProperty(BOX_DTMB_OTA_OPTIONS, "update", NULL);
            break;
       case GXBUS_PM_SAT_T:
       case GXBUS_PM_SAT_DVBT2:
            GUI_GetProperty(BOX_DTMB_OTA_OPTIONS, "select", &box_sel);

            if((OTA_BOX_DTMB_ITEM_OK == box_sel) && (0 == s_nFlagOta))
            {
                s_nFlagOta = 1;
                _check_modify_pid();
                timer_ota_set_upgrade();
            }

            GUI_SetProperty(BOX_DTMB_OTA_OPTIONS, "update", NULL);
            break;

        default:
            break;
    }

}

static void _ota_set_diseqc(void)
{
	AppFrontend_DiseqcParameters diseqc10 = {0};
	AppFrontend_DiseqcParameters diseqc11 = {0};

	diseqc10.type = DiSEQC10;
	diseqc10.u.params_10.chDiseqc = s_CurOtaSat.sat_data.sat_s.diseqc10;
	diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&s_CurOtaSat.sat_data);
	if(diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
	{
		if(s_wTpTotal == 0)
		{
			diseqc10.u.params_10.bPolar = SEC_VOLTAGE_OFF;
		}
		else
		{
			diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&s_CurOtaTp.tp_data);
		}
	}
	diseqc10.u.params_10.b22k = app_show_to_sat_22k(s_CurOtaSat.sat_data.sat_s.switch_22K, &s_CurOtaTp.tp_data);

	// add in 20121012
	// for polar
	//GxFrontend_SetVoltage(sat_node.sat_data.tuner, polar);
	AppFrontend_PolarState Polar =diseqc10.u.params_10.bPolar ;
	app_ioctl(s_CurOtaSat.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);

	// for 22k
	//GxFrontend_Set22K(sat_node.sat_data.tuner, sat22k);
	AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
	app_ioctl(s_CurOtaSat.sat_data.tuner, FRONTEND_22K_SET, (void*)&_22K);

	app_set_diseqc_10(s_CurOtaSat.sat_data.tuner, &diseqc10, true);

	diseqc11.type = DiSEQC11;
	diseqc11.u.params_11.chUncom = s_CurOtaSat.sat_data.sat_s.diseqc11;
	diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
	diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
	diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
	app_set_diseqc_11(s_CurOtaSat.sat_data.tuner,&diseqc11, true);

    int32_t motor_state = app_get_motor_state_in_fullscreen();
    MotorState tip = DISH_NONE;

	if((s_CurOtaSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
	{
		tip = app_diseqc12_goto_position(s_CurOtaSat.sat_data.id, s_CurOtaSat.sat_data.tuner,
				s_CurOtaSat.sat_data.sat_s.diseqc12_pos, false, motor_state);
		app_create_position_tip(tip, motor_state);
	}
	else if((s_CurOtaSat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
	{
		LongitudeInfo local_longitude = {0};
		local_longitude.longitude = s_CurOtaSat.sat_data.sat_s.longitude;
		local_longitude.longitude_direct = s_CurOtaSat.sat_data.sat_s.longitude_direct;
		tip = app_usals_goto_position(s_CurOtaSat.sat_data.id, s_CurOtaSat.sat_data.tuner,&local_longitude, NULL, false, motor_state);
        app_create_position_tip(tip, motor_state);
	}
	app_diseqc_sat_id_bak_set(s_CurOtaSat.sat_data.id);

// post sem
	GxFrontendOccur(s_CurOtaSat.sat_data.tuner);

}


static void _ota_set_tp(SetTpMode set_mode)
{
    AppFrontend_SetTp params = {0};

    switch(s_CurOtaSat.sat_data.type)
    {
        case GXBUS_PM_SAT_S:
        {
            if(s_wTpTotal == 0)
            {
                params.fre = 0;
                params.symb = 0;
                params.polar = 0;
            }
            else
            {
                params.fre = app_show_to_tp_freq(&s_CurOtaSat.sat_data, &s_CurOtaTp.tp_data);
                params.symb = s_CurOtaTp.tp_data.tp_s.symbol_rate;

                params.polar = app_show_to_lnb_polar(&s_CurOtaSat.sat_data);
                if(params.polar == SEC_VOLTAGE_ALL)
                {
                    params.polar = app_show_to_tp_polar(&s_CurOtaTp.tp_data);
                }
                params.pls_n = s_CurOtaTp.tp_data.tp_s.pls_n;
                params.stream_size = s_CurOtaTp.tp_data.tp_s.stream_size;
            }
            params.sat22k = app_show_to_sat_22k(s_CurOtaSat.sat_data.sat_s.switch_22K, &s_CurOtaTp.tp_data);

            #if UNICABLE_SUPPORT
            {
                extern int app_unicable_param_check(GxBusPmDataSat sat);
                uint32_t centre_frequency=0 ;
                uint32_t lnb_index=0 ;
                uint32_t set_tp_req=0;
                if(app_unicable_param_check(s_CurOtaSat.sat_data))
                {
                    lnb_index = app_calculate_set_freq_and_initfreq(&s_CurOtaSat.sat_data, &s_CurOtaTp.tp_data,&set_tp_req,&centre_frequency);
                    params.fre  = set_tp_req;
                    params.lnb_index  = lnb_index;
                }
                else
                {
                    params.lnb_index  = 0x20;
                }
                params.centre_fre  = centre_frequency;
                params.if_channel_index  = s_CurOtaSat.sat_data.sat_s.unicable_para.if_channel;
                params.if_fre  = s_CurOtaSat.sat_data.sat_s.unicable_para.centre_fre[params.if_channel_index];
            }
            #endif

            params.type = FRONTEND_DVB_S;
            break;
        }
        case GXBUS_PM_SAT_C:
        {
            params.fre = previous_ota_param.fre;
            params.symb = previous_ota_param.symbol_rate;
            params.qam = previous_ota_param.qam + 3;
            params.type = FRONTEND_DVB_C;
            break;
        }
        case GXBUS_PM_SAT_T:
        case GXBUS_PM_SAT_DVBT2:
        {
            params.fre = previous_ota_param.fre;
            params.bandwidth = GxDLP_Get_FeBandwidth();
            params.set_tp_mode = SET_AUTO_PLP;
            params.type_1501 = DVBT_AUTO_MODE;//PLP Data not in tp note,should be auto
            params.type = FRONTEND_DVB_T2;
            params.data_plp_id = 0xff;
            params.common_plp_exist = 0xff;
            params.common_plp_id = 0xff;
            break;
        }
        case GXBUS_PM_SAT_DTMB:
        {
            params.fre = previous_ota_param.fre;
            params.bandwidth = GxDLP_Get_FeBandwidth();
            params.type = FRONTEND_DTMB;
            break;
        }
        default:
            params.type = FRONTEND_DVB_S;
            break;
    }

	app_set_tp(s_CurOtaSat.sat_data.tuner, &params, set_mode);

	return;
}

SIGNAL_HANDLER int app_ota_upgrade_create(GuiWidget *widget, void *usrdata)
{
    char a=0;
    GxDLP_Set_ForceUpgradeFlag(&a);
    GxMsgProperty_NodeNumGet node_num = {0};
    GxMsgProperty_NodeByPosGet prog_node = {0};
    GxMsgProperty_NodeByIdGet sat_node = {0};
    uint32_t cur_num = 0;
    char buf[10] = {0};
    unsigned int pid = 0;
    char App_Fre[7] = {0};
    char sApp_Sym[5] = {0};
    uint32_t sApp_Mod = 0;
    int32_t sym_value = 0;
    int32_t freq_value = 0;
    int32_t qam_value = 0;
#if OTA_BACKEND_POLL
    app_ota_backend_polling_stop();
#endif

    prog_node.node_type = NODE_PROG;
    prog_node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node));
    if(prog_node.prog_data.id != 0)
    {
        sat_node.node_type = NODE_SAT;
        sat_node.id = prog_node.prog_data.sat_id;
    }
    else
    {
        fe_type_t type;
        GxDLP_Get_FeType(&type);
        if(type == FE_DVB_T2 || type == FE_DVB_T)
            sat_node.id = app_sat_id_get_by_type(GXBUS_PM_SAT_DVBT2);
        else if(type == FE_DVB_C)
            sat_node.id = app_sat_id_get_by_type(GXBUS_PM_SAT_C);
        else if(type == FE_DTMB)
            sat_node.id = app_sat_id_get_by_type(GXBUS_PM_SAT_DTMB);
        else
            sat_node.id = 0;
    }
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node);

    if(GXBUS_PM_SAT_S == sat_node.sat_data.type)
    {
        GUI_SetProperty(BOX_DVBS_OTA_OPTIONS, "state", "show");
        GUI_SetFocusWidget(BOX_DVBS_OTA_OPTIONS);
        // pid
        pid = GxDLP_Get_DmxPid();
        memset(buf, 0, 10);
        sprintf(buf, "%04d", pid);
        GUI_SetProperty(EDIT_DVBS_OTA_PID, "string", buf);

        //get total sat num
        node_num.node_type = NODE_SAT;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
        s_wSatTotal = node_num.node_num;

        _ota_cmb_sat_rebuild();
        _ota_cmb_tp_rebuild();

        cur_num = _ota_get_dvbs_sat_sel(s_CurOtaSat.pos);
        GUI_SetProperty(COMBOX_DVBS_OTA_SAT, "select", &cur_num);

        if(s_wTpTotal > 0)
        {
            cur_num = s_wTpTotal - 1;
        }
        else
        {
            cur_num = s_wTpTotal;
        }
        GUI_SetProperty(COMBOX_DVBS_OTA_TP,"select", &cur_num);

        _ota_set_diseqc();
    }
    else if(GXBUS_PM_SAT_T == sat_node.sat_data.type || GXBUS_PM_SAT_DVBT2 == sat_node.sat_data.type)
    {
        GUI_SetProperty(BOX_DTMB_OTA_OPTIONS, "state", "show");
        GUI_SetFocusWidget(BOX_DTMB_OTA_OPTIONS);

        memset(&previous_ota_param,0,sizeof(ota_dvbc_dtmb_param));

        previous_ota_param.fre=GxDLP_Get_FeFrequency();
        memset(App_Fre,0,7);
        sprintf(App_Fre,"%d",  previous_ota_param.fre/1000);
        GUI_SetProperty(EDIT_DTMB_OTA_FREQ, "string",  App_Fre);

        previous_ota_param.fre=previous_ota_param.fre;
        previous_ota_param.flag = 1;
        pid = GxDLP_Get_DmxPid();
        memset(buf, 0, 10);
        sprintf(buf, "%04d", pid);
        GUI_SetProperty(EDIT_DTMB_OTA_PID, "string", buf);
        memcpy(&s_CurOtaSat, &sat_node, sizeof(GxMsgProperty_NodeByPosGet));
    }
    else if(GXBUS_PM_SAT_C == sat_node.sat_data.type)
    {
        GUI_SetProperty(BOX_DVBC_OTA_OPTIONS, "state", "show");
        GUI_SetFocusWidget(BOX_DVBC_OTA_OPTIONS);

        memset(&previous_ota_param,0,sizeof(ota_dvbc_dtmb_param));

        GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
        memset(App_Fre,0,7);
        sprintf(App_Fre,"%03d.%d", freq_value/1000,(freq_value%1000)/100);
        GUI_SetProperty(EDIT_DVBC_OTA_FREQ, "string", App_Fre);

        GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &sym_value, MAIN_FREQ_SYM_VALUE);
        GxBus_ConfigGetInt(MAIN_FREQ_QAM_KEY, &qam_value, MAIN_FREQ_QAM_VALUE);

        memset(sApp_Sym,0,5);
        sprintf(sApp_Sym, "%04d", sym_value);
        sApp_Mod = qam_value;

        GUI_SetProperty(EDIT_DVBC_OTA_SYM, "string", sApp_Sym);
        GUI_SetProperty(COMBOX_DVBC_OTA_QAM, "select", &sApp_Mod);

        previous_ota_param.fre= freq_value;
        previous_ota_param.symbol_rate= sym_value;
        previous_ota_param.qam= qam_value;
        previous_ota_param.flag = 1;

        // pid
        pid = GxDLP_Get_DmxPid();
        memset(buf, 0, 10);
        sprintf(buf, "%04d", pid);
        GUI_SetProperty(EDIT_DVBC_OTA_PID, "string", buf);

        memcpy(&s_CurOtaSat, &sat_node, sizeof(GxMsgProperty_NodeByPosGet));
    }
    else if(GXBUS_PM_SAT_DTMB == sat_node.sat_data.type)
    {
        GUI_SetProperty(BOX_DTMB_OTA_OPTIONS, "state", "show");
        GUI_SetFocusWidget(BOX_DTMB_OTA_OPTIONS);

        memset(&previous_ota_param,0,sizeof(ota_dvbc_dtmb_param));

        GxBus_ConfigGetInt(MAIN_FREQ_KEY, &freq_value, MAIN_FREQ_VALUE);
        memset(App_Fre,0,7);
        sprintf(App_Fre,"%03d.%d", freq_value/1000,(freq_value%1000)/100);
        GUI_SetProperty(EDIT_DTMB_OTA_FREQ, "string", App_Fre);

        previous_ota_param.fre= freq_value;
        previous_ota_param.flag = 1;

        // pid
        pid = GxDLP_Get_DmxPid();
        memset(buf, 0, 10);
        sprintf(buf, "%04d", pid);
        GUI_SetProperty(EDIT_DTMB_OTA_PID, "string", buf);

        memcpy(&s_CurOtaSat, &sat_node, sizeof(GxMsgProperty_NodeByPosGet));
    }
    _ota_set_tp(SET_TP_FORCE);

    if (reset_timer(sp_OtaTimerSignal) != 0)
    {
        sp_OtaTimerSignal = create_timer(timer_ota_show_signal, 100, NULL, TIMER_REPEAT);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ota_upgrade_destroy(GuiWidget *widget, void *usrdata)
{
	APP_TIMER_REMOVE(sp_OtaTimerSignal);
	APP_TIMER_REMOVE(sp_OtaLockTimer);
    APP_TIMER_REMOVE(sp_OtaUpgradeTimer);
	GxBus_MessageEmptyByOps(&g_service_list[SERVICE_FRONTEND]);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ota_upgrade_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					if(0 == s_nFlagOta)
					{
						GUI_EndDialog("after wnd_full_screen");
					}
					break;

				case STBK_EXIT:
                case STBK_MENU:
                    {
                        _check_modify_pid();
                        GUI_EndDialog(WND_OTA_UPGRADE);
                    }
                    break;

                case STBK_OK:
                    _ota_ok_keypress();
                    break;

                default:
                    break;
            }
        default:
            break;
    }
	return ret;
}


SIGNAL_HANDLER int app_ota_cmb_sat_change(GuiWidget *widget, void *usrdata)
{
	uint32_t sel;

	//show new sat
	GUI_GetProperty(COMBOX_DVBS_OTA_SAT, "select", &sel);
	s_CurOtaSat.node_type = NODE_SAT;
	if(_ota_get_dvbs_sat_pos(sel) != s_CurOtaSat.pos)
	{
		s_CurOtaSat.pos = _ota_get_dvbs_sat_pos(sel);
		s_wTpTotal = 0;
	}

	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurOtaSat);
	_ota_cmb_tp_rebuild();
	_ota_set_tp(SAVE_TP_PARAM);

	//refresh sat para
	sel = 0;
	GUI_SetProperty(COMBOX_DVBS_OTA_TP,"select",&sel);

	s_set_tp_mode = SET_TP_FORCE;
	if(reset_timer(sp_OtaLockTimer) != 0)
	{
		sp_OtaLockTimer = create_timer(timer_ota_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_ota_pid_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_OTA_PID_LEN 4
    GUI_GetProperty("edit_ota_pid", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_OTA_PID_LEN - 1 : 0);
    GUI_SetProperty("edit_ota_pid", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_dvbc_ota_freq_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_DVBC_OTA_FREQ_LEN 5
    GUI_GetProperty("edit_dvbc_ota_freq", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_DVBC_OTA_FREQ_LEN - 1 : 0);
    GUI_SetProperty("edit_dvbc_ota_freq", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_dvbc_ota_sym_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_DVBC_OTA_SYM_LEN 4
    GUI_GetProperty("edit_dvbc_ota_sym", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_DVBC_OTA_SYM_LEN - 1 : 0);
    GUI_SetProperty("edit_dvbc_ota_sym", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_dvbc_ota_pid_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_DVBC_OTA_PID_LEN 4
    GUI_SetProperty("edit_dvbc_ota_pid", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_DVBC_OTA_PID_LEN - 1 : 0);
    GUI_SetProperty("edit_dvbc_ota_pid", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_dtmb_ota_freq_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_DTMB_OTA_FREQ_LEN 5
    GUI_GetProperty("edit_dtmb_ota_freq", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_DTMB_OTA_FREQ_LEN - 1 : 0);
    GUI_SetProperty("edit_dtmb_ota_freq", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_dtmb_ota_pid_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_DTMB_OTA_PID_LEN 4
    GUI_GetProperty("edit_dtmb_ota_pid", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_DTMB_OTA_PID_LEN - 1 : 0);
    GUI_SetProperty("edit_dtmb_ota_pid", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ota_cmb_tp_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;

	GUI_GetProperty(COMBOX_DVBS_OTA_TP, "select", &box_sel);

	s_CurOtaTp.node_type = NODE_TP;
	s_CurOtaTp.pos = box_sel;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &s_CurOtaTp);

	s_set_tp_mode = SET_TP_AUTO;
	if(reset_timer(sp_OtaLockTimer) != 0)
	{
		sp_OtaLockTimer = create_timer(timer_ota_set_frontend, FRONTEND_SET_DELAY_MS, (void *)&s_set_tp_mode, TIMER_ONCE);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_ota_box_keypress(GuiWidget *widget, void *usrdata)
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
            switch(event->key.sym)
            {
                case STBK_UP:
                    {
                        uint32_t box_sel;
                        GUI_GetProperty(BOX_DVBS_OTA_OPTIONS, "select", &box_sel);
                        if(OTA_BOX_ITEM_SAT == box_sel)
                        {
                            box_sel = (OTA_BOX_ITEM_MAX - 1);
                            GUI_SetProperty(BOX_DVBS_OTA_OPTIONS, "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;

                case STBK_DOWN:
                    {
                        uint32_t box_sel;
                        GUI_GetProperty(BOX_DVBS_OTA_OPTIONS, "select", &box_sel);
                        if((OTA_BOX_ITEM_MAX - 1) == box_sel)
                        {
                            box_sel = OTA_BOX_ITEM_SAT;
                            GUI_SetProperty(BOX_DVBS_OTA_OPTIONS, "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;

				case STBK_OK:
				case STBK_MENU:
				case STBK_EXIT:
				case STBK_LEFT:
				case STBK_RIGHT:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
		default:
			break;
	}
	return ret;
}
#if (DEMOD_DVB_C > 0)
SIGNAL_HANDLER int app_dvbc_ota_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    char *value = NULL;
    uint32_t value2 = 0;
    uint32_t fre = 0;
    uint32_t symbol_rate = 0;
    uint32_t qam = 0;
    float edit_fre = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_UP:
                    {
                        GUI_GetProperty(BOX_DVBC_OTA_OPTIONS, "select", &box_sel);
                        if(OTA_BOX_DVBC_ITEM_FREQ == box_sel)
                        {
                            box_sel = (OTA_BOX_DVBC_ITEM_MAX - 1);
                            GUI_SetProperty(BOX_DVBC_OTA_OPTIONS, "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;

                case STBK_DOWN:
                    {
                        GUI_GetProperty(BOX_DVBC_OTA_OPTIONS, "select", &box_sel);
                        if((OTA_BOX_DVBC_ITEM_MAX - 1) == box_sel)
                        {
                            box_sel = OTA_BOX_DVBC_ITEM_FREQ;
                            GUI_SetProperty(BOX_DVBC_OTA_OPTIONS, "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;
                case STBK_1:
                case STBK_2:
                case STBK_3:
                case STBK_4:
                case STBK_5:
                case STBK_6:
                case STBK_7:
                case STBK_8:
                case STBK_9:
                case STBK_0:
                    GUI_GetProperty(BOX_DVBC_OTA_OPTIONS, "select", &box_sel);
                    if(OTA_BOX_DVBC_ITEM_FREQ == box_sel)
                    {
                        GUI_GetProperty(EDIT_DVBC_OTA_FREQ, "string", &value);
                        edit_fre = app_float_edit_str_to_value(value);
                        fre = 1000* edit_fre;
                        if (FALSE ==  app_search_check_fre_valid(fre,DEMOD_TYPE_DVBC))
                        {
                            previous_ota_param.fre = fre;
                            previous_ota_param.flag = 0;
                            break;
                        }

                        previous_ota_param.flag = 1;
                        if (fre != previous_ota_param.fre)
                        {
                            previous_ota_param.fre = fre;
                            _ota_set_tp(SET_TP_FORCE);
                        }
                    }
                    else if (OTA_BOX_DVBC_ITEM_SYM == box_sel)
                    {
                        GUI_GetProperty(EDIT_DVBC_OTA_SYM, "string", &value);
                        symbol_rate = atoi(value);
                        if (FALSE ==  app_search_check_sym_valid(symbol_rate))
                        {
                            previous_ota_param.symbol_rate = symbol_rate;
                            previous_ota_param.flag = 0;
                            break;
                        }

                        previous_ota_param.flag = 1;
                        if (symbol_rate != previous_ota_param.symbol_rate)
                        {
                            previous_ota_param.symbol_rate = symbol_rate;
                            _ota_set_tp(SET_TP_FORCE);
                        }
                    }
                    break;

                case STBK_LEFT:
                case STBK_RIGHT:
                    GUI_GetProperty(COMBOX_DVBC_OTA_QAM, "select", &value2);
                    qam = value2;
                    previous_ota_param.flag = 1;
                    if (qam != previous_ota_param.qam)
                    {
                        previous_ota_param.qam = qam;
                        _ota_set_tp(SET_TP_FORCE);
                    }
                    break;
                case STBK_OK:
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
    return ret;
}
#endif
#if (DEMOD_DTMB > 0 || DEMOD_DVB_T > 0)
SIGNAL_HANDLER int app_dtmb_ota_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    char *value = NULL;
    uint32_t fre = 0;
    float edit_fre = 0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_UP:
                    {
                        GUI_GetProperty(BOX_DTMB_OTA_OPTIONS, "select", &box_sel);
                        if(OTA_BOX_DTMB_ITEM_FREQ == box_sel)
                        {
                            box_sel = (OTA_BOX_DTMB_ITEM_MAX - 1);
                            GUI_SetProperty(BOX_DTMB_OTA_OPTIONS, "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;

                case STBK_DOWN:
                    {
                        GUI_GetProperty(BOX_DTMB_OTA_OPTIONS, "select", &box_sel);
                        if((OTA_BOX_DTMB_ITEM_MAX - 1) == box_sel)
                        {
                            box_sel = OTA_BOX_DTMB_ITEM_FREQ;
                            GUI_SetProperty(BOX_DTMB_OTA_OPTIONS, "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;
                case STBK_1:
                case STBK_2:
                case STBK_3:
                case STBK_4:
                case STBK_5:
                case STBK_6:
                case STBK_7:
                case STBK_8:
                case STBK_9:
                case STBK_0:
                    GUI_GetProperty(BOX_DTMB_OTA_OPTIONS, "select", &box_sel);
                    if(OTA_BOX_DTMB_ITEM_FREQ == box_sel)
                    {
                        GUI_GetProperty(EDIT_DTMB_OTA_FREQ, "string", &value);
                        edit_fre = app_float_edit_str_to_value(value);
                        fre = 1000* edit_fre;
#if DEMOD_DTMB
                        if (FALSE ==  app_search_check_fre_valid(fre,DEMOD_TYPE_DTMB))
                        {
                            previous_ota_param.fre = fre;
                            previous_ota_param.flag = 0;
                            break;
                        }
#endif

                        previous_ota_param.flag = 1;
                        if (fre != previous_ota_param.fre)
                        {
                            previous_ota_param.fre = fre;
                            _ota_set_tp(SET_TP_FORCE);
                        }
                    }
                    break;
                case STBK_OK:
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
    return ret;
}
#endif
bool app_ota_idle(void)
{
	return (s_nFlagOta == 0);
}

void app_ota_menu_exec(void)
{
    fe_type_t type;
    GxDLP_Get_FeType(&type);
    if(type == FE_DVB_T2 || type == FE_DVB_T)
    {
        app_sat_id_get_by_type(GXBUS_PM_SAT_DVBT2);
    }
    else if(type == FE_DVB_C)
    {
        app_sat_id_get_by_type(GXBUS_PM_SAT_C);
    }
    else if(type == FE_DTMB)
    {
        app_sat_id_get_by_type(GXBUS_PM_SAT_DTMB);
    }
    GxMsgProperty_NodeNumGet sat_num = {0};
    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
    if(sat_num.node_num > 0)
    {
        GUI_CreateDialog(WND_OTA_UPGRADE);
    }
    else
    {
        app_mainmenu_tip_exec(MM_NO_SAT);
    }
}
#endif
