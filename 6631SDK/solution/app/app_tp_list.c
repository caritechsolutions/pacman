#include "app.h"
#if (DEMOD_DVB_S > 0)

#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "full_screen.h"
#include "app_book.h"
#include "app_dvbs_sat_opt.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"

#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif

extern unsigned int  app_get_muti_ts_num(int32_t tuner);
/* Private variable------------------------------------------------------- */

enum TP_PLS_N_MODE
{
    TP_PLS_N = 0,
    TP_PLS_ROOT,
    TP_PLS_SAVE
};

enum TP_LIST_MODE
{
    TP_ADD = 0,
    TP_EDIT,
    TP_DELETE,
    TP_SEARCH,
    TP_AUTO_PLSN,
    TP_INPUT_N,
    TP_RECORD_S2L3
};

enum TP_LIST_POPUP_BOX
{
    TP_LIST_POPUP_BOX_ITEM_FREQ = 0,
    TP_LIST_POPUP_BOX_ITEM_POLAR,
    TP_LIST_POPUP_BOX_ITEM_SYM,
    TP_LIST_POPUP_BOX_ITEM_OK
};

enum TP_PARA_VALID_OR_NOT
{
    TP_PARA_VALID=0,
    TP_PARA_INVALID
};

typedef struct
{
    uint32_t frequency;
    uint32_t symbol_rate;
    GxBusPmTpPolar polar;
    uint32_t pls_n;
}TpListTpPara;

#define TP_DETAIL_INFO_KEY  (8811)
#define TP_PROGLIST_KEY     (8812)
#define TP_PLSN_ROOT_KEY    (8813)
#define TP_AUTO_PLSN_KEY    (8814)
#define TP_RECORD_S2L3_KEY  (8815)

#define MAX_INPUT_NUM        (10)
#define MAX_FRE_LEN          (5)
#define IMG_OPT              "img_plsn_root_back2"
#define BMP_CANCEL          "s_bar_choice_blue4_l"
#define BMP_OK              "s_bar_choice_blue4"

#if TP_LIST_MOVE_SORT_SUPPORT
typedef enum
{
    TP_SORT_FRE_0_9 = 0,
    TP_SORT_FRE_9_0,
    TP_SORT_POLAR_H_V,
    TP_SORT_POLAR_V_H,
    TP_SORT_SYM_0_9,
    TP_SORT_SYM_9_0
}tp_sort_mode;

static char *s_tp_sort_list[]={"Frequency(0~9)","Frequency(9~0)","H-V","V-H","Symbol(0~9)","Symbol(9~0)"};
int app_tp_list_sort(tp_sort_mode sort_mode);
int app_tp_list_move(uint32_t* selected_pos, int selected_num, int dst_pos);
#endif

static uint8_t s_CurMode=0;
static uint16_t s_tplist_tp_choise = 0;
static event_list* sp_TpListTimerSignal = NULL;
static event_list* sp_TpListLockTimer=NULL;

// Tl - tp list
static AppIdOps* s_TlIdOps = NULL;

static char s_tplist_edit_input[MAX_INPUT_NUM] = {0};
static int match_exist_sel =0;
static bool match_status = 0;
static bool edit_command_mode = 1;
static bool edit_del_to_zero = 0;
static bool up_down_status = 0;
static bool tp_exist_status = 0;
char buffer_fre_bak[MAX_FRE_LEN +1] = {0};
char buffer_sym_bak[MAX_FRE_LEN +1] = {0};

static uint32_t s_plsn_bak_before_auto;
static bool s_plsn_bak_revert_flag = true;

// create dialog
int app_tp_list_init(int SatSel, int TpSel)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST))
    {
        GUI_EndDialog(WND_TP_LIST);
    }
    GUI_CreateDialog(WND_TP_LIST);
    SatSel = s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] ;
    if(SatSel >= 0)
        GUI_SetProperty("cmb_tp_list_sat", "select", &SatSel);
    if(TpSel >= 0)
        GUI_SetProperty("list_tp_list", "select", &TpSel);

    return 0;
}

/*
function:
data :2012 11 1
*/
status_t app_factory_test_add_tp(uint32_t sat_id,uint32_t * ptp_id)
{
    status_t ret = GXCORE_ERROR;
    GxMsgProperty_NodeAdd NodeAdd;
    memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
    NodeAdd.node_type = NODE_TP;
    NodeAdd.tp_data.sat_id = sat_id ;//s_dvbs_sat_data.cur_sat.sat_data.id;
    NodeAdd.tp_data.frequency = 4000;// InputTpPara.frequency;
    NodeAdd.tp_data.tp_s.symbol_rate = 26850;//InputTpPara.symbol_rate;
    NodeAdd.tp_data.tp_s.polar = 0;// 0:H  1:V   InputTpPara.polar;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
    {
        *ptp_id = NodeAdd.tp_data.id;
        ret = GXCORE_SUCCESS;
    }
    return ret;
}

/* widget macro -------------------------------------------------------- */
static int app_tp_list_clean_signal(void)
{
    app_clean_signal("progbar_tp_list_strength", "text_tp_list_strength_value",
                        "progbar_tp_list_quality", "text_tp_list_quality_value");

    return 0;
}

static int timer_tp_list_show_signal(void *userdata)
{
    app_show_signal("progbar_tp_list_strength", "text_tp_list_strength_value",
                        "progbar_tp_list_quality", "text_tp_list_quality_value");

    return 0;
}

static void timer_tp_list_show_signal_stop(void)
{
    if(sp_TpListTimerSignal != NULL)
        timer_stop(sp_TpListTimerSignal);

    GUI_SetInterface("flush",NULL);
}

static void timer_tp_list_show_signal_reset(void)
{
    if(sp_TpListTimerSignal != NULL)
        reset_timer(sp_TpListTimerSignal);
}

static int timer_tp_list_set_frontend(void *userdata)
{
    s_dvbs_sat_data.set_diseqc(NULL);
    s_dvbs_sat_data.set_tp(SET_TP_FORCE);
    sp_TpListLockTimer = NULL;
    return 0;
}

#define TP_INFO_ALL 0
#define TP_INFO_ADD 1
static void _tp_list_show_tip(void)
{
#define IMG_RED         "img_tp_list_tip_red"
#define TXT_RED         "text_tp_list_tip_red"
#define IMG_GREEN       "img_tp_list_tip_green"
#define TXT_GREEN       "text_tp_list_tip_green"
#define IMG_YELLOW  "img_tp_list_tip_yellow"
#define TXT_YELLOW  "text_tp_list_tip_yellow"
#define IMG_BLUE    "img_tp_list_tip_blue"
#define TXT_BLUE        "text_tp_list_tip_blue"
#define IMG_0       "img_tp_list_tip_0"
#define TXT_0       "text_tp_list_tip_0"
#define IMG_INFO    "img_tp_list_tip_info"
#define TXT_INFO  "text_tp_list_tip_info"

    GUI_SetProperty(IMG_0, "state", "show");
    GUI_SetProperty(TXT_0, "state", "show");

    GUI_SetProperty(IMG_INFO, "state", "show");
    GUI_SetProperty(TXT_INFO, "state", "show");

    GUI_SetProperty(IMG_RED, "state", "show");
    GUI_SetProperty(IMG_GREEN, "state", "show");
    GUI_SetProperty(IMG_YELLOW, "state", "show");
    GUI_SetProperty(IMG_BLUE, "state", "show");

    GUI_SetProperty(TXT_GREEN, "state", "show");
    GUI_SetProperty(TXT_RED, "state", "show");
    GUI_SetProperty(TXT_YELLOW, "state", "show");
    GUI_SetProperty(TXT_BLUE, "state", "show");

#if TP_LIST_MOVE_SORT_SUPPORT
    GUI_SetProperty("img_tp_list_tip_1", "state", "show");
    GUI_SetProperty("text_tp_list_tip_1", "state", "show");
    GUI_SetProperty("img_tp_list_tip_2", "state", "show");
    GUI_SetProperty("text_tp_list_tip_2", "state", "show");
#else
    GUI_SetProperty("img_tp_list_tip_1", "state", "hide");
    GUI_SetProperty("text_tp_list_tip_1", "state", "hide");
    GUI_SetProperty("img_tp_list_tip_2", "state", "hide");
    GUI_SetProperty("text_tp_list_tip_2", "state", "hide");
#endif
}

static void _tp_list_hide_tip(int tp_help_info)
{
#define IMG_RED         "img_tp_list_tip_red"
#define TXT_RED         "text_tp_list_tip_red"
#define IMG_GREEN       "img_tp_list_tip_green"
#define TXT_GREEN       "text_tp_list_tip_green"
#define IMG_YELLOW  "img_tp_list_tip_yellow"
#define TXT_YELLOW  "text_tp_list_tip_yellow"
#define IMG_BLUE    "img_tp_list_tip_blue"
#define TXT_BLUE        "text_tp_list_tip_blue"
#define IMG_0       "img_tp_list_tip_0"
#define TXT_0       "text_tp_list_tip_0"
#define IMG_INFO    "img_tp_list_tip_info"
#define TXT_INFO  "text_tp_list_tip_info"

    GUI_SetProperty(IMG_0, "state", "hide");
    GUI_SetProperty(TXT_0, "state", "hide");

    GUI_SetProperty(IMG_RED, "state", "hide");
    if(tp_help_info == TP_INFO_ALL)
    {
        GUI_SetProperty(IMG_GREEN, "state", "hide");
        GUI_SetProperty(TXT_GREEN, "state", "hide");
        GUI_SetProperty(IMG_INFO, "state", "hide");
        GUI_SetProperty(TXT_INFO, "state", "hide");
    }
    else if (tp_help_info == TP_INFO_ADD)
    {
        GUI_SetProperty(IMG_GREEN, "state", "show");
        GUI_SetProperty(TXT_GREEN, "state", "show");
        GUI_SetProperty(IMG_INFO, "state", "show");
        GUI_SetProperty(TXT_INFO, "state", "show");
    }
    GUI_SetProperty(IMG_YELLOW, "state", "hide");
    GUI_SetProperty(IMG_BLUE, "state", "hide");

    GUI_SetProperty(TXT_RED, "state", "hide");
    GUI_SetProperty(TXT_YELLOW, "state", "hide");
    GUI_SetProperty(TXT_BLUE, "state", "hide");

#if TP_LIST_MOVE_SORT_SUPPORT
    GUI_SetProperty("img_tp_list_tip_1", "state", "hide");
    GUI_SetProperty("text_tp_list_tip_1", "state", "hide");
    GUI_SetProperty("img_tp_list_tip_2", "state", "hide");
    GUI_SetProperty("text_tp_list_tip_2", "state", "hide");
#endif

}

static void _tp_list_set_frontend_s2l3_mode(unsigned int mode)
{
    handle_t fe_handle;
    fe_handle = GxFrontend_IdToHandle(s_dvbs_sat_data.cur_sat.sat_data.tuner);
    if(ioctl(fe_handle, FE_SET_S2L3_MODE, mode) < 0)
        app_log_error("%s fail\n", __func__);
}


SIGNAL_HANDLER int app_tp_list_create(GuiWidget *widget, void *usrdata)
{
#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_set_dialog_pause();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif

    app_tp_list_clean_signal();
    // add in 20120702
    g_AppFullArb.timer_stop();

    s_tplist_tp_choise = 0;

    s_dvbs_sat_data.cur_tuner = TUNER_ALL;
    uint32_t cur_sat_pos = s_dvbs_sat_data.cur_sat.pos;
#if DOUBLE_S2_SUPPORT
    s_dvbs_sat_data.sat_rebuild("cmb_tp_list_sat", true);
#else
    s_dvbs_sat_data.sat_rebuild("cmb_tp_list_sat", false);
#endif
    s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = s_dvbs_sat_data.sat_gui_sel_get_by_pos(cur_sat_pos);
    s_dvbs_sat_data.cur_sat_get();
    GUI_SetProperty("cmb_tp_list_sat", "select", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
    s_dvbs_sat_data.cur_tp_get();
    GUI_SetProperty("list_tp_list", "select", &s_dvbs_sat_data.cur_tp_sel);
    GUI_SetProperty("list_tp_list", "update_all", NULL);

    if(!((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0)))
    {
        GUI_SetFocusWidget("cmb_tp_list_sat");
        _tp_list_hide_tip(TP_INFO_ADD);
    }

    // id manage
    s_TlIdOps = new_id_ops();
    if (s_TlIdOps != NULL)
    {
        s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_dvbs_sat_data.cur_sat_tp_total);
    }

    //set tp
    s_dvbs_sat_data.set_diseqc(NULL);
    s_dvbs_sat_data.set_tp(SET_TP_FORCE);

    APP_TIMER_ADD(sp_TpListTimerSignal, timer_tp_list_show_signal, 100, TIMER_REPEAT);
    app_panel_show_prog_num(PANEL_UNDISPLAY_TIME);
#if !TP_LIST_MOVE_SORT_SUPPORT
    GUI_SetProperty("img_tp_list_tip_1", "state", "hide");
    GUI_SetProperty("text_tp_list_tip_1", "state", "hide");
    GUI_SetProperty("img_tp_list_tip_2", "state", "hide");
    GUI_SetProperty("text_tp_list_tip_2", "state", "hide");
#endif

    return EVENT_TRANSFER_STOP;
}

static int app_tp_list_info_func(int sat_num)
{
    if(sat_num > 0)
    {
#if DOUBLE_S2_SUPPORT
        uint32_t cur_sat_pos = s_dvbs_sat_data.cur_sat.pos;
        s_dvbs_sat_data.sat_rebuild(NULL, false);
        s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = s_dvbs_sat_data.sat_gui_sel_get_by_pos(cur_sat_pos);
        s_dvbs_sat_data.cur_sat_get();
#endif
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
            GUI_EndDialog(WND_TP_LIST);
        else
            timer_tp_list_show_signal_stop();

        app_antenna_setting_init(s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner], s_dvbs_sat_data.cur_tp_sel);
        if(0 == s_dvbs_sat_data.cur_sat_tp_total)
        {
            extern int app_antenna_clean_signal(void);
            app_antenna_clean_signal();
        }
    }
    else
    {// TODO: need to notice , no match date to set
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_NO_SAT;
        pop.title = NULL;
        pop.mode = POP_MODE_UNBLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        pop.timeout_sec = 3;
        popdlg_create(&pop);
    }

    return 0;
}

#if DVB_CLOUD_SUPPORT
#include <gx_dvbonline.h>
extern void app_create_tpsource_menu(char *sat_name);
static bool _check_tp_para_valid(GxBusPmDataSat *p_sat, TpListTpPara *tp_para);
void app_tp_list_display_restore(void)
{
	_tp_list_show_tip();
}

void app_dvbcloud_tplist_add(gx_dvbonline_transponderlist_t *tplist)
{
	GxMsgProperty_NodeAdd NodeAdd;
	TpListTpPara InputTpPara={0};
	uint16_t tp_id;
	int max = 0;
	int i;

	if((!tplist)||(tplist->transponder_num<=0))
	{
		return;
	}

	if(s_dvbs_sat_data.sat_total<=0)
	{
		return;
	}

	max = SYS_MAX_TP - GxBus_PmTpNumGet();
	if(max<=0)
	{
		return;
	}

	for(i=0; (i<tplist->transponder_num)&&(i<max); i++)
	{
		memset(&InputTpPara, 0, sizeof(TpListTpPara));
		InputTpPara.frequency = tplist->transponders[i].frequency;
		InputTpPara.symbol_rate = tplist->transponders[i].symbol_rate;
		if(tplist->transponders[i].polarization == 0)
		{
			InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
		}
		else
		{
			InputTpPara.polar = GXBUS_PM_TP_POLAR_V;
		}

		if(false == _check_tp_para_valid(&s_dvbs_sat_data.cur_sat.sat_data, &InputTpPara))
		{
			continue;
		}

		if(0 != GxBus_PmTpExistChek(s_dvbs_sat_data.cur_sat.sat_data.id, InputTpPara.frequency,
			InputTpPara.symbol_rate, InputTpPara.polar, DVBS_QPSK_12, &tp_id))
		{
			continue;
		}

		memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
		NodeAdd.node_type = NODE_TP;
		NodeAdd.tp_data.sat_id = s_dvbs_sat_data.cur_sat.sat_data.id;
		NodeAdd.tp_data.frequency = InputTpPara.frequency;
		NodeAdd.tp_data.tp_s.symbol_rate = InputTpPara.symbol_rate;
		NodeAdd.tp_data.tp_s.polar = InputTpPara.polar;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
		{
			s_dvbs_sat_data.change |= (TP_LIST_UPDATE_TPINFO | ANT_SET_UPDATE_TPINFO);
		}
	}
}

#endif

SIGNAL_HANDLER int app_tp_list_destroy(GuiWidget *widget, void *usrdata)
{
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    APP_TIMER_REMOVE(sp_TpListTimerSignal);
    APP_TIMER_REMOVE(sp_TpListLockTimer);
    del_id_ops(s_TlIdOps);
    s_TlIdOps = NULL;

    app_epg_info_clean();
    app_panel_show_prog_num(PANEL_DISPLAY_TIME);
    return EVENT_TRANSFER_STOP;
}

#if TP_LIST_MOVE_SORT_SUPPORT
static int _tp_list_sort_cb(int select,unsigned short key)
{
    uint8_t i = 0 ;

    if(key == STBK_OK)
    {
        app_tp_list_sort(select);
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
            s_dvbs_sat_data.change |= ANT_SET_UPDATE_TPINFO;
        else
            s_dvbs_sat_data.tp_rebuild(NULL);
        for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
        {
            s_TlIdOps->id_delete(s_TlIdOps, i);
        }
        s_tplist_tp_choise = 0;
        GUI_SetProperty("list_tp_list", "update", NULL);
        APP_TIMER_ADD(sp_TpListLockTimer, timer_tp_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_ONCE);
    }
    return 0;
}
#endif

void app_tp_list_to_search(WndStatus wnd_ok)
{
    if ( wnd_ok == WND_OK )
    {
        GUI_SetFocusWidget("list_tp_list");
        timer_stop(sp_TpListLockTimer);
        timer_tp_list_show_signal_stop();
        s_CurMode = TP_SEARCH;
        app_search_start();
    }
}

SIGNAL_HANDLER int app_tp_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;

    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                {
                    //update the gui sel in current sat tuner
                    #if DOUBLE_S2_SUPPORT
                    GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

                    memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
                    s_dvbs_sat_data.cur_tuner = s_dvbs_sat_data.cur_sat.sat_data.tuner;
                    s_dvbs_sat_data.sat_gui_sel_update();
                    //reset original sat data
                    memcpy(&s_dvbs_sat_data.cur_sat, &cur_sat_bak, sizeof(GxMsgProperty_NodeByPosGet));
                    #endif
                    GUI_EndDialog("after wnd_full_screen");
                    GUI_SetInterface("flush", NULL);
                }
                break;

            case STBK_EXIT:
            case STBK_MENU:
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
                {
                    int num;
                    #if DOUBLE_S2_SUPPORT
                        s_dvbs_sat_data.cur_tuner = s_dvbs_sat_data.cur_sat.sat_data.tuner;
                    #endif
                    num = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner);
                    if(num == 0)
                    {
                        GUI_EndDialog(WND_TP_LIST);
                        GUI_EndDialog(WND_ANTENNA_SETTING);
                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }
                    else if(num > 0)
                    {
                        app_tp_list_info_func(num);
                    }
                }
                else
                {
                    //update the gui sel in current sat tuner
                    #if DOUBLE_S2_SUPPORT
                    GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

                    memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
                    s_dvbs_sat_data.cur_tuner = s_dvbs_sat_data.cur_sat.sat_data.tuner;
                    s_dvbs_sat_data.sat_gui_sel_update();
                    //reset original sat data
                    memcpy(&s_dvbs_sat_data.cur_sat, &cur_sat_bak, sizeof(GxMsgProperty_NodeByPosGet));
                    #endif
                    GUI_EndDialog(WND_TP_LIST);
                }
                ret = EVENT_TRANSFER_STOP;
                break;
            case STBK_LEFT:
            case STBK_RIGHT:
                GUI_SendEvent("cmb_tp_list_sat", event);
                ret = EVENT_TRANSFER_STOP;
                break;

            case STBK_GREEN:    //add
                {
                    if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                        break;
                    uint32_t tp_num = GxBus_PmTpNumGet();
                    if(tp_num >= SYS_MAX_TP)
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_MAX_TP;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop);
                    }
                    else
                    {
                        memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);

                        s_CurMode = TP_ADD;
                        _tp_list_hide_tip(TP_INFO_ALL);
#if DVB_CLOUD_SUPPORT
						app_create_tpsource_menu((char *)s_dvbs_sat_data.cur_sat.sat_data.sat_s.sat_name);
#else
                        GUI_CreateDialog(WND_TP_LIST_POPUP);
#endif
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }

            case STBK_YELLOW:   //edit
                if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                    break;

                memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);
                if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
                {
                    s_CurMode = TP_EDIT;
                    _tp_list_hide_tip(TP_INFO_ALL);
                    GUI_CreateDialog(WND_TP_LIST_POPUP);
                    ret = EVENT_TRANSFER_STOP;
                }
                break;
            case STBK_RED:      //del
                if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                    break;

                if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
                {
                    s_CurMode = TP_DELETE;
                    if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0))
                    {
                        uint32_t opt_num = 0;
                        opt_num = s_TlIdOps->id_total_get(s_TlIdOps);
                        if(opt_num == 0)
                        {
                            s_TlIdOps->id_add(s_TlIdOps, s_dvbs_sat_data.cur_tp_sel, s_dvbs_sat_data.cur_tp.tp_data.id);
                            GUI_SetProperty("list_tp_list", "update_row", &s_dvbs_sat_data.cur_tp_sel);
                        }
                    }
                    _tp_list_hide_tip(TP_INFO_ALL);
                    GUI_CreateDialog(WND_TP_LIST_POPUP);
                    ret = EVENT_TRANSFER_STOP;
                }
                break;
            case STBK_BLUE:     //search
            {
                if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                    break;

                #if TKGS_SUPPORT
                if(app_tkgs_get_operatemode() == GX_TKGS_AUTOMATIC)
                {
                    PopDlg  pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_OK;
                    pop.str = STR_ID_TKGS_CANT_WORK;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.format = POP_FORMAT_DLG;
                    popdlg_create(&pop);
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
                #endif
                if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
                {
                    SearchIdSet search_id;
                    uint32_t opt_num = 0;

                    if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0))
                    {
                        opt_num = s_TlIdOps->id_total_get(s_TlIdOps);
                        if(opt_num == 0)
                        {
                            s_TlIdOps->id_add(s_TlIdOps, s_dvbs_sat_data.cur_tp_sel, s_dvbs_sat_data.cur_tp.tp_data.id);
                            GUI_SetProperty("list_tp_list", "update_row", &s_dvbs_sat_data.cur_tp_sel);
                            search_id.id_num = 1;
                        }
                        else
                        {
                            search_id.id_num = opt_num;
                        }

                        search_id.id_array = (uint32_t *)GxCore_Calloc(search_id.id_num, sizeof(uint32_t));
                        if(search_id.id_array != NULL)
                        {
                            s_TlIdOps->id_get(s_TlIdOps, (uint32_t *)search_id.id_array);

                            _tp_list_hide_tip(TP_INFO_ALL);
                            app_search_opt_dlg(SEARCH_TP, &search_id);
                            GxCore_Free(search_id.id_array);
                        }
                    }
                    else
                    {
                        s_CurMode = TP_SEARCH;
                        GUI_CreateDialog(WND_TP_LIST_POPUP); //tp not exsit
                    }
                    ret = EVENT_TRANSFER_STOP;
                }
                break;
            }

#if TP_LIST_MOVE_SORT_SUPPORT
            case STBK_1:
            {
                if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0) && (0 == strcasecmp("list_tp_list", GUI_GetFocusWidget())))
                {
                    PopList pop_list;
                    memset(&pop_list, 0, sizeof(PopList));
                    pop_list.title = STR_ID_SORT;
                    pop_list.item_num = 6;
                    pop_list.item_content = s_tp_sort_list;
                    pop_list.sel = 0;
                    pop_list.show_num= false;
                    pop_list.mode= POP_MODE_UNBLOCK;
                    pop_list.exit_cb = _tp_list_sort_cb;
                    poplist_create(&pop_list);
                    ret = EVENT_TRANSFER_STOP;
                }
            }
            break;
            case STBK_2:
            {
                if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0) && (0 == strcasecmp("list_tp_list", GUI_GetFocusWidget())))
                {
                    int opt_num;
                    opt_num = s_TlIdOps->id_total_get(s_TlIdOps);
                    if(opt_num > 0)
                    {
                        uint32_t *selected_pos = NULL;
                        int i,cur_sel;
                        selected_pos = (uint32_t*)GxCore_Malloc(opt_num*sizeof(uint32_t));
                        s_TlIdOps->id_pos_get(s_TlIdOps,selected_pos);
                        GUI_GetProperty("list_tp_list", "select", &cur_sel);
                        app_tp_list_move(selected_pos,opt_num,cur_sel);
                        GxCore_Free(selected_pos);
                        for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
                        {
                            s_TlIdOps->id_delete(s_TlIdOps, i);
                        }
                        s_tplist_tp_choise = 0;
                        GUI_SetProperty("list_tp_list", "update", NULL);
                        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
                            s_dvbs_sat_data.change |= ANT_SET_UPDATE_TPINFO;
                        else
                            s_dvbs_sat_data.tp_rebuild(NULL);
                        APP_TIMER_ADD(sp_TpListLockTimer, timer_tp_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_ONCE);
                    }
                }
                break;
            }
#endif
            case STBK_PAGE_UP:
            case STBK_PAGE_DOWN:
            {
                ret = EVENT_TRANSFER_STOP;
            }
            break;

            case STBK_INFO:
                {
                    int num;
                    if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                        break;
                    #if DOUBLE_S2_SUPPORT
                        s_dvbs_sat_data.cur_tuner = s_dvbs_sat_data.cur_sat.sat_data.tuner;
                    #endif
                        num = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner);
                        if(num>0)
                            app_tp_list_info_func(num);
                    break;
                }
            case STBK_REC_STOP:
                {
                    extern int app_pvr_stop_cb(PopDlgRet ret);
                    PopDlg  pop;
                    if ((g_AppPvrOps.state != PVR_RECORD)
                            && (g_AppPvrOps.state != PVR_TIMESHIFT)
                            && (g_AppPvrOps.state != PVR_TMS_AND_REC)
                            && (g_AppPvrOps.state != PVR_TP_RECORD)){
                        break;
                    }
                    app_pvr_stop();

                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_NO_BTN;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = "TP Record Stop";
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.create_cb = NULL;
                    pop.exit_cb = NULL;
                    pop.timeout_sec= 2;
                    popdlg_create(&pop);

                    GUI_SetProperty("img_tp_list_rec", "state", "hide");
                    _tp_list_set_frontend_s2l3_mode(0);
#if NDEMOD_WITH_1TUNER
                    app_only_cur_frontend_monitor_start(app_tuner_cur_tuner_get());
#else
                    app_all_frontend_monitor_control(FRONTEND_MONITOR_ON, false);
#endif
                    break;
                }

            default:
                break;
        }
    }

    return ret;
}

SIGNAL_HANDLER int app_tp_list_lost_focus(GuiWidget *widget, void *usrdata)
{
    if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0))
    {
        if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
        {
            GUI_SetProperty("list_tp_list", "active", &s_dvbs_sat_data.cur_tp_sel);
            GUI_SetFocusWidget("list_tp_list");
        }
    }

    if(sp_TpListTimerSignal)
        timer_stop(sp_TpListTimerSignal);

    _tp_list_hide_tip(TP_INFO_ALL);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_got_focus(GuiWidget *widget, void *usrdata)
{
    int sel =-1;
    int act_sel= -1;

    GUI_SetProperty("list_tp_list", "active", &act_sel);

    sel = s_dvbs_sat_data.cur_tp_sel;

    if(TP_EDIT == s_CurMode && (s_dvbs_sat_data.change & TP_LIST_UPDATE_TPINFO))//edit tp and changed
    {
        s_dvbs_sat_data.cur_tp_get();
        GUI_SetProperty("list_tp_list", "update_row", &sel);
    }
    else if(TP_ADD == s_CurMode && (s_dvbs_sat_data.change & TP_LIST_UPDATE_TPINFO))//add tp
    {
        // get total tp number
        s_dvbs_sat_data.cur_sat_tp_total = s_dvbs_sat_data.cur_sat_tp_num_get();
        if(s_dvbs_sat_data.cur_sat_tp_total > 0)
        {
            sel = s_dvbs_sat_data.cur_sat_tp_total - 1;
            s_dvbs_sat_data.cur_tp_sel = sel;
            s_dvbs_sat_data.cur_tp_get();

            if(s_TlIdOps != NULL)
            {
                s_TlIdOps->id_close(s_TlIdOps);
                s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_dvbs_sat_data.cur_sat_tp_total);
            }
        }
        else
        {
            sel =-1;
        }

        GUI_SetProperty("list_tp_list", "update_all", NULL);
        GUI_SetProperty("list_tp_list", "select", &sel);
    }
    else if(TP_DELETE == s_CurMode && (s_dvbs_sat_data.change & TP_LIST_UPDATE_TPINFO))  //del tp
    {
        int i;
        int del_count = 0;
        // get total tp number
        s_dvbs_sat_data.cur_sat_tp_total = s_dvbs_sat_data.cur_sat_tp_num_get();
        if(s_dvbs_sat_data.cur_sat_tp_total > 0)
        {
            if(s_TlIdOps->id_total_get(s_TlIdOps) > 0)
            {
                del_count = 0;
                for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total + s_TlIdOps->id_total_get(s_TlIdOps); i++)
                {
                    if(true == s_TlIdOps->id_check(s_TlIdOps, i))
                    {
                        del_count++;
                        continue;
                    }

                    if(i >= sel)
                    {
                        break;
                    }
                }
                sel = i - del_count;
            }

            if(sel >= s_dvbs_sat_data.cur_sat_tp_total)
            {
                sel = s_dvbs_sat_data.cur_sat_tp_total - 1;
            }

            s_dvbs_sat_data.cur_tp_sel = sel;
            s_dvbs_sat_data.cur_tp_get();
        }
        else
        {
            sel = -1;
        }
        if(s_TlIdOps != NULL)
        {
            s_TlIdOps->id_close(s_TlIdOps);
            s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_dvbs_sat_data.cur_sat_tp_total);
        }
        GUI_SetProperty("list_tp_list", "update_all", NULL);
        GUI_SetProperty("list_tp_list", "select", &sel);
    }
    else if(TP_SEARCH == s_CurMode)
    {
        int i;
        SearchIdSet search_id = {0};
        GxMsgProperty_NodeByPosGet node = {0};
        int node_num = s_dvbs_sat_data.cur_sat_tp_num_get();

        if(s_dvbs_sat_data.cur_sat_tp_total != node_num)
        {
            s_dvbs_sat_data.cur_sat_tp_total = node_num;
            if(s_dvbs_sat_data.cur_sat_tp_total > 0)
            {
                search_id.id_num = s_TlIdOps->id_total_get(s_TlIdOps);
                if(search_id.id_num > 0)
                {
                    search_id.id_array = (uint32_t *)GxCore_Calloc(search_id.id_num, sizeof(uint32_t));
                    if(search_id.id_array != NULL)
                    {
                        search_id.id_num = 0;
                        for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
                        {
                            if(true == s_TlIdOps->id_check(s_TlIdOps, i))
                            {
                                search_id.id_array[search_id.id_num] = i;
                                search_id.id_num ++;
                            }
                        }
                    }
                }

                if(s_TlIdOps != NULL)
                {
                    s_TlIdOps->id_close(s_TlIdOps);
                    s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_dvbs_sat_data.cur_sat_tp_total);
                }

                if(search_id.id_num > 0)
                {
                    for(i = 0; i < search_id.id_num; i++)
                    {
                        node.node_type = NODE_TP;
                        node.pos = search_id.id_array[i];
                        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
                        s_TlIdOps->id_add(s_TlIdOps, search_id.id_array[i], node.tp_data.id);
                    }
                    if(search_id.id_array != NULL)
                        GxCore_Free(search_id.id_array);
                }

                GUI_SetProperty("list_tp_list", "update_all", NULL);
            }
        }
    }
    else
    {
        GUI_SetProperty("list_tp_list", "update_all", NULL);
        GUI_SetProperty("list_tp_list", "select", &sel);
    }
    if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
    {
        if(strcmp(GUI_GetFocusWidget(), "list_tp_list") == 0)
            _tp_list_show_tip();
        else
            _tp_list_hide_tip(TP_INFO_ADD);
    }
    else
        _tp_list_hide_tip(TP_INFO_ALL);

    APP_TIMER_ADD(sp_TpListLockTimer, timer_tp_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    s_dvbs_sat_data.set_diseqc(NULL);
    s_dvbs_sat_data.set_tp(SET_TP_FORCE);

    //clear
    s_CurMode=0;
    s_dvbs_sat_data.change &= ~(TP_LIST_UPDATE_TPINFO);

    timer_tp_list_show_signal_reset();

    if(1 == match_status)
    {
        int i;
        static GxMsgProperty_NodeByPosGet node_tp;

        for(i=0;i<s_dvbs_sat_data.cur_sat_tp_total;i++)
        {
            node_tp.node_type = NODE_TP;
            node_tp.pos = i;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp);
            if(node_tp.sat_data.id == match_exist_sel)
            {
                if((GUI_GetFocusWidget()==NULL)
                    ||(0 == strcasecmp("btn_tplist_tip_ok_exist", GUI_GetFocusWidget())))
                {
                    GUI_SetProperty("list_tp_list", "select", &i);
                }
                return EVENT_TRANSFER_STOP;
            }
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_list_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_list_change(GuiWidget *widget, void *usrdata)
{
    uint32_t sel;

    GUI_GetProperty("list_tp_list", "select", &sel);
    s_dvbs_sat_data.cur_tp_sel = sel;
    s_dvbs_sat_data.cur_tp_get();

    APP_TIMER_ADD(sp_TpListLockTimer, timer_tp_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_list_get_total(GuiWidget *widget, void *usrdata)
{
    s_dvbs_sat_data.cur_sat_tp_total = s_dvbs_sat_data.cur_sat_tp_num_get();
    return s_dvbs_sat_data.cur_sat_tp_total;
}

SIGNAL_HANDLER int app_tp_list_list_get_data(GuiWidget *widget, void *usrdata)
{
#define MAX_LEN 30
    ListItemPara* item = NULL;
    static GxMsgProperty_NodeByPosGet node;
    static char buffer1[MAX_LEN];
    static char buffer2[MAX_LEN];
    static char buffer3[MAX_LEN];
    static char buffer4[MAX_LEN];

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    //col-0: choice
    item->x_offset = 0;
    if (s_TlIdOps->id_check(s_TlIdOps, item->sel) == 0)//ID_UNEXIST
        item->image = NULL;
    else
        item->image = "s_choice";
    item->string = NULL;

    //col-1: num
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer1, 0, MAX_LEN);
    sprintf(buffer1, " %d", (item->sel+1));
    item->string = buffer1;

    //get tp node
    node.node_type = NODE_TP;
    node.pos = item->sel;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

    //col-1: freq
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
    if(GXBUS_PM_SAT_DVBT2 == s_dvbs_sat_data.cur_sat.sat_data.type)
    {
        char* bandwidth_str[3] = {"8M", "7M","6M"};
        item = item->next;//col-1: freq
        item->x_offset = 0;
        item->image = NULL;
        memset(buffer2, 0, MAX_LEN);
        sprintf(buffer2,"%d.%d",node.tp_data.frequency/1000, (node.tp_data.frequency/100)%10);
        item->string = buffer2;

        item = item->next;//col-1: polar
        item = item->next;//col-1: symb
        item->x_offset = 0;
        item->image = NULL;
        memset(buffer4, 0, MAX_LEN);
        if(node.tp_data.tp_dvbt2.bandwidth <BANDWIDTH_AUTO )
        strcpy(buffer4,bandwidth_str[node.tp_data.tp_dvbt2.bandwidth]);
        item->string = buffer4;
    }else
#endif
#if (DEMOD_DVB_C > 0)
    if(GXBUS_PM_SAT_C == s_dvbs_sat_data.cur_sat.sat_data.type)
    {
        char* modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};
        item = item->next;//col-1: freq
        item->x_offset = 0;
        item->image = NULL;
        memset(buffer2, 0, MAX_LEN);
        sprintf(buffer2,"%d",node.tp_data.frequency/1000);
        item->string = buffer2;

        item = item->next;//col-1: polar
        item->x_offset = 0;
        item->image = NULL;
        memset(buffer3, 0, MAX_LEN);
		if((node.tp_data.tp_c.modulation >= DVBC_16QAM) && (node.tp_data.tp_c.modulation <= DVBC_256QAM))
        strcpy(buffer3,modulation_str[node.tp_data.tp_c.modulation-DVBC_16QAM]);
        item->string = buffer3;

        item = item->next;//col-1: symb
        item->x_offset = 0;
        item->image = NULL;
        memset(buffer4, 0, MAX_LEN);
        sprintf(buffer4,"%d",node.tp_data.tp_c.symbol_rate);
        item->string = buffer4;
    }else
#endif
#if (DEMOD_DTMB > 0)
    if(GXBUS_PM_SAT_DTMB == s_dvbs_sat_data.cur_sat.sat_data.type)
    {
        if(GXBUS_PM_SAT_1501_DTMB == s_dvbs_sat_data.cur_sat.sat_data.sat_dtmb.work_mode)
        {
            char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};// have 7M?
            item = item->next;//col-1: freq
            item->x_offset = 0;
            item->image = NULL;
            memset(buffer2, 0, MAX_LEN);
            sprintf(buffer2,"%d.%d",node.tp_data.frequency/1000, (node.tp_data.frequency/100)%10);
            item->string = buffer2;

            item = item->next;//col-1: polar
            item = item->next;//col-1: symb
            item->x_offset = 0;
            item->image = NULL;
            memset(buffer4, 0, MAX_LEN);
            if(node.tp_data.tp_dtmb.symbol_rate < BANDWIDTH_AUTO )
            strcpy(buffer4,bandwidth_str[node.tp_data.tp_dtmb.bandwidth]);
            item->string = buffer4;
        }
        else
        {
            char* modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};
            item = item->next;//col-1: freq
            item->x_offset = 0;
            item->image = NULL;
            memset(buffer2, 0, MAX_LEN);
            sprintf(buffer2,"%d",node.tp_data.frequency/1000);
            item->string = buffer2;

            item = item->next;//col-1: polar
            item->x_offset = 0;
            item->image = NULL;
            memset(buffer3, 0, MAX_LEN);
			if((node.tp_data.tp_dtmb.modulation >= DVBC_16QAM) && (node.tp_data.tp_dtmb.modulation <= DVBC_256QAM))
            strcpy(buffer3,modulation_str[node.tp_data.tp_dtmb.modulation-DVBC_16QAM]);
            item->string = buffer3;

            item = item->next;//col-1: symb
            item->x_offset = 0;
            item->image = NULL;
            memset(buffer4, 0, MAX_LEN);
            sprintf(buffer4,"%d",node.tp_data.tp_dtmb.symbol_rate);
            item->string = buffer4;
        }
    }else
#endif
    {
        //col-1: freq
        item = item->next;
        item->x_offset = 0;
        item->image = NULL;
        memset(buffer2, 0, MAX_LEN);
        if(node.tp_data.frequency > 99999)
        {
            sprintf(buffer2,"%05d",node.tp_data.frequency);
        }
        else
        {
            sprintf(buffer2,"%d",node.tp_data.frequency);
        }
        item->string = buffer2;

        //col-1: polar
        item = item->next;
        item->x_offset = 0;
        item->image = NULL;
        memset(buffer3, 0, MAX_LEN);
        if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
        {
            sprintf(buffer3,"%s", "V");
        }
        else if(node.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
        {
            sprintf(buffer3,"%s", "H");
        }
        item->string = buffer3;

        //col-1: symb
        item = item->next;
        item->x_offset = 0;
        item->image = NULL;
        memset(buffer4, 0, MAX_LEN);
        if(node.tp_data.tp_s.symbol_rate > 99999)
        {
            sprintf(buffer4,"%05d",node.tp_data.tp_s.symbol_rate);
        }
        else
        {
            sprintf(buffer4,"%d",node.tp_data.tp_s.symbol_rate);
        }

        item->string = buffer4;
    }

    return EVENT_TRANSFER_STOP;
}

static int _key_to_number(int key)
{
    int num = 0;

    switch(key)
    {
        case STBK_0:
            num = 0;
            break;
        case STBK_1:
            num = 1;
            break;
        case STBK_2:
            num = 2;
            break;
        case STBK_3:
            num = 3;
            break;
        case STBK_4:
            num = 4;
            break;
        case STBK_5:
            num = 5;
            break;
        case STBK_6:
            num = 6;
            break;
        case STBK_7:
            num = 7;
            break;
        case STBK_8:
            num = 8;
            break;
        case STBK_9:
            num = 9;
            break;

        default:
            break;
    }

    return num;
}

static void _tp_list_tp_record_s2l3(unsigned int mode)
{
    usb_check_state usb_state = USB_OK;
    PopDlg pop;
    if(g_AppPvrOps.state == PVR_DUMMY)
    {
        // check usb state
        usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
        if(usb_state == USB_ERROR)
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_INSERT_USB;
            pop.mode = POP_MODE_UNBLOCK;
            pop.create_cb = NULL;
            pop.exit_cb = NULL;
            pop.timeout_sec= 3;
            popdlg_create(&pop);
            return;
        }
        if(usb_state == USB_NO_SPACE)
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_NO_SPACE;
            pop.mode = POP_MODE_UNBLOCK;
            pop.create_cb = NULL;
            pop.exit_cb = NULL;
            pop.timeout_sec= 3;
            popdlg_create(&pop);
            return;
        }

        app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, false);
        _tp_list_set_frontend_s2l3_mode(mode);
        app_tplist_rec_start();
        GUI_SetProperty("img_tp_list_rec", "state", "show");
    }
    else
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_STOP_PVR_FIRST;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 3;
        popdlg_create(&pop);
    }

}

static int _tp_list_extended_func(int key)
{
#define DEFAULT_ENTRY_KEY_LEN 4

    int key_code[DEFAULT_ENTRY_KEY_LEN] = {STBK_8, STBK_8, STBK_1, 0};
    static int correct_code_num = 0;
    uint32_t extended_num = 0;

    if((key_code[correct_code_num] == key)
            || (DEFAULT_ENTRY_KEY_LEN - 1 == correct_code_num))
    {
        key_code[correct_code_num++] = key;
        if(correct_code_num == DEFAULT_ENTRY_KEY_LEN)
        {
            correct_code_num = 0;
            extended_num = 1000 * _key_to_number(key_code[0])
                            + 100 * _key_to_number(key_code[1])
                            + 10 * _key_to_number(key_code[2])
                            + _key_to_number(key_code[3]);

            switch(extended_num)
            {
                case TP_DETAIL_INFO_KEY:
                    if (s_dvbs_sat_data.cur_sat.sat_data.type != GXBUS_PM_SAT_S)
                        break;
                    timer_tp_list_show_signal_stop();
                    if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
                    {
                        _tp_list_hide_tip(TP_INFO_ALL);
                        GUI_CreateDialog(WND_TP_DETAIL_INFO);
                    }
                    break;

                case TP_PROGLIST_KEY:
                    if (s_dvbs_sat_data.cur_sat.sat_data.type != GXBUS_PM_SAT_S)
                        break;
                    timer_tp_list_show_signal_stop();
                    if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
                    {
                        _tp_list_hide_tip(TP_INFO_ALL);
                        GUI_CreateDialog(WND_TP_PROGLIST);
                    }
                    break;

                case TP_PLSN_ROOT_KEY:
                    if (s_dvbs_sat_data.cur_sat.sat_data.type != GXBUS_PM_SAT_S)
                        break;
                    _tp_list_hide_tip(TP_INFO_ALL);
                    memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);
                    edit_command_mode = 1;
                    GUI_CreateDialog(WND_PLSN_ROOT);
                    break;

                case TP_AUTO_PLSN_KEY:
                    if (s_dvbs_sat_data.cur_sat.sat_data.type != GXBUS_PM_SAT_S)
                        break;
                    if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
                    {
                        s_CurMode = TP_AUTO_PLSN;
                        s_plsn_bak_before_auto = s_dvbs_sat_data.cur_tp.tp_data.tp_s.pls_n;
                        _tp_list_hide_tip(TP_INFO_ALL);
                        GUI_CreateDialog(WND_TP_LIST_POPUP);
                    }
                    break;

                case TP_RECORD_S2L3_KEY:
                    {
                        if (s_dvbs_sat_data.cur_sat.sat_data.type != GXBUS_PM_SAT_S)
                            break;
                        if(0 == strcasecmp("list_tp_list", GUI_GetFocusWidget()))
                        {
                            s_CurMode = TP_RECORD_S2L3;
                            GUI_CreateDialog(WND_TP_LIST_POPUP);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
    else
        correct_code_num = 0;

    return 0;
}

SIGNAL_HANDLER int app_tp_list_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    int cur_sel = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            _tp_list_extended_func(event->key.sym);

            switch(event->key.sym)
            {
                case STBK_OK:
                    if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0))
                    {
                        if (s_TlIdOps->id_check(s_TlIdOps, s_dvbs_sat_data.cur_tp_sel) == 0)//ID_UNEXIST
                        {
                            s_TlIdOps->id_add(s_TlIdOps, s_dvbs_sat_data.cur_tp_sel, s_dvbs_sat_data.cur_tp.tp_data.id);
                            s_tplist_tp_choise++;
                        }
                        else
                        {
                            s_TlIdOps->id_delete(s_TlIdOps, s_dvbs_sat_data.cur_tp_sel);
                            s_tplist_tp_choise--;
                        }
                        GUI_SetProperty("list_tp_list", "update_row", &s_dvbs_sat_data.cur_tp_sel);
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_0:
                    if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0))
                    {
                        uint16_t i = 0;
                        GxMsgProperty_NodeByPosGet node = {0};

                        if(s_tplist_tp_choise < s_dvbs_sat_data.cur_sat_tp_total)
                        {
                            for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
                            {
                                node.node_type = NODE_TP;
                                node.pos = i;
                                app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
                                s_TlIdOps->id_add(s_TlIdOps, i, node.tp_data.id);
                            }
                            s_tplist_tp_choise = s_dvbs_sat_data.cur_sat_tp_total;
                        }
                        else
                        {
                            for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
                            {
                                s_TlIdOps->id_delete(s_TlIdOps, i);
                            }
                            s_tplist_tp_choise = 0;
                        }
                        GUI_SetProperty("list_tp_list", "update", NULL);
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_UP:
                    memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);

                    GUI_GetProperty("list_tp_list", "select", &cur_sel);
                    if(cur_sel == 0)
                    {
                        GUI_SetFocusWidget("cmb_tp_list_sat");
                        if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                            _tp_list_hide_tip(TP_INFO_ADD);
                        else
                            _tp_list_hide_tip(TP_INFO_ALL);

                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                case STBK_DOWN:
                    memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);

                    GUI_GetProperty("list_tp_list", "select", &cur_sel);
                    if(cur_sel == (s_dvbs_sat_data.cur_sat_tp_total - 1))
                    {
                        GUI_SetFocusWidget("cmb_tp_list_sat");
                        if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                            _tp_list_hide_tip(TP_INFO_ADD);
                        else
                            _tp_list_hide_tip(TP_INFO_ALL);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                case STBK_PAGE_UP:
                    {
                        uint32_t sel;
                        GUI_GetProperty("list_tp_list", "select", &sel);
                        if (sel == 0)
                        {
                            sel = s_dvbs_sat_data.cur_sat_tp_num_get() - 1;
                            GUI_SetProperty("list_tp_list", "select", &sel);
                            ret =   EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret =  EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;

                case STBK_PAGE_DOWN:
                    {
                        uint32_t sel;
                        GUI_GetProperty("list_tp_list", "select", &sel);
                        if (s_dvbs_sat_data.cur_sat_tp_num_get() == sel+1)
                        {
                            sel = 0;
                            GUI_SetProperty("list_tp_list", "select", &sel);
                            ret =  EVENT_TRANSFER_STOP;
                        }
                        else
                        {
                            ret =  EVENT_TRANSFER_KEEPON;
                        }
                    }
                    break;

                default:
                    break;
            }

        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_tp_list_cmb_sat_change(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel = 0;
    GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

    memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
    if(s_dvbs_sat_data.sat_total == 0)
        return EVENT_TRANSFER_STOP;

    GUI_GetProperty("cmb_tp_list_sat", "select", &box_sel);
    s_tplist_tp_choise = 0;

    //get current sat para
    s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = box_sel;
    s_dvbs_sat_data.cur_sat_get();

    if(cur_sat_bak.sat_data.id == s_dvbs_sat_data.cur_sat.sat_data.id)
        return EVENT_TRANSFER_STOP;

    // get tp number
    s_dvbs_sat_data.cur_tp_sel = 0;
    s_dvbs_sat_data.cur_tp_get();
    s_dvbs_sat_data.tp_rebuild(NULL);
    GUI_SetProperty("list_tp_list", "update_all", NULL);
    GUI_SetProperty("list_tp_list", "select", &s_dvbs_sat_data.cur_tp_sel);

    if(0 == s_dvbs_sat_data.cur_sat_tp_total)
        GUI_SetFocusWidget("cmb_tp_list_sat");

    if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
    {
        if(strcmp(GUI_GetFocusWidget(), "list_tp_list") == 0)
            _tp_list_show_tip();
        else
            _tp_list_hide_tip(TP_INFO_ADD);
    }
    else
        _tp_list_hide_tip(TP_INFO_ALL);

    if(s_TlIdOps != NULL)
    {
        s_TlIdOps->id_close(s_TlIdOps);
        s_TlIdOps->id_open(s_TlIdOps, MANAGE_SAME_LIST, s_dvbs_sat_data.cur_sat_tp_total);
    }

    APP_TIMER_ADD(sp_TpListLockTimer, timer_tp_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_ONCE);

    return EVENT_TRANSFER_STOP;
}

static int32_t _app_tp_list_cmb_sat_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty("cmb_tp_list_sat", "select", &ret_sel);
    poplist_destory_cb_free_item_content();
    return 0;
}

static int _app_tp_list_cmb_sat_poplist_create(int sel)
{
	PopList pop_list;
	int ret =-1;
	char *p_item = NULL ;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.sel = sel;

	ret = s_dvbs_sat_data.sat_poplist_get_name(&p_item);
	if (ret < 0)
		return -1 ;
	pop_list.title = STR_ID_SATELLITE;
	pop_list.item_num = s_dvbs_sat_data.sat_total;
	pop_list.item_content = (char**)p_item;
	pop_list.show_num= true;
	pop_list.mode = POP_MODE_UNBLOCK;
	pop_list.exit_cb = _app_tp_list_cmb_sat_poplist_cb ;
	poplist_create(&pop_list);

	return ret;
}
SIGNAL_HANDLER int app_tp_list_cmb_sat_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    int cmb_sel = 0;
    int cur_sel = 0;

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
                case STBK_OK:
                    GUI_GetProperty("cmb_tp_list_sat", "select", &cmb_sel);
                    _app_tp_list_cmb_sat_poplist_create(cmb_sel);
                    break;
                case STBK_UP:
                    if(s_dvbs_sat_data.cur_sat_tp_total > 0)
                    {
                        cur_sel = s_dvbs_sat_data.cur_sat_tp_total - 1;
                        GUI_SetFocusWidget("list_tp_list");
                        GUI_SetProperty("list_tp_list", "select", &cur_sel);
                        if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                            _tp_list_show_tip();
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;
                case STBK_DOWN:
                    if(s_dvbs_sat_data.cur_sat_tp_total > 0)
                    {
                        cur_sel = 0;
                        GUI_SetFocusWidget("list_tp_list");
                        GUI_SetProperty("list_tp_list", "select", &cur_sel);
                        if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                            _tp_list_show_tip();
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_GREEN: //add tp
                    break;

                case STBK_RED:
                case STBK_YELLOW:
                case STBK_BLUE:
                    ret = EVENT_TRANSFER_STOP;
                    break;
                default:
                    break;
            }

        default:
            break;
    }

    return ret;
}


static bool _check_tp_para_valid(GxBusPmDataSat *p_sat, TpListTpPara *tp_para)
{
    if((p_sat == NULL) || (tp_para == NULL))
        return false;


    if((tp_para->symbol_rate >= 800) && (tp_para->symbol_rate <= 60000))
    {
        uint32_t tp_frequency = 0;
        GxBusPmDataTP temp_tp = {0};

        temp_tp.frequency= tp_para->frequency;
        temp_tp.tp_s.polar = tp_para->polar;
        tp_frequency = app_show_to_tp_freq(p_sat, &temp_tp);

        return app_frontend_dvbs_frequency_check(tp_frequency);
    }

    return false;
}

static status_t _tplist_tp_edit(void)
{
#define TP_LEN  20

    GxMsgProperty_NodeModify NodeModify;
    char *atoi_buf = NULL;
    uint32_t sel=0;
    TpListTpPara InputTpPara={0};
    char buffer[TP_LEN] = {0};
    status_t ret = GXCORE_ERROR;

    //get input value
    //freq
    GUI_GetProperty("text_tplist_popup_freq2", "string", &atoi_buf);
    InputTpPara.frequency = atoi(atoi_buf);
    //sym
    GUI_GetProperty("text_tplist_popup_sym2", "string", &atoi_buf);
    InputTpPara.symbol_rate = atoi(atoi_buf);
    GUI_GetProperty("cmb_tplist_popup_polar", "select", &sel);

    if(0 == sel)
        InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
    else
        InputTpPara.polar = GXBUS_PM_TP_POLAR_V;

    if((false == _check_tp_para_valid(&s_dvbs_sat_data.cur_sat.sat_data, &InputTpPara))
            || ((InputTpPara.symbol_rate == 0) && (InputTpPara.frequency != 0)))
    {//show: invalid TP value

        GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
        GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
        memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);

        //hide the box
        GUI_SetProperty("box_tplist_popup_para", "state", "hide");
        //show info
        GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_INVALID_TP);

        GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        memset(buffer, 0 ,TP_LEN);
        sprintf(buffer, "%05d", s_dvbs_sat_data.cur_tp.tp_data.frequency);
        GUI_SetProperty("edit_tplist_popup_freq", "string", buffer);
        GUI_SetProperty("text_tplist_popup_freq2", "string", buffer);
        //polar
        sel = s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar;
        GUI_SetProperty("cmb_tplist_popup_polar", "select", &sel);
        //sym
        memset(buffer, 0 ,TP_LEN);
        sprintf(buffer, "%05d", s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate);
        GUI_SetProperty("edit_tplist_popup_sym", "string", buffer);
        GUI_SetProperty("text_tplist_popup_sym2", "string", buffer);

        //resume
        GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
        //show the box
        GUI_SetProperty("box_tplist_popup_para", "state", "show");
        GUI_SetFocusWidget("box_tplist_popup_para");

        GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
        GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
        GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
        GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
        GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);
    }
    else//value is ok, check if the tp is exist
    {
        uint16_t tp_id;
        if(0 != GxBus_PmTpExistChek(s_dvbs_sat_data.cur_tp.tp_data.sat_id, InputTpPara.frequency,
                    InputTpPara.symbol_rate, InputTpPara.polar, DVBS_QPSK_12, &tp_id))
        {//tp already exist
            //hide the box
            match_status = 1;
            match_exist_sel = tp_id;

            GUI_SetProperty("box_tplist_popup_para", "state", "hide");
            //show info
            GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_EXIST);

            GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);

            //zl
            GUI_SetProperty("text_tplist_popup_tip1_exist", "state", "show");
            GUI_SetProperty("btn_tplist_tip_cancel_exist", "state", "show");
            GUI_SetProperty("btn_tplist_tip_ok_exist", "state", "show");
            GUI_SetProperty("img_tplist_tip_opt_exist", "state", "show");

            GUI_SetFocusWidget("btn_tplist_tip_ok_exist");
            GUI_SetProperty("img_tplist_tip_opt_exist","img","s_bar_choice_blue4");
            tp_exist_status = 1;
        }
        else//modify
        {
            match_status = 0;

            NodeModify.node_type = NODE_TP;
            NodeModify.tp_data = s_dvbs_sat_data.cur_tp.tp_data;
            NodeModify.tp_data.frequency = InputTpPara.frequency;
            NodeModify.tp_data.tp_s.symbol_rate = InputTpPara.symbol_rate;
            NodeModify.tp_data.tp_s.polar = InputTpPara.polar;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
            {
                s_dvbs_sat_data.change |= (TP_LIST_UPDATE_TPINFO | ANT_SET_UPDATE_TPINFO);
                ret = GXCORE_SUCCESS;
            }
        }
    }

    return ret;
}

static status_t _tplist_check_tp_add(void)
{
    char *atoi_buf = NULL;
    uint32_t sel=0;
    TpListTpPara InputTpPara={0};
    status_t ret = GXCORE_ERROR;

    //get input value
    //freq
    GUI_GetProperty("text_tplist_popup_freq2", "string", &atoi_buf);
    InputTpPara.frequency = atoi(atoi_buf);
    //sym
    GUI_GetProperty("text_tplist_popup_sym2", "string", &atoi_buf);
    InputTpPara.symbol_rate = atoi(atoi_buf);
    GUI_GetProperty("cmb_tplist_popup_polar", "select", &sel);
    if(0 == sel)
        InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
    else
        InputTpPara.polar = GXBUS_PM_TP_POLAR_V;

    if(false == _check_tp_para_valid(&s_dvbs_sat_data.cur_sat.sat_data, &InputTpPara))
    {//show: invalid TP value
        //hide the box
        GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
                GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
                memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);

        GUI_SetProperty("box_tplist_popup_para", "state", "hide");
        //show info
        GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_INVALID_TP);

        GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        GUI_SetProperty("edit_tplist_popup_freq", "string", buffer_fre_bak);
        GUI_SetProperty("text_tplist_popup_freq2", "string", buffer_fre_bak);
        //sym
        GUI_SetProperty("edit_tplist_popup_sym", "string", buffer_sym_bak);
        GUI_SetProperty("text_tplist_popup_sym2", "string", buffer_sym_bak);

        //resume
        GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
        //show the box
        GUI_SetProperty("box_tplist_popup_para", "state", "show");
        GUI_SetFocusWidget("box_tplist_popup_para");

        GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
        GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
        GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
        GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
        GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);
    }

    return ret ;
}


static status_t _tplist_tp_add(void)
{
    GxMsgProperty_NodeAdd NodeAdd;
    char *atoi_buf = NULL;
    uint16_t tp_id;
    uint32_t sel=0;
    TpListTpPara InputTpPara={0};
    status_t ret = GXCORE_ERROR;

    //get input value
    //freq
    GUI_GetProperty("text_tplist_popup_freq2", "string", &atoi_buf);
    InputTpPara.frequency = atoi(atoi_buf);
    //sym
    GUI_GetProperty("text_tplist_popup_sym2", "string", &atoi_buf);
    InputTpPara.symbol_rate = atoi(atoi_buf);
    GUI_GetProperty("cmb_tplist_popup_polar", "select", &sel);
    if(0 == sel)
        InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
    else
        InputTpPara.polar = GXBUS_PM_TP_POLAR_V;

    if(false == _check_tp_para_valid(&s_dvbs_sat_data.cur_sat.sat_data, &InputTpPara))
    {//show: invalid TP value
        //hide the box

        GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
        GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
        memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);

        GUI_SetProperty("box_tplist_popup_para", "state", "hide");
        //show info
        GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_INVALID_TP);


        GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        GUI_SetProperty("edit_tplist_popup_freq", "string", buffer_fre_bak);
        GUI_SetProperty("text_tplist_popup_freq2", "string", buffer_fre_bak);
        //sym
        GUI_SetProperty("edit_tplist_popup_sym", "string", buffer_sym_bak);
        GUI_SetProperty("text_tplist_popup_sym2", "string", buffer_sym_bak);

        //resume
        GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
        //show the box
        GUI_SetProperty("box_tplist_popup_para", "state", "show");
        GUI_SetFocusWidget("box_tplist_popup_para");

        GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
        GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
        GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
        GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
        GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);

    }
    else//value is ok, check if the tp is exist
    {
        if(0 != GxBus_PmTpExistChek(s_dvbs_sat_data.cur_sat.sat_data.id, InputTpPara.frequency,
            InputTpPara.symbol_rate, InputTpPara.polar, DVBS_QPSK_12, &tp_id))
        {//tp already exist
            //hide the box
            //zl
            match_status = 1;
            match_exist_sel = tp_id;

            GUI_SetProperty("box_tplist_popup_para", "state", "hide");
            //show info
            GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_EXIST);

            GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);

            //zl
            GUI_SetProperty("text_tplist_popup_tip1_exist", "state", "show");
            GUI_SetProperty("btn_tplist_tip_cancel_exist", "state", "show");
            GUI_SetProperty("btn_tplist_tip_ok_exist", "state", "show");
            GUI_SetProperty("img_tplist_tip_opt_exist", "state", "show");

            GUI_SetFocusWidget("btn_tplist_tip_ok_exist");
            GUI_SetProperty("img_tplist_tip_opt_exist","img","s_bar_choice_blue4");
            tp_exist_status = 1;

        }
        else//save
        {
            match_status = 0;
            memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
            NodeAdd.node_type = NODE_TP;
            NodeAdd.tp_data.sat_id = s_dvbs_sat_data.cur_sat.sat_data.id;
            NodeAdd.tp_data.frequency = InputTpPara.frequency;
            NodeAdd.tp_data.tp_s.symbol_rate = InputTpPara.symbol_rate;
            NodeAdd.tp_data.tp_s.polar = InputTpPara.polar;

            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
            {
                s_dvbs_sat_data.change |= (TP_LIST_UPDATE_TPINFO | ANT_SET_UPDATE_TPINFO);
                _tp_list_show_tip();

                ret = GXCORE_SUCCESS;
            }
        }
    }

    return ret ;
}

static status_t _tplist_tp_del(void)
{
    GxMsgProperty_NodeDelete NodeDel;
    status_t ret = GXCORE_ERROR;

    NodeDel.node_type = NODE_TP;
    NodeDel.num = s_TlIdOps->id_total_get(s_TlIdOps);
    if(NodeDel.num > 0)
    {
        NodeDel.id_array = GxCore_Calloc(NodeDel.num, sizeof(uint32_t));
        if(NodeDel.id_array != NULL)
        {
            s_TlIdOps->id_get(s_TlIdOps, (uint32_t*)(NodeDel.id_array));
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &NodeDel))
            {
                s_dvbs_sat_data.change |= (TP_LIST_UPDATE_TPINFO | ANT_SET_UPDATE_TPINFO);
                g_AppBook.invalid_remove();
                ret = GXCORE_SUCCESS;
            }
            GxCore_Free(NodeDel.id_array);
        }
    }

    return ret;
}

static status_t app_tp_list_update_paras_pls_n(uint32_t num)
{
    #define MAX_BUF  20
    status_t ret = GXCORE_SUCCESS;
    char buff[MAX_BUF];
    memset(buff, 0 ,MAX_BUF);
    sprintf(buff, "Value: %d",num);
    GUI_SetProperty("text_tplist_popup_tip1", "string", buff);
    GUI_SetProperty("text_tplist_popup_tip1", "draw_now", NULL);

    return ret;
}

static status_t _tp_list_modify_paras_pls_n(uint32_t menu_mode,uint32_t num)
{
    GxMsgProperty_NodeModify NodeModify;
    status_t ret = GXCORE_ERROR;

    NodeModify.node_type = NODE_TP;
    NodeModify.tp_data = s_dvbs_sat_data.cur_tp.tp_data;
    NodeModify.tp_data.tp_s.pls_n = num;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
    {
        s_dvbs_sat_data.cur_tp_get();
        s_dvbs_sat_data.set_tp(SET_TP_FORCE);
        if(menu_mode)
        {
            app_tp_list_update_paras_pls_n(num);
        }
        ret = GXCORE_SUCCESS;
    }

    return ret;
}

static status_t _tplist_auto_calculation_pls_n(void)
{
    status_t ret = GXCORE_ERROR;
    uint32_t pls_num = 0;

    app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_PLS_N_GET, (void*)&pls_num);

    ret = _tp_list_modify_paras_pls_n(1,pls_num);

    return ret;
}

static void _tplist_popup_ok_keypress(void)
{
    status_t ret = GXCORE_ERROR;
    switch(s_CurMode)
    {
        case TP_ADD:
            if(s_dvbs_sat_data.sat_total > 0)
            {
                ret = _tplist_tp_add();
            }
            break;

        case TP_EDIT:
            if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0))
            {
                ret= _tplist_tp_edit();
            }
            break;

        case TP_DELETE:
            if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0)
                && (0 == strcasecmp("btn_tplist_popup_ok", GUI_GetFocusWidget()))
            )
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_DELETING);
                GUI_SetProperty("text_tplist_popup_tip2", "string", NULL);
                GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");
                GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);
                ret = _tplist_tp_del();
#if SAT2IP_SERVER_SUPPORT
				if (ret == GXCORE_SUCCESS)
				{
					app_sat2ip_update_prog_total();
				}
#endif
#if DVB2IP_SERVER_SUPPORT
                if (ret == GXCORE_SUCCESS)
                    app_dvb2ip_prog_num_update();
#endif
            }
            if(0 == strcasecmp("btn_tplist_popup_cancel", GUI_GetFocusWidget()))
            {
                GUI_EndDialog(WND_TP_LIST_POPUP);
            }
            break;

        case TP_SEARCH:
            break;
        case TP_AUTO_PLSN:
            if((s_dvbs_sat_data.sat_total > 0) && (s_dvbs_sat_data.cur_sat_tp_total > 0)
                && (0 == strcasecmp("btn_tplist_popup_ok", GUI_GetFocusWidget()))
            )
            {
                s_plsn_bak_revert_flag = false;
                ret = GXCORE_SUCCESS;
            }
            if(0 == strcasecmp("btn_tplist_popup_cancel", GUI_GetFocusWidget()))
            {
                GUI_EndDialog(WND_TP_LIST_POPUP);
            }
            GxFrontend_PlsNSourceSelect(s_dvbs_sat_data.cur_sat.sat_data.tuner, FE_PLSN_FROM_AUTO_CALC);//start auto PlsN
            break;
    }

    if(ret == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_TP_LIST_POPUP);
        GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);
    }

    return;
}

static uint32_t *s_tv_prog_id = NULL;
static uint32_t *s_radio_prog_id = NULL;
static uint16_t s_tv_prog_count = 0;
static uint16_t s_radio_prog_count = 0;

static void app_wnd_tp_proglist_update(void)
{
#define MAX_PROG_LIST_OF_TP 10
    if(s_tv_prog_count > MAX_PROG_LIST_OF_TP)
        GUI_SetProperty("scrollbar_tp_proglist_tv", "format", "scroll_show");
    else
        GUI_SetProperty("scrollbar_tp_proglist_tv", "format", "scroll_hide");

    if(s_radio_prog_count > MAX_PROG_LIST_OF_TP)
        GUI_SetProperty("scrollbar_tp_proglist_radio", "format", "scroll_show");
    else
        GUI_SetProperty("scrollbar_tp_proglist_radio", "format", "scroll_hide");

    if(s_tv_prog_count == 0 && s_radio_prog_count > 0 )
        GUI_SetFocusWidget("list_tp_proglist_radio");
    else
        GUI_SetFocusWidget("list_tp_proglist_tv");

    GUI_SetProperty("list_tp_proglist_tv", "update_all", NULL);
    GUI_SetProperty("list_tp_proglist_radio", "update_all", NULL);
}

SIGNAL_HANDLER int app_wnd_tp_proglist_create(GuiWidget *widget, void *usrdata)
{
#define TP_LEN  20
    uint32_t sel = 0;
    char buffer[TP_LEN];
    memset(buffer, 0, TP_LEN);

    sel = s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar;
    if (0 == sel)
        sprintf(buffer, "%d / %s / %d", s_dvbs_sat_data.cur_tp.tp_data.frequency, "H", s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate);
    else
        sprintf(buffer, "%d / %s / %d", s_dvbs_sat_data.cur_tp.tp_data.frequency, "V", s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate);
    GUI_SetProperty("text_tp_proglist_tpname", "string", buffer);

    GxBusPmViewInfo view_info_bak;
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_bak)))
    {
        app_log_error("[err]GXMSG_PM_VIEWINFO_GET-----[%s:%d]-----\n",__func__, __LINE__);
        app_wnd_tp_proglist_update();
        return EVENT_TRANSFER_STOP;
    }
    GxBusPmViewInfo view_info_use;
    memcpy(&view_info_use, &view_info_bak, sizeof(GxBusPmViewInfo));
    view_info_use.group_mode = GROUP_MODE_SAT;
    view_info_use.stream_type = GXBUS_PM_PROG_TV;
    view_info_use.sat_id = s_dvbs_sat_data.cur_sat.sat_data.id;
    view_info_use.tp_id = s_dvbs_sat_data.cur_tp.tp_data.id;
    view_info_use.taxis_mode = TAXIS_MODE_TP;
    view_info_use.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    // get total prog
    s_tv_prog_count = GxBus_PmProgNumGetByViewInfo(&view_info_use);
    if(0 != s_tv_prog_count)
    {
        s_tv_prog_id = (uint32_t *)GxCore_Calloc(s_tv_prog_count, sizeof(uint32_t));
        if(NULL == s_tv_prog_id)
        {
            app_log_error("[err]malloc failed!-----[%s:%d]-----\n",__func__, __LINE__);
            goto proglist_create_end1;
        }
        GxBus_PmProgIdArryGetByViewInfo(&view_info_use,s_tv_prog_count,s_tv_prog_id);
    }

    view_info_use.stream_type = GXBUS_PM_PROG_RADIO;
    s_radio_prog_count = GxBus_PmProgNumGetByViewInfo(&view_info_use);
    if(s_radio_prog_count != 0)
    {
        s_radio_prog_id = (uint32_t *)GxCore_Calloc(s_radio_prog_count, sizeof(uint32_t));
        if(NULL == s_radio_prog_id)
        {
            app_log_error("[err]malloc failed!-----[%s:%d]-----\n",__func__, __LINE__);
            goto proglist_create_end1;
        }
        GxBus_PmProgIdArryGetByViewInfo(&view_info_use,s_radio_prog_count,s_radio_prog_id);
    }

proglist_create_end1:
    app_wnd_tp_proglist_update();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wnd_tp_proglist_destroy(GuiWidget *widget, void *usrdata)
{
    s_tv_prog_count = 0;
    s_radio_prog_count = 0;
    APP_FREE(s_tv_prog_id);
    APP_FREE(s_radio_prog_id);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_wnd_tp_detail_info_create(GuiWidget *widget, void *usrdata)
{
#define TP_LEN  20
    int sel = 0;
    char buffer[TP_LEN];

    //lnb
    memset(buffer, 0 ,TP_LEN);
    sprintf(buffer, "%d / %d", s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb1, s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb2);
    GUI_SetProperty("text_tp_detail_info_lnb", "string", buffer);

    //diseqc
    memset(buffer, 0 ,TP_LEN);
    sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc10;
    if (sel == 0)
        sprintf(buffer, "%s", "Off");
    else
        sprintf(buffer, "Port %d", sel);
    GUI_SetProperty("text_tp_detail_info_diseqc", "string", buffer);

    //sat
    memset(buffer, 0 ,TP_LEN);
    sprintf(buffer, "%s", s_dvbs_sat_data.cur_sat.sat_data.sat_s.sat_name);
    GUI_SetProperty("text_tp_detail_info_sat", "string", buffer);

    //frequency
    memset(buffer, 0 ,TP_LEN);
    sprintf(buffer, "%d", s_dvbs_sat_data.cur_tp.tp_data.frequency);
    GUI_SetProperty("text_tp_detail_info_fre", "string", buffer);

    //polar
    memset(buffer, 0, TP_LEN);
    sel = s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar;
    if (0 == sel)
        sprintf(buffer, "%s", "H");
    else
        sprintf(buffer, "%s", "V");
    GUI_SetProperty("text_tp_detail_info_pol", "string", buffer);

    //sym
    memset(buffer, 0 ,TP_LEN);
    sprintf(buffer, "%d", s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate);
    GUI_SetProperty("text_tp_detail_info_symbol", "string", buffer);

    //muti
    GUI_SetProperty("text_tp_detail_info_mts", "string", "None");
#if MULTISTREAM_SUPPORT
    unsigned int mts_num = 0;
    mts_num = app_get_muti_ts_num(s_dvbs_sat_data.cur_sat.sat_data.tuner);
    if(mts_num != 0)
    {
        memset(buffer, 0 ,TP_LEN);
        sprintf(buffer, "%d", mts_num);
        GUI_SetProperty("text_tp_detail_info_mts", "string", buffer);
    }
#endif

    GUI_SetProperty("text_tp_detail_info_netname", "string", "None");

    GUI_SetProperty("text_tp_detail_info_proname", "string", "None");

    return EVENT_TRANSFER_STOP;

}

SIGNAL_HANDLER int app_wnd_tp_proglist_keypress(GuiWidget *widget, void *usrdata)
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
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    GUI_SetProperty(WND_FULL_SCREEN, "draw_now", NULL);
                    break;

                case STBK_MENU:
                case STBK_EXIT:
                    GUI_EndDialog(WND_TP_PROGLIST);
                    if(0 == strcasecmp(WND_TP_LIST, GUI_GetFocusWindow()))
                    {
                        if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                            _tp_list_show_tip();
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_tp_list_prog_tv_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    int sel = 0;
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;
        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
                case STBK_MENU:
                case STBK_EXIT:
                case STBK_DOWN:
                case STBK_UP:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                case STBK_PAGE_UP:
                    GUI_GetProperty("list_tp_proglist_tv", "select", &sel);
                    if (sel == 0)
                    {
                        sel = s_tv_prog_count - 1;
                        GUI_SetProperty("list_tp_proglist_tv", "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_UP;
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;
                case STBK_PAGE_DOWN:
                    GUI_GetProperty("list_tp_proglist_tv", "select", &sel);
                    if (sel+1 == s_tv_prog_count)
                    {
                        sel = 0;
                        GUI_SetProperty("list_tp_proglist_tv", "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_DOWN;
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:
                    if(s_radio_prog_count > 0)
                        GUI_SetFocusWidget("list_tp_proglist_radio");
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_tp_list_prog_radio_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;

    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    int sel = 0;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;
        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
                case STBK_MENU:
                case STBK_EXIT:
                case STBK_DOWN:
                case STBK_UP:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                case STBK_PAGE_UP:
                    GUI_GetProperty("list_tp_proglist_radio", "select", &sel);
                    if (sel == 0)
                    {
                        sel = s_radio_prog_count - 1;
                        GUI_SetProperty("list_tp_proglist_radio", "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_UP;
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;
                case STBK_PAGE_DOWN:
                    GUI_GetProperty("list_tp_proglist_radio", "select", &sel);
                    if (sel+1 == s_radio_prog_count)
                    {
                        sel = 0;
                        GUI_SetProperty("list_tp_proglist_radio", "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        event->key.sym = STBK_PAGE_DOWN;
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:
                    if(s_tv_prog_count > 0)
                       GUI_SetFocusWidget("list_tp_proglist_tv");
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                default:
                    break;
            }
            break;
        default:break;
    }
    return ret;
}

SIGNAL_HANDLER int app_wnd_tp_detail_info_keypress(GuiWidget *widget, void *usrdata)
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
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_MENU:
                case STBK_EXIT:
                    GUI_EndDialog(WND_TP_DETAIL_INFO);
                    if(0 == strcasecmp(WND_TP_LIST, GUI_GetFocusWindow()))
                    {
                        if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                            _tp_list_show_tip();
                    }
                    break;
                default:break;
            }
            break;
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_tplist_popup_create(GuiWidget *widget, void *usrdata)
{
#define POLAR_CONTENT   "[H,V]"
#define TP_LEN  20

    char buffer[TP_LEN];
    char* PolarStr[2] = {"H", "V"};
    uint32_t sel=0;
    match_exist_sel =0;
    match_status = 0;
    char add_fre_buffer[MAX_FRE_LEN +1]="04000";
    char add_sym_buffer[MAX_FRE_LEN +1]="27500";
    int i;
    GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
    GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");

    switch(s_CurMode)
    {
        case TP_ADD:
        {
            //title
            GUI_SetProperty("text_tplist_popup_title", "string", STR_ID_ADD);
            if(s_dvbs_sat_data.sat_total == 0)
            {
                GUI_SetProperty("box_tplist_popup_para", "state", "hide");
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
            {
                for(i=0;i<MAX_FRE_LEN;i++)
                {
                    buffer_fre_bak[i] =add_fre_buffer[i];
                    buffer_sym_bak[i] = add_sym_buffer[i];
                }
                //default value:4000/H/27500
                //freq
                GUI_SetProperty("text_tplist_popup_freq2", "string", "04000");
                GUI_SetProperty("edit_tplist_popup_freq", "string", "04000");
                //polar
                GUI_SetProperty("cmb_tplist_popup_polar","content",POLAR_CONTENT);
                //sym
                GUI_SetProperty("text_tplist_popup_sym2", "string", "27500");
                GUI_SetProperty("edit_tplist_popup_sym", "string", "27500");

                //hide
                GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");

                GUI_SetFocusWidget("box_tplist_popup_para");
            }
        }
        break;

        case TP_EDIT:
        {
            //title
            GUI_SetProperty("text_tplist_popup_title", "string", STR_ID_EDIT);

            if(s_dvbs_sat_data.sat_total == 0)
            {
                GUI_SetProperty("box_tplist_popup_para", "state", "hide");
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
            else if(s_dvbs_sat_data.cur_sat_tp_total == 0)
            {
                GUI_SetProperty("box_tplist_popup_para", "state", "hide");
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_NOT_EXIST);
            }
            else
            {
            //freq
                memset(buffer, 0 ,TP_LEN);
                sprintf(buffer, "%05d", s_dvbs_sat_data.cur_tp.tp_data.frequency);
                GUI_SetProperty("text_tplist_popup_freq2", "string", buffer);
                GUI_SetProperty("edit_tplist_popup_freq", "string", buffer);

            memset(buffer_fre_bak,0,MAX_FRE_LEN +1);
            sprintf(buffer_fre_bak, "%05d", s_dvbs_sat_data.cur_tp.tp_data.frequency);

            //polar
                GUI_SetProperty("cmb_tplist_popup_polar","content",POLAR_CONTENT);
                sel = s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar;
                GUI_SetProperty("cmb_tplist_popup_polar", "select", &sel);

            //sym
                memset(buffer, 0 ,TP_LEN);
                sprintf(buffer, "%05d", s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate);
                GUI_SetProperty("text_tplist_popup_sym2", "string", buffer);
                GUI_SetProperty("edit_tplist_popup_sym", "string", buffer);

                memset(buffer_sym_bak,0,5);
                sprintf(buffer_sym_bak, "%05d", s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate);
            //hide
                GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");

                GUI_SetFocusWidget("box_tplist_popup_para");
            }

        }
        break;

        case TP_DELETE:
        {
            uint32_t opt_num = 0;
            //title
            GUI_SetProperty("text_tplist_popup_title", "string", STR_ID_DELETE);
            //hide the box
            GUI_SetProperty("box_tplist_popup_para", "state", "hide");

            if(s_dvbs_sat_data.sat_total == 0)
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
            else if(s_dvbs_sat_data.cur_sat_tp_total == 0)
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_NOT_EXIST);
            }
            else
            {
                opt_num = s_TlIdOps->id_total_get(s_TlIdOps);
                if(1 == opt_num)//only one
                {
                    uint32_t id[1] = {0};
                    GxMsgProperty_NodeByIdGet tp_node = {0};

                    s_TlIdOps->id_get(s_TlIdOps, id);
                    tp_node.node_type = NODE_TP;
                    tp_node.id = id[0];
                    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_node);

                    GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_DLE_TP);
                //show tp para
                    memset(buffer, 0 ,TP_LEN);
                    sprintf(buffer, "%d / %s / %d",
                    tp_node.tp_data.frequency,
                    PolarStr[tp_node.tp_data.tp_s.polar],
                    tp_node.tp_data.tp_s.symbol_rate);
                    GUI_SetProperty("text_tplist_popup_tip2", "string", buffer);

                    GUI_SetFocusWidget("btn_tplist_popup_ok");
                     GUI_SetProperty("img_tplist_popup_ok_back", "img", "s_bar_choice_blue4");
                }
                else if(1 < opt_num)//del selected tp / tps
                {
                    GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_DLE_TP);
                    GUI_SetFocusWidget("btn_tplist_popup_ok");
                    GUI_SetProperty("img_tplist_popup_ok_back", "img", "s_bar_choice_blue4");
                }
                else
                {
                    GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_NOT_EXIST);
                }
            }
        }
        break;

        case TP_SEARCH:
        {
            //title
            GUI_SetProperty("text_tplist_popup_title", "string", STR_ID_TP_SEARCH);
            //hide the box
            GUI_SetProperty("box_tplist_popup_para", "state", "hide");

            if(s_dvbs_sat_data.sat_total == 0)
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
            else if(s_dvbs_sat_data.cur_sat_tp_total == 0)
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_NOT_EXIST);
            }
            else
                ;
        }
        break;

        case TP_AUTO_PLSN:
        {
            s_plsn_bak_revert_flag = true;
            //title
            GUI_SetProperty("text_tplist_popup_title", "string", "Auto N");
            //hide the box
            GUI_SetProperty("box_tplist_popup_para", "state", "hide");

            if(s_dvbs_sat_data.sat_total == 0)
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
            else if(s_dvbs_sat_data.cur_sat_tp_total == 0)
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_NOT_EXIST);
            }
            else
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string",STR_ID_PLZ_WAIT);// "Please press OK to Calculation."

                //hide
                GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");
                GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);
                _tplist_auto_calculation_pls_n();

                //show
                GUI_SetProperty("img_tplist_popup_ok_back", "state", "show");
                GUI_SetProperty("btn_tplist_popup_ok", "state", "show");
                GUI_SetProperty("btn_tplist_popup_cancel", "state", "show");

                GUI_SetFocusWidget("btn_tplist_popup_ok");
                GUI_SetProperty("img_tplist_popup_ok_back", "img", "s_bar_choice_blue4");
            }
        }
        break;
        case TP_INPUT_N:
        {
            //title
            GUI_SetProperty("text_tplist_popup_title", "string", "Input N");

            if(s_dvbs_sat_data.sat_total == 0)
            {
                GUI_SetProperty("box_tplist_popup_para", "state", "hide");
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
            else if(s_dvbs_sat_data.cur_sat_tp_total == 0)
            {
                GUI_SetProperty("box_tplist_popup_para", "state", "hide");
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_NOT_EXIST);
            }
            else
            {
            //freq
                memset(buffer, 0 ,TP_LEN);
                sprintf(buffer, "%05d", s_dvbs_sat_data.cur_tp.tp_data.frequency);
                GUI_SetProperty("text_tplist_popup_freq2", "string", buffer);
                GUI_SetProperty("edit_tplist_popup_freq", "string", buffer);

                memset(buffer_fre_bak,0,MAX_FRE_LEN +1);
                sprintf(buffer_fre_bak, "%05d", s_dvbs_sat_data.cur_tp.tp_data.frequency);

            //polar
                GUI_SetProperty("cmb_tplist_popup_polar","content",POLAR_CONTENT);
                sel = s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar;
                GUI_SetProperty("cmb_tplist_popup_polar", "select", &sel);

            //sym
                memset(buffer, 0 ,TP_LEN);
                sprintf(buffer, "%05d", s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate);
                GUI_SetProperty("text_tplist_popup_sym2", "string", buffer);
                GUI_SetProperty("edit_tplist_popup_sym", "string", buffer);

                memset(buffer_sym_bak,0,5);
                sprintf(buffer_sym_bak, "%05d", s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate);
            //hide
                GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");

                GUI_SetFocusWidget("box_tplist_popup_para");
            }

        }
        break;
        case TP_RECORD_S2L3:
        {
            GUI_SetProperty("text_tplist_popup_title", "string", "TP Record S2L3");
            GUI_SetProperty("box_tplist_popup_para", "state", "hide");

            if(s_dvbs_sat_data.sat_total == 0)
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
            else if(s_dvbs_sat_data.cur_sat_tp_total == 0)
            {
                GUI_SetProperty("text_tplist_popup_tip1", "string", STR_ID_TP_NOT_EXIST);
            }
            else
            {
                GUI_SetProperty("img_tplist_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_ok", "state", "hide");
                GUI_SetProperty("btn_tplist_popup_cancel", "state", "hide");

                GUI_SetProperty("box_tp_record_s2l3", "state", "show");
                GUI_SetFocusWidget("box_tp_record_s2l3");
            }
        }
        break;
    }


    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tplist_popup_destroy(GuiWidget *widget, void *usrdata)
{
    if(s_CurMode == TP_AUTO_PLSN && s_plsn_bak_revert_flag == true)
    {
        s_dvbs_sat_data.cur_tp.tp_data.tp_s.pls_n = s_plsn_bak_before_auto;
        GxMsgProperty_NodeModify NodeModify;
        NodeModify.node_type = NODE_TP;
        NodeModify.tp_data = s_dvbs_sat_data.cur_tp.tp_data;
        app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify);
    }
    s_dvbs_sat_data.cur_sat_tp_total = s_dvbs_sat_data.cur_sat_tp_num_get();
    if(s_dvbs_sat_data.cur_sat_tp_total > 0)
    {
        if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
            _tp_list_show_tip();
        GUI_SetFocusWidget("list_tp_list");
    }
    else
    {
        GUI_SetFocusWidget("cmb_tp_list_sat");
        _tp_list_hide_tip(TP_INFO_ADD);
    }
    return EVENT_TRANSFER_STOP;
}
static void check_input_valid(void)
{
    int sel =0;
    char *buffer_fre = NULL;
    char *buffer_sym = NULL;
    TpListTpPara InputTpPara={0};

    GUI_GetProperty("text_tplist_popup_freq2", "string", &buffer_fre);
    GUI_GetProperty("text_tplist_popup_sym2", "string", &buffer_sym);
    //check input valid?
    InputTpPara.frequency = atoi(buffer_fre);
    //sym
    InputTpPara.symbol_rate = atoi(buffer_sym);
    //polar
    GUI_GetProperty("cmb_tplist_popup_polar", "select", &sel);
    if(0 == sel)
        InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
    else
        InputTpPara.polar = GXBUS_PM_TP_POLAR_V;

    if(false == _check_tp_para_valid(&s_dvbs_sat_data.cur_sat.sat_data, &InputTpPara))
    {//show: invalid TP value
        GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
        GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
        memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);
        buffer_fre = buffer_fre_bak;
        buffer_sym = buffer_sym_bak;
    }
    else
    {
        int i;
        memset(buffer_fre_bak,'0',MAX_FRE_LEN);
        memset(buffer_sym_bak,'0',MAX_FRE_LEN);
        for(i=MAX_FRE_LEN-strlen(buffer_fre);i<MAX_FRE_LEN;i++)
        {
            buffer_fre_bak[i]=*buffer_fre;
            buffer_fre++;
        }
        for(i=MAX_FRE_LEN-strlen(buffer_sym);i<MAX_FRE_LEN;i++)
        {
            buffer_sym_bak[i]=*buffer_sym;
            buffer_sym++;
        }

        GUI_SetProperty("edit_tplist_popup_freq", "string",buffer_fre_bak);
        GUI_SetProperty("edit_tplist_popup_sym", "string",buffer_sym_bak);

    }

  GUI_SetProperty("text_tplist_popup_freq2", "string",buffer_fre_bak);
  GUI_SetProperty("text_tplist_popup_sym2", "string",buffer_sym_bak);

}
static void edit_change_show(void)
{
    int box_sel = 0;
    GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
    if((TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)&&(edit_command_mode == 0))
    {
        GUI_SetProperty("edit_tplist_popup_freq", "state", "show");
        GUI_SetProperty("text_tplist_popup_freq2", "state", "hide");

    }
    else if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
    {
        GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
        GUI_SetProperty("text_tplist_popup_freq2", "state", "show");

    }
    else if((TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)&&(edit_command_mode == 0))
    {
        GUI_SetProperty("edit_tplist_popup_sym", "state", "show");
        GUI_SetProperty("text_tplist_popup_sym2", "state", "hide");

    }
    else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
    {
        GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
        GUI_SetProperty("text_tplist_popup_sym2", "state", "show");

    }
    else
    {
        GUI_SetProperty("edit_tplist_popup_freq", "state", "hide");
                GUI_SetProperty("text_tplist_popup_freq2", "state", "show");
        GUI_SetProperty("edit_tplist_popup_sym", "state", "hide");
                GUI_SetProperty("text_tplist_popup_sym2", "state", "show");
    }
}

static uint32_t _pls_n_root_convert(fe_plsn_convert_t convert, uint32_t value)
{
	AppFrontend_PlsNRootConvert param = {0};

	param.convert = convert;
	if(convert == FE_PLS_N2ROOT)
		param.pls_n = value;
	else if(convert == FE_PLS_ROOT2N)
		param.root = value;
	else
		return 0;

	if(app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_PLSN_ROOT_CONVERT, (void*)&param) == GXCORE_ERROR)
		return 0;

	if(convert == FE_PLS_N2ROOT)
		return param.root;
	else if(convert == FE_PLS_ROOT2N)
		return param.pls_n;
	else
		return 0;
}

static uint32_t pls_edit_change_root_or_n_show(uint32_t box_sel)
{
    #define PLS_LEN 20
    char buffer[PLS_LEN];

    uint32_t pls_n = 0;
    uint32_t root = 0;
    char *edit_string = NULL;

    if(TP_PLS_N == box_sel)
    {
        GUI_GetProperty("text_tplist_pls_n_bk", "string", &edit_string);//edit_plsn_root_value_n
        pls_n = atoi(edit_string);
        memset(buffer, 0 ,PLS_LEN);
        root = _pls_n_root_convert(FE_PLS_N2ROOT, pls_n);
        sprintf(buffer, "%d", root);
        GUI_SetProperty("edit_plsn_root_value", "string", buffer);
        GUI_SetProperty("text_tplist_pls_root_bk", "string", buffer);
    }
    else if(TP_PLS_ROOT == box_sel)
    {
        GUI_GetProperty("text_tplist_pls_root_bk", "string", &edit_string);//edit_plsn_root_value
        root = atoi(edit_string);
        if (root == 0)
            return 0;

        memset(buffer, 0 ,PLS_LEN);
        pls_n = _pls_n_root_convert(FE_PLS_ROOT2N, root);
        sprintf(buffer, "%d", pls_n);
        GUI_SetProperty("edit_plsn_root_value_n", "string", buffer);
        GUI_SetProperty("text_tplist_pls_n_bk", "string", buffer);
    }

    return pls_n;
}

static void pls_n_edit_change_show(void)
{
    int box_sel = 0;
    GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
    if((TP_PLS_N == box_sel)&&(edit_command_mode == 0))
    {
        GUI_SetProperty("edit_plsn_root_value_n", "state", "show");
        GUI_SetProperty("text_tplist_pls_n_bk", "state", "hide");

    }
    else if(TP_PLS_N == box_sel)
    {
        GUI_SetProperty("edit_plsn_root_value_n", "state", "hide");
        GUI_SetProperty("text_tplist_pls_n_bk", "state", "show");

    }
    else if((TP_PLS_ROOT== box_sel)&&(edit_command_mode == 0))
    {
        GUI_SetProperty("edit_plsn_root_value", "state", "show");
        GUI_SetProperty("text_tplist_pls_root_bk", "state", "hide");

    }
    else if(TP_PLS_ROOT == box_sel)
    {
        GUI_SetProperty("edit_plsn_root_value", "state", "hide");
        GUI_SetProperty("text_tplist_pls_root_bk", "state", "show");
    }
    else
    {
        GUI_SetProperty("edit_plsn_root_value_n", "state", "hide");
        GUI_SetProperty("text_tplist_pls_n_bk", "state", "show");
        GUI_SetProperty("edit_plsn_root_value", "state", "hide");
        GUI_SetProperty("text_tplist_pls_root_bk", "state", "show");
    }
}

SIGNAL_HANDLER int  app_edit_tplist_popup_freq_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_TPLIST_POPUP_FREQ_LEN 5
    GUI_GetProperty("edit_tplist_popup_freq", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_TPLIST_POPUP_FREQ_LEN - 1 : 0);
    GUI_SetProperty("edit_tplist_popup_freq", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int  app_edit_tplist_popup_sym_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_TPLIST_POPUP_SYM_LEN 6
    GUI_GetProperty("edit_tplist_popup_sym", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_TPLIST_POPUP_SYM_LEN - 1 : 0);
    GUI_SetProperty("edit_tplist_popup_sym", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int  app_edit_tplist_popup_s2l3_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_TPLIST_POPUP_S2L3_LEN 1
    GUI_GetProperty("edit_tplist_popup_rec_s2l3", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_TPLIST_POPUP_S2L3_LEN - 1 : 0);
    GUI_SetProperty("edit_tplist_popup_rec_s2l3", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tplist_popup_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;
    uint8_t FocusdOnOk=0;

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
                case VK_BOOK_TRIGGER:
                    tp_exist_status = 0;
                    GUI_EndDialog("after wnd_full_screen");
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    {
                        if(tp_exist_status == 1)
                        {
                            GUI_SetProperty("text_tplist_popup_tip1_exist", "state", "hide");
                            GUI_SetProperty("btn_tplist_tip_cancel_exist", "state", "hide");
                            GUI_SetProperty("btn_tplist_tip_ok_exist", "state", "hide");
                            GUI_SetProperty("img_tplist_tip_opt_exist", "state", "hide");

                            //resume
                            GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
                            //show the box
                            GUI_SetProperty("box_tplist_popup_para", "state", "show");
                            GUI_SetFocusWidget("box_tplist_popup_para");
                            GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);

                            tp_exist_status = 0;
                        }
                        else
                        {
                            tp_exist_status = 0;
                            GUI_EndDialog(WND_TP_LIST_POPUP);
                        }
                    }
                    break;

                case STBK_OK:
                    // when tp exist,choose ok or cancel
                    if(tp_exist_status == 1)
                    {
                        if(0 == strcasecmp("btn_tplist_tip_ok_exist", GUI_GetFocusWidget()))
                        {
                            tp_exist_status = 0;
                            GUI_EndDialog(WND_TP_LIST_POPUP);
                            int i;
                            GxMsgProperty_NodeByPosGet node;
                            node.node_type = NODE_TP;
                            for(i = 0; i < s_dvbs_sat_data.cur_sat_tp_total; i++)
                            {
                                node.pos = i;
                                app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
                                if(match_exist_sel == node.tp_data.id)
                                {
                                    GUI_SetProperty("list_tp_list", "select", &i);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            GUI_SetProperty("text_tplist_popup_tip1_exist", "state", "hide");
                            GUI_SetProperty("btn_tplist_tip_cancel_exist", "state", "hide");
                            GUI_SetProperty("btn_tplist_tip_ok_exist", "state", "hide");
                            GUI_SetProperty("img_tplist_tip_opt_exist", "state", "hide");

                            //resume
                            GUI_SetProperty("text_tplist_popup_tip1", "string", " ");
                            //show the box
                            GUI_SetProperty("box_tplist_popup_para", "state", "show");
                            GUI_SetFocusWidget("box_tplist_popup_para");
                            GUI_SetProperty(WND_TP_LIST_POPUP, "draw_now", NULL);

                        }
                        tp_exist_status = 0;
                    }
                    else
                    {
                        if((s_CurMode==TP_ADD) && (s_CurMode==TP_EDIT))
                        {
                            edit_change_show();
                            check_input_valid();
                        }

                        _tplist_popup_ok_keypress();
                    }
                    break;

                case STBK_UP:
                    {
                        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
                        if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            box_sel = TP_LIST_POPUP_BOX_ITEM_OK;
                            GUI_SetProperty("box_tplist_popup_para", "select", &box_sel);
                        }
                    }
                    break;

                case STBK_DOWN:
                    {
                        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
                        if(TP_LIST_POPUP_BOX_ITEM_OK == box_sel)
                        {
                            box_sel = TP_LIST_POPUP_BOX_ITEM_FREQ;
                            GUI_SetProperty("box_tplist_popup_para", "select", &box_sel);
                        }
                    }
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:
                    // when tp exist,choose ok or cancel
                    if(tp_exist_status == 1)
                    {
                        if(0 == strcasecmp("btn_tplist_tip_ok_exist", GUI_GetFocusWidget()))
                        {
                            GUI_SetProperty("img_tplist_tip_opt_exist","img","s_bar_choice_blue4_l");
                            GUI_SetFocusWidget("btn_tplist_tip_cancel_exist");
                        }
                        else
                        {
                            GUI_SetProperty("img_tplist_tip_opt_exist","img","s_bar_choice_blue4");
                            GUI_SetFocusWidget("btn_tplist_tip_ok_exist");
                        }
                    }
                    if((s_CurMode == TP_DELETE)||(s_CurMode == TP_AUTO_PLSN))
                    {
                        if(0 == strcasecmp("btn_tplist_popup_ok", GUI_GetFocusWidget()))
                        {
                            FocusdOnOk = 1;
                        }
                        else if(0 == strcasecmp("btn_tplist_popup_cancel", GUI_GetFocusWidget()))
                        {
                            FocusdOnOk = 0;
                        }

                        if(1 == FocusdOnOk)
                        {
                            GUI_SetProperty("img_tplist_popup_ok_back","img","s_bar_choice_blue4_l");
                            GUI_SetFocusWidget("btn_tplist_popup_cancel");
                        }
                        else
                        {
                            GUI_SetProperty("img_tplist_popup_ok_back","img","s_bar_choice_blue4");
                            GUI_SetFocusWidget("btn_tplist_popup_ok");
                        }
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_tplist_popup_box_s2l3_rec_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    char *s2l3_mode_string = NULL;
    unsigned int s2l3_mode = 0;

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
                case STBK_OK:
                {
                    GUI_GetProperty("edit_tplist_popup_rec_s2l3", "string", &s2l3_mode_string);
                    s2l3_mode = atoi(s2l3_mode_string);
                    if((s2l3_mode != 0) &&
                        (s2l3_mode != 1) &&
                        (s2l3_mode != 2) &&
                        (s2l3_mode != 3) &&
                        (s2l3_mode != 7))
                    {
                        PopDlg pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.format = POP_FORMAT_DLG;
                        pop.str = "Invalid Mode";
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.create_cb = NULL;
                        pop.exit_cb = NULL;
                        pop.timeout_sec= 2;
                        popdlg_create(&pop);
                        break;
                    }
                    GUI_EndDialog(WND_TP_LIST_POPUP);
                    _tp_list_tp_record_s2l3(s2l3_mode);
                }
                    break;

                case STBK_MENU:
                case STBK_EXIT:
                    GUI_EndDialog(WND_TP_LIST_POPUP);
                    break;
                case STBK_UP:
                case STBK_DOWN:
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



SIGNAL_HANDLER int app_tplist_popup_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;
#define MAX_LEN_ 20
    char num[2] = "";
    char *edit_string = NULL;
    uint32_t edit_num =0;
    int edit_sel = 0;
    char edit_buffer[MAX_LEN_]={0};
    bool keypress_status = 0;

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
                        memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);

                        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
                        if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            _tplist_check_tp_add();
                            box_sel = TP_LIST_POPUP_BOX_ITEM_OK;
                            GUI_SetProperty("box_tplist_popup_para", "select", &box_sel);
                        }
                        else
                        {
                            if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
                            {
                                _tplist_check_tp_add();
                            }
                            box_sel = box_sel-1;
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                        up_down_status = 1;
                        edit_command_mode = 1;
                        break;
                    }

                case STBK_DOWN:
                    {
                        memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);

                        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
                        if(TP_LIST_POPUP_BOX_ITEM_OK == box_sel)
                        {
                            box_sel = TP_LIST_POPUP_BOX_ITEM_FREQ;
                            GUI_SetProperty("box_tplist_popup_para", "select", &box_sel);
                        }
                        else
                        {
                            if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel || TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                            {
                                _tplist_check_tp_add();
                            }

                            box_sel = box_sel+1;
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                        up_down_status = 1;
                        edit_command_mode = 1;
                    }
                    break;

                case STBK_OK:
                    edit_command_mode = 1;
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_MENU:
                case STBK_EXIT:
                    edit_command_mode = 1;
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_LEFT:
                    // left = delete
                    if(edit_command_mode == 0)
                    {
                        size_t tplist_edit_input_len = strlen(s_tplist_edit_input);
                        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);

                        if(tplist_edit_input_len == 1 || tplist_edit_input_len == 0)
                        {
                            edit_sel = 0;
                            s_tplist_edit_input[0] = '0';
                            s_tplist_edit_input[1] = '\0';
                            edit_del_to_zero = 1;
                            if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                            {
                                GUI_SetProperty("edit_tplist_popup_freq", "string", s_tplist_edit_input);
                                GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);

                                GUI_SetProperty("text_tplist_popup_freq2", "string", s_tplist_edit_input);
                            }
                            else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
                            {
                                GUI_SetProperty("edit_tplist_popup_sym", "string", s_tplist_edit_input);
                                GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);

                                GUI_SetProperty("text_tplist_popup_sym2", "string", s_tplist_edit_input);
                            }
                        }
                        else
                        {
                            if((tplist_edit_input_len-1) < 0)
                            {
                                ret = EVENT_TRANSFER_KEEPON;
                                break;
                            }
                            s_tplist_edit_input[tplist_edit_input_len-1] = '\0';

                            edit_sel = tplist_edit_input_len;

                            if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                            {
                                GUI_SetProperty("edit_tplist_popup_freq", "string", s_tplist_edit_input);
                                GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);

                                GUI_SetProperty("text_tplist_popup_freq2", "string", s_tplist_edit_input);
                            }
                            else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
                            {
                                GUI_SetProperty("edit_tplist_popup_sym", "string", s_tplist_edit_input);
                                GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);

                                GUI_SetProperty("text_tplist_popup_sym2", "string", s_tplist_edit_input);
                            }
                        }
                    }
                    // left = -
                    if(edit_command_mode == 1)
                    {
                        memset(edit_buffer,0,MAX_LEN_);
                        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
                        if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            GUI_GetProperty("text_tplist_popup_freq2", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num > 0)
                                edit_num = edit_num-1;
                            sprintf(edit_buffer,"%05d",edit_num);
                            GUI_SetProperty("text_tplist_popup_freq2", "string", edit_buffer);
                            GUI_SetProperty("edit_tplist_popup_freq", "string", edit_buffer);
                        }
                        else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
                        {
                            GUI_GetProperty("text_tplist_popup_sym2", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num > 0)
                                edit_num = edit_num-1;
                            sprintf(edit_buffer,"%05d",edit_num);
                            GUI_SetProperty("text_tplist_popup_sym2", "string", edit_buffer);
                            GUI_SetProperty("edit_tplist_popup_sym", "string", edit_buffer);
                        }
                    }


                    ret = EVENT_TRANSFER_KEEPON;
                    break;


                case STBK_RIGHT:
                    //right = +
                    if(edit_command_mode == 1)
                    {
                        memset(edit_buffer,0,MAX_LEN_);
                        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
                        if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            GUI_GetProperty("text_tplist_popup_freq2", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num<99999)
                            {
                                edit_num = edit_num+1;
                            }
                            sprintf(edit_buffer,"%05d",edit_num);
                            GUI_SetProperty("text_tplist_popup_freq2", "string", edit_buffer);
                            GUI_SetProperty("edit_tplist_popup_freq", "string", edit_buffer);
                        }
                        else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
                        {
                            GUI_GetProperty("text_tplist_popup_sym2", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num<99999)
                            {
                                edit_num = edit_num+1;
                            }
                            sprintf(edit_buffer,"%05d",edit_num);
                            GUI_SetProperty("text_tplist_popup_sym2", "string", edit_buffer);
                            GUI_SetProperty("edit_tplist_popup_sym", "string", edit_buffer);
                        }
                    }
                    else
                    {
                        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);
                        if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            edit_sel = strlen(s_tplist_edit_input);
                            GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);
                        }
                        else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
                        {
                            edit_sel = strlen(s_tplist_edit_input);
                            GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);
                        }
                    }

                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_0:
                    keypress_status = 1;
                    memset(num,'0',1);
                    break;
                case STBK_1:
                    keypress_status = 1;
                    memset(num,'1',1);
                    break;
                case STBK_2:
                    keypress_status = 1;
                    memset(num,'2',1);
                    break;
                case STBK_3:
                    keypress_status = 1;
                    memset(num,'3',1);
                    break;
                case STBK_4:
                    keypress_status = 1;
                    memset(num,'4',1);
                    break;
                case STBK_5:
                    keypress_status = 1;
                    memset(num,'5',1);
                    break;
                case STBK_6:
                    keypress_status = 1;
                    memset(num,'6',1);
                    break;
                case STBK_7:
                    keypress_status = 1;
                    memset(num,'7',1);
                    break;
                case STBK_8:
                    keypress_status = 1;
                    memset(num,'8',1);
                    break;
                case STBK_9:
                    keypress_status = 1;
                    memset(num,'9',1);
                    break;

                default:
                    break;
            }
        default:
            break;
    }
    //zl
    if(keypress_status == 1)
    {
        edit_command_mode = 0;
        keypress_status = 0;

        GUI_GetProperty("box_tplist_popup_para", "select", &box_sel);


        if(strlen(s_tplist_edit_input) < 5)
        {
            if(edit_del_to_zero == 1)
            {
                edit_del_to_zero = 0;
                memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);
            }

            if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
            {
                GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_tplist_popup_freq", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_popup_freq2", "string", s_tplist_edit_input);

                edit_sel = strlen(s_tplist_edit_input);
                GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);
            }
            else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
            {
                GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_tplist_popup_sym", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_popup_sym2", "string", s_tplist_edit_input);

                edit_sel = strlen(s_tplist_edit_input);
                GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);
            }
        }
        else if(strlen(s_tplist_edit_input) == 5)
        {
            edit_sel = 1;
            memset(s_tplist_edit_input,'\0',6);
            if(TP_LIST_POPUP_BOX_ITEM_FREQ == box_sel)
            {
                GUI_SetProperty("edit_tplist_popup_freq", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_tplist_popup_freq", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_popup_freq2", "string", s_tplist_edit_input);
                GUI_SetProperty("edit_tplist_popup_freq", "select", &edit_sel);
            }
            else if(TP_LIST_POPUP_BOX_ITEM_SYM == box_sel)
            {
                GUI_SetProperty("edit_tplist_popup_sym", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_tplist_popup_sym", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_popup_sym2", "string", s_tplist_edit_input);
                GUI_SetProperty("edit_tplist_popup_sym", "select", &edit_sel);
            }

        }

        if(strlen(s_tplist_edit_input) == 5)
            edit_command_mode = 1;
        edit_change_show();

    }
    else if(up_down_status == 1)
    {
        if(strlen(s_tplist_edit_input) == 5)
            edit_command_mode = 1;

        edit_change_show();
        check_input_valid();
        up_down_status = 0;
    }

    return ret;
}


SIGNAL_HANDLER int app_tp_list_prog_tv_get_total(GuiWidget *widget, void *usrdata)
{
    return s_tv_prog_count;
}


SIGNAL_HANDLER int app_tp_list_prog_tv_get_data(GuiWidget *widget, void *usrdata)
{
#define CHANNEL_NUM_LEN     (10)
    ListItemPara* item = NULL;
    static char buffer[CHANNEL_NUM_LEN];

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;
    if(NULL == s_tv_prog_id)
        return GXCORE_ERROR;

    static GxMsgProperty_NodeByIdGet node;
    node.node_type = NODE_PROG;
    node.id = s_tv_prog_id[item->sel];
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    //col-0: num
    item->x_offset = 2;
    item->image = NULL;
    memset(buffer, 0, CHANNEL_NUM_LEN);

    sprintf(buffer, "%d", (item->sel+1));

    item->string = buffer;

    //col-1: channel name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;

    item->string = (char*)node.prog_data.prog_name;

    //col-2: $
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        item->image = "s_icon_money";
    }
    else
    {
        item->image = NULL;
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_prog_radio_get_total(GuiWidget *widget, void *usrdata)
{
    return s_radio_prog_count;
}

SIGNAL_HANDLER int app_tp_list_prog_radio_get_data(GuiWidget *widget, void *usrdata)
{
#define CHANNEL_NUM_LEN     (10)
    ListItemPara* item = NULL;
    static char buffer[CHANNEL_NUM_LEN];


    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;
	if(NULL == s_radio_prog_id)
		return GXCORE_ERROR;

    static GxMsgProperty_NodeByIdGet node;
    node.node_type = NODE_PROG;
    node.id = s_radio_prog_id[item->sel];
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    //col-0: num
    item->x_offset = 2;
    item->image = NULL;
    memset(buffer, 0, CHANNEL_NUM_LEN);

    sprintf(buffer, "%d", (item->sel+1));

    item->string = buffer;

    //col-1: channel name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;

    item->string = (char*)node.prog_data.prog_name;

    //col-2: $
    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        item->image = "s_icon_money";
    }
    else
    {
        item->image = NULL;
    }

    return GXCORE_SUCCESS;
}

void app_tp_list_pls_n_edit_change(void)
{
    pls_edit_change_root_or_n_show(TP_PLS_N);
}

void app_tp_list_pls_root_edit_change(void)
{
    pls_edit_change_root_or_n_show(TP_PLS_ROOT);
}

SIGNAL_HANDLER int app_tp_list_plsn_root_create(GuiWidget *widget, void *usrdata)
{
#define PLS_LEN 20

    char buffer[PLS_LEN];
    uint32_t root = 0;

    GUI_SetProperty("edit_plsn_root_value_n", "state", "hide");
    GUI_SetProperty("edit_plsn_root_value", "state", "hide");
   //show pls para
    memset(buffer, 0 ,PLS_LEN);
    sprintf(buffer, "%d",s_dvbs_sat_data.cur_tp.tp_data.tp_s.pls_n);
    GUI_SetProperty("text_tplist_pls_n_bk", "string", buffer);

    memset(buffer, 0 ,PLS_LEN);
    root = _pls_n_root_convert(FE_PLS_N2ROOT, s_dvbs_sat_data.cur_tp.tp_data.tp_s.pls_n);
    sprintf(buffer, "%d",root);
    GUI_SetProperty("text_tplist_pls_root_bk", "string", buffer);
    GUI_SetFocusWidget("box_plsn_root_para");

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_plsn_root_value_n_keypress(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;

    GUI_SetProperty("edit_plsn_root_value_n", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_plsn_root_value_keypress(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;

    GUI_SetProperty("edit_plsn_root_value", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_list_plsn_root_keypress(GuiWidget *widget, void *usrdata)
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
                case VK_BOOK_TRIGGER:
                    tp_exist_status = 0;
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        if(tp_exist_status == 1)
                        {
                            //show the box
                            GUI_SetProperty("box_plsn_root_para", "state", "show");
                            GUI_SetFocusWidget("box_plsn_root_para");
                            GUI_SetProperty(WND_PLSN_ROOT, "draw_now", NULL);
                            tp_exist_status = 0;
                        }
                        else
                        {
                            tp_exist_status = 0;
                            GUI_EndDialog(WND_PLSN_ROOT);
                            if(0 == strcasecmp(WND_TP_LIST, GUI_GetFocusWindow()))
                            {
                                if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                                    _tp_list_show_tip();
                            }
                        }
                    }
                    break;

                case STBK_OK:
                    if(tp_exist_status == 1)
                    {
                        if(0 == strcasecmp("btn_tplist_tip_ok_exist", GUI_GetFocusWidget()))
                        {
                            tp_exist_status = 0;
                            GUI_EndDialog(WND_PLSN_ROOT);
                            if(0 == strcasecmp(WND_TP_LIST, GUI_GetFocusWindow()))
                            {
                                if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                                    _tp_list_show_tip();
                            }
                        }
                        else
                        {
                            //show the box
                            GUI_SetProperty("box_plsn_root_para", "state", "show");
                            GUI_SetFocusWidget("box_plsn_root_para");
                            GUI_SetProperty(WND_PLSN_ROOT, "draw_now", NULL);
                        }
                        tp_exist_status = 0;
                    }
                    else
                    {
                        pls_n_edit_change_show();
                    }
                    break;

                case STBK_UP:
                case STBK_DOWN:
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:

                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_box_plsn_root_keypress(GuiWidget *widget, void *usrdata)
{
    #define MAX_LEN_ 20
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel = 0;
    uint32_t pls_n = 0;
    int edit_sel = 0;
    char edit_buffer[MAX_LEN_]={0};
    char num[2] = "";
    char *edit_string = NULL;
    uint32_t  edit_num =0;
    bool keypress_status = 0;
    bool up_down_flag = 0;

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
                    memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);
                    GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
                    if (TP_PLS_N == box_sel)
                    {
                        box_sel = TP_PLS_SAVE;
                        GUI_SetProperty("box_plsn_root_para", "select", &box_sel);
                    }
                    else
                    {
                        if(TP_PLS_ROOT== box_sel)
                        {
                            edit_sel = 0;
                            GUI_SetProperty("edit_plsn_root_value_n", "select", &edit_sel);
                        }
                        box_sel--;
                        GUI_SetProperty("box_plsn_root_para", "select", &box_sel);
                    }
                    up_down_status = 1;
                    up_down_flag = 1;
                    edit_command_mode = 1;
                    pls_n_edit_change_show();
                    break;
                case STBK_DOWN:
                    memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);
                    GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
                    if (TP_PLS_SAVE == box_sel)
                    {
                        box_sel = TP_PLS_N;
                        GUI_SetProperty("box_plsn_root_para", "select", &box_sel);
                    }
                    else
                    {
                        if(TP_PLS_N == box_sel)
                        {
                            edit_sel = 0;
                            GUI_SetProperty("edit_plsn_root_value_n", "select", &edit_sel);
                        }

                        box_sel++;
                        GUI_SetProperty("box_plsn_root_para", "select", &box_sel);
                    }
                    up_down_status = 1;
                    up_down_flag = 1;
                    edit_command_mode = 1;
                    pls_n_edit_change_show();
                    break;
                case STBK_OK:
                    edit_command_mode = 1;
                    pls_n_edit_change_show();
                    GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
                    if(TP_PLS_SAVE == box_sel)
                    {
                        GxFrontend_PlsNSourceSelect(s_dvbs_sat_data.cur_sat.sat_data.tuner, FE_PLSN_FROM_MANUAL_SET);//stop auto PlsN
                        box_sel = TP_PLS_N;
                        pls_n = pls_edit_change_root_or_n_show(box_sel);
                        _tp_list_modify_paras_pls_n(0,pls_n);
                        GUI_EndDialog(WND_PLSN_ROOT);
                        if(0 == strcasecmp(WND_TP_LIST, GUI_GetFocusWindow()))
                        {
                             if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                                _tp_list_show_tip();
                        }
                    }
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                case STBK_MENU:
                case STBK_EXIT:
                    ret = EVENT_TRANSFER_KEEPON;
                    GUI_EndDialog(WND_PLSN_ROOT);
                    if(0 == strcasecmp(WND_TP_LIST, GUI_GetFocusWindow()))
                    {
                        if(s_dvbs_sat_data.cur_sat.sat_data.type == GXBUS_PM_SAT_S)
                            _tp_list_show_tip();
                    }
                    break;
                case STBK_LEFT:
                    if(edit_command_mode == 0)
                    {
                        size_t tplist_edit_input_len = strlen(s_tplist_edit_input);
                        GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
                        if(tplist_edit_input_len == 1 || tplist_edit_input_len == 0)
                        {
                            edit_sel = 0;
                            s_tplist_edit_input[0] = '0';
                            s_tplist_edit_input[1] = '\0';
                            edit_del_to_zero = 1;
                            if(TP_PLS_N == box_sel)
                            {
                                GUI_SetProperty("edit_plsn_root_value_n", "string", s_tplist_edit_input);
                                GUI_SetProperty("edit_plsn_root_value_n", "select", &edit_sel);
                                GUI_SetProperty("text_tplist_pls_n_bk", "string", s_tplist_edit_input);
                            }
                            else if(TP_PLS_ROOT == box_sel)
                            {
                                GUI_SetProperty("edit_plsn_root_value", "string", s_tplist_edit_input);
                                GUI_SetProperty("edit_plsn_root_value", "select", &edit_sel);

                                GUI_SetProperty("text_tplist_pls_root_bk", "string", s_tplist_edit_input);
                            }
                        }
                        else
                        {
                            if((tplist_edit_input_len-1) < 0)
                            {
                                ret = EVENT_TRANSFER_KEEPON;
                                break;
                            }
                            s_tplist_edit_input[tplist_edit_input_len-1] = '\0';

                            edit_sel = tplist_edit_input_len-1;

                            if(TP_PLS_N == box_sel)
                            {
                                GUI_SetProperty("edit_plsn_root_value_n", "string", s_tplist_edit_input);
                                GUI_SetProperty("edit_plsn_root_value_n", "select", &edit_sel);

                                GUI_SetProperty("text_tplist_pls_n_bk", "string", s_tplist_edit_input);
                            }
                            else if(TP_PLS_ROOT == box_sel)
                            {
                                GUI_SetProperty("edit_plsn_root_value", "string", s_tplist_edit_input);
                                GUI_SetProperty("edit_plsn_root_value", "select", &edit_sel);
                                GUI_SetProperty("text_tplist_pls_root_bk", "string", s_tplist_edit_input);
                            }
                        }

                    }

                    if(edit_command_mode == 1)
                    {
                        memset(edit_buffer,0,MAX_LEN_);
                        GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
                        if(TP_PLS_N == box_sel)
                        {
                            GUI_GetProperty("text_tplist_pls_n_bk", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num > 0)
                                edit_num = edit_num-1;
                            sprintf(edit_buffer,"%d",edit_num);
                            GUI_SetProperty("text_tplist_pls_n_bk", "string", edit_buffer);
                            GUI_SetProperty("edit_plsn_root_value_n", "string", edit_buffer);
                        }
                        else if(TP_PLS_ROOT == box_sel)
                        {
                            GUI_GetProperty("text_tplist_pls_root_bk", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num > 0)
                                edit_num = edit_num-1;
                            sprintf(edit_buffer,"%d",edit_num);
                            GUI_SetProperty("text_tplist_pls_root_bk", "string", edit_buffer);
                            GUI_SetProperty("edit_plsn_root_value", "string", edit_buffer);
                        }
                    }
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                case STBK_RIGHT:
                      if(edit_command_mode == 1)
                      {
                          memset(edit_buffer,0,MAX_LEN_);
                          GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
                          if(TP_PLS_N == box_sel)
                          {
                              GUI_GetProperty("text_tplist_pls_n_bk", "string", &edit_string);
                              edit_num = atoi(edit_string);
                              if(edit_num<99999999)
                              {
                                edit_num = edit_num+1;
                              }
                              sprintf(edit_buffer,"%d",edit_num);
                              GUI_SetProperty("text_tplist_pls_n_bk", "string", edit_buffer);
                              GUI_SetProperty("edit_plsn_root_value_n", "string", edit_buffer);
                          }
                          else if(TP_PLS_ROOT == box_sel)
                          {
                              GUI_GetProperty("text_tplist_pls_root_bk", "string", &edit_string);
                              edit_num = atoi(edit_string);
                              if(edit_num<99999999)
                              {
                                edit_num = edit_num+1;
                              }
                              sprintf(edit_buffer,"%d",edit_num);
                              GUI_SetProperty("text_tplist_pls_root_bk", "string", edit_buffer);
                              GUI_SetProperty("edit_plsn_root_value", "string", edit_buffer);
                          }
                      }
                      else
                      {
                          GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
                          if(TP_PLS_N == box_sel)
                          {
                            edit_sel = strlen(s_tplist_edit_input)-1;
                            GUI_SetProperty("edit_plsn_root_value_n", "select", &edit_sel);
                          }
                          else if(TP_PLS_ROOT == box_sel)
                          {
                            edit_sel = strlen(s_tplist_edit_input)-1;
                            GUI_SetProperty("edit_plsn_root_value", "select", &edit_sel);
                          }
                      }

                      ret = EVENT_TRANSFER_KEEPON;
                      break;

                case STBK_0:
                    keypress_status = 1;
                    memset(num,'0',1);
                    break;
                case STBK_1:
                    keypress_status = 1;
                    memset(num,'1',1);
                    break;
                case STBK_2:
                    keypress_status = 1;
                    memset(num,'2',1);
                    break;
                case STBK_3:
                    keypress_status = 1;
                    memset(num,'3',1);
                    break;
                case STBK_4:
                    keypress_status = 1;
                    memset(num,'4',1);
                    break;
                case STBK_5:
                    keypress_status = 1;
                    memset(num,'5',1);
                    break;
                case STBK_6:
                    keypress_status = 1;
                    memset(num,'6',1);
                    break;
                case STBK_7:
                    keypress_status = 1;
                    memset(num,'7',1);
                    break;
                case STBK_8:
                    keypress_status = 1;
                    memset(num,'8',1);
                    break;
                case STBK_9:
                    keypress_status = 1;
                    memset(num,'9',1);
                    break;
                default:
                    break;

            }
            break;
        default:
            break;
    }

    if(keypress_status == 1)
    {
        edit_command_mode = 0;
        keypress_status = 0;
        GUI_GetProperty("box_plsn_root_para", "select", &box_sel);

        if(strlen(s_tplist_edit_input) < 8)
        {
            if(edit_del_to_zero == 1)
            {
                edit_del_to_zero = 0;
                memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);
            }

            if(TP_PLS_N == box_sel)
            {
                GUI_SetProperty("edit_plsn_root_value_n", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_plsn_root_value_n", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_pls_n_bk", "string", s_tplist_edit_input);

                edit_sel = strlen(s_tplist_edit_input)-1;
                GUI_SetProperty("edit_plsn_root_value_n", "select", &edit_sel);
            }
            else if(TP_PLS_ROOT == box_sel)
            {
                GUI_SetProperty("edit_plsn_root_value", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_plsn_root_value", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_pls_root_bk", "string", s_tplist_edit_input);

                edit_sel = strlen(s_tplist_edit_input)-1;
                GUI_SetProperty("edit_plsn_root_value", "select", &edit_sel);

            }
        }
        else if(strlen(s_tplist_edit_input) == 8)
        {
            edit_sel = 1;
            memset(s_tplist_edit_input,'\0',MAX_INPUT_NUM);
            if(TP_PLS_N == box_sel)
            {
                GUI_SetProperty("edit_plsn_root_value_n", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_plsn_root_value_n", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_pls_n_bk", "string", s_tplist_edit_input);
                GUI_SetProperty("edit_plsn_root_value_n", "select", &edit_sel);
            }
            else if(TP_PLS_ROOT == box_sel)
            {
                GUI_SetProperty("edit_plsn_root_value", "clear", NULL);
                strcat(s_tplist_edit_input,num);
                GUI_SetProperty("edit_plsn_root_value", "string", s_tplist_edit_input);
                GUI_SetProperty("text_tplist_pls_root_bk", "string", s_tplist_edit_input);
                GUI_SetProperty("edit_plsn_root_value", "select", &edit_sel);
            }

        }

        if(strlen(s_tplist_edit_input) == 8)
            edit_command_mode = 1;
        pls_n_edit_change_show();
        pls_edit_change_root_or_n_show(box_sel);

    }

    if((0 == strcasecmp("box_plsn_root_para", GUI_GetFocusWidget())) && (up_down_flag == 0))
    {
        GUI_GetProperty("box_plsn_root_para", "select", &box_sel);
        if(box_sel == 0)// focus on N
        {
            app_tp_list_pls_n_edit_change();
        }
        else if(box_sel == 1)// focus on Root
        {
            app_tp_list_pls_root_edit_change();
        }
    }

    return ret;
}

void app_tp_list_menu_exec(void)
{
    if(s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner) > 0)
    {
#if SAT2IP_SERVER_SUPPORT
        if (app_sat2ip_operate_tip_get_force() < 0)
            return;
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
#endif
        GUI_CreateDialog(WND_TP_LIST);
    }
    else
    {
        app_mainmenu_tip_exec(MM_NO_SAT);
    }
}

#if WEBAPI_SUPPORT
#include <cJSON.h>
#include "app_webapi.h"
#include "netapps/webapi/app_webapi_priv.h"

char *app_webapi_get_dvbs_transponder_data(char *id)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json3 = NULL;
	char *out = NULL;
	int i = 0;
	int total = 0;
	GxMsgProperty_NodeNumGet node_num = {0};
	GxMsgProperty_NodeByPosGet node_tp = {0};
	char buffer[16];
	uint32_t sat_id = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	sat_id = (uint32_t)atoi(id);
	if(sat_id>0)
	{
		memset(&node_num, 0, sizeof(GxMsgProperty_NodeNumGet));
		node_num.node_type = NODE_TP;
		node_num.sat_id = sat_id;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num))
		{
			root = cJSON_CreateObject();
			cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
			json1 = cJSON_CreateObject();
			cJSON_AddItemToObject(root, WEBAPI_TRANSPONDER_STR, json1);
			json2 = cJSON_CreateArray();
			cJSON_AddItemToObject(json1, WEBAPI_DATA_STR, json2);

			for(i=0; i<node_num.node_num; i++)
			{
				memset(&node_tp, 0, sizeof(GxMsgProperty_NodeByPosGet));
				node_tp.node_type = NODE_TP;
				node_tp.pos = i;
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_tp))
				{
					json3 = cJSON_CreateObject();
					cJSON_AddItemToArray(json2, json3);
					snprintf(buffer, sizeof(buffer)-1, "%d", node_tp.tp_data.id);
					cJSON_AddStringToObject(json3, WEBAPI_ID_STR, buffer);
					cJSON_AddNumberToObject(json3, WEBAPI_FREQUENCY_STR, node_tp.tp_data.frequency);
					cJSON_AddNumberToObject(json3, WEBAPI_POLARIZATION_STR, node_tp.tp_data.tp_s.polar);
					cJSON_AddNumberToObject(json3, WEBAPI_SYMBOLRATE_STR, node_tp.tp_data.tp_s.symbol_rate);
					total++;
				}
			}
			cJSON_AddNumberToObject(json1, WEBAPI_TOTAL_STR, total);
			out = cJSON_PrintUnformatted(root);
			cJSON_Delete(root);
		}
	}

	return out;
}

webapi_ret_t app_webapi_set_dvbs_transponder_data(
					char *sat_id,
					char *id,
					int frequency,
					int polarization,
					int symbol_rate)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	GxMsgProperty_NodeByIdGet sat_node = {0};
	GxMsgProperty_NodeByIdGet tp_node = {0};
	GxMsgProperty_NodeAdd node_add = {0};
	GxMsgProperty_NodeModify node_modify = {0};
	TpListTpPara InputTpPara={0};
	uint32_t s_id = 0;
	uint16_t tp_id = 0;
	int max = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(sat_id)
	{
		s_id = atoi(sat_id);
		memset(&sat_node, 0, sizeof(GxMsgProperty_NodeByIdGet));
		sat_node.node_type = NODE_SAT;
		sat_node.id = s_id;
		if((GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node))
			&&(sat_node.sat_data.type == GXBUS_PM_SAT_S))
		{
			memset(&InputTpPara, 0, sizeof(TpListTpPara));
			InputTpPara.frequency = frequency;
			InputTpPara.symbol_rate = symbol_rate;
			if(polarization == 0)
			{
				InputTpPara.polar = GXBUS_PM_TP_POLAR_H;
			}
			else
			{
				InputTpPara.polar = GXBUS_PM_TP_POLAR_V;
			}
			if(true == _check_tp_para_valid(&sat_node.sat_data, &InputTpPara))
			{
				if(id)
				{
					memset(&tp_node, 0, sizeof(GxMsgProperty_NodeByIdGet));
					tp_node.node_type = NODE_TP;
					tp_node.id = atoi(id);
					if((GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_node))
						&&(tp_node.tp_data.sat_id == s_id))
					{
						memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
						node_modify.node_type = NODE_TP;
						memcpy(&node_modify.tp_data, &tp_node.tp_data, sizeof(GxBusPmDataTP));
						node_modify.tp_data.frequency = InputTpPara.frequency;
						node_modify.tp_data.tp_s.symbol_rate = InputTpPara.symbol_rate;
						node_modify.tp_data.tp_s.polar = InputTpPara.polar;
						if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
						{
							ret = WEBAPI_SUCCESS;
						}
					}
					else
					{
						ret = WEBAPI_TP_NOT_EXIST;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
					}
				}
				else //new tp
				{
					max = SYS_MAX_TP - GxBus_PmTpNumGetBySat(s_id);
					if(max>0)
					{
						if(0 == GxBus_PmTpExistChek(s_id, InputTpPara.frequency,InputTpPara.symbol_rate, InputTpPara.polar, 0, &tp_id))
						{
							memset(&node_add, 0, sizeof(GxMsgProperty_NodeAdd));
							node_add.node_type = NODE_TP;
							node_add.tp_data.sat_id = s_id;
							node_add.tp_data.frequency = InputTpPara.frequency;
							node_add.tp_data.tp_s.symbol_rate = InputTpPara.symbol_rate;
							node_add.tp_data.tp_s.polar = InputTpPara.polar;
							if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &node_add))
							{
								ret = WEBAPI_SUCCESS;
							}
						}
						else
						{
							ret = WEBAPI_TP_EXIST;
							WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
						}
					}
					else
					{
						ret = WEBAPI_TP_LIMIT_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
					}
				}
			}
			else
			{
				ret = WEBAPI_TP_INVALID;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
		}
		else
		{
			ret = WEBAPI_SATELLITE_NOT_EXIST;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
	}

	return ret;
}

webapi_ret_t app_webapi_dvbs_transponder_delete(cJSON *json)
{
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	GxMsgProperty_NodeDelete node= {0};
	cJSON *json1 = NULL;
	int sat_num = 0;
	int i = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	sat_num = cJSON_GetArraySize(json);
	if(sat_num>0)
	{
		memset(&node, 0, sizeof(GxMsgProperty_NodeDelete));
		node.node_type = NODE_TP;
		node.id_array = GxCore_Malloc(sat_num*sizeof(uint32_t));
		if(node.id_array)
		{
			for(i=0; i<sat_num; i++)
			{
				json1 = cJSON_GetArrayItem(json, i);
				if(json1 && (json1->type == cJSON_String))
				{
					WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, json1->valuestring);
					node.id_array[node.num] = atoi(json1->valuestring);
					node.num++;
				}
			}
			if(node.num>0)
			{
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node))
				{
					ret = WEBAPI_SUCCESS;
				}
				else
				{
					ret = WEBAPI_FUNCTION_PARAM_ERR;
					WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
				}
			}
			else
			{
				ret = WEBAPI_FUNCTION_PARAM_ERR;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			GxCore_Free(node.id_array);
		}
	}
	else
	{
		ret = WEBAPI_FUNCTION_PARAM_ERR;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}

	return ret;
}

#endif

#if TP_LIST_MOVE_SORT_SUPPORT
static int32_t _tp_cmp_fre(const void *p1, const void *p2)
{
    uint32_t name1 = 0;
    uint32_t name2 = 0;
    name1 = ((GxBusPmDataTP*)p1)->frequency;
    name2 = ((GxBusPmDataTP*)p2)->frequency;

    return name1-name2;
}

static int32_t _tp_cmp_fre_reverse(const void *p1, const void *p2)
{
    uint32_t name1 = 0;
    uint32_t name2 = 0;
    name1 = ((GxBusPmDataTP*)p1)->frequency;
    name2 = ((GxBusPmDataTP*)p2)->frequency;

    return name2-name1;
}
static int32_t _tp_cmp_polar(const void *p1, const void *p2)
{
    uint16_t name1 = 0;
    uint16_t name2 = 0;
    name1 = ((GxBusPmDataTP*)p1)->tp_s.polar;
    name2 = ((GxBusPmDataTP*)p2)->tp_s.polar;

    return name1-name2;
}

static int32_t _tp_cmp_polar_reverse(const void *p1, const void *p2)
{
    uint16_t name1 = 0;
    uint16_t name2 = 0;
    name1 = ((GxBusPmDataTP*)p1)->tp_s.polar;
    name2 = ((GxBusPmDataTP*)p2)->tp_s.polar;

    return name2-name1;
}

static int32_t _tp_cmp_sym(const void *p1, const void *p2)
{
    uint32_t name1 = 0;
    uint32_t name2 = 0;
    name1 = ((GxBusPmDataTP*)p1)->tp_s.symbol_rate;
    name2 = ((GxBusPmDataTP*)p2)->tp_s.symbol_rate;

    return name1-name2;
}

static int32_t _tp_cmp_sym_reverse(const void *p1, const void *p2)
{
    uint32_t name1 = 0;
    uint32_t name2 = 0;
    name1 = ((GxBusPmDataTP*)p1)->tp_s.symbol_rate;
    name2 = ((GxBusPmDataTP*)p2)->tp_s.symbol_rate;

    return name2-name1;
}

int app_tp_list_sort(tp_sort_mode sort_mode)
{
    int tp_num,i;
    GxBusPmDataTP *tp_list;

    tp_num = GxBus_PmTpNumGetBySat(s_dvbs_sat_data.cur_sat.sat_data.id);
    tp_list = (GxBusPmDataTP*)GxCore_Malloc(tp_num*sizeof(GxBusPmDataTP));
    if(tp_list == NULL)
        return -1;

    memset(tp_list,0,tp_num*sizeof(GxBusPmDataTP));
    GxBus_PmLock();
    GxBus_PmTpGetByPosInSat(0,tp_num,tp_list);
    if(sort_mode == TP_SORT_FRE_0_9)
    {
        qsort(tp_list,tp_num,sizeof(GxBusPmDataTP),_tp_cmp_fre);
    }
    else if(sort_mode == TP_SORT_FRE_9_0)
    {
        qsort(tp_list,tp_num,sizeof(GxBusPmDataTP),_tp_cmp_fre_reverse);
    }
    else if(sort_mode == TP_SORT_POLAR_H_V)
    {
        qsort(tp_list,tp_num,sizeof(GxBusPmDataTP),_tp_cmp_polar);
    }
    else if(sort_mode == TP_SORT_POLAR_V_H)
    {
        qsort(tp_list,tp_num,sizeof(GxBusPmDataTP),_tp_cmp_polar_reverse);
    }
    else if(sort_mode == TP_SORT_SYM_0_9)
    {
        qsort(tp_list,tp_num,sizeof(GxBusPmDataTP),_tp_cmp_sym);
    }
    else if(sort_mode == TP_SORT_SYM_9_0)
    {
        qsort(tp_list,tp_num,sizeof(GxBusPmDataTP),_tp_cmp_sym_reverse);
    }
    for(i=0;i<tp_num;i++)
    {
        tp_list[i].pos=i;
        GxBus_PmTpModify(&tp_list[i]);
    }
    GxBus_PmUnlock();
    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    GxCore_Free(tp_list);
    return 0;
}

int app_tp_list_move(uint32_t* selected_pos, int selected_num, int dst_pos)
{
    int tp_num,i,pos0;
    GxBusPmDataTP *selected_list=NULL,*tp_list=NULL,*new_tp_list=NULL;

    pos0=0;
    for(i=0;i<selected_num;i++)
    {
        if(selected_pos[i] == dst_pos)
            return 0;
    }

    tp_num = GxBus_PmTpNumGetBySat(s_dvbs_sat_data.cur_sat.sat_data.id);
    selected_list = (GxBusPmDataTP*)GxCore_Malloc(selected_num*sizeof(GxBusPmDataTP));
    tp_list = (GxBusPmDataTP*)GxCore_Malloc(tp_num*sizeof(GxBusPmDataTP));
    new_tp_list = (GxBusPmDataTP*)GxCore_Malloc(tp_num*sizeof(GxBusPmDataTP));
    if(selected_list == NULL || tp_list == NULL || selected_list == NULL)
        goto err;

    memset(tp_list,0,tp_num*sizeof(GxBusPmDataTP));
    memset(new_tp_list,0,tp_num*sizeof(GxBusPmDataTP));
    memset(selected_list,0,selected_num*sizeof(GxBusPmDataTP));

    GxBus_PmLock();
    GxBus_PmTpGetByPosInSat(0,tp_num,tp_list);
    for(i=0;i<selected_num;i++)
    {
        memcpy(&selected_list[i],&tp_list[selected_pos[i]],sizeof(GxBusPmDataTP));
        tp_list[selected_pos[i]].id=0;
    }

    for(i=0;i<dst_pos;i++)
    {
        if(tp_list[i].id!=0)
        {
            memcpy(&new_tp_list[pos0],&tp_list[i],sizeof(GxBusPmDataTP));
            pos0++;
        }
    }
    for(i=0;i<selected_num;i++)
    {
        memcpy(&new_tp_list[pos0],&selected_list[i],sizeof(GxBusPmDataTP));
        pos0++;
    }
    for(i=dst_pos;i<tp_num;i++)
    {
        if(tp_list[i].id != 0)
        {
            memcpy(&new_tp_list[pos0],&tp_list[i],sizeof(GxBusPmDataTP));
            pos0++;
        }
    }
    for(i=0;i<tp_num;i++)
    {
        new_tp_list[i].pos=i;
        GxBus_PmTpModify(&new_tp_list[i]);
    }

    GxBus_PmUnlock();
    GxBus_PmSync(GXBUS_PM_SYNC_TP);
    GxCore_Free(selected_list);
    GxCore_Free(tp_list);
    GxCore_Free(new_tp_list);
    return 0;
err:
    GxCore_Free(selected_list);
    GxCore_Free(tp_list);
    GxCore_Free(new_tp_list);
    return -1;

}
#endif
#endif
