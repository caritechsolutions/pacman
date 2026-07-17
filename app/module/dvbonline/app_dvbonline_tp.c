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
#define TEXT_TPONLINE_TIP_STATUS "text_tpsource_tip_status"
#define LISTVIEW_TPSOURCE_LIST "listview_tpsource_list"

typedef void(* tponline_thread_func)(void);

static gx_dvbonline_transponderlist_t *tponline_tplist = NULL;
static char *tponline_satname = NULL;
static int tponline_data_valid = 0;
static event_list* tponline_data_timer = NULL;
static int tponline_data_finish = 0;
static unsigned int tponline_data_handle = 0;
static gxapps_ret_t tponline_ret_status = GXAPPS_SUCCESS;

static char* tp_source_str[]=
{
	"Local",
	"Cloud",
};
#define TP_SOURCE_NUM 	(sizeof(tp_source_str)/sizeof(char *))

extern void app_dvbcloud_tplist_add(gx_dvbonline_transponderlist_t *tplist);
extern void app_tp_list_display_restore(void);

static void tponline_thread_body(void* arg)
{
	tponline_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (tponline_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_tponline_thread(tponline_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("tponline", &thread_id, tponline_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

static void app_tponline_tip_show(char *buf)
{
	GUI_SetProperty(TEXT_TPONLINE_TIP_STATUS, "string", buf);
	GUI_SetProperty(TEXT_TPONLINE_TIP_STATUS, "state", "show");
}

static void app_tponline_free_tplist(void)
{
	if(tponline_tplist)
	{
		gx_dvbonline_free_transponderlist(tponline_tplist);
		tponline_tplist = NULL;
	}
}

static void app_tponline_show_errormsg(void)
{
	if(tponline_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_tponline_tip_show("Server error!");
	}
	else if(tponline_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_tponline_tip_show("Network error!");
	}
	else if(tponline_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_tponline_tip_show("Internal error!");
	}
	else if(tponline_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_tponline_tip_show("Unknown error!");
	}
}

static void app_tponline_refresh_stop(void)
{
	tponline_data_finish = 0;
}
static void app_tponline_refresh_start(void)
{
	tponline_data_finish = 1;
}

static int tponline_refresh_timeout(void)
{
	if(tponline_data_valid)
	{
		app_dvbcloud_tplist_add(tponline_tplist);
		GUI_EndDialog(WND_TP_SOURCE_SELECT);
		app_tp_list_display_restore();
	}
	else
	{
		app_tponline_show_errormsg();
	}
	app_tponline_refresh_stop();
	return 0;
}

static void app_tponline_data_timer_stop(void)
{
	if(tponline_data_timer)
	{
		remove_timer(tponline_data_timer);
		tponline_data_timer = NULL;
	}
}

static int tponline_data_timer_timeout(void *usrdata)
{
	if(tponline_data_finish)
	{
		tponline_refresh_timeout();
	}
	return 0;
}

static void app_tponline_data_timer_start(void)
{
	app_tponline_data_timer_stop();
	tponline_data_timer = create_timer(tponline_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void tponline_start_get_tps(void)
{
	unsigned int handle = 0;
	gx_dvbonline_transponderlist_t *data = NULL;
	gxapps_ret_t ret = GXAPPS_SUCCESS;

	tponline_data_handle++;
	handle = tponline_data_handle;
	ret = gx_dvbonline_get_transponders(tponline_satname, &data);
	if(handle == tponline_data_handle)
	{
		tponline_ret_status = ret;
		if(tponline_ret_status == GXAPPS_SUCCESS)
		{
			tponline_data_valid = 1;
			tponline_tplist = data;
		}
		app_tponline_refresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_dvbonline_free_transponderlist(data);
		}
	}
}

static void app_tponline_tp_update(void)
{
	tponline_data_valid = 0;
	app_tponline_refresh_stop();
	app_tponline_free_tplist();
	app_tponline_tip_show("Updating...");
	create_tponline_thread(tponline_start_get_tps);
}

static void app_tpsource_list_select(int sel)
{
	if(sel == 0)
	{
		GUI_EndDialog(WND_TP_SOURCE_SELECT);
		GUI_CreateDialog(WND_TP_LIST_POPUP);
	}
	else
	{
		GUI_SetProperty(LISTVIEW_TPSOURCE_LIST, "state", "hide");
		app_tponline_tp_update();
	}
}

void app_create_tpsource_menu(char *sat_name)
{
	tponline_satname = GxCore_Strdup(sat_name);
	GUI_CreateDialog(WND_TP_SOURCE_SELECT);
}

SIGNAL_HANDLER int app_tpsource_create(GuiWidget *widget, void *usrdata)
{
	app_tponline_data_timer_start();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tpsource_destroy(GuiWidget *widget, void *usrdata)
{
	tponline_data_handle++;
	app_tponline_data_timer_stop();
	app_tponline_refresh_stop();
	app_tponline_free_tplist();
	APP_FREE(tponline_satname);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tpsource_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;

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
				GUI_EndDialog(WND_TP_SOURCE_SELECT);
				app_tp_list_display_restore();
				break;

			default:
				ret = EVENT_TRANSFER_STOP;
				break;
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_tpsource_list_create(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tpsource_list_destroy(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tpsource_list_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	int sel = 0;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_MENU:
			case STBK_EXIT:
				GUI_EndDialog(WND_TP_SOURCE_SELECT);
				app_tp_list_display_restore();
				break;

			case STBK_OK:
				GUI_GetProperty(LISTVIEW_TPSOURCE_LIST, "select", &sel);
				app_tpsource_list_select(sel);
				break;

			default:
				ret = EVENT_TRANSFER_KEEPON;
				break;
		}
	}
	return ret;
}

SIGNAL_HANDLER int app_tpsource_list_get_total(GuiWidget *widget, void *usrdata)
{
	return TP_SOURCE_NUM;
}

SIGNAL_HANDLER int app_tpsource_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return EVENT_TRANSFER_STOP;
	if(0 > item->sel)
		return EVENT_TRANSFER_STOP;

	item->image = NULL;
	item->string = tp_source_str[item->sel];

	return EVENT_TRANSFER_STOP;
}


#endif

