#include "app.h"
#include "app_windows.h"

#if DVB_CLOUD_SUPPORT
#include <gx_dvbonline.h>
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
#if DOUBLE_S2_SUPPORT
#include "app_dvbs_sat_opt.h"
#endif
#define LISTVIEW_SATSOURCE_LIST "listview_satsource_list"
#define IMG_SATONLINE_TIP_BK "img_satelite_online_tip_bk"
#define IMG_SATONLINE_TIP_STATUS "text_satelite_online_tip_status"
#define LISTVIEW_SAT_ONLINE_LIST "list_sat_online_list"

typedef void(* satonline_thread_func)(void);

static gx_dvbonline_region_t satelite_region = DVBONLINE_REGION_ALL;
static gx_dvbonline_satelitelist_t *satonline_satlist = NULL;
static int satonline_satlite_max = 0;
static int satonline_satlite_num = 0;
static int *satonline_satlist_sel = NULL;
static int satonline_data_valid = 0;
static event_list* satonline_data_timer = NULL;
static int satonline_data_finish = 0;
static unsigned int satonline_data_handle = 0;
static gxapps_ret_t satonline_ret_status = GXAPPS_SUCCESS;

extern void app_dvbcloud_satlist_add(gx_dvbonline_satelitelist_t *satlist, int *listsel);

static char* satelite_source_str[]=
{
	"Local",
	"All(Cloud)", //DVBONLINE_REGION_ALL
	"Asia & Pacific(Cloud)", //DVBONLINE_REGION_ASIA
	"Europe, Africa & Middle East(Cloud)", //DVBONLINE_REGION_EUROPE
	"Atlantic(Cloud)", //DVBONLINE_REGION_ATLANTIC
	"North & South America(Cloud)", //DVBONLINE_REGION_AMERICA
};
#define SATELITE_SOURCE_NUM 	(sizeof(satelite_source_str)/sizeof(char *))

static void satonline_thread_body(void* arg)
{
	satonline_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (satonline_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_satonline_thread(satonline_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("satonline", &thread_id, satonline_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static void app_satonline_tip_hide(void)
{
	GUI_SetProperty(IMG_SATONLINE_TIP_BK, "state", "hide");
	GUI_SetProperty(IMG_SATONLINE_TIP_STATUS, "state", "hide");
}

static void app_satonline_tip_show(char *buf)
{
	GUI_SetProperty(IMG_SATONLINE_TIP_BK, "state", "show");
	GUI_SetProperty(IMG_SATONLINE_TIP_STATUS, "string", buf);
	GUI_SetProperty(IMG_SATONLINE_TIP_STATUS, "state", "show");
}

static void app_satonline_satlist_show(void)
{
	int sel = 0;

	GUI_SetProperty(LISTVIEW_SAT_ONLINE_LIST, "state", "show");
	GUI_SetFocusWidget(LISTVIEW_SAT_ONLINE_LIST);
	GUI_SetProperty(LISTVIEW_SAT_ONLINE_LIST, "select", &sel);
	GUI_SetProperty(LISTVIEW_SAT_ONLINE_LIST, "update_all", NULL);

}

static void app_satonline_free_satlist(void)
{
	if(satonline_satlist)
	{
		gx_dvbonline_free_satelitelist(satonline_satlist);
		satonline_satlist = NULL;
	}
}

static void app_satonline_show_errormsg(void)
{
	if(satonline_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_satonline_tip_show("Server error!");
	}
	else if(satonline_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_satonline_tip_show("Network error!");
	}
	else if(satonline_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_satonline_tip_show("Internal error!");
	}
	else if(satonline_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_satonline_tip_show("Unknown error!");
	}
}

static void app_satonline_refresh_stop(void)
{
	satonline_data_finish = 0;
}
static void app_satonline_refresh_start(void)
{
	satonline_data_finish = 1;
}

static int satonline_refresh_timeout(void)
{
	if(satonline_data_valid)
	{
		app_satonline_tip_hide();
		app_satonline_satlist_show();
	}
	else
	{
		app_satonline_show_errormsg();
	}
	app_satonline_refresh_stop();
	return 0;
}

static void app_satonline_data_timer_stop(void)
{
	if(satonline_data_timer)
	{
		remove_timer(satonline_data_timer);
		satonline_data_timer = NULL;
	}
}

static int satonline_data_timer_timeout(void *usrdata)
{
	if(satonline_data_finish)
	{
		satonline_refresh_timeout();
	}
	return 0;
}

static void app_satonline_data_timer_start(void)
{
	app_satonline_data_timer_stop();
	satonline_data_timer = create_timer(satonline_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void satonline_start_get_satelites(void)
{
	unsigned int handle = 0;
	gx_dvbonline_satelitelist_t *data = NULL;
	gxapps_ret_t ret = GXAPPS_SUCCESS;

	satonline_data_handle++;
	handle = satonline_data_handle;
	ret = gx_dvbonline_get_satelites(satelite_region, &data);
	if(handle == satonline_data_handle)
	{
		satonline_ret_status = ret;
		if(satonline_ret_status == GXAPPS_SUCCESS)
		{
			satonline_data_valid = 1;
			satonline_satlist = data;
			if(satonline_satlist->satelite_num>0)
			{
				satonline_satlist_sel = GxCore_Calloc(sizeof(int), satonline_satlist->satelite_num);
				memset(satonline_satlist_sel, 0, sizeof(int)*satonline_satlist->satelite_num);
			}
		}
		app_satonline_refresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_dvbonline_free_satelitelist(data);
		}
	}
}

static void app_satonline_satelites_update(void)
{
	satonline_data_valid = 0;
	satonline_satlite_num = 0;
	app_satonline_refresh_stop();
	app_satonline_free_satlist();
	APP_FREE(satonline_satlist_sel);
	app_satonline_tip_show("Updating...");
	create_satonline_thread(satonline_start_get_satelites);
}

static void app_satsource_list_select(int sel)
{
	if(sel == 0)
	{
		GUI_CreateDialog(WND_SAT_LIST_POPUP);
	}
	else if((sel>0)&&(sel<=DVBONLINE_REGION_MAX))
	{
		satelite_region = sel-1;
		GUI_CreateDialog(WND_SAT_ONLINE_LIST);
	}
}

void app_create_satsource_menu(int sat_max)
{
	satonline_satlite_max = sat_max;
	GUI_CreateDialog(WND_SAT_SOURCE_SELECT);
}

SIGNAL_HANDLER int app_satsource_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satsource_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satsource_list_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satsource_list_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satsource_list_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	int sel = 0;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
            case VK_BOOK_TRIGGER:
                {
#if DOUBLE_S2_SUPPORT
                    GxMsgProperty_NodeByPosGet cur_sat_bak = {0};

                    memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
                    s_dvbs_sat_data.cur_tuner = s_dvbs_sat_data.cur_sat.sat_data.tuner;
                    s_dvbs_sat_data.sat_gui_sel_update();
                    memcpy(&s_dvbs_sat_data.cur_sat, &cur_sat_bak, sizeof(GxMsgProperty_NodeByPosGet));
#endif
                    GUI_EndDialog("after wnd_full_screen");
                    GUI_SetInterface("flush", NULL);
                }
                break;
            case STBK_MENU:
			case STBK_EXIT:
				GUI_EndDialog(WND_SAT_SOURCE_SELECT);
				break;

			case STBK_OK:
				GUI_GetProperty(LISTVIEW_SATSOURCE_LIST, "select", &sel);
				GUI_EndDialog(WND_SAT_SOURCE_SELECT);
				app_satsource_list_select(sel);
				break;

			default:
				ret = EVENT_TRANSFER_KEEPON;
				break;
		}
	}
	return ret;
}


SIGNAL_HANDLER int app_satsource_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	return SATELITE_SOURCE_NUM;
}

SIGNAL_HANDLER int app_satsource_list_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return EVENT_TRANSFER_STOP;
	if(0 > item->sel)
		return EVENT_TRANSFER_STOP;

	item->image = NULL;
	item->string = satelite_source_str[item->sel];

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_online_list_create(GuiWidget *widget, void *usrdata)
{
	app_satonline_data_timer_start();
	app_satonline_satelites_update();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_online_list_destroy(GuiWidget *widget, void *usrdata)
{
	satonline_data_handle++;
	app_satonline_data_timer_stop();
	app_satonline_refresh_stop();
	app_satonline_free_satlist();
	APP_FREE(satonline_satlist_sel);
	satonline_satlite_max = 0;
	satonline_satlite_num = 0;
	return EVENT_TRANSFER_STOP;
}

static int _app_sat_online_popdlg_exit_cb(PopDlgRet ret)
{
    if ( ret == POP_VAL_OK )
    {
        if((satonline_satlist)&&(satonline_satlist->satelite_num>0)&&satonline_satlist_sel
					&&(satonline_satlite_num>0)&&(satonline_satlite_max>0))
        {
            app_dvbcloud_satlist_add(satonline_satlist, satonline_satlist_sel);
        }
    }

    GUI_EndDialog(WND_SAT_ONLINE_LIST);

    return 0 ;
}

SIGNAL_HANDLER int app_sat_online_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                {
#if DOUBLE_S2_SUPPORT
                    GxMsgProperty_NodeByPosGet cur_sat_bak = {0};
                    memcpy(&cur_sat_bak, &s_dvbs_sat_data.cur_sat, sizeof(GxMsgProperty_NodeByPosGet));
                    s_dvbs_sat_data.cur_tuner = s_dvbs_sat_data.cur_sat.sat_data.tuner;
                    s_dvbs_sat_data.sat_gui_sel_update();
                    memcpy(&s_dvbs_sat_data.cur_sat, &cur_sat_bak, sizeof(GxMsgProperty_NodeByPosGet));
#endif
                    GUI_EndDialog("after wnd_full_screen");
                    GUI_SetInterface("flush", NULL);
                }
                break;
            case STBK_EXIT:
            case STBK_MENU:
				if((satonline_satlist)&&(satonline_satlist->satelite_num>0)&&satonline_satlist_sel
					&&(satonline_satlite_num>0)&&(satonline_satlite_max>0))
				{
					PopDlg  pop;
					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_YES_NO;
					pop.mode = POP_MODE_UNBLOCK;
					pop.str = STR_ID_CLOUD_SELECTED_SATELLITES;
					pop.exit_cb = _app_sat_online_popdlg_exit_cb;
					popdlg_create(&pop);
				}
				else
				{
					GUI_EndDialog(WND_SAT_ONLINE_LIST);
				}
                ret = EVENT_TRANSFER_STOP;
                break;

            default:
                break;
        }
    }

    return ret;
}

SIGNAL_HANDLER int app_sat_online_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	int num = 0;

	if((satonline_satlist)&&(satonline_satlist->satelite_num>0))
	{
		num = satonline_satlist->satelite_num;
	}

    return num;
}

SIGNAL_HANDLER int app_sat_online_list_list_get_data(GuiWidget *widget, void *usrdata)
{
#define MAX_LEN 30
	ListItemPara* item = NULL;
	static char buffer1[MAX_LEN];
	static char buffer2[MAX_LEN];
	int position = 0;
	char *direction = NULL;

	if((!satonline_satlist)||(!satonline_satlist_sel)||(satonline_satlist->satelite_num<=0))
		return GXCORE_ERROR;

	item = (ListItemPara*)usrdata;
	if((NULL == item) ||(0 > item->sel))
		return GXCORE_ERROR;


	//col-0: choice
	item->x_offset = 0;
	if(satonline_satlist_sel[item->sel] == 0)
		item->image = NULL;
	else
		item->image = "s_choice";
	item->string = NULL;

	//col-1: num
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer1, 0, sizeof(buffer1));
	sprintf(buffer1, " %d", (item->sel+1));
	item->string = buffer1;

	//col-2: name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = satonline_satlist->satelites[item->sel].mini_name;

	//col-3: longitude
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	memset(buffer2, 0, sizeof(buffer2));
	if(satonline_satlist->satelites[item->sel].position>0)
		direction = "E";
	else
		direction = "W";
	position = abs(satonline_satlist->satelites[item->sel].position);
	sprintf(buffer2, "%d.%d%s", position/10, position%10, direction);
	item->string = buffer2;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_sat_online_list_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t sel = -1;

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
					GUI_GetProperty(LISTVIEW_SAT_ONLINE_LIST, "select", &sel);
					if((satonline_satlist)&&(satonline_satlist->satelite_num>0)&&satonline_satlist_sel
						&&(sel>=0)&&(sel<satonline_satlist->satelite_num)
						&&(satonline_satlite_max>0))
                    {
                        if(satonline_satlist_sel[sel]==0)
                        {
                        	if(satonline_satlite_num<satonline_satlite_max)
                        	{
                        		satonline_satlite_num++;
                            	satonline_satlist_sel[sel] = 1;
                        	}
                        }
                        else
                        {
                        	if(satonline_satlite_num>0)
                        	{
                        		satonline_satlite_num--;
                        	}
                            satonline_satlist_sel[sel] = 0;
                        }
                        GUI_SetProperty(LISTVIEW_SAT_ONLINE_LIST, "update_row", &sel);
                    }

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


#endif

