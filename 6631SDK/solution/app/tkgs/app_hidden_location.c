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
#include "app_windows.h"

#if TKGS_SUPPORT

static int s_CurHiddenLocationSel = 0;    //location listview sel
/* widget macro -------------------------------------------------------- */
#define TKGS_DEBUG 0
#if TKGS_DEBUG
static bool init_state = 0;
#endif
static GxTkgsLocation s_TKGS_Location;

status_t _clear_hiddenLocation(void)
{
   // app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    #if TKGS_DEBUG == 0
    return app_send_msg_exec(GXMSG_TKGS_TEST_CLEAR_HIDDEN_LOCATIONS_LIST,NULL);
    #else
    return GXCORE_SUCCESS;
    #endif
}

void _location_getTkgsHiddenLocation(void)
{
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    #if TKGS_DEBUG == 0
    memset(&s_TKGS_Location, 0, sizeof(s_TKGS_Location));
    app_send_msg_exec(GXMSG_TKGS_GET_LOCATION,&s_TKGS_Location);
    #else
    if(init_state == FALSE)
    {

    init_state = TRUE;
    memset(&s_TKGS_Location, 0, sizeof(s_TKGS_Location));
    s_TKGS_Location.visible_num= 0;
    s_TKGS_Location.invisible_num = 4;

    s_TKGS_Location.invisible_location[0].sat_id = 28;
    s_TKGS_Location.invisible_location[0].search_demux_id = 0;
    s_TKGS_Location.invisible_location[0].frequency = 12380;
    s_TKGS_Location.invisible_location[0].symbol_rate= 27500;
    s_TKGS_Location.invisible_location[0].polar= GXBUS_PM_TP_POLAR_V;
    s_TKGS_Location.invisible_location[0].pid= 8181;

    s_TKGS_Location.invisible_location[1].sat_id = 28;
    s_TKGS_Location.invisible_location[1].search_demux_id = 0;
    s_TKGS_Location.invisible_location[1].frequency = 12423 ;
    s_TKGS_Location.invisible_location[1].symbol_rate= 27500;
    s_TKGS_Location.invisible_location[1].polar= GXBUS_PM_TP_POLAR_V;
    s_TKGS_Location.invisible_location[1].pid= 8181;

    s_TKGS_Location.invisible_location[2].sat_id = 28;
    s_TKGS_Location.invisible_location[2].search_demux_id = 0;
    s_TKGS_Location.invisible_location[2].frequency = 12423 ;
    s_TKGS_Location.invisible_location[2].symbol_rate= 30000 ;
    s_TKGS_Location.invisible_location[2].polar= GXBUS_PM_TP_POLAR_H;
    s_TKGS_Location.invisible_location[2].pid= 8181;

    s_TKGS_Location.invisible_location[3].sat_id = 28;
    s_TKGS_Location.invisible_location[3].search_demux_id = 0;
    s_TKGS_Location.invisible_location[3].frequency = 12458 ;
    s_TKGS_Location.invisible_location[3].symbol_rate= 30000 ;
    s_TKGS_Location.invisible_location[3].polar= GXBUS_PM_TP_POLAR_V;
    s_TKGS_Location.invisible_location[3].pid= 8181;

    }
    #endif
}

SIGNAL_HANDLER int app_hidden_location_create(GuiWidget *widget, void *usrdata)
{
   // app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);
    // add in 20120702
    g_AppFullArb.timer_stop();

    //get total sat num
    s_CurHiddenLocationSel = -1;

    _location_getTkgsHiddenLocation();

    if(s_TKGS_Location.invisible_num > 0)
    {
        s_CurHiddenLocationSel = 0;
    }

    GUI_SetProperty("list_hidden_location", "update_all", NULL);
    GUI_SetProperty("list_hidden_location", "select", &s_CurHiddenLocationSel);
    // id manage
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_hidden_location_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_hidden_location_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;

    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;

            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_HIDDEN_LOCATION);
                ret = EVENT_TRANSFER_STOP;
                #if TKGS_DEBUG
                init_state = FALSE;
                #endif
                break;

            case STBK_LEFT:
            case STBK_RIGHT:
                ret = EVENT_TRANSFER_STOP;
                break;

            case STBK_RED:      //del
				if(s_TKGS_Location.invisible_num >0)
				{
                    _clear_hiddenLocation();
					//_location_getTkgsHiddenLocation();
                    GUI_SetProperty("list_hidden_location", "update_all", NULL);
					s_CurHiddenLocationSel = -1;
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



SIGNAL_HANDLER int app_hidden_location_list_create(GuiWidget *widget, void *usrdata)
{
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_hidden_location_list_change(GuiWidget *widget, void *usrdata)
{
    uint32_t list_sel;
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    GUI_GetProperty("list_hidden_location", "select", &list_sel);
    s_CurHiddenLocationSel = list_sel;
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_hidden_location_list_get_total(GuiWidget *widget, void *usrdata)
{
    //app_log_flow("func=%s,line= %d\n",__FUNCTION__,__LINE__);

    //memset(&s_TKGS_Location, 0, sizeof(s_TKGS_Location));
    //app_send_msg_exec(GXMSG_TKGS_GET_LOCATION,&s_TKGS_Location);
    _location_getTkgsHiddenLocation();
    return s_TKGS_Location.invisible_num;
}

SIGNAL_HANDLER int app_hidden_location_list_get_data(GuiWidget *widget, void *usrdata)
{
    #define MAX_LEN 30
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
    pCurLocation = &(s_TKGS_Location.invisible_location[item->sel]);
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

SIGNAL_HANDLER int app_hidden_location_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    //uint32_t sel=0;
    int cur_sel = 0;
    //app_log_flow("func=%s,line=%d\n",__FUNCTION__,__LINE__);
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
                    GUI_GetProperty("list_hidden_location", "select", &cur_sel);
                    if(cur_sel == 0)
                    {
                        cur_sel = s_TKGS_Location.invisible_num - 1;
                        GUI_SetProperty("list_hidden_location", "select", &cur_sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;
                case STBK_DOWN:
                    GUI_GetProperty("list_hidden_location", "select", &cur_sel);
                    if(cur_sel == (s_TKGS_Location.invisible_num - 1))
                    {
                        cur_sel = 0;
                        GUI_SetProperty("list_hidden_location", "select", &cur_sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;
                case STBK_PAGE_UP:
                {
                    uint32_t sel;
                    GUI_GetProperty("list_hidden_location", "select", &sel);
                    if (sel == 0)
                    {
                        sel = s_TKGS_Location.invisible_num -1;
                        GUI_SetProperty("list_hidden_location", "select", &sel);
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
                    GUI_GetProperty("list_hidden_location", "select", &sel);
                    if (s_TKGS_Location.invisible_num == sel+1)
                    {
                        sel = 0;
                        GUI_SetProperty("list_hidden_location", "select", &sel);
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

// create dialog
int app_hidden_location_menu_exec(void)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_HIDDEN_LOCATION))
    {
        GUI_EndDialog(WND_HIDDEN_LOCATION);
    }
    GUI_CreateDialog(WND_HIDDEN_LOCATION);

    return 0;
}

#endif
