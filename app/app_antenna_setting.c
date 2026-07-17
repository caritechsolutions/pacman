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
#include "app_dvbs_sat_opt.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#endif
#if PDMX_SUPPORT
#include "app_pdmx.h"
#endif
#if MULTISTREAM_SUPPORT
#include "module/app_multistream.h"
#endif
#include "module/app_plsn.h"
#include "app_fuse_ops.h"

/* widget macro -------------------------------------------------------- */
#define BOX_ANTENNA_OPTIONS  "box_antenna_options"
#define CMBOX_ANTENNA_TUNER  "cmbox_antenna_tuner"
#define IMG_BAR_LONG_FOCUS   "s_bar_long_focus"
#define IMG_BAR_LONG_UNFOCUS "null"

#define IMG_BAR_OK_FOCUS     "s_button_ok"
#define TXT_TIP_BLUE         "text_antenna_setting_tip_blue"
#define IMG_TIP_BLUE         "img_antenna_setting_tip_blue"

#define UNIVERSAL_NUM (3)
#define ANTENNA_SETTING_HELP1_IMG       "img_antenna_setting_tip_red"
#define ANTENNA_SETTING_HELP1_TXT       "text_antenna_setting_tip_red"
#define ANTENNA_SETTING_HELP2_IMG       "img_antenna_setting_tip_green"
#define ANTENNA_SETTING_HELP2_TXT       "text_antenna_setting_tip_green"
#define ANTENNA_SETTING_HELP3_IMG       "img_antenna_setting_tip_blue"
#define ANTENNA_SETTING_HELP3_TXT       "text_antenna_setting_tip_blue"

#define LNB_POWER_CONTENT   "[Off,On,13V,18V]"
#if (UNICABLE_SUPPORT==1 && LNBF_SUPPORT==0)
#define LNB_FREQ_CONTENT    "[Universal1(9750/10600),Universal2(9750/10700),Universal3(9750/10750),OCS1(5150/5750),OCS2(5750/5150),5150,5750,5950,9750,10000,10600,10700,10750,11250,11300,Unicable,Unicable2]"
#elif (UNICABLE_SUPPORT==0 && LNBF_SUPPORT==1)
#define LNB_FREQ_CONTENT    "[Universal1(9750/10600),Universal2(9750/10700),Universal3(9750/10750),LNBF1(10250/11400),LNBF2(11400/10250),OCS1(5150/5750),OCS2(5750/5150),5150,5750,5950,9750,10000,10600,10700,10750,11250,11300]"
#elif (UNICABLE_SUPPORT==1 && LNBF_SUPPORT==1)
#define LNB_FREQ_CONTENT    "[Universal1(9750/10600),Universal2(9750/10700),Universal3(9750/10750),LNBF1(10250/11400),LNBF2(11400/10250),OCS1(5150/5750),OCS2(5750/5150),5150,5750,5950,9750,10000,10600,10700,10750,11250,11300,Unicable,Unicable2]"
#else
#define LNB_FREQ_CONTENT    "[Universal1(9750/10600),Universal2(9750/10700),Universal3(9750/10750),OCS1(5150/5750),OCS2(5750/5150),5150,5750,5950,9750,10000,10600,10700,10750,11250,11300]"
#endif
#define ANTENNA_22K_CONTENT         "[Off,On]"
#define ANTENNA_22K_AUTO_CONTENT    "[Auto]"
#define DISEQC10_CONTENT            "[Off,Port 1,Port 2,Port 3,Port 4]"
#define DISEQC11_CONTENT            "[Off,Port 1,Port 2,Port 3,Port 4,Port 5,Port 6,Port 7,Port 8,Port 9,Port 10,Port 11,Port 12,Port 13,Port 14,Port 15,Port 16]"
#define ANTENNA_SEARCH_CONTENT      "[Satellite,Blind,TP]"
#define ANTENNA_SAT_SEARCH_CONTENT  "[Blind]"
#if UNICABLE_SUPPORT
#define ANTENNA_SAT_SEARCH_CONTENT_UNICABLE "[Satellite,TP]"
#endif

#if SUPER_BLIND_SUPPORT
#define ANTENNA_SAT_SEARCH_CONTENT_SUPER "[Super Satellite,Super Blind,Super Tp]"
#define ANTENNA_SUPER_SEARCH_CONTENT     "[Super Blind]"
#endif
/* Private variable------------------------------------------------------- */

static char* s_LnbPower[4] = {STR_ID_OFF,STR_ID_ON,"13V","18V"};
static char* s_22k[2] = {STR_ID_OFF,STR_ID_ON};
static char* s_Diseqc10[5] = {STR_ID_OFF, "Port 1", "Port 2", "Port 3", "Port 4"};
static bool thiz_super_search = false;

#if (UNICABLE_SUPPORT==1 && LNBF_SUPPORT==0)
#define UNICABLE_POSITION_NUM       16
static char* s_LnbFreq[] = {
    "9750/10600","9750/10700",
    "9750/10750","5150/5750","5750/5150",//OCS2
    "5150","5750","5950","9750","10000","10600","10700","10750","11250","11300","Unicable","Unicable2"};
#elif (UNICABLE_SUPPORT==0 && LNBF_SUPPORT==1)
static char* s_LnbFreq[] = {
    "9750/10600","9750/10700","9750/10750",
    "10250/11400",//lnbf1
    "11400/10250",//lnbf2
    "5150/5750","5750/5150",//OCS2
    "5150","5750","5950","9750","10000","10600","10700","10750","11250","11300"};
#elif (UNICABLE_SUPPORT==1 && LNBF_SUPPORT==1)
#define UNICABLE_POSITION_NUM       16
static char* s_LnbFreq[] = {
    "9750/10600","9750/10700","9750/10750",
    "10250/11400",//lnbf1
    "11400/10250",//lnbf2
    "5150/5750","5750/5150",//OCS2
    "5150","5750","5950","9750","10000","10600","10700","10750","11250","11300","Unicable","Unicable2"};
#else
static char* s_LnbFreq[15] = {
    "9750/10600","9750/10700",
    "9750/10750","5150/5750","5750/5150",//OCS2
    "5150","5750","5950","9750","10000","10600","10700","10750","11250","11300"};//Universal1
#endif

static char* s_Diseqc11[17] = {
    STR_ID_OFF, "Port 1", "Port 2", "Port 3", "Port 4", "Port 5","Port 6", "Port 7", "Port 8",
    "Port 9", "Port 10", "Port 11", "Port 12", "Port 13", "Port 14", "Port 15", "Port 16"};

static char * s_antenna_boxitem[] =
{
    "boxitem_antenna_satellite",
    "boxitem_antenna_lnbpower",
    "boxitem_antenna_lnbfreq",
    "boxitem_antenna_22k",
    "boxitem_antenna_diseqc10",
    "boxitem_antenna_diseqc11",
    "boxitem_antenna_tp",
    "boxitem_antenna_search",
};

static char * s_antenna_combox[] =
{
    "cmbox_antenna_sat",
    "cmbox_antenna_lnb_power",
    "cmbox_antenna_lnb_freq",
    "cmbox_antenna_22k",
    "cmbox_antenna_diseqc10",
    "cmbox_antenna_diseqc11",
    "cmbox_antenna_tp",
    "cmbox_antenna_search",
};

enum BOX_ITEM_NAME
{
    BOX_ITEM_SAT = 0,
    BOX_ITEM_LNB_POWER,
    BOX_ITEM_LNB_FREQ,
    BOX_ITEM_22K,
    BOX_ITEM_DISEQC10,
    BOX_ITEM_DISEQC11,
    BOX_ITEM_TP,
    BOX_ITEM_SEARCH
};

enum
{
    SEARCH_SEL_SAT,
    SEARCH_SEL_BLIND,
    SEARCH_SEL_TP
};

bool scan_mode_disable_flag = false;//True:Disable the first boxitem
#if UNICABLE_SUPPORT
extern int app_cur_sat_unicable_type_check(void);
extern void app_unicable_init_if_channel_num(GxBusPmDataSat *p_sat);
static GxBusPmSatUnicableType _antenna_lnb_fre_sel_to_unicable_type(int lnb_fre_sel);
#endif
static event_list* sp_AtnTimerSignal = NULL;
static event_list* sp_AtnLockTimer = NULL;

static uint32_t s_n22KSelectRecord = 0;
static int  s_tp_total_bak = -1;

static void _antenna_sat_para_save(void);

extern void _stack_check_info(char *s, int line);

#if TKGS_SUPPORT
extern bool app_tkgs_check_sat_is_turish(GxMsgProperty_NodeByPosGet * p_nodesat);
extern void app_tkgs_cantwork_popup(void);
extern int app_check_tkgs_control_db(void);
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);

static int app_anternna_tkgs_key_handler(void)
{
    if(app_tkgs_check_sat_is_turish(&s_dvbs_sat_data.cur_sat))
    {
        if(app_tkgs_get_operatemode() != GX_TKGS_OFF)
        {
            app_tkgs_cantwork_popup();
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    return FALSE;
}

#endif

static int timer_atn_show_signal(void *userdata);
static void timer_atn_show_signal_stop(void);
static void _antenna_box_blind_search_display(void);

void app_antenna_setting_btn_draw(int show_hide)
{
	if ( show_hide )
	{
		GUI_SetProperty(ANTENNA_SETTING_HELP1_IMG, "state", "show");
		GUI_SetProperty(ANTENNA_SETTING_HELP1_TXT, "state", "show");
		GUI_SetProperty(ANTENNA_SETTING_HELP2_IMG, "state", "show");
		GUI_SetProperty(ANTENNA_SETTING_HELP2_TXT, "state", "show");
		GUI_SetProperty(ANTENNA_SETTING_HELP3_IMG, "state", "show");
		GUI_SetProperty(ANTENNA_SETTING_HELP3_TXT, "state", "show");
	}
	else
	{
		GUI_SetProperty(ANTENNA_SETTING_HELP1_IMG, "state", "hide");
		GUI_SetProperty(ANTENNA_SETTING_HELP1_TXT, "state", "hide");
		GUI_SetProperty(ANTENNA_SETTING_HELP2_IMG, "state", "hide");
		GUI_SetProperty(ANTENNA_SETTING_HELP2_TXT, "state", "hide");
		GUI_SetProperty(ANTENNA_SETTING_HELP3_IMG, "state", "hide");
		GUI_SetProperty(ANTENNA_SETTING_HELP3_TXT, "state", "hide");
	}

#if SUPER_BLIND_SUPPORT
	if ( show_hide )
	{
		GUI_SetProperty(TXT_TIP_BLUE, "state", "show");
		GUI_SetProperty(IMG_TIP_BLUE, "state", "show");
		if(false == thiz_super_search)
			GUI_SetProperty(TXT_TIP_BLUE, "string", STR_ID_SUPER_SEARCH);
		else
			GUI_SetProperty(TXT_TIP_BLUE, "string", STR_ID_NORMAL_SEARCH);
	}
	else
	{
		GUI_SetProperty(TXT_TIP_BLUE, "state", "hide");
		GUI_SetProperty(IMG_TIP_BLUE, "state", "hide");
	}
#endif
}

void app_antenna_setting_combox_search_content_update(char *buffer)
{
	GUI_SetProperty(s_antenna_combox[BOX_ITEM_SEARCH], "content", buffer);
}

void app_antenna_setting_update_data_by_combox_search_change(int update_cmb)
{
    int value = 0;
    int cmb_sel = 1 ;
    s_dvbs_sat_data.tp_rebuild(s_antenna_combox[BOX_ITEM_TP]);

#if UNICABLE_SUPPORT
    if(app_cur_sat_unicable_type_check()){
    }
    else
#endif
    {
        if(0 == s_dvbs_sat_data.cur_sat_tp_total)
        {
            _antenna_box_blind_search_display();
            GUI_SetProperty(s_antenna_combox[BOX_ITEM_SEARCH], "select", &cmb_sel);
            GUI_SetProperty("progbar_antenna_strength", "value", &value);
            GUI_SetProperty("progbar_antenna_quality", "value", &value);
        }
    }

    s_dvbs_sat_data.set_tp(SAVE_TP_PARAM);
    s_tp_total_bak = s_dvbs_sat_data.cur_sat_tp_total;

    s_dvbs_sat_data.set_diseqc(NULL);
    s_dvbs_sat_data.set_tp(SET_TP_FORCE);
    APP_TIMER_ADD(sp_AtnTimerSignal, timer_atn_show_signal, 100, TIMER_REPEAT);

    app_panel_show_prog_num(PANEL_UNDISPLAY_TIME);
}

static void _antenna_setting_combox_search_update(int sat_id,char *pcontent)
{
	app_antenna_setting_combox_search_content_update(pcontent);
	app_antenna_setting_btn_draw(1);
}

static void _antenna_setting_combox_search_change(int sat_id,char *psearch)
{
}
static int _antenan_setting_key_check(unsigned key)
{
	return 0;
}
static int _antenan_setting_key_press(void)
{
	return 0;
}
static void _antenan_setting_uninit(void)
{
}
static void _antenan_save_sat_params(int sat_id,GxBusPmDataSat *p_sat)
{
}

int app_antenan_setting_start_search(int search_mode)
{
    SearchIdSet search_id;

#if TKGS_SUPPORT
    if(app_check_tkgs_control_db() != 0)
    {
        return 0;
    }
#endif

    if(0 == s_dvbs_sat_data.cur_sat_tp_total)//add by yanzh 2012-07-20 10:30
        search_mode = SEARCH_SEL_BLIND;

#if UNICABLE_SUPPORT
    if(app_cur_sat_unicable_type_check())
    {
        if(0 == s_dvbs_sat_data.cur_sat_tp_total)
            search_mode = SEARCH_SEL_SAT;
        if(SEARCH_SEL_BLIND== search_mode)
            search_mode = SEARCH_SEL_TP;
    }
#endif
    if(SEARCH_SEL_SAT == search_mode)
    {
        search_id.id_num = 1;
        search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
        if(search_id.id_array != NULL)
        {
            SearchType search_type;

            search_id.id_array[0] = s_dvbs_sat_data.cur_sat.sat_data.id;
            search_type = SEARCH_SAT;
            scan_mode_disable_flag = true;
            app_search_opt_dlg(search_type, &search_id);
            GxCore_Free(search_id.id_array);
        }
    }
    else if(SEARCH_SEL_TP == search_mode)
    {
        search_id.id_num = 1;
        search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
        if(search_id.id_array != NULL)
        {
            search_id.id_array[0] = s_dvbs_sat_data.cur_tp.tp_data.id;
            app_search_opt_dlg(SEARCH_TP, &search_id);
            GxCore_Free(search_id.id_array);
        }
    }
    else if(SEARCH_SEL_BLIND == search_mode)
    {
        search_id.id_num = 1;
        search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
        if(search_id.id_array != NULL)
        {
            SearchType search_type;

            search_id.id_array[0] = s_dvbs_sat_data.cur_sat.sat_data.id;
            search_type = SEARCH_BLIND_ONLY;
            scan_mode_disable_flag = true;
            app_search_opt_dlg(search_type, &search_id);
            GxCore_Free(search_id.id_array);
        }
    }
    return 0;
}

static int _antenan_start_search(int sat_id,int tp_id,char *psearch)
{
	int search_mode;
	GUI_GetProperty(s_antenna_combox[BOX_ITEM_SEARCH], "select", &search_mode);

	return app_antenan_setting_start_search(search_mode);
}

static void _antenan_get_focus(char *psearch)
{
	app_antenna_setting_btn_draw(1);
}

static dvbs_antenna_setting_ops s_antenna_setting_ops = {
	.combox_search_update = _antenna_setting_combox_search_update,
	.combox_search_change = _antenna_setting_combox_search_change,
	.key_check      = _antenan_setting_key_check,
	.key_func_press = _antenan_setting_key_press,
	.save_sat_params = _antenan_save_sat_params,
	.start_search    = _antenan_start_search,
	.uninit = _antenan_setting_uninit,
	.get_focus = _antenan_get_focus
};

void app_antenna_setting_set_ops(dvbs_antenna_setting_ops *ops)
{
	if ( ops == NULL )
		return ;

	memcpy(&s_antenna_setting_ops,ops,sizeof(dvbs_antenna_setting_ops));
}

#if SUPER_BLIND_SUPPORT
static void _antenna_super_search_enable(void)
{
#if PDMX_SUPPORT
    app_pdmx_register();
#endif
#if MULTISTREAM_SUPPORT
    app_multistream_register();
#endif
    app_plsn_register();
    thiz_super_search = true;

    return;
}

static void _antenna_super_search_disable(void)
{
#if PDMX_SUPPORT
    app_pdmx_unregister();
#endif
#if MULTISTREAM_SUPPORT
    app_multistream_unregister();
#endif
    app_plsn_unregister();
    thiz_super_search = false;

    return;
}
#endif

static void _antenna_box_blind_search_display(void)
{
#if SUPER_BLIND_SUPPORT
    if(true == thiz_super_search)
        s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SUPER_SEARCH_CONTENT);
    else
        s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SAT_SEARCH_CONTENT);
#else
    s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SAT_SEARCH_CONTENT);
#endif
    return;
}

static void _antenna_box_sat_search_display(void)
{
#if SUPER_BLIND_SUPPORT
    if(true == thiz_super_search)
        s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SAT_SEARCH_CONTENT_SUPER);
    else
        s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SEARCH_CONTENT);
#else
    s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SEARCH_CONTENT);
#endif
    return;
}

bool app_get_super_search_flag(void)
{
    return thiz_super_search;
}

// create dialog
static int app_antenna_setting_display_update(void);
int app_antenna_setting_init(int SatSel, int TpSel)
{
    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_ANTENNA_SETTING))
    {
        GUI_CreateDialog(WND_ANTENNA_SETTING);
    }
    SatSel = s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] ;
    if(SatSel >= 0)
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_SAT], "select", &SatSel);

    if(TpSel >= 0)
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_TP], "select", &TpSel);

    app_antenna_setting_display_update();
    return 0;
}

int app_antenna_clean_signal(void)
{
    app_clean_signal("progbar_antenna_strength", "text_antenna_strength_value",
                        "progbar_antenna_quality", "text_antenna_quality_value");
    return 0;
}

static int timer_atn_show_signal(void *userdata)
{
    app_show_signal("progbar_antenna_strength", "text_antenna_strength_value",
                        "progbar_antenna_quality", "text_antenna_quality_value");
    return 0;
}

static void timer_atn_show_signal_stop(void)
{
    if(sp_AtnTimerSignal != NULL)
        timer_stop(sp_AtnTimerSignal);

    GUI_SetInterface("flush",NULL);
}

static void timer_atn_show_signal_reset(void)
{
    if(sp_AtnTimerSignal != NULL)
        reset_timer(sp_AtnTimerSignal);
}

static int timer_antenna_set_frontend(void *userdata)
{
    APP_TIMER_REMOVE(sp_AtnLockTimer);

    s_dvbs_sat_data.set_diseqc(NULL);
    s_dvbs_sat_data.set_tp(SET_TP_FORCE);

    return 0;
}

static int32_t _antenna_poplist_cb(int32_t ret_sel, unsigned short key)
{
    uint32_t box_sel;
    int cmb_sel;

    cmb_sel = ret_sel ;
    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    switch(box_sel)
    {
        case BOX_ITEM_SAT:
        case BOX_ITEM_LNB_POWER:
        case BOX_ITEM_22K:
        case BOX_ITEM_DISEQC10:
        case BOX_ITEM_DISEQC11:
        case BOX_ITEM_TP:
            GUI_SetProperty(s_antenna_combox[box_sel], "select", &cmb_sel);
            app_update_pop_dlg(WND_POP_BOOK);
            if ( (box_sel == BOX_ITEM_SAT) || (box_sel == BOX_ITEM_TP) )
            {
                poplist_destory_cb_free_item_content();
            }
            break;
        case BOX_ITEM_LNB_FREQ:
            {
                #if UNICABLE_SUPPORT
                if(ID_UNICABLE == cmb_sel)
                {
                    s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type = GXBUS_PM_SAT_UNICABLE;
                    app_lnb_freq_sel_to_data(s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index, &s_dvbs_sat_data.cur_sat.sat_data);
                }
                else if(ID_UNICABLE_2 == cmb_sel)
                {
                    s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type = GXBUS_PM_SAT_UNICABLE2;
                    app_lnb_freq_sel_to_data(s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index, &s_dvbs_sat_data.cur_sat.sat_data);
                }
                else
                {
                    s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type = GXBUS_PM_SAT_UNICABLE_OFF;
                    app_lnb_freq_sel_to_data(cmb_sel, &s_dvbs_sat_data.cur_sat.sat_data);
                }
                #else
                app_lnb_freq_sel_to_data(cmb_sel, &s_dvbs_sat_data.cur_sat.sat_data);
                #endif
                APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);
                GUI_SetProperty(s_antenna_combox[box_sel], "select", &cmb_sel);
                app_update_pop_dlg(WND_POP_BOOK);
            }
            break;
        default:
            return 0;
    }

    if((GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS) && (GUI_CheckDialog(WND_SEARCH) != GXCORE_SUCCESS))
    {
        GUI_SetProperty(BOX_ANTENNA_OPTIONS, "update", NULL);
    }

    return 0;
}

static int _antenna_pop(int box_sel, int cmb_sel)
{
    int ret =-1;
    char *p_item = NULL ;
    PopList pop_list;
    memset(&pop_list, 0, sizeof(PopList));
    pop_list.sel = cmb_sel;
    pop_list.show_num= false;

    switch(box_sel)
    {
        case BOX_ITEM_SAT:
            ret = s_dvbs_sat_data.sat_poplist_get_name(&p_item);
            if (ret < 0)
                return -1 ;
            pop_list.title = STR_ID_SATELLITE;
            pop_list.item_num = s_dvbs_sat_data.sat_total;
            pop_list.item_content = (char**)p_item;
            pop_list.show_num= true;
            break;
        case BOX_ITEM_LNB_POWER:
            pop_list.title = STR_ID_LNB_POWER;
            pop_list.item_num = sizeof(s_LnbPower)/sizeof(*s_LnbPower);
            pop_list.item_content = s_LnbPower;
            break;
        case BOX_ITEM_LNB_FREQ:
            pop_list.title = STR_ID_LNB_FREQ;
            pop_list.item_num = sizeof(s_LnbFreq)/sizeof(*s_LnbFreq);
            pop_list.item_content = s_LnbFreq;
            break;
        case BOX_ITEM_22K:
            pop_list.title = "22K";
            pop_list.item_num = sizeof(s_22k)/sizeof(*s_22k);
            pop_list.item_content = s_22k;
            break;
        case BOX_ITEM_DISEQC10:
            pop_list.title = "DiSEqC 1.0";
            pop_list.item_num = sizeof(s_Diseqc10)/sizeof(*s_Diseqc10);
            pop_list.item_content = s_Diseqc10;
            break;
        case BOX_ITEM_DISEQC11:
            pop_list.title = "DiSEqC 1.1";
            pop_list.item_num = sizeof(s_Diseqc11)/sizeof(*s_Diseqc11);
            pop_list.item_content = s_Diseqc11;
            break;
        case BOX_ITEM_TP:
            ret = s_dvbs_sat_data.tp_poplist_get_info(&p_item);
            if (ret < 0)
                return -1 ;
            pop_list.title = STR_ID_TP;
            pop_list.item_num = s_dvbs_sat_data.cur_sat_tp_total;
            pop_list.item_content = (char**)p_item;
            pop_list.show_num= true;
            break;
        default:
            return ret;
    }

    pop_list.mode = POP_MODE_UNBLOCK;
    pop_list.exit_cb = _antenna_poplist_cb ;
    ret = poplist_create(&pop_list);
    return ret;
}

void app_antenna_setting_to_search(WndStatus wnd_ok)
{
    timer_stop(sp_AtnLockTimer);
    _antenna_sat_para_save();
    timer_atn_show_signal_stop();
    app_search_start();
}

static void _antenna_ok_keypress(void)
{
    uint32_t box_sel;
    int cmb_sel;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);

    if((BOX_ITEM_SAT == box_sel && s_dvbs_sat_data.sat_total > 0)
            || (BOX_ITEM_LNB_POWER == box_sel)
            || (BOX_ITEM_22K == box_sel)
            || (BOX_ITEM_DISEQC10 == box_sel)
            || (BOX_ITEM_DISEQC11 == box_sel)
            || (BOX_ITEM_TP== box_sel && s_dvbs_sat_data.cur_sat_tp_total > 0))
    {
        GUI_GetProperty(s_antenna_combox[box_sel], "select", &cmb_sel);
        cmb_sel = _antenna_pop(box_sel, cmb_sel);
        return ;
    }

    if(BOX_ITEM_LNB_FREQ == box_sel)
    {
        #if TKGS_SUPPORT
        if(app_anternna_tkgs_key_handler())
        {
            return;
        }
        #endif
        GUI_GetProperty(s_antenna_combox[box_sel], "select", &cmb_sel);

        #if UNICABLE_SUPPORT
		if((ID_UNICABLE == cmb_sel) || (ID_UNICABLE_2 == cmb_sel))
        {
            app_unicable_init_if_channel_num(&s_dvbs_sat_data.cur_sat.sat_data);
            if(s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index >= ID_UNICANBLE_TATAL_LNB)
            {
                s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index = 0;
                app_lnb_freq_sel_to_data(s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index, &s_dvbs_sat_data.cur_sat.sat_data);
                app_log_info("!!!!!!!!!!lnb_freq_change!!!!!!!\n");
            }
            GUI_CreateDialog(WND_UNICABLE_SET);
            return;
        }

        if(0 == s_dvbs_sat_data.cur_sat_tp_total)
            _antenna_box_blind_search_display();
        else
            _antenna_box_sat_search_display();
        #endif
        cmb_sel = _antenna_pop(box_sel, cmb_sel);
        return ;
    }

    if(BOX_ITEM_SEARCH == box_sel)
    {
        char *search_str = NULL;
        GUI_GetProperty(s_antenna_combox[BOX_ITEM_SEARCH], "select_content", &search_str);
        s_antenna_setting_ops.start_search(s_dvbs_sat_data.cur_sat.sat_data.id,
						s_dvbs_sat_data.cur_tp_sel,search_str);
    }
    if((GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS) && (GUI_CheckDialog(WND_SEARCH) != GXCORE_SUCCESS))
    {
        GUI_SetProperty(BOX_ANTENNA_OPTIONS, "update", NULL);
    }
}

static void _antenna_sat_para_save(void)
{
    GxMsgProperty_NodeByPosGet node_get;
    GxMsgProperty_NodeModify node_modify;

    node_get.node_type = NODE_SAT;
    node_get.pos = s_dvbs_sat_data.cur_sat.pos;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

    if((node_get.sat_data.id != s_dvbs_sat_data.cur_sat.sat_data.id)
        || ((node_get.sat_data.sat_s.lnb_power == s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb_power)
            && (node_get.sat_data.sat_s.lnb1 == s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb1)
            && (node_get.sat_data.sat_s.lnb2 == s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb2)
            && (node_get.sat_data.sat_s.switch_22K == s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K)
            && (node_get.sat_data.sat_s.diseqc10 == s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc10)
            #if UNICABLE_SUPPORT
            && (node_get.sat_data.sat_s.unicable_para.type == s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type)
            && (node_get.sat_data.sat_s.unicable_para.if_channel== s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.if_channel)
            && (node_get.sat_data.sat_s.unicable_para.centre_fre[node_get.sat_data.sat_s.unicable_para.if_channel]== s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.centre_fre[s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.if_channel])
            && (node_get.sat_data.sat_s.unicable_para.lnb_fre_index== s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index)
            && (node_get.sat_data.sat_s.unicable_para.sat_pos== s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.sat_pos)
            #endif
            && (node_get.sat_data.sat_s.diseqc11 == s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc11)))
        return;

    //lnb power
    node_get.sat_data.sat_s.lnb_power = s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb_power;
    //lnb freq
    node_get.sat_data.sat_s.lnb1 = s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb1;
    node_get.sat_data.sat_s.lnb2 = s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb2;
    //22k
    node_get.sat_data.sat_s.switch_22K = s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K;
    //diseqc 1.0
    node_get.sat_data.sat_s.diseqc10 = s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc10;
    //diseqc 1.1
    node_get.sat_data.sat_s.diseqc11 = s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc11;

    #if UNICABLE_SUPPORT
    int index = 0;
    //centre fre
    index = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.if_channel;
    node_get.sat_data.sat_s.unicable_para.type = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type;
    node_get.sat_data.sat_s.unicable_para.centre_fre[index] = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.centre_fre[index];
    node_get.sat_data.sat_s.unicable_para.lnb_fre_index = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index;
    node_get.sat_data.sat_s.unicable_para.if_channel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.if_channel;
    node_get.sat_data.sat_s.unicable_para.sat_pos = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.sat_pos;
    #endif

    //save
    node_modify.node_type = NODE_SAT;
    memcpy(&node_modify.sat_data, &node_get.sat_data, sizeof(GxBusPmDataSat));
    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);

    s_antenna_setting_ops.save_sat_params(node_get.sat_data.id,&node_get.sat_data);

    return;
}
static void app_antenna_skip_tplist_green_func(void)
{
    if(sp_AtnTimerSignal != NULL)
        timer_stop(sp_AtnTimerSignal);

    _antenna_sat_para_save();
#if DOUBLE_S2_SUPPORT
    uint32_t cur_sat_pos = s_dvbs_sat_data.cur_sat.pos;
    s_dvbs_sat_data.cur_tuner = TUNER_ALL;
    s_dvbs_sat_data.sat_rebuild(NULL, false);
    s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = s_dvbs_sat_data.sat_gui_sel_get_by_pos(cur_sat_pos);
    s_dvbs_sat_data.cur_sat_get();
#endif
#if SUPER_BLIND_SUPPORT
    _antenna_super_search_disable();
#endif
    app_tp_list_init(s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner], s_dvbs_sat_data.cur_tp_sel);

    return;
}

static void app_antenna_skip_satlist_red_func(void)
{
    if(sp_AtnTimerSignal != NULL)
        timer_stop(sp_AtnTimerSignal);

    _antenna_sat_para_save();
#if DOUBLE_S2_SUPPORT
    uint32_t cur_sat_pos = s_dvbs_sat_data.cur_sat.pos;
    s_dvbs_sat_data.cur_tuner = TUNER_ALL;
    s_dvbs_sat_data.sat_rebuild(NULL, false);
    s_dvbs_sat_data.cur_sat_sel[TUNER_ALL] = s_dvbs_sat_data.sat_gui_sel_get_by_pos(cur_sat_pos);
    s_dvbs_sat_data.cur_sat_get();
#endif
#if SUPER_BLIND_SUPPORT
    _antenna_super_search_disable();
#endif
    app_satellite_list_init(s_dvbs_sat_data.cur_sat_sel[TUNER_ALL]);

    return;
}

static int app_antenna_setting_display_update(void)
{
    int value = 0;
    uint32_t cur_sel = 0;
    uint32_t freq_sel = 0;

    if((GXBUS_PM_SAT_DVBT2 == s_dvbs_sat_data.cur_sat.sat_data.type)
            || (GXBUS_PM_SAT_C == s_dvbs_sat_data.cur_sat.sat_data.type)
            || (GXBUS_PM_SAT_DTMB == s_dvbs_sat_data.cur_sat.sat_data.type))
    {
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_LNB_POWER], "disable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_LNB_FREQ], "disable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_22K], "disable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC10], "disable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC11], "disable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_SEARCH], "disable", NULL);

        if(0 == s_dvbs_sat_data.cur_sat_tp_total)
            _antenna_box_blind_search_display();
        else
            _antenna_box_sat_search_display();
    }
    else
    {
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_LNB_POWER], "enable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_LNB_FREQ], "enable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_22K], "enable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC10], "enable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC11], "enable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_SEARCH], "enable", NULL);
        #if DOUBLE_S2_SUPPORT
        if(TUNER_ALL == s_dvbs_sat_data.which_tuner_sat_exist)
            GUI_SetProperty(CMBOX_ANTENNA_TUNER, "select", &s_dvbs_sat_data.cur_tuner);
        #endif

        GUI_SetProperty(s_antenna_combox[BOX_ITEM_SAT], "select", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
        cur_sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb_power;
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_LNB_POWER], "select", &cur_sel);
        app_lnb_freq_data_to_sel(&(s_dvbs_sat_data.cur_sat.sat_data), &freq_sel);
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_LNB_FREQ], "select", &freq_sel);
        //disable 22K or not
        #if UNICABLE_SUPPORT
		if((freq_sel == ID_9750_10600)
                || (freq_sel == ID_9750_10700)
                || (freq_sel == ID_9750_10750)
                || (((freq_sel == ID_UNICABLE) || (freq_sel == ID_UNICABLE_2))
                    &&(app_cur_sat_unicable_type_check())))
        #else
        if(freq_sel < UNIVERSAL_NUM && freq_sel >= 0)
        #endif
        {
            GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "content", ANTENNA_22K_AUTO_CONTENT);
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_22K], "disable", NULL);
            s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
        }
        else
        {
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_22K], "enable", NULL);
            GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "content", ANTENNA_22K_CONTENT);
            cur_sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K;
            GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "select", &cur_sel);
        }

        cur_sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc10;
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_DISEQC10], "select", &cur_sel);

        cur_sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc11;
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_DISEQC11], "select", &cur_sel);
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_TP],"select", &s_dvbs_sat_data.cur_tp_sel);

        #if UNICABLE_SUPPORT
		if((freq_sel == ID_UNICABLE) || (freq_sel == ID_UNICABLE_2))
        {
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC10], "disable", NULL);
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC11], "disable", NULL);
        }
        else
        {
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC10], "enable", NULL);
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC11], "enable", NULL);
        }

        if(app_cur_sat_unicable_type_check())
        {
            s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SAT_SEARCH_CONTENT_UNICABLE);
        }
        else
        #endif
        {
            if(0 == s_dvbs_sat_data.cur_sat_tp_total)
            {
                _antenna_box_blind_search_display();
                GUI_SetProperty("progbar_antenna_strength", "value", &value);
                GUI_SetProperty("progbar_antenna_quality", "value", &value);
            }
            else
            {
                _antenna_box_sat_search_display();
            }
        }
    }
    return 0;
}

SIGNAL_HANDLER int app_antenna_setting_create(GuiWidget *widget, void *usrdata)
{
#if SAT2IP_SERVER_SUPPORT
    extern void app_sat2ip_set_dialog_pause(void);
    app_sat2ip_set_dialog_pause();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif

    int value = 0;

#if (0 == DOUBLE_S2_SUPPORT)
    s_dvbs_sat_data.cur_tuner = TUNER_ALL;
#endif

    GUI_SetProperty(s_antenna_combox[BOX_ITEM_LNB_POWER], "content", LNB_POWER_CONTENT);
    GUI_SetProperty(s_antenna_combox[BOX_ITEM_LNB_FREQ], "content", LNB_FREQ_CONTENT);
    GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "content", ANTENNA_22K_CONTENT);
    GUI_SetProperty(s_antenna_combox[BOX_ITEM_DISEQC10], "content", DISEQC10_CONTENT);
    GUI_SetProperty(s_antenna_combox[BOX_ITEM_DISEQC11], "content", DISEQC11_CONTENT);
    app_antenna_clean_signal();
    app_panel_ioctl_handle(PANEL_SHOW_QUALITY, &value);
    s_dvbs_sat_data.change &= ~(ANT_SET_UPDATE_SATINFO | ANT_SET_UPDATE_TPINFO);
#if DOUBLE_S2_SUPPORT
    s_dvbs_sat_data.tuner_rebuild(CMBOX_ANTENNA_TUNER);
#endif
    uint32_t cur_sat_pos = s_dvbs_sat_data.cur_sat.pos;
    s_dvbs_sat_data.sat_rebuild(s_antenna_combox[BOX_ITEM_SAT], false);
    s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = s_dvbs_sat_data.sat_gui_sel_get_by_pos(cur_sat_pos);
    s_dvbs_sat_data.tp_rebuild(s_antenna_combox[BOX_ITEM_TP]);

    s_dvbs_sat_data.set_tp(SAVE_TP_PARAM);
    s_tp_total_bak = s_dvbs_sat_data.cur_sat_tp_total;

    app_antenna_setting_display_update();

    s_dvbs_sat_data.set_diseqc(NULL);
    s_dvbs_sat_data.set_tp(SET_TP_FORCE);
    APP_TIMER_ADD(sp_AtnTimerSignal, timer_atn_show_signal, 100, TIMER_REPEAT);
#if SUPER_BLIND_SUPPORT
    _antenna_super_search_disable();
#endif
    app_panel_show_prog_num(PANEL_UNDISPLAY_TIME);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_antenna_setting_destroy(GuiWidget *widget, void *usrdata)
{
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    APP_TIMER_REMOVE(sp_AtnTimerSignal);
    APP_TIMER_REMOVE(sp_AtnLockTimer);
    //save current sat para
    _antenna_sat_para_save();
#if SUPER_BLIND_SUPPORT
    _antenna_super_search_disable();
#endif
    app_panel_show_prog_num(PANEL_DISPLAY_TIME);

    s_antenna_setting_ops.uninit();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_antenna_setting_got_focus(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel;

    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
    GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);

    char *search_str = NULL;
    GUI_GetProperty(s_antenna_combox[BOX_ITEM_SEARCH], "select_content", &search_str);
    s_antenna_setting_ops.get_focus(search_str);

#if (SUPER_BLIND_SUPPORT == 0)
    GUI_SetProperty(TXT_TIP_BLUE, "state", "hide");
    GUI_SetProperty(IMG_TIP_BLUE, "state", "hide");
#endif

    GxMsgProperty_NodeByPosGet node_sat = {0};
	node_sat.node_type = NODE_SAT;
	node_sat.pos = s_dvbs_sat_data.sat_buf[s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]];
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);

	int sat_total = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner);
	if((sat_total != s_dvbs_sat_data.sat_total)
            || (s_dvbs_sat_data.change & ANT_SET_UPDATE_SATINFO)
			|| (memcmp(&(node_sat.sat_data), &(s_dvbs_sat_data.cur_sat.sat_data), sizeof(GxBusPmDataSat))))
	{
		if(sat_total == 0)
		{
			GUI_SetProperty(s_antenna_combox[BOX_ITEM_SAT],"content","NONE");
			s_dvbs_sat_data.sat_total = 0;
		}
		else
		{
#if DOUBLE_S2_SUPPORT
            s_dvbs_sat_data.tuner_rebuild(CMBOX_ANTENNA_TUNER);
#endif
			uint32_t cur_sat_pos = s_dvbs_sat_data.cur_sat.pos;
			s_dvbs_sat_data.sat_rebuild(s_antenna_combox[BOX_ITEM_SAT],false);
			s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = s_dvbs_sat_data.sat_gui_sel_get_by_pos(cur_sat_pos);
			GUI_SetProperty(s_antenna_combox[BOX_ITEM_SAT], "select", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
			app_antenna_setting_display_update();
		}
	}

    if(s_dvbs_sat_data.sat_total > 0)
    {
        s_dvbs_sat_data.cur_sat_tp_total = s_dvbs_sat_data.cur_sat_tp_num_get();

        if(s_tp_total_bak != s_dvbs_sat_data.cur_sat_tp_total //after blind scan
                || (s_dvbs_sat_data.change & ANT_SET_UPDATE_TPINFO)
                || true == thiz_super_search)//经过超级搜索后tp里可能更新了pls_n,pdmx,multi_ts等信息
        {
        #if UNICABLE_SUPPORT
            if(app_cur_sat_unicable_type_check())
            {
                s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SAT_SEARCH_CONTENT_UNICABLE);
            }
            else
        #endif
            {
                if(0 == s_dvbs_sat_data.cur_sat_tp_total)
                {
                    _antenna_box_blind_search_display();
                }
                else if(0 == s_tp_total_bak)
                {
                    box_sel = 1;
                    _antenna_box_sat_search_display();
                    GUI_SetProperty(s_antenna_combox[BOX_ITEM_SEARCH], "select", &box_sel);
                }
            }
            s_dvbs_sat_data.tp_rebuild(s_antenna_combox[BOX_ITEM_TP]);
            GUI_SetProperty(s_antenna_combox[BOX_ITEM_TP],"select", &s_dvbs_sat_data.cur_tp_sel);
        }

#if LNB_SHORT_DETECT_SUPPORT
        GxBusPmDataSat sat = {0};
        if(GXCORE_SUCCESS == GxBus_PmSatGetById(s_dvbs_sat_data.cur_sat.sat_data.id, &sat))
        {
            int cur_sel = 0;

            GUI_GetProperty(s_antenna_combox[BOX_ITEM_LNB_POWER], "select", &cur_sel);

            if(GXBUS_PM_SAT_S == sat.type && (sat.sat_s.lnb_power != s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb_power || cur_sel != sat.sat_s.lnb_power))
            {
                cur_sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb_power = sat.sat_s.lnb_power;
                GUI_SetProperty(s_antenna_combox[BOX_ITEM_LNB_POWER], "select", &cur_sel);
            }
        }
#endif

        s_tp_total_bak = s_dvbs_sat_data.cur_sat_tp_total;

        APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

        timer_atn_show_signal_reset();
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_antenna_setting_lost_focus(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel;
#if DOUBLE_S2_SUPPORT
    if(strcmp(GUI_GetFocusWidget(),CMBOX_ANTENNA_TUNER) !=0)
#endif
    {
        GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
        GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_FOCUS);
    }

    // hide the help info
    GUI_SetProperty(ANTENNA_SETTING_HELP1_IMG, "state", "hide");
    GUI_SetProperty(ANTENNA_SETTING_HELP1_TXT, "state", "hide");
    GUI_SetProperty(ANTENNA_SETTING_HELP2_IMG, "state", "hide");
    GUI_SetProperty(ANTENNA_SETTING_HELP2_TXT, "state", "hide");
    GUI_SetProperty(ANTENNA_SETTING_HELP3_IMG, "state", "hide");
    GUI_SetProperty(ANTENNA_SETTING_HELP3_TXT, "state", "hide");
    if(sp_AtnTimerSignal)
        timer_stop(sp_AtnTimerSignal);
    s_dvbs_sat_data.change &= ~(ANT_SET_UPDATE_SATINFO | ANT_SET_UPDATE_TPINFO);
    _antenna_sat_para_save();

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_antenna_setting_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;
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
                    {
                        //update gui sel in the mode of TUNER_ALL
                    #if DOUBLE_S2_SUPPORT
                        GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

                        memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
                        s_dvbs_sat_data.cur_tuner = TUNER_ALL;
                        s_dvbs_sat_data.sat_gui_sel_update();
                        //reset original tuner and sat data
                        memcpy(&s_dvbs_sat_data.cur_sat, &cur_sat_bak, sizeof(GxMsgProperty_NodeByPosGet));
                        s_dvbs_sat_data.cur_tuner = cur_sat_bak.sat_data.tuner;
                    #endif
                        GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                        GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
                        GUI_EndDialog("after wnd_full_screen");
                        GUI_SetInterface("flush", NULL);
                    }
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST)
                                || GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST))
                        {
                            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST))
                                app_antenna_skip_tplist_green_func();
                            else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST))
                                app_antenna_skip_satlist_red_func();

                            GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                            GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
                            GUI_EndDialog(WND_ANTENNA_SETTING);
                        }
                        else
                        {
                            //update gui sel in the mode of TUNER_ALL
                            #if DOUBLE_S2_SUPPORT
                            GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

                            memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
                            s_dvbs_sat_data.cur_tuner = TUNER_ALL;
                            s_dvbs_sat_data.sat_gui_sel_update();
                            //reset original tuner and sat data
                            memcpy(&s_dvbs_sat_data.cur_sat, &cur_sat_bak, sizeof(GxMsgProperty_NodeByPosGet));
                            s_dvbs_sat_data.cur_tuner = cur_sat_bak.sat_data.tuner;
                            #endif
                            GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                            GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
                            GUI_EndDialog(WND_ANTENNA_SETTING);
                        }

                    }
                    break;

                case STBK_OK:
                    {

#if DOUBLE_S2_SUPPORT
                        const char *focus_widget = NULL;

                        focus_widget = GUI_GetFocusWidget();
                        if(focus_widget != NULL
                                && strcmp(CMBOX_ANTENNA_TUNER, focus_widget))
#endif
                            _antenna_ok_keypress();
                    }
                    break;

                case STBK_RED:
                    if ( s_antenna_setting_ops.key_func_press() )
                        return ret ;
                    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST))
                    {
                        GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
                        GUI_EndDialog(WND_ANTENNA_SETTING);
                    }
                    app_antenna_skip_satlist_red_func();
                    break;

                case STBK_GREEN:
                    if ( s_antenna_setting_ops.key_func_press() )
                        return ret ;
                    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST))
                    {
                        GUI_SetProperty(s_antenna_boxitem[box_sel], "unfocus_image", IMG_BAR_LONG_UNFOCUS);
                        GUI_EndDialog(WND_ANTENNA_SETTING);

                    }
                    app_antenna_skip_tplist_green_func();
                    break;

#if SUPER_BLIND_SUPPORT
                case STBK_BLUE:
                    if ( s_antenna_setting_ops.key_func_press() )
                        return ret ;
                    if(false == thiz_super_search)
                        _antenna_super_search_enable();
                    else
                        _antenna_super_search_disable();

                    app_antenna_setting_display_update();

                    char *search_str = NULL;
                    GUI_GetProperty(s_antenna_combox[BOX_ITEM_SEARCH], "select_content", &search_str);
                    s_antenna_setting_ops.combox_search_change(s_dvbs_sat_data.cur_sat.sat_data.id,search_str);

                    break;
#endif

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_atn_cmb_tuner_change(GuiWidget *widget, void *usrdata)
{
    uint32_t cur_tuner_bak = 0;

    if(s_dvbs_sat_data.which_tuner_sat_exist != TUNER_ALL)
        return EVENT_TRANSFER_STOP;

    _antenna_sat_para_save();
    cur_tuner_bak = s_dvbs_sat_data.cur_tuner;
    GUI_GetProperty(CMBOX_ANTENNA_TUNER, "select", &s_dvbs_sat_data.cur_tuner);

    if(cur_tuner_bak == s_dvbs_sat_data.cur_tuner)
        return EVENT_TRANSFER_STOP;

    s_dvbs_sat_data.sat_rebuild(s_antenna_combox[BOX_ITEM_SAT], false);
    s_dvbs_sat_data.tp_rebuild(s_antenna_combox[BOX_ITEM_TP]);

    s_dvbs_sat_data.set_tp(SAVE_TP_PARAM);
    s_tp_total_bak = s_dvbs_sat_data.cur_sat_tp_total;

    app_antenna_setting_display_update();

    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_tuner_keypress(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel;
    int ret = EVENT_TRANSFER_KEEPON;
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
                case STBK_LEFT:
                case STBK_RIGHT:
                    if(!(s_dvbs_sat_data.sat_num_get(TUNER1) > 0
                                && s_dvbs_sat_data.sat_num_get(TUNER2) > 0))
                        ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_UP:
                case STBK_DOWN:
                    GUI_SetFocusWidget(BOX_ANTENNA_OPTIONS);
                    if(STBK_UP == find_virtualkey_ex(event->key.scancode,event->key.sym))
                        box_sel = BOX_ITEM_SEARCH;
                    else
                        box_sel = BOX_ITEM_SAT;
                    GUI_SetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                    ret = EVENT_TRANSFER_STOP;
                    break;
            }
            break;

        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_atn_cmb_sat_change(GuiWidget *widget, void *usrdata)
{
    uint32_t sel;
    GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

    //save current sat para
    _antenna_sat_para_save();

    memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));

    //show new sat
    GUI_GetProperty(s_antenna_combox[BOX_ITEM_SAT], "select", &sel);
    s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = sel;
    s_dvbs_sat_data.cur_sat_get();

    if(cur_sat_bak.sat_data.id == s_dvbs_sat_data.cur_sat.sat_data.id)
        return EVENT_TRANSFER_STOP;

    s_dvbs_sat_data.cur_tp_sel = 0;
    s_dvbs_sat_data.tp_rebuild(s_antenna_combox[BOX_ITEM_TP]);
    s_dvbs_sat_data.set_tp(SAVE_TP_PARAM);
    s_tp_total_bak = s_dvbs_sat_data.cur_sat_tp_total;

    app_antenna_setting_display_update();

    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_lnb_power_change(GuiWidget *widget, void *usrdata)
{
    uint32_t cmb_sel;

    GUI_GetProperty(s_antenna_combox[BOX_ITEM_LNB_POWER], "select", &cmb_sel);
    s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb_power = cmb_sel;

    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_lnb_freq_change(GuiWidget *widget, void *usrdata)
{
    uint32_t freq_sel = 0;
    uint32_t s22k_sel = 0;

    GUI_GetProperty(s_antenna_combox[BOX_ITEM_LNB_FREQ], "select", &freq_sel);
    //disable 22K
#if UNICABLE_SUPPORT
	if((ID_UNICABLE == freq_sel) || (ID_UNICABLE_2 == freq_sel))
    {
        GxBusPmSatUnicableType cur_type = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type;

        if(cur_type != _antenna_lnb_fre_sel_to_unicable_type(freq_sel))
            s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type = _antenna_lnb_fre_sel_to_unicable_type(freq_sel);

        if(s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index >= ID_UNICANBLE_TATAL_LNB)
            s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index = 0;

        app_lnb_freq_sel_to_data(s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.lnb_fre_index, &s_dvbs_sat_data.cur_sat.sat_data);
        app_unicable_init_if_channel_num(&s_dvbs_sat_data.cur_sat.sat_data);
        app_log_info("!!!!!!!!!!app_atn_cmb_lnb_freq_change!!!!!!!\n");
        s_antenna_setting_ops.combox_search_update(s_dvbs_sat_data.cur_sat.sat_data.id,ANTENNA_SAT_SEARCH_CONTENT_UNICABLE);
    }
    else
    {
        s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type = GXBUS_PM_SAT_UNICABLE_OFF;
        app_lnb_freq_sel_to_data(freq_sel, &s_dvbs_sat_data.cur_sat.sat_data);
        if(0 == s_dvbs_sat_data.cur_sat_tp_total)
            _antenna_box_blind_search_display();
        else
            _antenna_box_sat_search_display();
    }

    //disable 22K
    if(freq_sel == ID_9750_10600
            || freq_sel == ID_9750_10700
            || freq_sel == ID_9750_10750
            || (((freq_sel == ID_UNICABLE)||(freq_sel == ID_UNICABLE_2))
                &&(app_cur_sat_unicable_type_check()) ))
    {
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "content", ANTENNA_22K_AUTO_CONTENT);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_22K], "disable", NULL);
        s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
		if((freq_sel == ID_UNICABLE) ||(freq_sel == ID_UNICABLE_2))
        {
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC10], "disable", NULL);
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC11], "disable", NULL);
        }
        else
        {
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC10], "enable", NULL);
            GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC11], "enable", NULL);
        }
    }
    // error
    else
    {
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_22K], "enable", NULL);
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "content", ANTENNA_22K_CONTENT);

        // for 22K record
        if(s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K == GXBUS_PM_SAT_22K_AUTO)
            s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K = s_n22KSelectRecord;
        s22k_sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K;

        GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "select", &s22k_sel);
    }

	if((freq_sel == ID_UNICABLE) || (freq_sel == ID_UNICABLE_2))
    {
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC10], "disable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC11], "disable", NULL);
    }
    else
    {
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC10], "enable", NULL);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_DISEQC11], "enable", NULL);
    }

    #else
    app_lnb_freq_sel_to_data(freq_sel, &s_dvbs_sat_data.cur_sat.sat_data);
    if(freq_sel < UNIVERSAL_NUM && freq_sel >= 0)
    {
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "content", ANTENNA_22K_AUTO_CONTENT);
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_22K], "disable", NULL);
        s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
    }
    // error
    else
    {
        GUI_SetProperty(s_antenna_boxitem[BOX_ITEM_22K], "enable", NULL);
        GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "content", ANTENNA_22K_CONTENT);

        // for 22K record
        if(s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K == GXBUS_PM_SAT_22K_AUTO)
            s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K = s_n22KSelectRecord;
        s22k_sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K;

        GUI_SetProperty(s_antenna_combox[BOX_ITEM_22K], "select", &s22k_sel);
    }
    #endif

    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_22k_change(GuiWidget *widget, void *usrdata)
{
    uint32_t sel_lnb;
    uint32_t sel_22k;

    GUI_GetProperty(s_antenna_combox[BOX_ITEM_LNB_FREQ], "select", &sel_lnb);
    if(sel_lnb >= UNIVERSAL_NUM)
    {
        GUI_GetProperty(s_antenna_combox[BOX_ITEM_22K], "select", &sel_22k);
        s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K = sel_22k;
        // for 22K record
        s_n22KSelectRecord = sel_22k;
    }

    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_diseqc10_change(GuiWidget *widget, void *usrdata)
{
    uint32_t cmb_sel;

    GUI_GetProperty(s_antenna_combox[BOX_ITEM_DISEQC10], "select", &cmb_sel);
    s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc10 = cmb_sel;

    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_diseqc11_change(GuiWidget *widget, void *usrdata)
{
    uint32_t cmb_sel;

    GUI_GetProperty(s_antenna_combox[BOX_ITEM_DISEQC11], "select", &cmb_sel);
    s_dvbs_sat_data.cur_sat.sat_data.sat_s.diseqc11 = cmb_sel;

    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_cmb_tp_change(GuiWidget *widget, void *usrdata)
{
    uint32_t cmb_sel;

    GUI_GetProperty(s_antenna_combox[BOX_ITEM_TP], "select", &cmb_sel);
    s_dvbs_sat_data.cur_tp_sel = cmb_sel;
    s_dvbs_sat_data.cur_tp_get();

    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_atn_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;


    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            if ( s_antenna_setting_ops.key_check(event->key.sym) )
            {
                return EVENT_TRANSFER_STOP;
            }
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_UP:
                    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                    if(BOX_ITEM_SAT == box_sel)
                    {
#if DOUBLE_S2_SUPPORT
                        if( APP_THEME_CLASSIC_BLUE ){
                            GUI_SetFocusWidget(CMBOX_ANTENNA_TUNER);
                        }else{
                            int32_t box_sel = BOX_ITEM_SEARCH;
                            GUI_SetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                        }
#else
                        int32_t box_sel = BOX_ITEM_SEARCH;
                        GUI_SetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
#endif
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                        ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_DOWN:
                    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                    if(BOX_ITEM_SEARCH == box_sel)
                    {
#if DOUBLE_S2_SUPPORT
                        if( APP_THEME_CLASSIC_BLUE ){
                            GUI_SetFocusWidget(CMBOX_ANTENNA_TUNER);
                        }else{
                            int32_t box_sel = BOX_ITEM_SAT;
                            GUI_SetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                        }
#else
                        int32_t box_sel = BOX_ITEM_SAT;
                        GUI_SetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
#endif
                        ret = EVENT_TRANSFER_STOP;
                    }
                    else
                        ret = EVENT_TRANSFER_KEEPON;
                    break;

                #if TKGS_SUPPORT
                case STBK_LEFT:
                case STBK_RIGHT:
                {
                    int cmb_sel;
                    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &cmb_sel);
                    if(cmb_sel == BOX_ITEM_LNB_FREQ)
                    {
                        app_log_debug("function = %s,line%d\n",__FUNCTION__,__LINE__);
                        if(app_anternna_tkgs_key_handler())
                        {
                            ret = EVENT_TRANSFER_STOP;
                            break;
                        }
                    }

                    ret = EVENT_TRANSFER_KEEPON;

                }
                break;
                #endif
                case STBK_OK:
                case STBK_MENU:
                case STBK_EXIT:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                #if TKGS_SUPPORT == 0
                case STBK_LEFT:
                case STBK_RIGHT:
                {
                    GUI_GetProperty(BOX_ANTENNA_OPTIONS, "select", &box_sel);
                    if(box_sel == BOX_ITEM_SEARCH)
                    {
                        char *search_str = NULL;
                        GUI_GetProperty(s_antenna_combox[BOX_ITEM_SEARCH], "select_content", &search_str);
                        s_antenna_setting_ops.combox_search_change(s_dvbs_sat_data.cur_sat.sat_data.id,search_str);
                    }
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
                }
                #endif
                case STBK_RED:
                case STBK_GREEN:
                case STBK_BLUE:
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


#if UNICABLE_SUPPORT
static GxBusPmSatUnicableType _antenna_lnb_fre_sel_to_unicable_type(int lnb_fre_sel)
{
    GxBusPmSatUnicableType type = GXBUS_PM_SAT_UNICABLE_OFF;

    if(ID_UNICABLE == lnb_fre_sel)
        type = GXBUS_PM_SAT_UNICABLE;
    else if(ID_UNICABLE_2 == lnb_fre_sel)
        type = GXBUS_PM_SAT_UNICABLE2;
    else
        type = GXBUS_PM_SAT_UNICABLE_OFF;

    return type;
}


int app_antenna_get_cur_sat_info(GxMsgProperty_NodeByPosGet* p_node_sat )
{
    memcpy(p_node_sat,&s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
    return 0;
}

int app_antenna_timer_unicable_set_frontend(void)
{
    APP_TIMER_ADD(sp_AtnLockTimer, timer_antenna_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);
    return 0;
}

int app_antenna_unicable_sat_para_save(GxMsgProperty_NodeByPosGet* p_node_sat )
{
    GxMsgProperty_NodeByPosGet node_get;
    GxMsgProperty_NodeModify node_modify;
    int index = 0;

    if(p_node_sat!=NULL)
    {
        memcpy(&s_dvbs_sat_data.cur_sat,p_node_sat, sizeof(GxMsgProperty_NodeByPosGet));
    }

    node_get.node_type = NODE_SAT;
    node_get.pos = s_dvbs_sat_data.cur_sat.pos;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

    app_lnb_freq_sel_to_data(p_node_sat->sat_data.sat_s.unicable_para.lnb_fre_index, &p_node_sat->sat_data);
    if((node_get.sat_data.id != p_node_sat->sat_data.id)
            || ((node_get.sat_data.sat_s.unicable_para.if_channel== p_node_sat->sat_data.sat_s.unicable_para.if_channel)
                && (node_get.sat_data.sat_s.unicable_para.type == p_node_sat->sat_data.sat_s.unicable_para.type)
                && (node_get.sat_data.sat_s.unicable_para.centre_fre[node_get.sat_data.sat_s.unicable_para.if_channel]== p_node_sat->sat_data.sat_s.unicable_para.centre_fre[p_node_sat->sat_data.sat_s.unicable_para.if_channel])
                && (node_get.sat_data.sat_s.unicable_para.lnb_fre_index== p_node_sat->sat_data.sat_s.unicable_para.lnb_fre_index)
                && (node_get.sat_data.sat_s.unicable_para.sat_pos== p_node_sat->sat_data.sat_s.unicable_para.sat_pos)
                && (node_get.sat_data.sat_s.lnb1 == p_node_sat->sat_data.sat_s.lnb1)
                && (node_get.sat_data.sat_s.lnb2 == p_node_sat->sat_data.sat_s.lnb2)))
    {
        return 1;
    }

    //centre fre
    index = s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.if_channel;
    node_get.sat_data.sat_s.unicable_para.type = p_node_sat->sat_data.sat_s.unicable_para.type;
    node_get.sat_data.sat_s.unicable_para.centre_fre[index] = p_node_sat->sat_data.sat_s.unicable_para.centre_fre[index];
    node_get.sat_data.sat_s.unicable_para.lnb_fre_index = p_node_sat->sat_data.sat_s.unicable_para.lnb_fre_index;
    node_get.sat_data.sat_s.unicable_para.if_channel = p_node_sat->sat_data.sat_s.unicable_para.if_channel;
    node_get.sat_data.sat_s.unicable_para.sat_pos = p_node_sat->sat_data.sat_s.unicable_para.sat_pos;

    //lnb
    node_get.sat_data.sat_s.lnb1 = p_node_sat->sat_data.sat_s.lnb1;
    node_get.sat_data.sat_s.lnb2 = p_node_sat->sat_data.sat_s.lnb2;

    //save
    node_modify.node_type = NODE_SAT;
    memcpy(&node_modify.sat_data, &node_get.sat_data, sizeof(GxBusPmDataSat));
    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);

    return 0;
}
#endif

void app_antenna_setting_menu_exec(void)
{
    int dvbs_sat_num = 0;
    dvbs_sat_num = s_dvbs_sat_data.sat_num_get(TUNER_ALL);
    if(dvbs_sat_num > 0)
    {
#if SAT2IP_SERVER_SUPPORT
        extern int app_sat2ip_operate_tip_get_force(void);
        if (app_sat2ip_operate_tip_get_force() < 0)
            return;
#endif
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_operate_tip_get_force() < 0)
            return;
#endif
        GUI_CreateDialog(WND_ANTENNA_SETTING);
    }
    else
    {
        app_mainmenu_tip_exec(MM_NO_SAT);
    }
}

#if WEBAPI_SUPPORT
#include <cJSON.h>

cJSON *app_webapi_make_antenna_data(uint32_t sat_id)
{
	GxMsgProperty_NodeByIdGet sat_node = {0};
	cJSON *json = NULL;
	cJSON *json1 = NULL;
	int i = 0;
	uint32_t freq_sel = 0;

	memset(&sat_node, 0, sizeof(GxMsgProperty_NodeByIdGet));
	sat_node.node_type = NODE_SAT;
	sat_node.id = sat_id;
	if((GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node))
		&&(sat_node.sat_data.type == GXBUS_PM_SAT_S))
	{
		json = cJSON_CreateObject();

		cJSON_AddNumberToObject(json, "lnb_power", sat_node.sat_data.sat_s.lnb_power);
		json1 = cJSON_CreateArray();
		cJSON_AddItemToObject(json, "lnb_power_options", json1);
		for(i=0; i<sizeof(s_LnbPower)/sizeof(*s_LnbPower); i++)
		{
			cJSON_AddItemToArray(json1, cJSON_CreateString(s_LnbPower[i]));
		}

		app_lnb_freq_data_to_sel(&(sat_node.sat_data), &freq_sel);
		cJSON_AddNumberToObject(json, "lnb_frequency", freq_sel);
		json1 = cJSON_CreateArray();
		cJSON_AddItemToObject(json, "lnb_frequency_options", json1);
		for(i=0; i<sizeof(s_LnbFreq)/sizeof(*s_LnbFreq); i++)
		{
			cJSON_AddItemToArray(json1, cJSON_CreateString(s_LnbFreq[i]));
		}

		cJSON_AddNumberToObject(json, "switch_22k", sat_node.sat_data.sat_s.switch_22K);
		json1 = cJSON_CreateArray();
		cJSON_AddItemToObject(json, "switch_22k_options", json1);
		for(i=0; i<sizeof(s_22k)/sizeof(*s_22k); i++)
		{
			cJSON_AddItemToArray(json1, cJSON_CreateString(s_22k[i]));
		}

		cJSON_AddNumberToObject(json, "diseqc10", sat_node.sat_data.sat_s.diseqc10);
		json1 = cJSON_CreateArray();
		cJSON_AddItemToObject(json, "diseqc10_options", json1);
		for(i=0; i<sizeof(s_Diseqc10)/sizeof(*s_Diseqc10); i++)
		{
			cJSON_AddItemToArray(json1, cJSON_CreateString(s_Diseqc10[i]));
		}

		cJSON_AddNumberToObject(json, "diseqc11", sat_node.sat_data.sat_s.diseqc11);
		json1 = cJSON_CreateArray();
		cJSON_AddItemToObject(json, "diseqc11_options", json1);
		for(i=0; i<sizeof(s_Diseqc11)/sizeof(*s_Diseqc11); i++)
		{
			cJSON_AddItemToArray(json1, cJSON_CreateString(s_Diseqc11[i]));
		}
	}

	return json;
}

int app_webapi_set_antenna_data(
					uint32_t sat_id,
					int power_sel,
					int freq_sel,
					int s22k_sel,
					int diseqc10_sel,
					int diseqc11_sel)
{
	int ret = -1;
	GxMsgProperty_NodeByIdGet sat_node = {0};
	GxMsgProperty_NodeModify node_modify = {0};

	if((sat_id>0)&&(power_sel>=0)&&(power_sel<sizeof(s_LnbPower)/sizeof(*s_LnbPower))
		&&(freq_sel>=0)&&(freq_sel<sizeof(s_LnbFreq)/sizeof(*s_LnbFreq))
		&&(s22k_sel>=0)
		&&(diseqc10_sel>=0)&&(diseqc10_sel<sizeof(s_Diseqc10)/sizeof(*s_Diseqc10))
		&&(diseqc11_sel>=0)&&(diseqc11_sel<sizeof(s_Diseqc11)/sizeof(*s_Diseqc11)))
	{
		sat_node.node_type = NODE_SAT;
		sat_node.id = sat_id;
		if((GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node))
			&&(sat_node.sat_data.type == GXBUS_PM_SAT_S))
		{
			memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
			node_modify.node_type = NODE_SAT;
			memcpy(&node_modify.sat_data, &sat_node.sat_data, sizeof(GxBusPmDataSat));
			node_modify.sat_data.sat_s.lnb_power = power_sel;
			app_lnb_freq_sel_to_data(freq_sel, &node_modify.sat_data);
			if(s22k_sel<sizeof(s_22k)/sizeof(*s_22k))
			{
				node_modify.sat_data.sat_s.switch_22K = s22k_sel;
			}
			else
			{
				node_modify.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_AUTO;
			}
			node_modify.sat_data.sat_s.diseqc10 = diseqc10_sel;
			node_modify.sat_data.sat_s.diseqc11 = diseqc11_sel;
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
			{
				ret = 0;
			}
		}
	}

	return ret;
}

#endif

#endif

