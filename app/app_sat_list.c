#include "app.h"
#include "app_windows.h"
#if (DEMOD_DVB_S > 0)

#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "app_default_params.h"
#include "full_screen.h"
#include "app_book.h"
#include "app_factory_test_date.h"
#include "app_keyboard_language.h"
#include "app_dvbs_sat_opt.h"

#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#endif

extern bool scan_mode_disable_flag;

/* Private variable------------------------------------------------------- */

enum SAT_LIST_POPUP_BOX
{
    SAT_LIST_POPUP_BOX_ITEM_NAME = 0,
    SAT_LIST_POPUP_BOX_ITEM_LONGITUDE,
    SAT_LIST_POPUP_BOX_ITEM_DIRECT,
    SAT_LIST_POPUP_BOX_ITEM_INPUT_SELECT,
    SAT_LIST_POPUP_BOX_ITEM_OK
};

static enum
{
    SAT_ADD = 0,
    SAT_EDIT,
    SAT_DELETE,
    SAT_SEARCH
}s_CurMode;

#if SAT_LIST_MOVE_SORT_SUPPORT
typedef enum
{
    SAT_SORT_NAEM_A_Z = 0,
    SAT_SORT_NAEM_Z_A,
    SAT_SORT_LONGITUDE_0_9,
    SAT_SORT_LONGITUDE_9_0
}sat_sort_mode;
static char *s_sat_sort_list[]={"Sat Name(A-Z)","Sat Name(Z-A)","Longitude(0~9)","Longitude(9~0)"};

int app_sat_list_sort(sat_sort_mode sort_mode);
int app_sat_list_move(uint32_t* selected_pos, int selected_num, int dst_pos);
#endif
static uint32_t *s_sat_id_array_bak = NULL;
static uint16_t s_satlist_sat_choise = 0;
static AppIdOps* s_SatIdOps = NULL;
static AppIdOps* s_SatLastIdOps = NULL;

static event_list* sp_SatListTimerSignal = NULL;
static event_list* sp_SatListLockTimer=NULL;
static int app_satellite_list_info_func(int sat_num);
static status_t app_satlist_sat_check_name(char *name_str , int type);

// create dialog
int app_satellite_list_init(int SatSel)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST))
    {
        GUI_EndDialog(WND_SAT_LIST);
    }
    GUI_CreateDialog(WND_SAT_LIST);
    SatSel = s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] ;
    if(SatSel >= 0)
        GUI_SetProperty("list_sat_list", "select", &SatSel);

    return 0;
}

/**
 * Function:    app_sat_list_del_sel_status
 * Date:        2013-06-19
 * Description: delete sat sel status after factory reset!
 * Callby:
 * Bug:         mantis bug2078
 */
void app_sat_list_del_sel_status(void)
{
    uint16_t i;

    if(NULL == s_SatLastIdOps) //复位时其他文件会调用这条语句，不加，没进sat list界面就恢复出厂设置会死机
        return;

    for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
    {
        if (s_SatLastIdOps->id_check(s_SatLastIdOps, i) != 0)
        {
            s_SatLastIdOps->id_delete(s_SatLastIdOps, i);
            s_satlist_sat_choise--;
        }
    }
}
void app_sat_list_close_lnb(void)
{
    AppFrontend_PolarState Polar = SEC_VOLTAGE_OFF ;
    app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);
    return;
}

#if FACTORY_TEST_SUPPORT
/*
**
** propose :just for factory test
** function:add sat to satlist
**data :2012 10 31
*/
//status_t MKTech_Add_Sat(uint32_t tuner , uint32_t * psat_id)
status_t app_factory_test_add_sat(uint32_t tuner ,FRONTPARA *para, uint32_t * psat_id)
{
    GxMsgProperty_NodeAdd NodeAdd;
    memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
    NodeAdd.node_type = NODE_SAT;
    NodeAdd.sat_data.type = GXBUS_PM_SAT_S;
    NodeAdd.sat_data.tuner = 0;
    NodeAdd.sat_data.sat_s.lnb1 = 5150;
    NodeAdd.sat_data.sat_s.lnb2 = 5750;
    NodeAdd.sat_data.sat_s.lnb_power = para->lnb_power;//GXBUS_PM_SAT_LNB_POWER_ON;
    NodeAdd.sat_data.sat_s.switch_22K = para->switch22k;//GXBUS_PM_SAT_22K_OFF;
    //app_log_debug("g debug:fuc:%s:line:%d:22k%d\n",__FUNCTION__,__LINE__,NodeAdd.sat_data.sat_s.switch_22K);
    NodeAdd.sat_data.sat_s.diseqc11 = 0;
    NodeAdd.sat_data.sat_s.diseqc12_pos = DISEQ12_USELESS_ID;
    NodeAdd.sat_data.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
    NodeAdd.sat_data.sat_s.diseqc10 = GXBUS_PM_SAT_DISEQC_OFF;
    NodeAdd.sat_data.sat_s.switch_12V = GXBUS_PM_SAT_12V_OFF;
    NodeAdd.sat_data.sat_s.longitude_direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    NodeAdd.sat_data.sat_s.reserved = 0;
    NodeAdd.sat_data.sat_s.longitude = 0;//longitude;
    memset((char *)NodeAdd.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
    if( 0 == tuner )
        memcpy((char *)NodeAdd.sat_data.sat_s.sat_name, "AsiaSatA 3s"/*name_str*/, MAX_SAT_NAME -1);
    else
        memcpy((char *)NodeAdd.sat_data.sat_s.sat_name, "AsiaSatB 3s"/*name_str*/, MAX_SAT_NAME -1);
    NodeAdd.sat_data.tuner = tuner;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
        *psat_id = NodeAdd.sat_data.id;

    //app_log_debug("g debug :line:%d:sat_id:%d:tuner:%d\n",__LINE__,*psat_id,tuner);

    return GXCORE_SUCCESS;
}
#endif


#if TKGS_SUPPORT
extern int app_check_tkgs_control_db(void);
extern bool app_tkgs_check_sat_is_turish(GxMsgProperty_NodeByPosGet * p_nodesat);
extern int app_tkgs_get_turksat_pInfo(GxMsgProperty_NodeByPosGet ** p_TKGSSat);
extern void app_tkgs_cantwork_popup(void);


static bool app_satlist_focus_is_turk(void)
{
    if(app_tkgs_check_sat_is_turish(&s_dvbs_sat_data.cur_sat))
    {
        app_tkgs_cantwork_popup();
        return TRUE;
    }

    return FALSE;
}


#endif

/* widget macro -------------------------------------------------------- */

static int app_sat_list_clean_signal(void)
{
    app_clean_signal("progbar_sat_list_strength", "text_sat_list_strength_value",
                        "progbar_sat_list_quality", "text_sat_list_quality_value");

    return 0;
}


static int timer_sat_list_show_signal(void *userdata)
{
    app_show_signal("progbar_sat_list_strength", "text_sat_list_strength_value",
                        "progbar_sat_list_quality", "text_sat_list_quality_value");

    return 0;
}

static void timer_sat_list_show_signal_stop(void)
{
    if(sp_SatListTimerSignal != NULL)
        timer_stop(sp_SatListTimerSignal);

    GUI_SetInterface("flush",NULL);
}

static void timer_sat_list_show_signal_reset(void)
{
    if(sp_SatListTimerSignal != NULL)
        reset_timer(sp_SatListTimerSignal);
}

static int timer_sat_list_set_frontend(void *userdata)
{
    APP_TIMER_REMOVE(sp_SatListLockTimer);

    s_dvbs_sat_data.set_diseqc(NULL);
    s_dvbs_sat_data.set_tp(SET_TP_FORCE);

    return 0;
}

#define S_INFO_ALL 0
#define S_INFO_ADD 1
#define S_INFO_NOEDIT 3
#if SAT_LIST_MOVE_SORT_SUPPORT
#define S_INFO_SORT 4
#define S_INFO_MOVE_TO 5
#endif
static void sat_list_hide_help_info(int s_help_info)
{
    //All
    if(S_INFO_NOEDIT != s_help_info)
    {
        GUI_SetProperty("img_sat_list_tip_0", "state", "hide");
        GUI_SetProperty("text_sat_list_tip_0", "state", "hide");
    }
    else
    {
        GUI_SetProperty("img_sat_list_tip_0", "state", "show");
        GUI_SetProperty("text_sat_list_tip_0", "state", "show");
    }

#if SAT_LIST_MOVE_SORT_SUPPORT
    GUI_SetProperty("img_tp_list_tip_1", "state", "hide");
    GUI_SetProperty("text_tp_list_tip_1", "state", "hide");
    GUI_SetProperty("img_tp_list_tip_2", "state", "hide");
    GUI_SetProperty("text_tp_list_tip_2", "state", "hide");
#endif
    //Delete
    GUI_SetProperty("img_sat_list_tip_red", "state", "hide");
    GUI_SetProperty("text_sat_list_tip_red", "state", "hide");

    //Add
    if(S_INFO_ALL == s_help_info)
    {
        GUI_SetProperty("img_sat_list_tip_green", "state", "hide");
        GUI_SetProperty("text_sat_list_tip_green", "state", "hide");
    }
    else if(S_INFO_ADD == s_help_info || S_INFO_NOEDIT == s_help_info)
    {
        GUI_SetProperty("text_sat_list_tip_green", "string", STR_ID_ADD);
        GUI_SetProperty("img_sat_list_tip_green", "state", "show");
        GUI_SetProperty("text_sat_list_tip_green", "state", "show");
    }
    else;;

    //Edit
    GUI_SetProperty("img_sat_list_tip_yellow", "state", "hide");
    GUI_SetProperty("text_sat_list_tip_yellow", "state", "hide");

    //Search
    GUI_SetProperty("img_sat_list_tip_blue", "state", "hide");
    GUI_SetProperty("text_sat_list_tip_blue", "state", "hide");

    //Antenna
    GUI_SetProperty("img_satellite_tip_info", "state", "hide");
    GUI_SetProperty("text_satellite_tip_info", "state", "hide");

}

static void sat_list_show_help_info(void)
{
    GUI_SetProperty("img_sat_list_tip_0", "state", "show");
    GUI_SetProperty("text_sat_list_tip_0", "state", "show");

    GUI_SetProperty("img_sat_list_tip_red", "state", "show");
    GUI_SetProperty("text_sat_list_tip_red", "state", "show");

    GUI_SetProperty("img_sat_list_tip_green", "state", "show");
    GUI_SetProperty("text_sat_list_tip_green", "state", "show");
    GUI_SetProperty("text_sat_list_tip_green", "string", STR_ID_ADD);

    GUI_SetProperty("img_sat_list_tip_yellow", "state", "show");
    GUI_SetProperty("text_sat_list_tip_yellow", "state", "show");

    GUI_SetProperty("img_sat_list_tip_blue", "state", "show");
    GUI_SetProperty("text_sat_list_tip_blue", "state", "show");

    GUI_SetProperty("img_satellite_tip_info", "state", "show");
    GUI_SetProperty("text_satellite_tip_info", "state", "show");
#if SAT_LIST_MOVE_SORT_SUPPORT
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

static int app_satellite_list_info_func(int sat_num)
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
            GUI_EndDialog(WND_SAT_LIST);
        else
            timer_sat_list_show_signal_stop();

        app_antenna_setting_init(s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner], s_dvbs_sat_data.cur_tp_sel);
    }
    else
    {
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

SIGNAL_HANDLER int app_sat_list_create(GuiWidget *widget, void *usrdata)
{
#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_set_dialog_pause();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif

    s_satlist_sat_choise = 0;
    s_SatIdOps = new_id_ops();
    if(s_SatIdOps == NULL)
        return GXCORE_ERROR;

    s_dvbs_sat_data.cur_tuner = TUNER_ALL;
    uint32_t cur_sat_pos = s_dvbs_sat_data.cur_sat.pos;
    s_dvbs_sat_data.sat_rebuild(NULL, false);
    s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = s_dvbs_sat_data.sat_gui_sel_get_by_pos(cur_sat_pos);
    s_dvbs_sat_data.cur_sat_get();
    s_dvbs_sat_data.cur_tp_get();

    if(s_dvbs_sat_data.sat_total > 0)
    {
        s_SatIdOps->id_open(s_SatIdOps, MANAGE_SAME_LIST, s_dvbs_sat_data.sat_total);

        if(s_SatLastIdOps != NULL)
        {
            uint16_t i;
            for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
            {
                if (s_SatLastIdOps->id_check(s_SatLastIdOps, i) != 0)
                {
                    GxMsgProperty_NodeByPosGet sat_node = {0};

                    sat_node.node_type = NODE_SAT;
                    sat_node.pos = s_dvbs_sat_data.sat_buf[i];
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &sat_node);

                    s_SatIdOps->id_add(s_SatIdOps, i, sat_node.sat_data.id);
                    s_satlist_sat_choise++;

                }
            }
        }
    }
    else
    {
        sat_list_hide_help_info(S_INFO_ADD);
    }

    GUI_SetProperty("list_sat_list", "select", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
    app_sat_list_clean_signal();

    s_dvbs_sat_data.set_diseqc(NULL);
    s_dvbs_sat_data.set_tp(SET_TP_FORCE);

    APP_TIMER_ADD(sp_SatListTimerSignal, timer_sat_list_show_signal, 100, TIMER_REPEAT);
#if DOUBLE_S2_SUPPORT
    GUI_SetProperty("text_satlist_tuner_select","state","show");
#endif
#if !SAT_LIST_MOVE_SORT_SUPPORT
    GUI_SetProperty("img_sat_list_tip_1", "state", "hide");
    GUI_SetProperty("text_sat_list_tip_1", "state", "hide");
    GUI_SetProperty("img_sat_list_tip_2", "state", "hide");
    GUI_SetProperty("text_sat_list_tip_2", "state", "hide");
#endif
    app_panel_show_prog_num(PANEL_UNDISPLAY_TIME);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_list_destroy(GuiWidget *widget, void *usrdata)
{
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
    APP_FREE(s_sat_id_array_bak);
    APP_TIMER_REMOVE(sp_SatListTimerSignal);
    APP_TIMER_REMOVE(sp_SatListLockTimer);

    if(s_SatLastIdOps != NULL)
        del_id_ops(s_SatLastIdOps);
    s_SatLastIdOps = s_SatIdOps;
    s_SatIdOps = NULL;
    app_panel_show_prog_num(PANEL_DISPLAY_TIME);

    return EVENT_TRANSFER_STOP;
}

#if DVB_CLOUD_SUPPORT
#include <gx_dvbonline.h>
extern void app_create_satsource_menu(int sat_max);
void app_dvbcloud_satlist_add(gx_dvbonline_satelitelist_t *satlist, int *listsel)
{
    GxMsgProperty_NodeAdd NodeAdd;
    GxBusPmDataSatLongitudeDirect direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    int i;
    int len = 0;
	if((!satlist)||(satlist->satelite_num<=0)||(!listsel))
	{
		return;
	}

    APP_FREE(s_sat_id_array_bak);
    s_sat_id_array_bak = GxCore_Calloc(SYS_MAX_SAT, sizeof(uint32_t));
    if(!s_sat_id_array_bak)
    {
        return;
    }
    for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
    {
        GxMsgProperty_NodeByPosGet sat_node = {0};
        sat_node.node_type = NODE_SAT;
        sat_node.pos = s_dvbs_sat_data.sat_buf[i];
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &sat_node);
        s_sat_id_array_bak[i] = sat_node.sat_data.id;
    }

	for(i=0; i<satlist->satelite_num; i++)
	{
		if((listsel[i]>0)&&(app_satlist_sat_check_name(satlist->satelites[i].mini_name, 0) == GXCORE_SUCCESS))
		{
			if(satlist->satelites[i].position>0)
				direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
			else
				direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST;
			memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
			NodeAdd.node_type = NODE_SAT;
			NodeAdd.sat_data.type = GXBUS_PM_SAT_S;
			NodeAdd.sat_data.tuner = 0;
			if(satlist->satelites[i].lnb_type == 0)
			{
				NodeAdd.sat_data.sat_s.lnb1 = 5150;
				NodeAdd.sat_data.sat_s.lnb2 = 5750;
			}
			else
			{
				NodeAdd.sat_data.sat_s.lnb1 = 9750;
				NodeAdd.sat_data.sat_s.lnb2 = 10600;
			}
			NodeAdd.sat_data.sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_ON;
			NodeAdd.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_OFF;
			NodeAdd.sat_data.sat_s.diseqc11 = 0;
			NodeAdd.sat_data.sat_s.diseqc12_pos = DISEQ12_USELESS_ID;
			NodeAdd.sat_data.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
			NodeAdd.sat_data.sat_s.diseqc10 = 0;
			NodeAdd.sat_data.sat_s.switch_12V = GXBUS_PM_SAT_12V_OFF;
			NodeAdd.sat_data.sat_s.longitude_direct = direct;
			NodeAdd.sat_data.sat_s.reserved = 0;
			NodeAdd.sat_data.sat_s.longitude = abs(satlist->satelites[i].position);
			memset((char *)NodeAdd.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
            len = strlen(satlist->satelites[i].mini_name);
            if(len < MAX_SAT_NAME)
            {
                strncpy((char *)NodeAdd.sat_data.sat_s.sat_name, satlist->satelites[i].mini_name, len);
            }
            else
            {
                strncpy((char *)NodeAdd.sat_data.sat_s.sat_name, satlist->satelites[i].mini_name, MAX_SAT_NAME-1);
            }
#if UNICABLE_SUPPORT
			{
				int index = 0;
				//centre fre
				for(index=0;index<MAX_IF_INDEX_NUM;index++)
				{
					NodeAdd.sat_data.sat_s.unicable_para.centre_fre[index] = 0;
				}
                NodeAdd.sat_data.sat_s.unicable_para.type = GXBUS_PM_SAT_UNICABLE_OFF;
				NodeAdd.sat_data.sat_s.unicable_para.lnb_fre_index = 0;
				NodeAdd.sat_data.sat_s.unicable_para.if_channel = 0;
				NodeAdd.sat_data.sat_s.unicable_para.sat_pos = 0;
			}
#endif
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
			{
				s_dvbs_sat_data.change |= (SAT_LIST_UPDATE_SATINFO | ANT_SET_UPDATE_SATINFO);
			}

		}
	}

}

#endif
void app_sat_list_to_search(WndStatus wnd_ok)
{
    timer_sat_list_show_signal_stop();
    timer_stop(sp_SatListLockTimer);
    app_search_start();
}


#if SAT_LIST_MOVE_SORT_SUPPORT
static int _tp_list_sort_cb(int select,unsigned short key)
{
    uint8_t i = 0;

    if(key == STBK_OK)
    {
        app_sat_list_sort(select);
        s_dvbs_sat_data.sat_rebuild(NULL, false);
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
            s_dvbs_sat_data.change |= ANT_SET_UPDATE_SATINFO;
        for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
        {
            s_SatIdOps->id_delete(s_SatIdOps, i);
        }
        s_satlist_sat_choise = 0;
        GUI_SetProperty("list_sat_list", "update", NULL);
        APP_TIMER_ADD(sp_SatListLockTimer, timer_sat_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    }
    return 0;
}
#endif

SIGNAL_HANDLER int app_sat_list_keypress(GuiWidget *widget, void *usrdata)
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
                    if(0 == num)
                    {
                        GUI_EndDialog(WND_SAT_LIST);
                        GUI_EndDialog(WND_ANTENNA_SETTING);
                        ret = EVENT_TRANSFER_STOP;
                        break;
                    }
                    else
                    {
                        app_satellite_list_info_func(num);
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
                    GUI_EndDialog(WND_SAT_LIST);
                }

                ret = EVENT_TRANSFER_STOP;
                break;
            case STBK_GREEN:    //add
                {
#if (DEMOD_DVB_S > 0 && ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0)))
                    if(s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner) >= SYS_MAX_SAT-1)
#else
                    GxMsgProperty_NodeNumGet node_num = {0};
                    node_num.node_type = NODE_SAT;
                    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
                    if(node_num.node_num >= SYS_MAX_SAT)
#endif
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_MAX_SAT;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop);
                    }
                    else
                    {
                        s_CurMode = SAT_ADD;
                    #if DVB_CLOUD_SUPPORT
                        app_create_satsource_menu(SYS_MAX_SAT-node_num.node_num);
                    #else
                        GUI_CreateDialog(WND_SAT_LIST_POPUP);
                    #endif
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }

            case STBK_YELLOW:   //edit 500k 6605
                if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                    break;

                #if TKGS_SUPPORT
                if(FALSE == app_satlist_focus_is_turk())
                {
                    s_CurMode = SAT_EDIT;
                    if(s_dvbs_sat_data.sat_total > 0)
                    {
                        GUI_CreateDialog(WND_SAT_LIST_POPUP);
                    }
                }
                #else
                s_CurMode = SAT_EDIT;
                if(s_dvbs_sat_data.sat_total > 0)
                {
                    GUI_CreateDialog(WND_SAT_LIST_POPUP);
                }
                #endif
                ret = EVENT_TRANSFER_STOP;
                break;

            case STBK_RED:      //del
                if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                    break;

                #if TKGS_SUPPORT
                if(s_dvbs_sat_data.sat_total > 0)
                {
                    uint32_t opt_num = 0;
                    opt_num = s_SatIdOps->id_total_get(s_SatIdOps);

                    if(opt_num == 0) //no sat selected
                    {
                        if(app_satlist_focus_is_turk())
                        {
                            ret = EVENT_TRANSFER_STOP;
                            break;
                        }
                        s_SatIdOps->id_add(s_SatIdOps, s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner], s_dvbs_sat_data.cur_sat.sat_data.id);
                        GUI_SetProperty("list_sat_list", "update_row", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
                    }
                    else
                    {
                        GxMsgProperty_NodeByPosGet * p_TKGSSat = NULL;
                        int tkgsSatId = app_tkgs_get_turksat_pInfo(&p_TKGSSat);
                        if(1== s_SatIdOps->id_check(s_SatIdOps,(uint32_t)tkgsSatId))
                        {
                            s_SatIdOps->id_delete(s_SatIdOps,tkgsSatId);
                            GUI_SetProperty("list_sat_list", "update_row", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
                            if(opt_num == 1)
                            {
                                app_tkgs_cantwork_popup();
                                ret = EVENT_TRANSFER_STOP;
                                break;
                            }
                        }
                    }

                    s_CurMode = SAT_DELETE;
                    GUI_CreateDialog(WND_SAT_LIST_POPUP);
                }
                #else
                s_CurMode = SAT_DELETE;

                if(s_dvbs_sat_data.sat_total > 0)
                {
                    uint32_t opt_num = 0;
                    opt_num = s_SatIdOps->id_total_get(s_SatIdOps);
                    if(opt_num == 0) //no sat selected
                    {
                        s_SatIdOps->id_add(s_SatIdOps, s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner], s_dvbs_sat_data.cur_sat.sat_data.id);
                        GUI_SetProperty("list_sat_list", "update_row", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
                    }

                    GUI_CreateDialog(WND_SAT_LIST_POPUP);
                }
                #endif
                ret = EVENT_TRANSFER_STOP;
                break;

            case STBK_BLUE:
            {
                if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                    break;

                SearchIdSet search_id;
                SearchType search_type;
                GxMsgProperty_NodeNumGet tp_num = {0};
                uint32_t opt_num = 0;
                #if TKGS_SUPPORT
                if(app_check_tkgs_control_db() != 0)
                {
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
                #endif
                if(s_dvbs_sat_data.sat_total > 0)
                {
                    opt_num = s_SatIdOps->id_total_get(s_SatIdOps);
                    if(opt_num == 0) //no sat selected
                    {
                        s_SatIdOps->id_add(s_SatIdOps, s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner], s_dvbs_sat_data.cur_sat.sat_data.id);
                        GUI_SetProperty("list_sat_list", "update_row", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
                        search_id.id_num = 1;
                    }
                    else
                    {
                        search_id.id_num = opt_num;
                    }

                    search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num * sizeof(uint32_t));
                    if(search_id.id_array != NULL)
                    {
                        s_SatIdOps->id_get(s_SatIdOps, (uint32_t *)search_id.id_array);

                        for(opt_num = 0; opt_num < search_id.id_num; opt_num++)
                        {
                            tp_num.sat_id = search_id.id_array[opt_num];
                            tp_num.node_type = NODE_TP;
                            app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &tp_num);
                            if(tp_num.node_num > 0)
                                break;
                        }

                        if(tp_num.node_num > 0)
                        {
                            search_type = SEARCH_SAT;
                            scan_mode_disable_flag = false;
                        }
                        else
                        {
                            search_type = SEARCH_BLIND_ONLY;
                            scan_mode_disable_flag = true;
                        }
                        app_search_opt_dlg(search_type, &search_id);
                        GxCore_Free(search_id.id_array);
                    }
                }
                else
                {
                    s_CurMode = SAT_SEARCH;
                }
                ret = EVENT_TRANSFER_STOP;
                break;
            }
#if SAT_LIST_MOVE_SORT_SUPPORT
            case STBK_1:
            {
                if(s_dvbs_sat_data.sat_total > 0)
                {
                    PopList pop_list;
                    memset(&pop_list, 0, sizeof(PopList));
                    pop_list.title = STR_ID_SORT;
                    pop_list.item_num = 4;
                    pop_list.item_content = s_sat_sort_list;
                    pop_list.sel = 0;
                    pop_list.show_num= false;
                    pop_list.mode=POP_MODE_UNBLOCK;
                    pop_list.exit_cb = _tp_list_sort_cb;
                    poplist_create(&pop_list);
                    ret = EVENT_TRANSFER_STOP;
                    sat_list_hide_help_info(S_INFO_SORT);
                    break;
                }
            }
            case STBK_2:
            {
                if(s_dvbs_sat_data.sat_total > 0)
                {
                    int opt_num;
                    opt_num = s_SatIdOps->id_total_get(s_SatIdOps);
                    if(opt_num > 0)
                    {
                        uint32_t *selected_pos = NULL;
                        int i,cur_sel;
                        selected_pos = (uint32_t*)GxCore_Malloc(opt_num*sizeof(uint32_t));
                        s_SatIdOps->id_pos_get(s_SatIdOps,selected_pos);
                        GUI_GetProperty("list_sat_list", "select", &cur_sel);
                        app_sat_list_move(selected_pos,opt_num,cur_sel);
                        GxCore_Free(selected_pos);
                        for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
                        {
                            s_SatIdOps->id_delete(s_SatIdOps, i);
                        }
                        s_satlist_sat_choise = 0;
                        s_dvbs_sat_data.sat_rebuild(NULL, false);
                        s_dvbs_sat_data.tp_rebuild(NULL);
                        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
                            s_dvbs_sat_data.change |= ANT_SET_UPDATE_SATINFO;
                        GUI_SetProperty("list_sat_list", "update", NULL);
                        APP_TIMER_ADD(sp_SatListLockTimer, timer_sat_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);
                    }
                }
            }
            break;
#endif
            case STBK_PAGE_UP:
            case STBK_PAGE_DOWN:
                ret = EVENT_TRANSFER_STOP;
            break;

            // add in 20131027
            case STBK_INFO:
                {
                    if(GXBUS_PM_SAT_S != s_dvbs_sat_data.cur_sat.sat_data.type)
                        break;
                    int num;
                    #if DOUBLE_S2_SUPPORT
                        s_dvbs_sat_data.cur_tuner = s_dvbs_sat_data.cur_sat.sat_data.tuner;
                    #endif
                    num = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner);
                    if(num > 0)
                        app_satellite_list_info_func(num);
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
            default:
                break;
        }
    }

    return ret;
}

SIGNAL_HANDLER int app_sat_list_lost_focus(GuiWidget *widget, void *usrdata)
{
    if(s_dvbs_sat_data.sat_total > 0)
    {
        GUI_SetProperty("list_sat_list", "active", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
    }

    if(sp_SatListTimerSignal)
        timer_stop(sp_SatListTimerSignal);

    sat_list_hide_help_info(S_INFO_ALL);

    return EVENT_TRANSFER_STOP;
}

bool _id_not_in_array(uint32_t id, uint32_t *id_array, uint32_t num)
{
    if(!id_array)
        return true;

    int i = 0;
    for(i = 0; (i < num) && (0 != id_array[i]); i++)
    {
        if(id == id_array[i])
            return false;
    }
    return true;
}

SIGNAL_HANDLER int app_sat_list_got_focus(GuiWidget *widget, void *usrdata)
{
    int sel = -1;
    int act_sel = -1;

    GUI_SetProperty("list_sat_list", "active", &act_sel);
    sel = s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner];

    if((SAT_EDIT == s_CurMode)&&(s_dvbs_sat_data.change & SAT_LIST_UPDATE_SATINFO))
    {
        s_dvbs_sat_data.cur_sat_get();
        GUI_SetProperty("list_sat_list", "update_row", &sel);
    }
    else if((SAT_ADD == s_CurMode)&&(s_dvbs_sat_data.change & SAT_LIST_UPDATE_SATINFO))
    {
        sel = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner) - 1;
        s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = sel;
        s_dvbs_sat_data.sat_rebuild(NULL, false);

        bool find_sat_id_add_flag = false;
        find_sat_id_add_flag = _id_not_in_array(s_dvbs_sat_data.cur_sat.sat_data.id, s_sat_id_array_bak, SYS_MAX_SAT);

        if(s_dvbs_sat_data.sat_total > 0)
        {
            if(s_SatIdOps != NULL)
            {
                int i;

                if(s_SatLastIdOps != NULL)
                    del_id_ops(s_SatLastIdOps);

                s_SatLastIdOps = s_SatIdOps;
                s_SatIdOps = new_id_ops();

                if(s_SatIdOps != NULL)
                {
                    s_SatIdOps->id_open(s_SatIdOps, MANAGE_SAME_LIST, s_dvbs_sat_data.sat_total);

                    for(i = 0; i < s_dvbs_sat_data.sat_total-1; i++)
                    {
                        if (s_SatLastIdOps->id_check(s_SatLastIdOps, i) != 0)
                        {
                            GxMsgProperty_NodeByPosGet sat_node = {0};

                            sat_node.node_type = NODE_SAT;
                            sat_node.pos = s_dvbs_sat_data.sat_buf[i];
                            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &sat_node);

                            s_SatIdOps->id_add(s_SatIdOps, i, sat_node.sat_data.id);
                        }
                    }
                }
            }

            if(!find_sat_id_add_flag)
            {
                int i;
                for(i = 0; i < s_dvbs_sat_data.sat_total-1; i++)
                {
                    GxMsgProperty_NodeByPosGet sat_node = {0};
                    sat_node.node_type = NODE_SAT;
                    sat_node.pos = s_dvbs_sat_data.sat_buf[i];
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &sat_node);
                    if(_id_not_in_array(sat_node.sat_data.id, s_sat_id_array_bak, SYS_MAX_SAT))
                    {
                        find_sat_id_add_flag = true;
                        sel = i;
                        s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = sel;
                        s_dvbs_sat_data.cur_sat_get();
                    }
                }
            }
        }
        else
            sel = -1;

        s_dvbs_sat_data.cur_tp_sel = 0;
        s_dvbs_sat_data.tp_rebuild(NULL);
        APP_FREE(s_sat_id_array_bak);

        GUI_SetProperty("list_sat_list", "update_all", NULL);
        GUI_SetProperty("list_sat_list", "select", &sel);
        APP_TIMER_ADD(sp_SatListLockTimer, timer_sat_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);
    }
    else if((SAT_DELETE == s_CurMode)&&(s_dvbs_sat_data.change & SAT_LIST_UPDATE_SATINFO))
    {
        int i;
        int del_count = 0;

        s_dvbs_sat_data.sat_total = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner);
        if(s_dvbs_sat_data.sat_total > 0)
        {
            if(s_SatIdOps->id_total_get(s_SatIdOps) > 0)
            {
                del_count = 0;
                for(i = 0; i < s_dvbs_sat_data.sat_total + s_SatIdOps->id_total_get(s_SatIdOps); i++)
                {
                    if(true == s_SatIdOps->id_check(s_SatIdOps, i))
                    {
                        del_count++;
                        continue;
                    }

                    if(i >= sel)
                        break;
                }
                sel = i - del_count;
            }

            if(sel >= s_dvbs_sat_data.sat_total )
                sel = s_dvbs_sat_data.sat_total - 1;

            s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = sel;
            s_dvbs_sat_data.sat_rebuild(NULL, false);
            s_dvbs_sat_data.cur_tp_sel = 0;
            s_dvbs_sat_data.cur_tp_get();
        }
        else
            sel = -1;

        if(s_SatIdOps != NULL)
        {
            s_SatIdOps->id_close(s_SatIdOps);
            s_SatIdOps->id_open(s_SatIdOps, MANAGE_SAME_LIST, s_dvbs_sat_data.sat_total);
        }
                GUI_SetProperty("list_sat_list", "update_all", NULL);
        GUI_SetProperty("list_sat_list", "select", &sel);
    }
    else //SEARCH
        s_dvbs_sat_data.cur_tp_get();

    if(s_dvbs_sat_data.sat_total > 0)
    {
        if(s_dvbs_sat_data.cur_sat.sat_data.type != GXBUS_PM_SAT_S)
            sat_list_hide_help_info(S_INFO_NOEDIT);
        else
            sat_list_show_help_info();
    }
    else
        sat_list_hide_help_info(S_INFO_ADD);

    APP_TIMER_ADD(sp_SatListLockTimer, timer_sat_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    //clear
    s_dvbs_sat_data.change &= ~(SAT_LIST_UPDATE_SATINFO);
    timer_sat_list_show_signal_reset();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_list_list_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_list_list_change(GuiWidget *widget, void *usrdata)
{
    GxMsgProperty_NodeByPosGet cur_sat_bak = {0};
    memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));

    uint32_t sel;
    GUI_GetProperty("list_sat_list", "select", &sel);
    s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner] = sel;
    s_dvbs_sat_data.cur_sat_get();

    if(cur_sat_bak.sat_data.id == s_dvbs_sat_data.cur_sat.sat_data.id)
        return EVENT_TRANSFER_STOP;

    s_dvbs_sat_data.cur_tp_sel = 0;
    s_dvbs_sat_data.cur_tp_get();

    if(s_dvbs_sat_data.cur_sat.sat_data.type != GXBUS_PM_SAT_S)
        sat_list_hide_help_info(S_INFO_NOEDIT);
    else
        sat_list_show_help_info();

    APP_TIMER_ADD(sp_SatListLockTimer, timer_sat_list_set_frontend, FRONTEND_SET_DELAY_MS, TIMER_REPEAT);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_list_list_get_total(GuiWidget *widget, void *usrdata)
{
    s_dvbs_sat_data.sat_total = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner);
    return s_dvbs_sat_data.sat_total;
}

SIGNAL_HANDLER int app_sat_list_list_get_data(GuiWidget *widget, void *usrdata)
{
#define MAX_LEN 30
    ListItemPara* item = NULL;
    static GxMsgProperty_NodeByPosGet node;
    static char buffer1[MAX_LEN];
    static char buffer2[MAX_LEN];

    item = (ListItemPara*)usrdata;
    if((NULL == item) ||(0 > item->sel))
        return GXCORE_ERROR;

    //col-0: choice
    item->x_offset = 0;
    if (s_SatIdOps->id_check(s_SatIdOps, item->sel) == 0)//ID_UNEXIST
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

    //col-2: name
    item = item->next;
    //get tp node
    node.node_type = NODE_SAT;
    node.pos = s_dvbs_sat_data.sat_buf[item->sel];
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
    item->string = (char*)node.sat_data.sat_s.sat_name;

    //col-3: longitude
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer2, 0, MAX_LEN);
    if(GXBUS_PM_SAT_S != node.sat_data.type)
        sprintf(buffer2, "N/A");
    else
        sprintf(buffer2, "%d.%d", node.sat_data.sat_s.longitude/10, node.sat_data.sat_s.longitude%10);

    item->string = buffer2;

    //col-4: direct
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    if(GXBUS_PM_SAT_S != node.sat_data.type)
        item->string = buffer2;
    else if(node.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST)
        item->string = "E";
    else
        item->string = "W";

#if DOUBLE_S2_SUPPORT
    //col: tuner input
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    if(node.sat_data.tuner)
        item->string = "T2";
    else
        item->string = "T1";
#endif
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_sat_list_list_keypress(GuiWidget *widget, void *usrdata)
{
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
            switch(event->key.sym)
            {
                case STBK_OK:
                {
                    if(s_dvbs_sat_data.cur_sat.sat_data.type != GXBUS_PM_SAT_S)
                        break;
                    if(s_dvbs_sat_data.sat_total > 0)
                    {
                        if (s_SatIdOps->id_check(s_SatIdOps, s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]) == 0)//ID_UNEXIST
                        {
                            s_SatIdOps->id_add(s_SatIdOps, s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner], s_dvbs_sat_data.cur_sat.sat_data.id);
                            s_satlist_sat_choise++;
                        }
                        else
                        {
                            s_SatIdOps->id_delete(s_SatIdOps, s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
                            s_satlist_sat_choise--;
                        }
                        GUI_SetProperty("list_sat_list", "update_row", &s_dvbs_sat_data.cur_sat_sel[s_dvbs_sat_data.cur_tuner]);
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
                case STBK_0:
                {
                    if(s_dvbs_sat_data.sat_total > 0)
                    {
                        uint16_t i = 0;
                        GxMsgProperty_NodeByPosGet node = {0};

                        if(s_satlist_sat_choise < s_dvbs_sat_data.sat_total)
                        {
                            for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
                            {
                                node.node_type = NODE_SAT;
                                node.pos = s_dvbs_sat_data.sat_buf[i];
                                app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
                                if(node.sat_data.type != GXBUS_PM_SAT_S)
                                {
                                    continue;
                                }
                                s_SatIdOps->id_add(s_SatIdOps, i, node.sat_data.id);
                            }
                            s_satlist_sat_choise = s_dvbs_sat_data.sat_total;
                        }
                        else
                        {
                            for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
                            {
                                s_SatIdOps->id_delete(s_SatIdOps, i);
                            }
                            s_satlist_sat_choise = 0;
                        }
                        GUI_SetProperty("list_sat_list", "update", NULL);
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;
                }
                break;
                case STBK_PAGE_UP:
                {
                    uint32_t sel;
                    GUI_GetProperty("list_sat_list", "select", &sel);
                    if (sel == 0)
                    {
                        sel = s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner) - 1;
                        GUI_SetProperty("list_sat_list", "select", &sel);
                        ret =   EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        ret =  EVENT_TRANSFER_KEEPON;
                    }
                    break;
                }
                case STBK_PAGE_DOWN:
                {
                    uint32_t sel;
                    GUI_GetProperty("list_sat_list", "select", &sel);
                    if (s_dvbs_sat_data.sat_num_get(s_dvbs_sat_data.cur_tuner) == sel+1)
                    {
                        sel = 0;
                        GUI_SetProperty("list_sat_list", "select", &sel);
                        ret =  EVENT_TRANSFER_STOP;
                    }
                    else
                    {
                        ret =  EVENT_TRANSFER_KEEPON;
                    }

                    break;
                }

                default:
                    break;
            }

        default:
            break;
    }

    return ret;
}

static void _sat_rename_release_cb(PopKeyboard *data)
{
    if(data->in_ret == POP_VAL_CANCEL)
        return;

    if (data->out_name == NULL)
        return;

    GUI_SetProperty("btn_sat_list_name", "string", data->out_name);
}

static status_t app_satlist_sat_check_name(char *name_str , int type)
{
    uint32_t i = 0;
    GxMsgProperty_NodeByPosGet node;

    for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
    {
        node.node_type = NODE_SAT;
        node.pos = s_dvbs_sat_data.sat_buf[i];
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

        //type = 1表示edit,不与自己比较
        if(type == 1 && node.sat_data.id == s_dvbs_sat_data.cur_sat.sat_data.id)
            continue;

        if(strcmp((char*)node.sat_data.sat_s.sat_name, name_str ) == 0)
        {
            return GXCORE_ERROR;
        }
    }
    return GXCORE_SUCCESS;
}

static status_t app_satlist_sat_edit(void)
{
    GxMsgProperty_NodeModify NodeModify;
    char *name_str = NULL;
    char *atoi_str = NULL;
    int longitude = 0;
    GxBusPmDataSatLongitudeDirect direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    uint32_t sel = 0;

    //name
    GUI_GetProperty("btn_sat_list_name", "string", &name_str);
    if(app_satlist_sat_check_name(name_str, 1) == GXCORE_ERROR)
    {
        GUI_SetProperty("box_satlist_popup_para", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_SAT_EXIT);
        GUI_SetInterface("flush",NULL);

        GxCore_ThreadDelay(1000);

        GUI_SetProperty("box_satlist_popup_para", "state", "show");
        GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
        GUI_SetFocusWidget("box_satlist_popup_para");
        GUI_SetInterface("flush",NULL);

        return GXCORE_ERROR;
    }

    //longitude
    GUI_GetProperty("edit_satlist_popup_longitude", "string", &atoi_str);
    longitude = 10 * app_float_edit_str_to_value(atoi_str);
    if(longitude > 1800)
    {
        char buffer[20] = {0};
        //hide the box
        GUI_SetProperty("box_satlist_popup_para", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_ERR_LONGITUDE);
        GUI_SetProperty(WND_SAT_LIST_POPUP, "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        GUI_SetProperty("edit_satlist_popup_longitude", "clear", NULL);
        sprintf(buffer, "%03d.%d", s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude/10, s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude%10);
        GUI_SetProperty("edit_satlist_popup_longitude", "string", buffer);

        GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
        GUI_SetProperty("box_satlist_popup_para", "state", "show");
        GUI_SetProperty(WND_SAT_LIST_POPUP, "draw_now", NULL);
        GUI_SetFocusWidget("box_satlist_popup_para");

        return GXCORE_ERROR;
    }


    GUI_GetProperty("cmb_satlist_popup_direct", "select", &sel);
    if(0 == sel)
        direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    else
        direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST;

    NodeModify.node_type = NODE_SAT;
    memcpy(&NodeModify.sat_data, &s_dvbs_sat_data.cur_sat.sat_data, sizeof(GxBusPmDataSat));
    memset((char *)NodeModify.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
    memcpy((char *)NodeModify.sat_data.sat_s.sat_name, name_str, MAX_SAT_NAME - 1);
    NodeModify.sat_data.sat_s.longitude = longitude;
    NodeModify.sat_data.sat_s.longitude_direct = direct;

#if DOUBLE_S2_SUPPORT
    GUI_GetProperty("cmb_satlist_popup_input_select", "select", &sel);
    NodeModify.sat_data.tuner = sel;// 0:tuner; 1:tuner2
    s_dvbs_sat_data.cur_sat.sat_data.tuner = sel;
#endif

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &NodeModify))
    {
#if DOUBLE_S2_SUPPORT
        // for view mode backup
        uint32_t prog_num = 0;
        GxBusPmViewInfo view_info_temp = {0};
        uint32_t ModifyFlag = 0;
        uint32_t FlagForChangeViewMode = 0;
        uint32_t *p_id_array = NULL;
        uint32_t i = 0;

        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
           // step one : all tv program
GHANGE_MODE:
        if(FlagForChangeViewMode == 0)
        {
            view_info_temp.stream_type = GXBUS_PM_PROG_TV;
            view_info_temp.group_mode = GROUP_MODE_ALL;//all
            view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        }
        else
        {
            view_info_temp.stream_type = GXBUS_PM_PROG_RADIO;
            view_info_temp.group_mode = GROUP_MODE_ALL;//all
            view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        }
        FlagForChangeViewMode++;
        prog_num = GxBus_PmProgNumGetByViewInfo(&view_info_temp);
        if(prog_num > 0)
        {
            p_id_array = (uint32_t *)GxCore_Malloc(prog_num * sizeof(uint32_t)); //need GxCore_Free
            if(p_id_array == NULL)
            {
                app_log_error("malloc p_id_array failed!!!!!!!!!!!!\n");
                return GXCORE_ERROR;
            }
            GxBus_PmProgIdArryGetByViewInfo(&view_info_temp, prog_num, p_id_array);
            GxMsgProperty_NodeByIdGet ProgramNode = {0};
            for(i = 0; i < prog_num; i++)
            {
                ProgramNode.node_type = NODE_PROG;
                ProgramNode.id = p_id_array[i];
                app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &ProgramNode);
                if(NodeModify.sat_data.id == ProgramNode.prog_data.sat_id)
                {
                    if(ProgramNode.prog_data.tuner == sel)
                        break;
                    else
                    {
                        GxMsgProperty_NodeModify node_modify = {0};
                        node_modify.node_type = NODE_PROG;
                        node_modify.prog_data = ProgramNode.prog_data;
                        node_modify.prog_data.tuner = sel;
                        if(GXCORE_SUCCESS != GxBus_PmProgInfoModify(&(node_modify.prog_data)))
                        {
                            app_log_error("\nmodify TUNER error\n");
                            break;
                        }
                        ModifyFlag = 1;
                    }
                }
            }
            GxCore_Free(p_id_array);
            p_id_array = NULL;

        }

        if(FlagForChangeViewMode < 2)
            goto GHANGE_MODE;

        if(ModifyFlag)
        {
            GxBus_PmSync(GXBUS_PM_SYNC_PROG);
        }

        // recover the view mode
        app_ioctl(s_dvbs_sat_data.cur_sat.sat_data.tuner, FRONTEND_DEMUX_CFG, NULL);
#endif
        s_dvbs_sat_data.change |= (SAT_LIST_UPDATE_SATINFO | ANT_SET_UPDATE_SATINFO);
    }

    return GXCORE_SUCCESS;
}

static status_t app_satlist_sat_add(void)
{
    GxMsgProperty_NodeAdd NodeAdd;
    char *name_str = NULL;
    char *atoi_str = NULL;
    int longitude = 0;
    GxBusPmDataSatLongitudeDirect direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    uint32_t sel = 0;
    int name_len=0;
    //name
    GUI_GetProperty("btn_sat_list_name", "string", &name_str);
    if(app_satlist_sat_check_name(name_str, 0) == GXCORE_ERROR)
    {
        GUI_SetProperty("box_satlist_popup_para", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_SAT_EXIT);
        GUI_SetInterface("flush",NULL);

        GxCore_ThreadDelay(1000);

        GUI_SetProperty("box_satlist_popup_para", "state", "show");
        GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
        GUI_SetFocusWidget("box_satlist_popup_para");
        GUI_SetInterface("flush",NULL);

        return GXCORE_ERROR;
    }

    //longitude
    GUI_GetProperty("edit_satlist_popup_longitude", "string", &atoi_str);
    longitude = 10 * app_float_edit_str_to_value(atoi_str);
    if(longitude > 1800)
    {
        //hide the box
        GUI_SetProperty("box_satlist_popup_para", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_ERR_LONGITUDE);
        GUI_SetProperty(WND_SAT_LIST_POPUP, "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        GUI_SetProperty("edit_satlist_popup_longitude", "clear", NULL);
        GUI_SetProperty("edit_satlist_popup_longitude", "string", "000.0");

        GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
        GUI_SetProperty("box_satlist_popup_para", "state", "show");
        GUI_SetProperty(WND_SAT_LIST_POPUP, "draw_now", NULL);
        GUI_SetFocusWidget("box_satlist_popup_para");

        return GXCORE_ERROR;
    }

    int i;
    APP_FREE(s_sat_id_array_bak);
    s_sat_id_array_bak = GxCore_Calloc(SYS_MAX_SAT, sizeof(uint32_t));
    if(NULL == s_sat_id_array_bak)
    {
        app_log_error("\033[31m !!!!!!!!!!![%s, %d] GxCore_Calloc failed !!!!!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
        return GXCORE_ERROR;
    }

    for(i = 0; i < s_dvbs_sat_data.sat_total; i++)
    {
        GxMsgProperty_NodeByPosGet sat_node = {0};
        sat_node.node_type = NODE_SAT;
        sat_node.pos = s_dvbs_sat_data.sat_buf[i];
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &sat_node);
        s_sat_id_array_bak[i] = sat_node.sat_data.id;
    }

    GUI_GetProperty("cmb_satlist_popup_direct", "select", &sel);
    if(0 == sel)
        direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
    else
        direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST;

    memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));
    NodeAdd.node_type = NODE_SAT;
    NodeAdd.sat_data.type = GXBUS_PM_SAT_S;
    NodeAdd.sat_data.tuner = 0;
    NodeAdd.sat_data.sat_s.lnb1 = 5150;
    NodeAdd.sat_data.sat_s.lnb2 = 5750;
    NodeAdd.sat_data.sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_ON;
    NodeAdd.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_OFF;
    NodeAdd.sat_data.sat_s.diseqc11 = 0;
    NodeAdd.sat_data.sat_s.diseqc12_pos = DISEQ12_USELESS_ID;
    NodeAdd.sat_data.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
    NodeAdd.sat_data.sat_s.diseqc10 = 0;
    NodeAdd.sat_data.sat_s.switch_12V = GXBUS_PM_SAT_12V_OFF;
    NodeAdd.sat_data.sat_s.longitude_direct = direct;
    NodeAdd.sat_data.sat_s.reserved = 0;
    NodeAdd.sat_data.sat_s.longitude = longitude;
    name_len=strlen(name_str)<=MAX_SAT_NAME-1?strlen(name_str):MAX_SAT_NAME-1;
    memset((char *)NodeAdd.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
    strncpy((char *)NodeAdd.sat_data.sat_s.sat_name, name_str, name_len);
#if UNICABLE_SUPPORT
    {
        int index = 0;
        //centre fre
        for(index=0; index<MAX_IF_INDEX_NUM; index++)
        {
            NodeAdd.sat_data.sat_s.unicable_para.centre_fre[index] = 0;
        }
        NodeAdd.sat_data.sat_s.unicable_para.type = GXBUS_PM_SAT_UNICABLE_OFF;
        NodeAdd.sat_data.sat_s.unicable_para.lnb_fre_index = 0;
        NodeAdd.sat_data.sat_s.unicable_para.if_channel = 0;
        NodeAdd.sat_data.sat_s.unicable_para.sat_pos = 0;
    }
    #endif


#if DOUBLE_S2_SUPPORT
    GUI_GetProperty("cmb_satlist_popup_input_select", "select", &sel);
    NodeAdd.sat_data.tuner = sel;
#endif
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &NodeAdd))
    {
        s_dvbs_sat_data.change |= (SAT_LIST_UPDATE_SATINFO | ANT_SET_UPDATE_SATINFO);
    }
    return GXCORE_SUCCESS;
}

static status_t app_satlist_sat_del(void)
{
    GxMsgProperty_NodeDelete node= {0};
    status_t ret = GXCORE_ERROR;

    node.node_type = NODE_SAT;
    node.num = s_SatIdOps->id_total_get(s_SatIdOps);
    if(node.num > 0)
    {
        node.id_array = GxCore_Malloc(node.num*sizeof(uint32_t));
        if(node.id_array != NULL)
        {
            s_SatIdOps->id_get(s_SatIdOps, (uint32_t*)(node.id_array));

            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node))
            {
                s_dvbs_sat_data.change |= (SAT_LIST_UPDATE_SATINFO | ANT_SET_UPDATE_SATINFO);
                g_AppBook.invalid_remove();
                ret = GXCORE_SUCCESS;
            }

            GxCore_Free(node.id_array);
        }
    }
    return ret;
}

static status_t app_satlist_popup_ok_keypress(void)
{
    status_t ret = GXCORE_SUCCESS;
    switch(s_CurMode)
    {
        case SAT_ADD:
            ret = app_satlist_sat_add();
            break;

        case SAT_EDIT:
            if(s_dvbs_sat_data.sat_total > 0)
            {
                ret = app_satlist_sat_edit();
            }
            break;

        case SAT_DELETE:
        {
            if(s_dvbs_sat_data.sat_total > 0 && (0 == strcasecmp("btn_satlist_popup_ok", GUI_GetFocusWidget())))
            {
                GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_DELETING);
                GUI_SetProperty("text_satlist_popup_tip2", "string", NULL);
                GUI_SetProperty("img_satlist_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
                GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
                GUI_SetProperty(WND_SAT_LIST_POPUP, "draw_now", NULL);
                ret = app_satlist_sat_del();
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
                s_dvbs_sat_data.tp_rebuild(NULL);
            }
            break;
        }

        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_satlist_popup_longitude_edit_change(GuiWidget *widget, void *usrdata)
{
    int longitude = 0;
    char *atoi_str = NULL;
    char buffer[10] = {0};



    //longitude
    GUI_GetProperty("edit_satlist_popup_longitude", "string", &atoi_str);
    longitude = 10 * app_float_edit_str_to_value(atoi_str);
    if(longitude > 1800)
    {
        //hide the box
        longitude = 1800;
        GUI_SetProperty("box_satlist_popup_para", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_ERR_LONGITUDE);
        GUI_SetProperty(WND_SAT_LIST_POPUP, "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        GUI_SetProperty("edit_satlist_popup_longitude", "clear", NULL);

        if(s_CurMode == SAT_EDIT)
        {
            sprintf(buffer, "%03d.%d", s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude/10, s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude%10);
            GUI_SetProperty("edit_satlist_popup_longitude", "string", buffer);
        }
        else if(s_CurMode == SAT_ADD)
        {
            GUI_SetProperty("edit_satlist_popup_longitude", "string", "000.0");
        }
        else ;;

        GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
        GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
        GUI_SetProperty("text_satlist_popup_tip1", "state", "hide");
        GUI_SetProperty("box_satlist_popup_para", "state", "show");
        GUI_SetProperty(WND_SAT_LIST_POPUP, "draw_now", NULL);
        GUI_SetFocusWidget("box_satlist_popup_para");
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satlist_popup_longitude_edit_reachend(GuiWidget *widget, void *usrdata)
{
#define EDIT_MAX_LEN (5) //根据xml中edit长度定的
    uint32_t sel = 0;
    uint32_t cur_sel = 0;

    GUI_GetProperty("edit_satlist_popup_longitude", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_MAX_LEN - 1 : 0);
    GUI_SetProperty("edit_satlist_popup_longitude", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satlist_popup_create(GuiWidget *widget, void *usrdata)
{
#define DIRECT_CONTENT  "[E,W]"
#define STR_LEN 20

    char buffer[STR_LEN];
    uint32_t sel=0;

    switch(s_CurMode)
    {
        case SAT_ADD:
        {
            //title
            GUI_SetProperty("text_satlist_popup_title", "string", STR_ID_ADD);
            //name
            GUI_SetProperty("btn_sat_list_name", "string", "New Sat");
            //longitude
            GUI_SetProperty("edit_satlist_popup_longitude","string","000.0");
            //direction
            GUI_SetProperty("cmb_satlist_popup_direct", "content", DIRECT_CONTENT);
            //hide
            GUI_SetProperty("img_satlist_popup_ok_back", "state", "hide");
            GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
            GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");

            GUI_SetProperty("box_satlist_popup_para", "state", "show");
            GUI_SetFocusWidget("box_satlist_popup_para");

        #if DOUBLE_S2_SUPPORT
            GUI_SetProperty("text_satlist_popup_input_chennel","string", "Tuner");
            GUI_SetProperty("cmb_satlist_popup_input_select","content", "[T1,T2]");
        #else
            // for tuner select show control, if not support the double tuner
            GUI_SetProperty("text_satlist_popup_input_chennel","string", "");
            GUI_SetProperty("cmb_satlist_popup_input_select","content", "[]");
            GUI_SetProperty("boxitem_satplist_popup_input_sel","state", "disable");
        #endif

        }
        break;

        case SAT_EDIT:
        {
            //title
            GUI_SetProperty("text_satlist_popup_title", "string", STR_ID_EDIT);

            if(s_dvbs_sat_data.sat_total > 0)
            {
                //name
                GUI_SetProperty("btn_sat_list_name", "string", s_dvbs_sat_data.cur_sat.sat_data.sat_s.sat_name);
                //longitude
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "%03d.%d", s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude/10, s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude%10);
                GUI_SetProperty("edit_satlist_popup_longitude", "string", buffer);
                //direction
                GUI_SetProperty("cmb_satlist_popup_direct", "content", DIRECT_CONTENT);
                sel = s_dvbs_sat_data.cur_sat.sat_data.sat_s.longitude_direct;
                GUI_SetProperty("cmb_satlist_popup_direct", "select", &sel);
                // tuner
            #if DOUBLE_S2_SUPPORT
                sel = s_dvbs_sat_data.cur_sat.sat_data.tuner;
                GUI_SetProperty("cmb_satlist_popup_input_select","content", "[T1,T2]");
                GUI_SetProperty("cmb_satlist_popup_input_select", "select", &sel);
                GUI_SetProperty("text_satlist_popup_input_chennel","string", "Tuner");
            #else
                // for tuner select show control, if not support the double tuner
                GUI_SetProperty("text_satlist_popup_input_chennel","string", "");
                GUI_SetProperty("cmb_satlist_popup_input_select","content", "[]");
                GUI_SetProperty("boxitem_satplist_popup_input_sel","state", "disable");
            #endif
                //hide
                GUI_SetProperty("img_satlist_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_satlist_popup_ok", "state", "hide");
                GUI_SetProperty("btn_satlist_popup_cancel", "state", "hide");

                GUI_SetProperty("box_satlist_popup_para", "state", "show");
                GUI_SetFocusWidget("box_satlist_popup_para");
            }
            else
            {
                GUI_SetProperty("box_satlist_popup_para", "state", "hide");
                GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
        }
        break;

        case SAT_DELETE:
        {
            uint32_t opt_num = 0;
            //title
            GUI_SetProperty("text_satlist_popup_title", "string", "Delete");
            //hide the box
            GUI_SetProperty("box_satlist_popup_para", "state", "hide");
            GUI_SetProperty("btn_satlist_popup_ok", "state", "show");
            GUI_SetProperty("btn_satlist_popup_cancel", "state", "show");
            GUI_SetProperty("text_satlist_popup_tip1", "state", "show");
            GUI_SetProperty("text_satlist_popup_tip2", "state", "show");

            opt_num = s_SatIdOps->id_total_get(s_SatIdOps);
            if((s_dvbs_sat_data.sat_total > 0) && (opt_num > 0))
            {
                if(1 == opt_num)//only one
                {
                    uint32_t id[1] = {0};
                    GxMsgProperty_NodeByIdGet sat_node = {0};

                    s_SatIdOps->id_get(s_SatIdOps, id);
                    sat_node.node_type = NODE_SAT;
                    sat_node.id = id[0];
                    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node);

                    GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_DLE_SAT);
                    //show sat name
                    GUI_SetProperty("text_satlist_popup_tip2", "string", sat_node.sat_data.sat_s.sat_name);

                    GUI_SetFocusWidget("btn_satlist_popup_ok");
                    GUI_SetProperty("img_satlist_popup_ok_back","img","s_bar_choice_blue4");
                }
                else//del selected sat
                {
                    GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_DLE_SAT);
                    GUI_SetFocusWidget("btn_satlist_popup_ok");
                    GUI_SetProperty("img_satlist_popup_ok_back","img","s_bar_choice_blue4");
                }
            }
            else
            {
                GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
            }
        }
        break;

        case SAT_SEARCH:
        {
            //title
            GUI_SetProperty("text_satlist_popup_title", "string", "SAT Search");
            //hide the box
            GUI_SetProperty("box_satlist_popup_para", "state", "hide");

            GUI_SetProperty("text_satlist_popup_tip1", "string", STR_ID_SAT_NOT_EXIST);
        }

        default:
            break;

    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satlist_popup_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_satlist_popup_keypress(GuiWidget *widget, void *usrdata)
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
                    GUI_EndDialog("after wnd_full_screen");
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_SAT_LIST_POPUP);
                    break;

                case STBK_OK:
                    if((app_satlist_popup_ok_keypress() == GXCORE_SUCCESS) || (s_dvbs_sat_data.sat_total == 0))
                        GUI_EndDialog(WND_SAT_LIST_POPUP);
                    break;
                case STBK_RIGHT:
                case STBK_LEFT:
                    if(s_CurMode == SAT_DELETE)
                    {
                        if(0 == strcasecmp("btn_satlist_popup_ok", GUI_GetFocusWidget()))
                        {
                            GUI_SetProperty("img_satlist_popup_ok_back","img","s_bar_choice_blue4_l");
                            GUI_SetFocusWidget("btn_satlist_popup_cancel");
                        }
                        else if(0 == strcasecmp("btn_satlist_popup_cancel", GUI_GetFocusWidget()))
                        {
                            GUI_SetProperty("img_satlist_popup_ok_back","img","s_bar_choice_blue4");
                            GUI_SetFocusWidget("btn_satlist_popup_ok");
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

SIGNAL_HANDLER int app_satlist_popup_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
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
                case STBK_UP:
                    {
                        GUI_GetProperty("box_satlist_popup_para", "select", &box_sel);
                        if(SAT_LIST_POPUP_BOX_ITEM_NAME == box_sel)
                        {
                            box_sel = SAT_LIST_POPUP_BOX_ITEM_OK;
                            GUI_SetProperty("box_satlist_popup_para", "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                    #if (DOUBLE_S2_SUPPORT == 0)// not support double tuner input
                        else if(SAT_LIST_POPUP_BOX_ITEM_OK == box_sel)
                        {
                            box_sel = SAT_LIST_POPUP_BOX_ITEM_DIRECT;
                            GUI_SetProperty("box_satlist_popup_para", "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                    #endif

                    }
                    break;

                case STBK_DOWN:
                    {
                        GUI_GetProperty("box_satlist_popup_para", "select", &box_sel);
                        if(SAT_LIST_POPUP_BOX_ITEM_OK == box_sel)
                        {
                            box_sel = SAT_LIST_POPUP_BOX_ITEM_NAME;
                            GUI_SetProperty("box_satlist_popup_para", "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                    #if (DOUBLE_S2_SUPPORT == 0)// not support double tuner input
                        else if(SAT_LIST_POPUP_BOX_ITEM_DIRECT == box_sel)
                        {
                            box_sel = SAT_LIST_POPUP_BOX_ITEM_OK;
                            GUI_SetProperty("box_satlist_popup_para", "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                    #endif
                    }
                    break;

                case STBK_LEFT:
                case STBK_RIGHT:

                    break;

                case STBK_OK:
                    {
                        GUI_GetProperty("box_satlist_popup_para", "select", &box_sel);
                        if(SAT_LIST_POPUP_BOX_ITEM_NAME == box_sel)
                        {
#define WND_KEYBOARD_X 360
#define WND_KEYBOARD_Y 130
                            static PopKeyboard keyboard;
                            memset(&keyboard, 0, sizeof(PopKeyboard));
                            keyboard.pos.x = WND_KEYBOARD_X;
                            keyboard.pos.y = WND_KEYBOARD_Y;
                            GUI_GetProperty("btn_sat_list_name", "string", &keyboard.in_name);
                            keyboard.max_num = MAX_SAT_NAME - 1;
                            keyboard.out_name   = NULL;
                            keyboard.change_cb  = NULL;
                            keyboard.release_cb = _sat_rename_release_cb;
                            keyboard.usr_data   = KEY_LAN_INVALID_CHAR_NAME;//sat name is not ','
                            multi_language_keyboard_create(&keyboard);
                            ret = EVENT_TRANSFER_STOP;
                        }
                        break;
                    }
                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

void app_sat_list_menu_exec(void)
{
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif
    GUI_CreateDialog(WND_SAT_LIST);
}

#if SAT_LIST_MOVE_SORT_SUPPORT
static int32_t _sat_cmp_name(const void *p1, const void *p2)
{
    uint8_t* name1 = 0;
    uint8_t* name2 = 0;
    name1 = ((GxBusPmDataSat*)p1)->sat_s.sat_name;
    name2 = ((GxBusPmDataSat*)p2)->sat_s.sat_name;

    return strcasecmp((const char*)name1,(const char*)name2);
}

static int32_t _sat_cmp_name_reverse(const void *p1, const void *p2)
{
    uint8_t* name1 = 0;
    uint8_t* name2 = 0;
    name1 = ((GxBusPmDataSat*)p1)->sat_s.sat_name;
    name2 = ((GxBusPmDataSat*)p2)->sat_s.sat_name;

    return strcasecmp((const char*)name2,(const char*)name1);
}

static int32_t _sat_cmp_longitude(const void *p1, const void *p2)
{
    uint16_t name1 = 0;
    uint16_t name2 = 0;
    name1 = ((GxBusPmDataSat*)p1)->sat_s.longitude;
    name2 = ((GxBusPmDataSat*)p2)->sat_s.longitude;

    return name1-name2;
}

static int32_t _sat_cmp_longitude_reverse(const void *p1, const void *p2)
{
    uint16_t name1 = 0;
    uint16_t name2 = 0;
    name1 = ((GxBusPmDataSat*)p1)->sat_s.longitude;
    name2 = ((GxBusPmDataSat*)p2)->sat_s.longitude;

    return name2-name1;
}

int app_sat_list_sort(sat_sort_mode sort_mode)
{
    int sat_num,i;
    GxBusPmDataSat *sat_list=NULL;
    sat_num = GxBus_PmSatNumGet();
    sat_list = (GxBusPmDataSat*)GxCore_Malloc(sat_num*sizeof(GxBusPmDataSat));
    if(sat_list == NULL)
        return -1;

    memset(sat_list,0,sat_num*sizeof(GxBusPmDataSat));
    GxBus_PmLock();
    GxBus_PmSatsGetByPos(0,sat_num,sat_list);

    if(sort_mode == SAT_SORT_NAEM_A_Z)
    {
        qsort(sat_list,sat_num,sizeof(GxBusPmDataSat),_sat_cmp_name);
    }
    else if(sort_mode == SAT_SORT_NAEM_Z_A)
    {
        qsort(sat_list,sat_num,sizeof(GxBusPmDataSat),_sat_cmp_name_reverse);
    }
    else if(sort_mode == SAT_SORT_LONGITUDE_0_9)
    {
        qsort(sat_list,sat_num,sizeof(GxBusPmDataSat),_sat_cmp_longitude);
    }
    else if(sort_mode == SAT_SORT_LONGITUDE_9_0)
    {
        qsort(sat_list,sat_num,sizeof(GxBusPmDataSat),_sat_cmp_longitude_reverse);
    }
    for(i=0;i<sat_num;i++)
    {
        sat_list[i].pos=i;
        GxBus_PmSatModify(&sat_list[i]);
    }
    GxBus_PmUnlock();
    GxBus_PmSync(GXBUS_PM_SYNC_SAT);
    GxCore_Free(sat_list);
    return 0;
}


int app_sat_list_move(uint32_t* selected_pos, int selected_num, int dst_pos)
{
    int sat_num,i,pos0;
    GxBusPmDataSat *selected_list=NULL,*sat_list=NULL,*new_sat_list=NULL;

    pos0=0;
    for(i=0;i<selected_num;i++)
    {
        if(selected_pos[i] == dst_pos)
            return 0;
    }

    sat_num = GxBus_PmSatNumGet();
    selected_list = (GxBusPmDataSat*)GxCore_Malloc(selected_num*sizeof(GxBusPmDataSat));
    sat_list = (GxBusPmDataSat*)GxCore_Malloc(sat_num*sizeof(GxBusPmDataSat));
    new_sat_list = (GxBusPmDataSat*)GxCore_Malloc(sat_num*sizeof(GxBusPmDataSat));
    if(selected_list == NULL || sat_list == NULL||new_sat_list == NULL)
        goto err;

    memset(sat_list,0,sat_num*sizeof(GxBusPmDataSat));
    memset(new_sat_list,0,sat_num*sizeof(GxBusPmDataSat));
    memset(selected_list,0,selected_num*sizeof(GxBusPmDataSat));

    GxBus_PmLock();
    GxBus_PmSatsGetByPos(0,sat_num,sat_list);
    for(i=0;i<selected_num;i++)
    {
        memcpy(&selected_list[i],&sat_list[selected_pos[i]],sizeof(GxBusPmDataSat));
        sat_list[selected_pos[i]].id=0;
    }
    for(i=0;i<dst_pos;i++)
    {
        if(sat_list[i].id!=0)
        {
            memcpy(&new_sat_list[pos0],&sat_list[i],sizeof(GxBusPmDataSat));
            pos0++;
        }
    }
    for(i=0;i<selected_num;i++)
    {
        memcpy(&new_sat_list[pos0],&selected_list[i],sizeof(GxBusPmDataSat));
        pos0++;
    }
    for(i=dst_pos;i<sat_num;i++)
    {
        if(sat_list[i].id != 0)
        {
            memcpy(&new_sat_list[pos0],&sat_list[i],sizeof(GxBusPmDataSat));
            pos0++;
        }
    }
    for(i=0;i<sat_num;i++)
    {
        new_sat_list[i].pos=i;
        GxBus_PmSatModify(&new_sat_list[i]);
    }
    GxBus_PmUnlock();
    GxBus_PmSync(GXBUS_PM_SYNC_SAT);
    GxCore_Free(selected_list);
    GxCore_Free(sat_list);
    GxCore_Free(new_sat_list);
    return 0;
err:
    GxCore_Free(selected_list);
    GxCore_Free(sat_list);
    GxCore_Free(new_sat_list);
    return -1;

}
#endif
#endif
