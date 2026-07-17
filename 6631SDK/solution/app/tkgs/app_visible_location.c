#include "app.h"
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "module/app_frontend.h"
#include "app_pop.h"
#include "app_epg.h"
#include "full_screen.h"
#include "app_book.h"
#include "module/tkgs/gxtkgs.h"
#include "app_wnd_system_setting_opt.h"
#include "app_windows.h"

#if TKGS_SUPPORT
enum VISIABLE_LOCATION_MODE
{
    LOCATION_ADD = 0,
    LOCATION_EDIT,
    LOCATION_DELETE,
    LOCATION_CHANGED
};

enum VISIABLE_LOCATION_POPUP_BOX
{
    VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ = 0,
    VISIABLE_LOCATION_POPUP_BOX_ITEM_POLAR,
    VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM,
    VISIABLE_LOCATION_POPUP_BOX_ITEM_PID,
    VISIABLE_LOCATION_POPUP_BOX_ITEM_OK
};

enum VISIABLE_LOCATION_PARA_VALID_OR_NOT
{
    VISIABLE_LOCATION_PARA_VALID=0,
    VISIABLE_LOCATION_PARA_INVALID
};

#define MAX_PID_LEN (4)
#define MAX_FRE_LEN (5)
#define MAX_LEN 30

static GxMsgProperty_NodeByPosGet s_TKGSSat = {0};
static uint8_t s_CurMode=0;
static uint8_t s_Changed=0;
static int s_CurVisiableLocationSel = 0;    //location listview sel
static uint16_t s_visiblelocation_choise = 0;


static char s_visiblelocation_edit_input[6] = {0};
static int match_exist_sel =0;
static bool match_status = 0;
static bool edit_command_mode = 1;
static bool edit_del_to_zero = 0;
static bool up_down_status = 0;
static bool visible_location_exist_status = 0;
static char buffer_fre_bak[MAX_FRE_LEN+1] = {0};
static char buffer_sym_bak[MAX_FRE_LEN+1] = {0};
static char buffer_pid_bak[MAX_PID_LEN+1] = {0};
/* widget macro -------------------------------------------------------- */
#define TKGS_DEBUG 0
#if TKGS_DEBUG
static bool init_state = 0;
#endif
static GxTkgsLocation s_TKGS_Location;

void _visiblelocation_getTKGSSatelliteInfo(void);

extern status_t GxBus_TkgsLocationGet(GxTkgsLocation* location);
extern status_t  GxBus_TkgsLocationSet(GxTkgsLocation* location);

status_t app_tkgs_set_default_location(void)
{
	GxTkgsLocation location = {0};
    uint32_t pos;
    GxMsgProperty_NodeNumGet sat_num = {0};
    GxMsgProperty_NodeByPosGet node_sat = {0};

    // 直接使用接口可以不受制于tkgs的服务初始化与否，特别是起机过程，消息接口的话需要保证在service初始化后调用才行。
	//app_send_msg_exec(GXMSG_TKGS_GET_LOCATION,&location);
    GxBus_TkgsLocationGet(&location);
	if ((location.visible_num != 0) || (location.invisible_num != 0))
	{
		app_log_debug("========%s, %d, %d, %d\n", __FUNCTION__, __LINE__, location.visible_num, location.invisible_num);
		return GXCORE_SUCCESS;
	}

    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
    for(pos = 0;pos<sat_num.node_num;pos++)
    {
        node_sat.node_type = NODE_SAT;
        node_sat.pos = pos;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
        if(0 == strncasecmp((const char*)node_sat.sat_data.sat_s.sat_name,"turksat",7))
        {
            break;
        }
    }

	if (pos == sat_num.node_num)
	{
		app_log_flow("====%s, %d\n", __FUNCTION__, __LINE__);
		return GXCORE_ERROR;
	}

	//location.visible_location[0].sat_id = node_sat.sat_data.id;
	//location.visible_location[0].search_demux_id = 0;
	//location.visible_location[0].frequency = 12130;
	//location.visible_location[0].symbol_rate = 27500;
	//location.visible_location[0].polar = GXBUS_PM_TP_POLAR_H;
	//location.visible_location[0].pid = 8181;
	//location.visible_num += 1;

	location.visible_location[0].sat_id = node_sat.sat_data.id;
	location.visible_location[0].search_demux_id = 0;
	location.visible_location[0].frequency = 12380 ;
	location.visible_location[0].symbol_rate = 27500;
	location.visible_location[0].polar = GXBUS_PM_TP_POLAR_V;
	location.visible_location[0].pid = 8181;
	location.visible_num += 1;

	location.visible_location[1].sat_id = node_sat.sat_data.id;
	location.visible_location[1].search_demux_id = 0;
	location.visible_location[1].frequency = 12423 ;
	location.visible_location[1].symbol_rate = 27500;
	location.visible_location[1].polar = GXBUS_PM_TP_POLAR_V;
	location.visible_location[1].pid = 8181;
	location.visible_num += 1;

	location.visible_location[2].sat_id = node_sat.sat_data.id;
	location.visible_location[2].search_demux_id = 0;
	location.visible_location[2].frequency = 12423 ;
	location.visible_location[2].symbol_rate = 30000 ;
	location.visible_location[2].polar = GXBUS_PM_TP_POLAR_H;
	location.visible_location[2].pid = 8181;
	location.visible_num += 1;

	location.visible_location[3].sat_id = node_sat.sat_data.id;
	location.visible_location[3].search_demux_id = 0;
	location.visible_location[3].frequency = 12458 ;
	location.visible_location[3].symbol_rate = 30000;
	location.visible_location[3].polar = GXBUS_PM_TP_POLAR_V;
	location.visible_location[3].pid = 8181;
	location.visible_num += 1;

	location.visible_location[4].sat_id = node_sat.sat_data.id;
	location.visible_location[4].search_demux_id = 0;
	location.visible_location[4].frequency = 12559 ;
	location.visible_location[4].symbol_rate = 30000;
	location.visible_location[4].polar = GXBUS_PM_TP_POLAR_V;
	location.visible_location[4].pid = 8181;
	location.visible_num += 1;

    // 直接使用接口可以不受制于tkgs的服务初始化与否，特别是起机过程，消息接口的话需要保证在service初始化后调用才行。
	//app_send_msg_exec(GXMSG_TKGS_SET_LOCATION,&location);
    GxBus_TkgsLocationSet(&location);
	return GXCORE_SUCCESS;
}

status_t _visiblelocation_setTkgsLocation(void)
{
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    #if TKGS_DEBUG == 0
    return app_send_msg_exec(GXMSG_TKGS_SET_LOCATION,&s_TKGS_Location);
    #else
    return GXCORE_SUCCESS;
    #endif
}

extern void _set_disqec_callback(uint32_t sat_id,fe_sec_voltage_t polar,int32_t sat22k);

void _visiblelocation_getTkgsLocation(void)
{
  //  app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    #if TKGS_DEBUG == 0
    memset(&s_TKGS_Location, 0, sizeof(s_TKGS_Location));
    app_send_msg_exec(GXMSG_TKGS_GET_LOCATION,&s_TKGS_Location);
    #else
    if(init_state == FALSE)
    {

    init_state = TRUE;
    memset(&s_TKGS_Location, 0, sizeof(s_TKGS_Location));
    s_TKGS_Location.visible_num= 4;
    s_TKGS_Location.invisible_num = 0;

    s_TKGS_Location.visible_location[0].sat_id = s_TKGSSat.id;
    s_TKGS_Location.visible_location[0].search_demux_id = 0;
   // s_TKGS_Location.visible_location[0].search_diseqc = _set_disqec_callback;
    s_TKGS_Location.visible_location[0].frequency = 11746;
    s_TKGS_Location.visible_location[0].symbol_rate= 27500;
    s_TKGS_Location.visible_location[0].polar= GXBUS_PM_TP_POLAR_H;
    s_TKGS_Location.visible_location[0].pid= 8181;

    s_TKGS_Location.visible_location[1].sat_id = s_TKGSSat.id;
    s_TKGS_Location.visible_location[1].search_demux_id = 0;
    //s_TKGS_Location.visible_location[1].search_diseqc = _set_disqec_callback;
    s_TKGS_Location.visible_location[1].frequency = 12729;
    s_TKGS_Location.visible_location[1].symbol_rate= 30000;
    s_TKGS_Location.visible_location[1].polar= GXBUS_PM_TP_POLAR_H;
    s_TKGS_Location.visible_location[1].pid= 8181;

    s_TKGS_Location.visible_location[2].sat_id = s_TKGSSat.id;
    s_TKGS_Location.visible_location[2].search_demux_id = 0;
   // s_TKGS_Location.visible_location[2].search_diseqc = _set_disqec_callback;
    s_TKGS_Location.visible_location[2].frequency = 12015;
    s_TKGS_Location.visible_location[2].symbol_rate= 27500;
    s_TKGS_Location.visible_location[2].polar= GXBUS_PM_TP_POLAR_H;
    s_TKGS_Location.visible_location[2].pid= 8181;

    s_TKGS_Location.visible_location[3].sat_id = s_TKGSSat.id;
    s_TKGS_Location.visible_location[3].search_demux_id = 0;
   // s_TKGS_Location.visible_location[3].search_diseqc = _set_disqec_callback;
    s_TKGS_Location.visible_location[3].frequency = 12130;
    s_TKGS_Location.visible_location[3].symbol_rate= 27500;
    s_TKGS_Location.visible_location[3].polar= GXBUS_PM_TP_POLAR_V;
    s_TKGS_Location.visible_location[3].pid= 8181;

    }
    #endif
}


#define TIP_INFO_ALL 0
#define TIP_INFO_ADD 1
#define TIP_INFO_EDITDEL 2

static void _visible_location_show_tip(void)
{
    #define IMG_RED         "img_visible_location_tip_red"
    #define TXT_RED         "text_visible_location_tip_red"
    #define IMG_GREEN       "img_visible_location_tip_green"
    #define TXT_GREEN       "text_visible_location_tip_green"
    #define IMG_YELLOW  "img_visible_location_tip_yellow"
    #define TXT_YELLOW  "text_visible_location_tip_yellow"

    if(s_TKGS_Location.visible_num == 0)
    {
        GUI_SetProperty(IMG_RED, "state", "hide");
        GUI_SetProperty(TXT_RED, "state", "hide");
        GUI_SetProperty(IMG_GREEN, "state", "show");
        GUI_SetProperty(TXT_GREEN, "state", "show");
        GUI_SetProperty(IMG_YELLOW, "state", "hide");
        GUI_SetProperty(TXT_YELLOW, "state", "hide");
    }
    else if(s_TKGS_Location.visible_num == 5)
    {
        GUI_SetProperty(IMG_RED, "state", "show");
        GUI_SetProperty(TXT_RED, "state", "show");
        GUI_SetProperty(IMG_GREEN, "state", "hide");
        GUI_SetProperty(TXT_GREEN, "state", "hide");
        GUI_SetProperty(IMG_YELLOW, "state", "show");
        GUI_SetProperty(TXT_YELLOW, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_RED, "state", "show");
        GUI_SetProperty(IMG_GREEN, "state", "show");
        GUI_SetProperty(IMG_YELLOW, "state", "show");

        GUI_SetProperty(TXT_GREEN, "state", "show");
        GUI_SetProperty(TXT_RED, "state", "show");
        GUI_SetProperty(TXT_YELLOW, "state", "show");
    }
}

static void _visible_location_hide_tip(void)
{
    #define IMG_RED         "img_visible_location_tip_red"
    #define TXT_RED         "text_visible_location_tip_red"
    #define IMG_GREEN       "img_visible_location_tip_green"
    #define TXT_GREEN       "text_visible_location_tip_green"
    #define IMG_YELLOW  "img_visible_location_tip_yellow"
    #define TXT_YELLOW  "text_visible_location_tip_yellow"

    GUI_SetProperty(IMG_RED, "state", "hide");
    GUI_SetProperty(IMG_GREEN, "state", "hide");
    GUI_SetProperty(IMG_YELLOW, "state", "hide");

    GUI_SetProperty(TXT_RED, "state", "hide");
    GUI_SetProperty(TXT_GREEN, "state", "hide");
    GUI_SetProperty(TXT_YELLOW, "state", "hide");


}


int app_tkgs_get_turksat_pInfo(GxMsgProperty_NodeByPosGet ** p_TKGSSat)
{
    _visiblelocation_getTKGSSatelliteInfo();
    if(s_TKGSSat.sat_data.sat_s.longitude == 420)
    {
        *p_TKGSSat = &s_TKGSSat;
        return s_TKGSSat.sat_data.id;
    }

    *p_TKGSSat = NULL;
    return -1;

}

bool app_tkgs_check_sat_is_turish(GxMsgProperty_NodeByPosGet * p_nodesat)
{
    if(0 == strncasecmp((const char*)p_nodesat->sat_data.sat_s.sat_name,"turksat",7))
    {
        if((p_nodesat->sat_data.sat_s.longitude == 420)
            &&(p_nodesat->sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST))
        {
            return TRUE;
        }
    }
    return FALSE;

}

void _visiblelocation_getTKGSSatelliteInfo(void)
{
    uint32_t pos;
    GxMsgProperty_NodeNumGet sat_num = {0};
    GxMsgProperty_NodeByPosGet node_sat = {0};
   // app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
    for(pos = 0;pos<sat_num.node_num;pos++)
    {
        node_sat.node_type = NODE_SAT;
        node_sat.pos = pos;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
        if(0 == strncasecmp((const char*)node_sat.sat_data.sat_s.sat_name,"turksat",7))
        {
            if((node_sat.sat_data.sat_s.longitude == 420)
                &&(node_sat.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST))
            {
                memcpy(&s_TKGSSat, &node_sat , sizeof(GxMsgProperty_NodeByPosGet));
                break;
            }
        }
    }
    //app_log_debug("satname= %s\n",s_TKGSSat.sat_data.sat_s.sat_name);
}


static int _app_visible_location_proc_delete_cb(PopDlgRet ret)
{
    uint32_t i;
    uint32_t list_sel = 0 ;
    if (POP_VAL_OK == ret)
    {
        GUI_GetProperty("list_visible_location", "select", &list_sel);
        if((list_sel <s_TKGS_Location.visible_num) &&(s_TKGS_Location.visible_num > 0 ))
        {
            for(i = list_sel;i+1<s_TKGS_Location.visible_num;i++)
            {
                 memcpy(&(s_TKGS_Location.visible_location[i]),&(s_TKGS_Location.visible_location[i+1]) , sizeof(GxTkgsLocationParm));
            }
            s_TKGS_Location.visible_num --;
            _visiblelocation_setTkgsLocation();
            _visible_location_show_tip();

            GUI_SetProperty("list_visible_location", "update_all", NULL);
            if(s_TKGS_Location.visible_num >0)
            {
                 if(list_sel >= s_TKGS_Location.visible_num)
                 {
                      list_sel = s_TKGS_Location.visible_num -1;
                 }
                 GUI_SetProperty("list_visible_location", "select", &list_sel);
            }
        }
    }
    return 0 ;
}


SIGNAL_HANDLER int app_visible_location_create(GuiWidget *widget, void *usrdata)
{

//GXMSG_TKGS_GET_LOCATION
  //  app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);
    // add in 20120702
    g_AppFullArb.timer_stop();

    s_visiblelocation_choise = 0;
    //get total sat num
    s_CurVisiableLocationSel = -1;
    //memset(&s_TKGSSat,0,sizeof(GxMsgProperty_NodeByPosGet));
    _visiblelocation_getTKGSSatelliteInfo();

    _visiblelocation_getTkgsLocation();

    if(s_TKGS_Location.visible_num > 0)
    {
        s_CurVisiableLocationSel = 0;
        if(s_TKGSSat.sat_data.sat_s.longitude != 420)
        {
            s_TKGSSat.node_type = NODE_SAT;
            s_TKGSSat.id = s_TKGS_Location.visible_location[0].sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &s_TKGSSat);
        }
    }

    GUI_SetProperty("list_visible_location", "update_all", NULL);
    GUI_SetProperty("list_visible_location", "select", &s_CurVisiableLocationSel);
    _visible_location_show_tip();
    // id manage
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_visible_location_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}



SIGNAL_HANDLER int app_visible_location_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    uint32_t list_sel;

    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
   // app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {

            case VK_BOOK_TRIGGER:
				app_system_reset_unfocus_image();
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_VISIBLE_LOCATION);
                ret = EVENT_TRANSFER_STOP;
                #if TKGS_DEBUG
                init_state = FALSE;
                #endif
                break;

            case STBK_LEFT:
            case STBK_RIGHT:
                ret = EVENT_TRANSFER_STOP;
                break;

            case STBK_GREEN:    //add
                //zl
                if(s_TKGS_Location.visible_num < 5)
                {
                    memset(s_visiblelocation_edit_input,'\0',6);

                    s_CurMode = LOCATION_ADD;
                    _visible_location_hide_tip();
                    GUI_CreateDialog(WND_VISIBLELOCATION_POPUP);
                    ret = EVENT_TRANSFER_STOP;
                }
                break;

            case STBK_YELLOW:   //edit
                //zl
                if(s_TKGS_Location.visible_num > 0)
                {
                    memset(s_visiblelocation_edit_input,'\0',6);
                    if(0 == strcasecmp("list_visible_location", GUI_GetFocusWidget()))
                    {
                        s_CurMode = LOCATION_EDIT;
                        _visible_location_hide_tip();
                        GUI_CreateDialog(WND_VISIBLELOCATION_POPUP);
                        ret = EVENT_TRANSFER_STOP;
                    }
                }
                break;

            case STBK_RED:      //del
                GUI_GetProperty("list_visible_location", "select", &list_sel);
                if((list_sel <s_TKGS_Location.visible_num) &&(s_TKGS_Location.visible_num > 0 ))
                {
                     PopDlg pop ;
                     memset(&pop, 0, sizeof(PopDlg));

                     pop.type = POP_TYPE_YES_NO;
                     pop.str = STR_ID_DLE_TP;
                     pop.format = POP_FORMAT_DLG;
                     pop.mode = POP_MODE_UNBLOCK;
                     pop.create_cb = NULL;
                     pop.exit_cb = _app_visible_location_proc_delete_cb;
                     popdlg_create(&pop);

                }
                ret = EVENT_TRANSFER_STOP;
                break;


            case STBK_PAGE_UP:
         case STBK_PAGE_DOWN:
            {
                ret = EVENT_TRANSFER_STOP;
            }
            break;

        default:
            break;
        }
    }

    return ret;
}

SIGNAL_HANDLER int app_visible_location_lost_focus(GuiWidget *widget, void *usrdata)
{
    if(s_TKGS_Location.visible_num > 0)
    {
        if(0 == strcasecmp("list_visible_location", GUI_GetFocusWidget()))
        {
            GUI_SetProperty("list_visible_location", "active", &s_CurVisiableLocationSel);
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_visible_location_got_focus(GuiWidget *widget, void *usrdata)
{
    int sel =-1;
    int act_sel= -1;

    GUI_SetProperty("list_visible_location", "active", &act_sel);

    sel = s_CurVisiableLocationSel;

    if(LOCATION_EDIT == s_CurMode && LOCATION_CHANGED == s_Changed)//edit tp and changed
    {
        GUI_SetProperty("list_visible_location", "update_row", &sel);
    }
    else    if(LOCATION_ADD == s_CurMode && LOCATION_CHANGED == s_Changed)//add tp
    {
        // get total tp number
        if(s_TKGS_Location.visible_num > 0)
        {
            sel = s_TKGS_Location.visible_num-1;
        }
        else
        {
            sel = -1;
        }
        GUI_SetProperty("list_visible_location", "update_all", NULL);
        GUI_SetProperty("list_visible_location", "select", &sel);

    }
    else if(LOCATION_DELETE == s_CurMode && LOCATION_CHANGED == s_Changed)  //del tp
    {

        if(sel > 0)
        {
            sel -= 1;
        }
        else
        {
            if(s_TKGS_Location.visible_num == 0)
            sel = -1;
        }
        GUI_SetProperty("list_visible_location", "update_all", NULL);
        GUI_SetProperty("list_visible_location", "select", &sel);
    }


    // for clear the frontend mseeage queue

       s_CurVisiableLocationSel = sel;
    //clear
    s_CurMode=0;
    s_Changed=0;

    if(1 == match_status)
    {
           GUI_SetProperty("list_visible_location", "select", &match_exist_sel);
        s_CurVisiableLocationSel = match_exist_sel;
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_visible_location_list_create(GuiWidget *widget, void *usrdata)
{
   // app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_visible_location_list_change(GuiWidget *widget, void *usrdata)
{
    uint32_t list_sel;
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    GUI_GetProperty("list_visible_location", "select", &list_sel);
    s_CurVisiableLocationSel = list_sel;
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_visible_location_list_get_total(GuiWidget *widget, void *usrdata)
{
   // app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    //memset(&s_TKGS_Location, 0, sizeof(s_TKGS_Location));
    //app_send_msg_exec(GXMSG_TKGS_GET_LOCATION,&s_TKGS_Location);
    _visiblelocation_getTkgsLocation();
    return s_TKGS_Location.visible_num;
}

SIGNAL_HANDLER int app_visible_location_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;
    static char buffer1[MAX_LEN];
    static char buffer2[MAX_LEN];
    static char buffer3[MAX_LEN];
    static char buffer4[MAX_LEN];
    static char buffer5[MAX_LEN];
    GxTkgsLocationParm *pCurLocation;
    item = (ListItemPara*)usrdata;
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;
    pCurLocation = &(s_TKGS_Location.visible_location[item->sel]);
    /*
    //col-0: choice
    item->x_offset = 0;

    if (item->sel >= s_TKGS_Location.visible_num)//ID_UNEXIST
    {
        item->image = NULL;
    }
    else
    {
        item->image = "s_choice";
    }
    item->string = NULL;



    item = item->next;
    */
    //col-1: num
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer1, 0, MAX_LEN);
    sprintf(buffer1, " %d", (item->sel+1));
    item->string = buffer1;



    //col-1: freq
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer2, 0, MAX_LEN);
    sprintf(buffer2,"%d MHz",pCurLocation->frequency);
    item->string = buffer2;

    //col-1: polar
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer3, 0, MAX_LEN);
    if(pCurLocation->polar == GXBUS_PM_TP_POLAR_V)
    {
        sprintf(buffer3,"%s", "V");
    }
    else if(pCurLocation->polar == GXBUS_PM_TP_POLAR_H)
    {
        sprintf(buffer3,"%s", "H");
    }
    item->string = buffer3;

    //col-1: symb
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer4, 0, MAX_LEN);
    sprintf(buffer4,"%d Ks/s",pCurLocation->symbol_rate);
    item->string = buffer4;

    //col-1: PID
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    memset(buffer5, 0, MAX_LEN);
    sprintf(buffer5,"%d",pCurLocation->pid);
    item->string = buffer5;
    return EVENT_TRANSFER_STOP;
}





SIGNAL_HANDLER int app_visible_location_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    //uint32_t sel=0;
    int cur_sel = 0;
   // app_log_flow("func=%s,line=%d\n",__FUNCTION__,__LINE__);
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
                             memset(s_visiblelocation_edit_input,'\0',6);
                    GUI_GetProperty("list_visible_location", "select", &cur_sel);
                    if(cur_sel == 0)
                    {
                        cur_sel = s_TKGS_Location.visible_num - 1;
                        GUI_SetProperty("list_visible_location", "select", &cur_sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;
                case STBK_DOWN:
                        memset(s_visiblelocation_edit_input,'\0',6);

                    GUI_GetProperty("list_visible_location", "select", &cur_sel);
                    if(cur_sel == (s_TKGS_Location.visible_num - 1))
                    {
                        cur_sel = 0;
                        GUI_SetProperty("list_visible_location", "select", &cur_sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;
                case STBK_PAGE_UP:
                {
                    uint32_t sel;
                    GUI_GetProperty("list_visible_location", "select", &sel);
                    if (sel == 0)
                    {
                        sel = s_TKGS_Location.visible_num -1;
                        GUI_SetProperty("list_visible_location", "select", &sel);
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
                    GUI_GetProperty("list_visible_location", "select", &sel);
                    if (s_TKGS_Location.visible_num == sel+1)
                    {
                        sel = 0;
                        GUI_SetProperty("list_visible_location", "select", &sel);
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







static bool _check_visible_location_para_valid(GxBusPmDataSat *p_sat, GxTkgsLocationParm* tp_para)
{
    if((p_sat == NULL) || (tp_para == NULL))
        return false;

    if((tp_para->symbol_rate >= 800) && (tp_para->symbol_rate <= 46000))
    {
        uint32_t tp_frequency = 0;
        GxBusPmDataTP temp_tp = {0};
        temp_tp.frequency= tp_para->frequency;
        temp_tp.tp_s.polar = tp_para->polar;
        //app_log_debug("pid =  %d\n",tp_para->pid);
        if((tp_para->pid >0) &&(tp_para->pid <= 8191))
        {
            tp_frequency = app_show_to_tp_freq(p_sat, &temp_tp);
         //   app_log_debug("tp_frequency =  %d\n",tp_frequency);

            return app_frontend_dvbs_frequency_check(tp_frequency);
        }
    }
  //  app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    return false;
}


static uint32_t _check_visible_location_para_exist(GxTkgsLocationParm*input_para)
{
    uint32_t i;
    GxTkgsLocationParm * pVisiableLocation;
    GxTkgsLocationParm tp_para;

    if(input_para == NULL)
        return -1;
    memcpy(&tp_para,input_para,sizeof(GxTkgsLocationParm));
    pVisiableLocation = s_TKGS_Location.visible_location;
    for(i=0;i<s_TKGS_Location.visible_num;i++)
    {
        if((tp_para.frequency == pVisiableLocation->frequency)
            &&(tp_para.symbol_rate == pVisiableLocation->symbol_rate)
            &&(tp_para.polar == pVisiableLocation->polar)
            &&(tp_para.pid == pVisiableLocation->pid))
        {
            return i;
        }
        pVisiableLocation ++;
    }

    return -1;

}



status_t _visiblelocation_edit(void)
{
#define TP_LEN  20

    //GxMsgProperty_NodeModify NodeModify;
    char *atoi_buf = NULL;
    uint32_t sel=0;
    GxTkgsLocationParm InputVisiableLocationPara={0};
    char buffer[TP_LEN] = {0};
    status_t ret = GXCORE_ERROR;
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    //get input value
    //freq
    GUI_GetProperty("text_visiblelocation_popup_freq2", "string", &atoi_buf);
    InputVisiableLocationPara.frequency = atoi(atoi_buf);
    //sym
    GUI_GetProperty("text_visiblelocation_popup_sym2", "string", &atoi_buf);
    InputVisiableLocationPara.symbol_rate = atoi(atoi_buf);
    GUI_GetProperty("cmb_visiblelocation_popup_polar", "select", &sel);

    GUI_GetProperty("text_visiblelocation_popup_pid2", "string", &atoi_buf);
    InputVisiableLocationPara.pid = atoi(atoi_buf);

    if((InputVisiableLocationPara.symbol_rate == 0) || (InputVisiableLocationPara.frequency == 0)||(InputVisiableLocationPara.pid == 0))
        return ret;


    if(0 == sel)
        InputVisiableLocationPara.polar = GXBUS_PM_TP_POLAR_H;
    else
        InputVisiableLocationPara.polar = GXBUS_PM_TP_POLAR_V;



    if(false == _check_visible_location_para_valid(&s_TKGSSat.sat_data, &InputVisiableLocationPara))
    {//show: invalid TP value

        GUI_SetProperty("edit_visiblelocation_popup_freq", "clear", NULL);
              GUI_SetProperty("edit_visiblelocation_popup_sym", "clear", NULL);
              memset(s_visiblelocation_edit_input,'\0',6);

        //hide the box
        GUI_SetProperty("box_visiblelocation_popup_para", "state", "hide");
        //show info
        GUI_SetProperty("text_visiblelocation_popup_tip1", "string", STR_ID_INVALID_TP);

        GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        memset(buffer, 0 ,TP_LEN);
        sprintf(buffer, "%05d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].frequency);
        GUI_SetProperty("edit_visiblelocation_popup_freq", "string", buffer);
        GUI_SetProperty("text_visiblelocation_popup_freq2", "string", buffer);
        //polar
        sel = s_TKGS_Location.visible_location[s_CurVisiableLocationSel].polar;
        GUI_SetProperty("cmb_visiblelocation_popup_polar", "select", &sel);
        //sym
        memset(buffer, 0 ,TP_LEN);
        sprintf(buffer, "%05d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].symbol_rate);
        GUI_SetProperty("edit_visiblelocation_popup_sym", "string", buffer);
        GUI_SetProperty("text_visiblelocation_popup_sym2", "string", buffer);
        //pid
        memset(buffer, 0 ,TP_LEN);
        sprintf(buffer, "%04d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].pid);
        GUI_SetProperty("edit_visiblelocation_popup_pid", "string", buffer);
        GUI_SetProperty("text_visiblelocation_popup_pid2", "string", buffer);

        //resume
        GUI_SetProperty("text_visiblelocation_popup_tip1", "string", " ");
        //show the box
        GUI_SetProperty("box_visiblelocation_popup_para", "state", "show");
        GUI_SetFocusWidget("box_visiblelocation_popup_para");

        GUI_SetProperty("edit_visiblelocation_popup_freq", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_freq2", "state", "show");
        GUI_SetProperty("edit_visiblelocation_popup_sym", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_sym2", "state", "show");
        GUI_SetProperty("edit_visiblelocation_popup_pid", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_pid2", "state", "show");
        GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);
    }
    else//value is ok, check if the tp is exist
    {
        uint32_t location_id;
        location_id = _check_visible_location_para_exist(&InputVisiableLocationPara);
        if(location_id != -1)
        {//tp already exist
            //hide the box
            match_status = 1;
            match_exist_sel = location_id;

            GUI_SetProperty("box_visiblelocation_popup_para", "state", "hide");
            //show info
            GUI_SetProperty("text_visiblelocation_popup_tip1", "string", STR_ID_LOCATION_EXIST);

            GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);

            //zl
            GUI_SetProperty("text_visiblelocation_popup_tip1_exist", "state", "show");
            GUI_SetProperty("btn_visiblelocation_tip_cancel_exist", "state", "show");
            GUI_SetProperty("btn_visiblelocation_tip_ok_exist", "state", "show");
            GUI_SetProperty("img_visiblelocation_tip_opt_exist", "state", "show");

            GUI_SetFocusWidget("btn_visiblelocation_tip_ok_exist");
            GUI_SetProperty("img_visiblelocation_tip_opt_exist","img","s_bar_choice_blue4");
            visible_location_exist_status = 1;
        }
        else//modify
        {
            match_status = 0;

            s_TKGS_Location.visible_location[s_CurVisiableLocationSel].frequency = InputVisiableLocationPara.frequency;
            s_TKGS_Location.visible_location[s_CurVisiableLocationSel].symbol_rate= InputVisiableLocationPara.symbol_rate;
            s_TKGS_Location.visible_location[s_CurVisiableLocationSel].polar= InputVisiableLocationPara.polar;
            s_TKGS_Location.visible_location[s_CurVisiableLocationSel].pid = InputVisiableLocationPara.pid;

            if(GXCORE_SUCCESS == _visiblelocation_setTkgsLocation())
            {
                //int SatSel = 0;
                s_Changed = LOCATION_CHANGED;
                ret = GXCORE_SUCCESS;

                visible_location_exist_status = 0;
            }
        }
    }

    return ret;
}

/*
function:
data :2012 11 1
*/

status_t _visiblelocation_add(void)
{
    //GxMsgProperty_NodeAdd NodeAdd;
    char *atoi_buf = NULL;
    uint32_t location_id;
    uint32_t sel=0;
    GxTkgsLocationParm VisibleLocationPara={0};
    status_t ret = GXCORE_ERROR;
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    //get input value
    //freq
    GUI_GetProperty("text_visiblelocation_popup_freq2", "string", &atoi_buf);
    VisibleLocationPara.frequency = atoi(atoi_buf);
    //sym
    GUI_GetProperty("text_visiblelocation_popup_sym2", "string", &atoi_buf);
    VisibleLocationPara.symbol_rate = atoi(atoi_buf);
    GUI_GetProperty("cmb_visiblelocation_popup_polar", "select", &sel);

    GUI_GetProperty("text_visiblelocation_popup_pid2", "string", &atoi_buf);
    VisibleLocationPara.pid = atoi(atoi_buf);

    if(0 == sel)
        VisibleLocationPara.polar = GXBUS_PM_TP_POLAR_H;
    else
        VisibleLocationPara.polar = GXBUS_PM_TP_POLAR_V;

    if(false == _check_visible_location_para_valid(&s_TKGSSat.sat_data, &VisibleLocationPara))
    {//show: invalid TP value
        //hide the box

        GUI_SetProperty("edit_visiblelocation_popup_freq", "clear", NULL);
              GUI_SetProperty("edit_visiblelocation_popup_sym", "clear", NULL);
              memset(s_visiblelocation_edit_input,'\0',6);

        GUI_SetProperty("box_visiblelocation_popup_para", "state", "hide");
        //show info
        GUI_SetProperty("text_visiblelocation_popup_tip1", "string", STR_ID_INVALID_TP);

        GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);

        GxCore_ThreadDelay(1200);

        GUI_SetProperty("edit_visiblelocation_popup_freq", "string", "03000");
        GUI_SetProperty("text_visiblelocation_popup_freq2", "string", "03000");
        //polar
        sel = 0;
        GUI_SetProperty("cmb_visiblelocation_popup_polar", "select", &sel);
        //sym
        GUI_SetProperty("edit_visiblelocation_popup_sym", "string", "01000");
        GUI_SetProperty("text_visiblelocation_popup_sym2", "string", "01000");
        //pid
        GUI_SetProperty("edit_visiblelocation_popup_pid", "string", "8181");
        GUI_SetProperty("text_visiblelocation_popup_pid2", "string", "8181");

        //resume
        GUI_SetProperty("text_visiblelocation_popup_tip1", "string", " ");
        //show the box
        GUI_SetProperty("box_visiblelocation_popup_para", "state", "show");
        GUI_SetFocusWidget("box_visiblelocation_popup_para");

        GUI_SetProperty("edit_visiblelocation_popup_freq", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_freq2", "state", "show");
        GUI_SetProperty("edit_visiblelocation_popup_sym", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_sym2", "state", "show");
        GUI_SetProperty("edit_visiblelocation_popup_pid", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_pid2", "state", "show");

        GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);
    }
    else//value is ok, check if the tp is exist
    {
        location_id = _check_visible_location_para_exist(&VisibleLocationPara);
        if(location_id != -1)
        {//tp already exist
            //hide the box
            //zl
            match_status = 1;
            match_exist_sel = location_id;


            GUI_SetProperty("box_visiblelocation_popup_para", "state", "hide");
            //show info
            GUI_SetProperty("text_visiblelocation_popup_tip1", "string", STR_ID_LOCATION_EXIST);

            GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);

            //zl
                        GUI_SetProperty("text_visiblelocation_popup_tip1_exist", "state", "show");
                        GUI_SetProperty("btn_visiblelocation_tip_cancel_exist", "state", "show");
                        GUI_SetProperty("btn_visiblelocation_tip_ok_exist", "state", "show");
                        GUI_SetProperty("img_visiblelocation_tip_opt_exist", "state", "show");

                        GUI_SetFocusWidget("btn_visiblelocation_tip_ok_exist");
                        GUI_SetProperty("img_visiblelocation_tip_opt_exist","img","s_bar_choice_blue4");
                        visible_location_exist_status = 1;

        }
        else//save
        {
            GxTkgsLocationParm * pVisibleLocation;
            match_status = 0;
            location_id = s_TKGS_Location.visible_num;
            pVisibleLocation = &s_TKGS_Location.visible_location[location_id];
            memset(pVisibleLocation, 0, sizeof(GxTkgsLocationParm));
            s_TKGS_Location.visible_num = location_id + 1;
            pVisibleLocation->frequency = VisibleLocationPara.frequency;
            pVisibleLocation->symbol_rate = VisibleLocationPara.symbol_rate;
            pVisibleLocation->polar = VisibleLocationPara.polar;
            pVisibleLocation->pid = VisibleLocationPara.pid;

            pVisibleLocation->sat_id = s_TKGSSat.sat_data.id;
            pVisibleLocation->search_demux_id = 0;
            //pVisibleLocation->search_diseqc= _set_disqec_callback;
            //memset(&NodeAdd, 0, sizeof(GxMsgProperty_NodeAdd));


            if(GXCORE_SUCCESS == _visiblelocation_setTkgsLocation())
            {
                s_Changed = LOCATION_CHANGED;
                _visible_location_show_tip();
                ret = GXCORE_SUCCESS;
                visible_location_exist_status = 0;
            }
        }

    }

    return ret ;
}



void _visiblelocation_popup_ok_keypress(void)
{
    status_t ret = GXCORE_ERROR;
  //  app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    switch(s_CurMode)
    {
        case LOCATION_ADD:
            ret = _visiblelocation_add();
            break;

        case LOCATION_EDIT:
            if(s_TKGS_Location.visible_num > 0)
            {
                ret= _visiblelocation_edit();
            }
            break;
    }

    if(ret == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_VISIBLELOCATION_POPUP);
        GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);
        if(s_TKGS_Location.visible_num== 0)
        {
            _visible_location_show_tip();
            GUI_SetFocusWidget("list_visible_location");
        }
        else
        {
            _visible_location_show_tip();
            GUI_SetFocusWidget("list_visible_location");
        }
    }
    else
    {
        //GUI_EndDialog(WND_VISIBLELOCATION_POPUP); For Mantis Bug:1733 2013-02-27
    }

    return;
}

SIGNAL_HANDLER int app_visiblelocation_popup_create(GuiWidget *widget, void *usrdata)
{
    #define POLAR_CONTENT   "[H,V]"
    #define TP_LEN  20

    char buffer[TP_LEN];
    //char* PolarStr[2] = {"H", "V"};
    uint32_t sel=0;
    char add_fre_buffer[MAX_FRE_LEN+1]="03000";
    char add_sym_buffer[MAX_FRE_LEN+1]="01000";
    char add_pid_buffer[MAX_PID_LEN+1]="8181";
    int i;
    //GxMsgProperty_NodeNumGet tp_num = {0};
    match_exist_sel =0;
    match_status = 0;
    //app_log_flow("func=%s,line=%d\n",__FUNCTION__,__LINE__);

    GUI_SetProperty("edit_visiblelocation_popup_freq", "state", "hide");
    GUI_SetProperty("edit_visiblelocation_popup_sym", "state", "hide");
    switch(s_CurMode)
    {
        case LOCATION_ADD:
        {
            //title
            GUI_SetProperty("text_visiblelocation_popup_title", "string", STR_ID_ADD);
            {

                memset(buffer_pid_bak,0,MAX_PID_LEN+1);

                memset(buffer_fre_bak,0,MAX_FRE_LEN+1);

                memset(buffer_fre_bak,0,MAX_FRE_LEN+1);
                for(i=0;i<MAX_FRE_LEN;i++)
                {
                    buffer_fre_bak[i] =add_fre_buffer[i];
                    buffer_sym_bak[i] = add_sym_buffer[i];
                }
                buffer_pid_bak[0] = add_pid_buffer[0];
                buffer_pid_bak[1] = add_pid_buffer[1];
                buffer_pid_bak[2] = add_pid_buffer[2];
                buffer_pid_bak[3] = add_pid_buffer[3];
                //default value:3000/H/1000
                //freq
                GUI_SetProperty("text_visiblelocation_popup_freq2", "string", "03000");
                GUI_SetProperty("edit_visiblelocation_popup_freq", "string", "03000");
                //polar
                GUI_SetProperty("cmb_visiblelocation_popup_polar","content",POLAR_CONTENT);
                //sym
                GUI_SetProperty("text_visiblelocation_popup_sym2", "string", "01000");
                            GUI_SetProperty("edit_visiblelocation_popup_sym", "string", "01000");
                //pid
                GUI_SetProperty("text_visiblelocation_popup_pid2", "string", "8181");
                            GUI_SetProperty("edit_visiblelocation_popup_pid", "string", "8181");

                //hide
                GUI_SetProperty("img_visiblelocation_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_visiblelocation_popup_ok", "state", "hide");

                GUI_SetFocusWidget("box_visiblelocation_popup_para");
            }
        }
        break;

        case LOCATION_EDIT:
        {
            //title
            GUI_SetProperty("text_visiblelocation_popup_title", "string", STR_ID_EDIT);

            if(s_TKGS_Location.visible_num == 0)
            {
                GUI_SetProperty("box_visiblelocation_popup_para", "state", "hide");
                GUI_SetProperty("text_visiblelocation_popup_tip1", "string", STR_ID_TP_NOT_EXIST);
            }
            else
            {
                //freq
                memset(buffer, 0 ,TP_LEN);
                sprintf(buffer, "%05d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].frequency);
                GUI_SetProperty("text_visiblelocation_popup_freq2", "string", buffer);
                GUI_SetProperty("edit_visiblelocation_popup_freq", "string", buffer);

                memset(buffer_fre_bak,0,MAX_FRE_LEN+1);
                sprintf(buffer_fre_bak, "%05d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].frequency);

                //polar
                GUI_SetProperty("cmb_visiblelocation_popup_polar","content",POLAR_CONTENT);
                sel = s_TKGS_Location.visible_location[s_CurVisiableLocationSel].polar;
                GUI_SetProperty("cmb_visiblelocation_popup_polar", "select", &sel);

                //sym
                memset(buffer, 0 ,TP_LEN);
                sprintf(buffer, "%05d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].symbol_rate);
                GUI_SetProperty("text_visiblelocation_popup_sym2", "string", buffer);
                GUI_SetProperty("edit_visiblelocation_popup_sym", "string", buffer);

                memset(buffer_sym_bak,0,MAX_FRE_LEN+1);
                sprintf(buffer_sym_bak, "%05d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].symbol_rate);

                //pid
                memset(buffer, 0 ,TP_LEN);
                sprintf(buffer, "%04d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].pid);
                GUI_SetProperty("text_visiblelocation_popup_pid2", "string", buffer);
                GUI_SetProperty("edit_visiblelocation_popup_pid", "string", buffer);

                memset(buffer_pid_bak,0,MAX_PID_LEN+1);
                sprintf(buffer_pid_bak, "%04d", s_TKGS_Location.visible_location[s_CurVisiableLocationSel].pid);
                //app_log_debug("buffer_pid_bak=%s,pid= %d",buffer_pid_bak,s_TKGS_Location.visible_location[s_CurVisiableLocationSel].pid);

            //hide
                GUI_SetProperty("img_visiblelocation_popup_ok_back", "state", "hide");
                GUI_SetProperty("btn_visiblelocation_popup_ok", "state", "hide");
                GUI_SetFocusWidget("box_visiblelocation_popup_para");
            }

        }
        break;

    }


    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_visiblelocation_popup_destroy(GuiWidget *widget, void *usrdata)
{

    _visible_location_show_tip();
    GUI_SetFocusWidget("list_visible_location");
    return EVENT_TRANSFER_STOP;
}
static void check_input_valid(void)
{
    int sel =0;
    char *buffer_fre = NULL;
        char *buffer_sym = NULL;
     char *buffer_pid = NULL;
        GxTkgsLocationParm VisibleLocationPara={0};

    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    GUI_GetProperty("text_visiblelocation_popup_freq2", "string", &buffer_fre);
    GUI_GetProperty("text_visiblelocation_popup_sym2", "string", &buffer_sym);
    GUI_GetProperty("text_visiblelocation_popup_pid2", "string", &buffer_pid);

    //check input valid?
    VisibleLocationPara.frequency = atoi(buffer_fre);
    //app_log_debug("frequency= %d\n",VisibleLocationPara.frequency);
    //sym
    VisibleLocationPara.symbol_rate = atoi(buffer_sym);

   // app_log_debug("symbol_rate= %d\n",VisibleLocationPara.symbol_rate);
    //polar
    GUI_GetProperty("cmb_visiblelocation_popup_polar", "select", &sel);
    if(0 == sel)
        VisibleLocationPara.polar = GXBUS_PM_TP_POLAR_H;
    else
        VisibleLocationPara.polar = GXBUS_PM_TP_POLAR_V;

    VisibleLocationPara.pid = atoi(buffer_pid);
    //app_log_debug("pid=%d\n",VisibleLocationPara.pid);

    if(false == _check_visible_location_para_valid(&s_TKGSSat.sat_data, &VisibleLocationPara))
    {
           //show: invalid TP value
        GUI_SetProperty("edit_visiblelocation_popup_freq", "clear", NULL);
              GUI_SetProperty("edit_visiblelocation_popup_sym", "clear", NULL);
        GUI_SetProperty("edit_visiblelocation_popup_pid", "clear", NULL);

              memset(s_visiblelocation_edit_input,'\0',6);
        buffer_fre = buffer_fre_bak;
        buffer_sym = buffer_sym_bak;
        buffer_pid = buffer_pid_bak;
    }
    else
    {
        int i;
        memset(buffer_fre_bak,'0',MAX_FRE_LEN);
        memset(buffer_sym_bak,'0',MAX_FRE_LEN);
        memset(buffer_pid_bak,'0',4);
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

        //app_log_debug("buffer_pid_len =%d,buffer_pid=%s\n",strlen(buffer_pid),buffer_pid);


        if(strlen(buffer_pid) >MAX_PID_LEN)
        {
            i= 0;
        }
        else
        {
            i=MAX_PID_LEN-strlen(buffer_pid);
        }

        for(;i<MAX_PID_LEN;i++)
        {
            buffer_pid_bak[i]=*buffer_pid;
            buffer_pid++;
        }

        GUI_SetProperty("edit_visiblelocation_popup_freq", "string",buffer_fre_bak);
            GUI_SetProperty("edit_visiblelocation_popup_sym", "string",buffer_sym_bak);
            GUI_SetProperty("edit_visiblelocation_popup_pid", "string",buffer_pid_bak);

    }
    GUI_SetProperty("text_visiblelocation_popup_freq2", "string",buffer_fre_bak);
        GUI_SetProperty("text_visiblelocation_popup_sym2", "string",buffer_sym_bak);
        GUI_SetProperty("text_visiblelocation_popup_pid2", "string",buffer_pid_bak);

}
static void edit_change_show(void)
{
    int box_sel = 0;
   // app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);
    if((VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)&&(edit_command_mode == 0))
    {
        GUI_SetProperty("edit_visiblelocation_popup_freq", "state", "show");
        GUI_SetProperty("text_visiblelocation_popup_freq2", "state", "hide");

    }
    else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
    {
        GUI_SetProperty("edit_visiblelocation_popup_freq", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_freq2", "state", "show");

    }
    else if((VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)&&(edit_command_mode == 0))
    {
        GUI_SetProperty("edit_visiblelocation_popup_sym", "state", "show");
        GUI_SetProperty("text_visiblelocation_popup_sym2", "state", "hide");

    }
    else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)
    {
        GUI_SetProperty("edit_visiblelocation_popup_sym", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_sym2", "state", "show");

    }
    else if((VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)&&(edit_command_mode == 0))
    {
        GUI_SetProperty("edit_visiblelocation_popup_pid", "state", "show");
        GUI_SetProperty("text_visiblelocation_popup_pid2", "state", "hide");

    }
    else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)
    {
        GUI_SetProperty("edit_visiblelocation_popup_pid", "state", "hide");
        GUI_SetProperty("text_visiblelocation_popup_pid2", "state", "show");
    }
    else
    {
        GUI_SetProperty("edit_visiblelocation_popup_freq", "state", "hide");
                GUI_SetProperty("text_visiblelocation_popup_freq2", "state", "show");
        GUI_SetProperty("edit_visiblelocation_popup_sym", "state", "hide");
                GUI_SetProperty("text_visiblelocation_popup_sym2", "state", "show");
        GUI_SetProperty("edit_visiblelocation_popup_pid", "state", "hide");
                GUI_SetProperty("text_visiblelocation_popup_pid2", "state", "show");
    }
}

SIGNAL_HANDLER int app_edit_visiblelocation_popup_freq_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_VISIBLELOCATION_POPUP_FREQ_LEN 5
    GUI_GetProperty("edit_visiblelocation_popup_freq", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_VISIBLELOCATION_POPUP_FREQ_LEN - 1 : 0);
    GUI_SetProperty("edit_visiblelocation_popup_freq", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_visiblelocation_popup_sym_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_VISIBLELOCATION_POPUP_SYM_LEN 6
    GUI_GetProperty("edit_visiblelocation_popup_sym", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_VISIBLELOCATION_POPUP_SYM_LEN - 1 : 0);

    GUI_SetProperty("edit_visiblelocation_popup_sym", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_visiblelocation_popup_pid_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_VISIBLELOCATION_POPUP_PID_LEN 4
    GUI_GetProperty("edit_visiblelocation_popup_pid", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_VISIBLELOCATION_POPUP_PID_LEN - 1 : 0);
    GUI_SetProperty("edit_visiblelocation_popup_pid", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_visiblelocation_popup_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

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
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        if(visible_location_exist_status == 1)
                        {
                            GUI_SetProperty("text_visiblelocation_popup_tip1_exist", "state", "hide");
                            GUI_SetProperty("btn_visiblelocation_tip_cancel_exist", "state", "hide");
                            GUI_SetProperty("btn_visiblelocation_tip_ok_exist", "state", "hide");
                            GUI_SetProperty("img_visiblelocation_tip_opt_exist", "state", "hide");
                            //resume
                            GUI_SetProperty("text_visiblelocation_popup_tip1", "string", " ");
                            //show the box
                            GUI_SetProperty("box_visiblelocation_popup_para", "state", "show");
                            GUI_SetFocusWidget("box_visiblelocation_popup_para");
                            GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);

                            visible_location_exist_status = 0;
                        }
                        else
                        {
                            visible_location_exist_status = 0;
                            GUI_EndDialog(WND_VISIBLELOCATION_POPUP);
                        }
                    }
                    break;

                case STBK_OK:
                    // when tp exist,choose ok or cancel
                  //  app_log_debug("visible_location_exist_status = %d\n",visible_location_exist_status);
                    if(visible_location_exist_status == 1)
                    {
                        if(0 == strcasecmp("btn_visiblelocation_tip_ok_exist", GUI_GetFocusWidget()))
                        {
                            visible_location_exist_status = 0;
                            GUI_EndDialog(WND_VISIBLELOCATION_POPUP);
                          //  app_log_info("end popup\n");

                        }
                        else
                        {

                            GUI_SetProperty("text_visiblelocation_popup_tip1_exist", "state", "hide");
                            GUI_SetProperty("btn_visiblelocation_tip_cancel_exist", "state", "hide");
                            GUI_SetProperty("btn_visiblelocation_tip_ok_exist", "state", "hide");
                            GUI_SetProperty("img_visiblelocation_tip_opt_exist", "state", "hide");
                            //resume
                            GUI_SetProperty("text_visiblelocation_popup_tip1", "string", " ");
                            //show the box
                            GUI_SetProperty("box_visiblelocation_popup_para", "state", "show");
                            GUI_SetFocusWidget("box_visiblelocation_popup_para");
                            GUI_SetProperty(WND_VISIBLELOCATION_POPUP, "draw_now", NULL);

                        }
                        visible_location_exist_status = 0;
                    }
                    else
                    {
                        if((s_CurMode== LOCATION_ADD) && (s_CurMode==LOCATION_EDIT))
                        {
                            edit_change_show();
                            check_input_valid();
                        }

                        _visiblelocation_popup_ok_keypress();
                    }
                    break;

                case STBK_UP:
                    {
                        GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                        if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            box_sel = VISIABLE_LOCATION_POPUP_BOX_ITEM_OK;
                            GUI_SetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                        }
                    }
                    break;

                case STBK_DOWN:
                    {
                        GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                        if(VISIABLE_LOCATION_POPUP_BOX_ITEM_OK == box_sel)
                        {
                            box_sel = VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ;
                            GUI_SetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                        }
                    }
                    break;
                case STBK_LEFT:
                    // when tp exist,choose ok or cancel
                    if(visible_location_exist_status == 1)
                    {
                        if(0 == strcasecmp("btn_visiblelocation_tip_ok_exist", GUI_GetFocusWidget()))
                        {
                            GUI_SetProperty("img_visiblelocation_tip_opt_exist","img","s_bar_choice_blue4_l");
                            GUI_SetFocusWidget("btn_visiblelocation_tip_cancel_exist");
                        }
                        else
                        {
                            GUI_SetProperty("img_visiblelocation_tip_opt_exist","img","s_bar_choice_blue4");
                            GUI_SetFocusWidget("btn_visiblelocation_tip_ok_exist");
                        }

                    }
                    break;
                case STBK_RIGHT:
                    // when tp exist,choose ok or cancel
                    if(visible_location_exist_status == 1)
                    {
                        if(0 == strcasecmp("btn_visiblelocation_tip_ok_exist", GUI_GetFocusWidget()))
                        {
                            GUI_SetProperty("img_visiblelocation_tip_opt_exist","img","s_bar_choice_blue4_l");
                            GUI_SetFocusWidget("btn_visiblelocation_tip_cancel_exist");
                        }
                        else
                        {
                            GUI_SetProperty("img_visiblelocation_tip_opt_exist","img","s_bar_choice_blue4");
                            GUI_SetFocusWidget("btn_visiblelocation_tip_ok_exist");
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


SIGNAL_HANDLER int app_visiblelocation_popup_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;
    char num[2] = "";
    char *edit_string = NULL;
    uint32_t  edit_num =0;
    int edit_sel = 0;
    char edit_buffer[MAX_LEN]={0};
    bool keypress_status = 0;
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

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
                        memset(s_visiblelocation_edit_input,'\0',6);

                        //Modify by yanwzh 2012-07-19 15:36
                        GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                        if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            box_sel = VISIABLE_LOCATION_POPUP_BOX_ITEM_OK;
                            GUI_SetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                        }
                        else
                        {
                            box_sel = box_sel-1;
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                        up_down_status = 1;
                        edit_command_mode = 1;
                        //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

                    break;
                    }
                case STBK_DOWN:
                    {
                        memset(s_visiblelocation_edit_input,'\0',6);

                        //Modify by yanwzh 2012-07-19 15:36
                        GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                        if(VISIABLE_LOCATION_POPUP_BOX_ITEM_OK == box_sel)
                        {
                            box_sel = VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ;
                            GUI_SetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                        }
                        else
                        {
                            box_sel = box_sel+1;
                            ret = EVENT_TRANSFER_KEEPON;
                        }
                        up_down_status = 1;
                        edit_command_mode = 1;
                        //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);
                    }
                    break;

                case STBK_OK:
                    edit_command_mode = 1;
                    ret = EVENT_TRANSFER_KEEPON;
                             break;
                case STBK_MENU:
                case STBK_EXIT:
                    ret = EVENT_TRANSFER_KEEPON;
                             break;

                case STBK_LEFT:
                // left = delete
                if(edit_command_mode == 0)
                {
                    GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);

                    if(strlen(s_visiblelocation_edit_input) == 1)
                    {
                        if ( APP_THEME_CLASSIC_DARK )
                            edit_sel = 0;
                        else
                            edit_sel = 1;

                        s_visiblelocation_edit_input[0] = '0';
                        s_visiblelocation_edit_input[1] = '\0';
                        edit_del_to_zero = 1;
                        if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            GUI_SetProperty("edit_visiblelocation_popup_freq", "string", s_visiblelocation_edit_input);
                            GUI_SetProperty("edit_visiblelocation_popup_freq", "select", &edit_sel);

                        }
                        else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)
                        {
                            GUI_SetProperty("edit_visiblelocation_popup_sym", "string", s_visiblelocation_edit_input);
                            GUI_SetProperty("edit_visiblelocation_popup_sym", "select", &edit_sel);

                        }
                        else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)
                        {
                            GUI_SetProperty("edit_visiblelocation_popup_pid", "string", s_visiblelocation_edit_input);
                            GUI_SetProperty("edit_visiblelocation_popup_pid", "select", &edit_sel);

                        }
                    }
                    else
                    {
                        s_visiblelocation_edit_input[strlen(s_visiblelocation_edit_input)-1] = '\0';

                        if ( APP_THEME_CLASSIC_DARK )
                            edit_sel = strlen(s_visiblelocation_edit_input)-1;
                        else
                            edit_sel = strlen(s_visiblelocation_edit_input);

                        if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            GUI_SetProperty("edit_visiblelocation_popup_freq", "string", s_visiblelocation_edit_input);
                            GUI_SetProperty("edit_visiblelocation_popup_freq", "select", &edit_sel);

                        }
                        else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)
                        {
                            GUI_SetProperty("edit_visiblelocation_popup_sym", "string", s_visiblelocation_edit_input);
                            GUI_SetProperty("edit_visiblelocation_popup_sym", "select", &edit_sel);

                        }
                        else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)
                        {
                            GUI_SetProperty("edit_visiblelocation_popup_pid", "string", s_visiblelocation_edit_input);
                            GUI_SetProperty("edit_visiblelocation_popup_pid", "select", &edit_sel);

                        }
                    }

                }
                // left = -
                if(edit_command_mode == 1)
                {
                    memset(edit_buffer,0,MAX_LEN);
                    GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                    if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                    {
                            GUI_GetProperty("text_visiblelocation_popup_freq2", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num > 0)
                                edit_num = edit_num-1;
                            sprintf(edit_buffer,"%05d",edit_num);
                            GUI_SetProperty("text_visiblelocation_popup_freq2", "string", edit_buffer);
                            GUI_SetProperty("edit_visiblelocation_popup_freq", "string", edit_buffer);
                    }
                    else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)
                    {
                            GUI_GetProperty("text_visiblelocation_popup_sym2", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num > 0)
                                edit_num = edit_num-1;
                            sprintf(edit_buffer,"%05d",edit_num);
                            GUI_SetProperty("text_visiblelocation_popup_sym2", "string", edit_buffer);
                            GUI_SetProperty("edit_visiblelocation_popup_sym", "string", edit_buffer);
                    }
                    else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)
                    {
                            GUI_GetProperty("text_visiblelocation_popup_pid2", "string", &edit_string);
                            edit_num = atoi(edit_string);
                            if(edit_num > 0)
                                edit_num = edit_num-1;
                            sprintf(edit_buffer,"%04d",edit_num);
                            GUI_SetProperty("text_visiblelocation_popup_pid2", "string", edit_buffer);
                            GUI_SetProperty("edit_visiblelocation_popup_pid", "string", edit_buffer);
                    }
                }


                ret = EVENT_TRANSFER_KEEPON;
                              break;

                case STBK_RIGHT:
                    //right = +
                      if(edit_command_mode == 1)
                      {
                          memset(edit_buffer,0,MAX_LEN);
                          GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                          if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                          {
                              GUI_GetProperty("text_visiblelocation_popup_freq2", "string", &edit_string);
                              edit_num = atoi(edit_string);
                              edit_num = edit_num+1;
                              sprintf(edit_buffer,"%05d",edit_num);
                              GUI_SetProperty("text_visiblelocation_popup_freq2", "string", edit_buffer);
                              GUI_SetProperty("edit_visiblelocation_popup_freq", "string", edit_buffer);
                          }
                          else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)
                          {
                              GUI_GetProperty("text_visiblelocation_popup_sym2", "string", &edit_string);
                              edit_num = atoi(edit_string);
                              edit_num = edit_num+1;
                              sprintf(edit_buffer,"%05d",edit_num);
                              GUI_SetProperty("text_visiblelocation_popup_sym2", "string", edit_buffer);
                              GUI_SetProperty("edit_visiblelocation_popup_sym", "string", edit_buffer);
                          }
                          else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)
                          {
                              GUI_GetProperty("text_visiblelocation_popup_pid2", "string", &edit_string);
                              edit_num = atoi(edit_string);
                              edit_num = edit_num+1;
                              sprintf(edit_buffer,"%04d",edit_num);
                              GUI_SetProperty("text_visiblelocation_popup_pid2", "string", edit_buffer);
                              GUI_SetProperty("edit_visiblelocation_popup_pid", "string", edit_buffer);
                          }
                      }
                      else
                      {
                          GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);
                          if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                          {
                            if( APP_THEME_CLASSIC_DARK )
                                edit_sel = strlen(s_visiblelocation_edit_input)-1;
                            else
                                edit_sel = strlen(s_visiblelocation_edit_input);

                            GUI_SetProperty("edit_visiblelocation_popup_freq", "select", &edit_sel);
                          }
                          else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)
                          {
                            if APP_THEME_CLASSIC_DARK
                                edit_sel = strlen(s_visiblelocation_edit_input)-1;
                            else
                                edit_sel = strlen(s_visiblelocation_edit_input);

                            GUI_SetProperty("edit_visiblelocation_popup_sym", "select", &edit_sel);
                          }
                          else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)
                          {
                            if( APP_THEME_CLASSIC_DARK )
                                edit_sel = strlen(s_visiblelocation_edit_input)-1;
                            else
                                edit_sel = strlen(s_visiblelocation_edit_input);

                            GUI_SetProperty("edit_visiblelocation_popup_pid", "select", &edit_sel);
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

        GUI_GetProperty("box_visiblelocation_popup_para", "select", &box_sel);

        //app_log_debug("func=%s,line= %d,inputLen =%d\n",__FUNCTION__,__LINE__,strlen(s_visiblelocation_edit_input));

        if(strlen(s_visiblelocation_edit_input) < 5)
        {
            if((strlen(s_visiblelocation_edit_input)  == 4)
                &&(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel))
            {
                memset(s_visiblelocation_edit_input,'\0',6);
                GUI_SetProperty("edit_visiblelocation_popup_pid", "clear", NULL);
                strcat(s_visiblelocation_edit_input,num);
                GUI_SetProperty("edit_visiblelocation_popup_pid", "string", s_visiblelocation_edit_input);
                GUI_SetProperty("text_visiblelocation_popup_pid2", "string", s_visiblelocation_edit_input);
            }
            else
            {
                if(edit_del_to_zero == 1)
                {
                    edit_del_to_zero = 0;
                    memset(s_visiblelocation_edit_input,'\0',6);
                }

                if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                {
                    GUI_SetProperty("edit_visiblelocation_popup_freq", "clear", NULL);
                    strcat(s_visiblelocation_edit_input,num);
                    GUI_SetProperty("edit_visiblelocation_popup_freq", "string", s_visiblelocation_edit_input);
                    GUI_SetProperty("text_visiblelocation_popup_freq2", "string", s_visiblelocation_edit_input);

                    if( APP_THEME_CLASSIC_DARK )
                        edit_sel = strlen(s_visiblelocation_edit_input)-1;
                    else
                        edit_sel = strlen(s_visiblelocation_edit_input);

                    GUI_SetProperty("edit_visiblelocation_popup_freq", "select", &edit_sel);
                }
                else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)
                {
                    GUI_SetProperty("edit_visiblelocation_popup_sym", "clear", NULL);
                    strcat(s_visiblelocation_edit_input,num);
                    GUI_SetProperty("edit_visiblelocation_popup_sym", "string", s_visiblelocation_edit_input);
                    GUI_SetProperty("text_visiblelocation_popup_sym2", "string", s_visiblelocation_edit_input);

                    if( APP_THEME_CLASSIC_DARK )
                        edit_sel = strlen(s_visiblelocation_edit_input)-1;
                    else
                        edit_sel = strlen(s_visiblelocation_edit_input);

                    GUI_SetProperty("edit_visiblelocation_popup_sym", "select", &edit_sel);

                }
                else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)
                {
                    GUI_SetProperty("edit_visiblelocation_popup_pid", "clear", NULL);
                    strcat(s_visiblelocation_edit_input,num);
                    GUI_SetProperty("edit_visiblelocation_popup_pid", "string", s_visiblelocation_edit_input);
                    GUI_SetProperty("text_visiblelocation_popup_pid2", "string", s_visiblelocation_edit_input);

                    if( APP_THEME_CLASSIC_DARK )
                        edit_sel = strlen(s_visiblelocation_edit_input)-1;
                    else
                        edit_sel = strlen(s_visiblelocation_edit_input);

                    GUI_SetProperty("edit_visiblelocation_popup_pid", "select", &edit_sel);
                }
            }
        }
        else if(strlen(s_visiblelocation_edit_input) == 5)
        {
           // app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

            memset(s_visiblelocation_edit_input,'\0',6);
            if(VISIABLE_LOCATION_POPUP_BOX_ITEM_FREQ == box_sel)
                        {
                            GUI_SetProperty("edit_visiblelocation_popup_freq", "clear", NULL);
                            strcat(s_visiblelocation_edit_input,num);
                            GUI_SetProperty("edit_visiblelocation_popup_freq", "string", s_visiblelocation_edit_input);
                GUI_SetProperty("text_visiblelocation_popup_freq2", "string", s_visiblelocation_edit_input);

                        }
                        else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_SYM == box_sel)
                        {
                                GUI_SetProperty("edit_visiblelocation_popup_sym", "clear", NULL);
                                strcat(s_visiblelocation_edit_input,num);
                                GUI_SetProperty("edit_visiblelocation_popup_sym", "string", s_visiblelocation_edit_input);
                    GUI_SetProperty("text_visiblelocation_popup_sym2", "string", s_visiblelocation_edit_input);
                        }
                        else if(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel)
                        {
                                GUI_SetProperty("edit_visiblelocation_popup_pid", "clear", NULL);
                                strcat(s_visiblelocation_edit_input,num);
                                GUI_SetProperty("edit_visiblelocation_popup_pid", "string", s_visiblelocation_edit_input);
                    GUI_SetProperty("text_visiblelocation_popup_pid2", "string", s_visiblelocation_edit_input);

                        }

        }

        if(strlen(s_visiblelocation_edit_input) == 5)
        {
            edit_command_mode = 1;
        }
        else if((strlen(s_visiblelocation_edit_input) == 4)
            &&(VISIABLE_LOCATION_POPUP_BOX_ITEM_PID == box_sel))
        {
            edit_command_mode = 1;
        }
       // app_log_debug("func=%s,line= %d,inputLen = %d\n",__FUNCTION__,__LINE__,strlen(s_visiblelocation_edit_input));
        edit_change_show();

    }
    else if(up_down_status == 1)
    {
        //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

        if(strlen(s_visiblelocation_edit_input) == 5)
        {
            edit_command_mode = 1;
        }

        //app_log_debug("edit Input_len = %d\n",strlen(s_visiblelocation_edit_input));
        edit_change_show();
        check_input_valid();
        up_down_status = 0;
    }

    return ret;

}


// create dialog
int app_visible_location_menu_exec(void)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_VISIBLE_LOCATION))
    {
        GUI_EndDialog(WND_VISIBLE_LOCATION);
    }
    GUI_CreateDialog(WND_VISIBLE_LOCATION);

    return 0;
}
#endif
