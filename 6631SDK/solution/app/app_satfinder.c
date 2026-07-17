#include "app_default_params.h"
#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_windows.h"
#include <cJSON.h>
#include <gx_apps.h>
#if SAT_FINDER_SUPPORT
#include "module/satfinder/dvbfinder.h"
#include "app_main_menu_tip.h"

#define TXT_SATFINDER_INFO   "text_sat_finder_info"

#ifdef ECOS_OS
#define APK_PIC_PATH "/mnt/apk.jpg"
#define SERVER_PIC_PATH "/mnt/server.jpg"
#else
#define APK_PIC_PATH "/tmp/apk.jpg"
#define SERVER_PIC_PATH "/tmp/server.jpg"
#endif

#define SAT_FINDER_ITEM_NUM (2)
#define SAT_FINDER_MAX_PIC_ITEM (SAT_FINDER_ITEM_NUM+1)

typedef enum
{
    SAT_EXCESS_MAX,
    TP_EXCESS_MAX,
    CHECK_OK
}SatFinderCheckStatus;

static unsigned int satfind_pic_download_handle[SAT_FINDER_MAX_PIC_ITEM];
static int satfind_pic_download_ok[SAT_FINDER_MAX_PIC_ITEM];
static int satfind_pic_download_abort=0;
extern int db_sat_release(void);

static event_list* satfinder_popup_timer = NULL;
static event_list* satfinder_picupdate_timer = NULL;
static int s_find_updating = 0;
static int satfinder_pic_app_show = 0;
static int satfinder_pic_server_show = 0;

static void app_satfinder_tip_show(char *buf, int time_out);

static SatFinderCheckStatus app_satfind_check(void)
{
    int i;
    GxMsgProperty_NodeNumGet sat_num = {0};
    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
    for(i = 0; i < sat_num.node_num; i++)
    {
        GxMsgProperty_NodeByPosGet sat_node = {0};
        sat_node.node_type = NODE_SAT;
        sat_node.pos = i;
        if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &sat_node))
        {
            if(0 == strcasecmp("FinderSat", (char *)sat_node.sat_data.sat_s.sat_name))
            {
                GxMsgProperty_NodeDelete node_del= {0};
                node_del.node_type = NODE_SAT;
                node_del.num = 1;
                node_del.id_array = &(sat_node.sat_data.id);
                app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_del);
                break;
            }
        }
    }
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);
    if(sat_num.node_num >= SYS_MAX_SAT)
    {
        return SAT_EXCESS_MAX;
    }
    else
    {
        if(GxBus_PmTpNumGet() >= SYS_MAX_TP)
        {
            return TP_EXCESS_MAX;
        }
        else
        {
            return CHECK_OK;
        }
    }
}

static void app_satfinder_tip_hide(void)
{
	s_find_updating = 0;
	GUI_SetProperty("img_sat_finder_tip_bk", "state", "hide");
	GUI_SetProperty("text_sat_finder_tip_status", "state", "hide");

	APP_TIMER_REMOVE(satfinder_popup_timer);

	GUI_SetProperty(WND_SATFINDER, "update", NULL);
}

static int app_satfinder_update_fail(void)
{
    s_find_updating=0;
    GUI_SetProperty("img_app", "load_scal_img", NULL);
    GUI_SetProperty("img_qr", "load_scal_img", NULL);
    GUI_SetProperty("img_app", "state", "hide");
    GUI_SetProperty("img_qr", "state", "hide");
    GUI_SetProperty("wnd_satfinder", "draw_now", NULL);
    app_satfinder_tip_show("Network error!", 2000);

    return 0;
}

static int satfinder_popup_timer_timeout(void *userdata)
{
    satfinder_popup_timer = NULL;

	if(s_find_updating == 1)
	{
		s_find_updating = 0;
		app_satfinder_tip_show("Network error!", 1000);
	}
	else
	{
		app_satfinder_tip_hide();
	}
	return 0;
}

static void app_satfinder_tip_show(char *buf, int time_out)
{
	APP_TIMER_REMOVE(satfinder_popup_timer);

	GUI_SetProperty("img_sat_finder_tip_bk", "state", "show");
	GUI_SetProperty("text_sat_finder_tip_status", "string", buf);
	GUI_SetProperty("text_sat_finder_tip_status", "state", "show");
	if(time_out > 0)
	{
		APP_TIMER_ADD(satfinder_popup_timer, satfinder_popup_timer_timeout, time_out, TIMER_ONCE);
	}
}

static void satfinder_update_timer_stop(void)
{
	APP_TIMER_REMOVE(satfinder_picupdate_timer);
}

static int satfinder_pic_update_timer_timeout(void *userdata)
{
    if(satfind_pic_download_ok[0] && satfinder_pic_app_show == 0)
    {
        GUI_SetProperty("img_app", "state", "show");
        if(GXCORE_SUCCESS == GUI_SetProperty("img_app", "load_scal_img", APK_PIC_PATH))
        {
            app_satfinder_tip_hide();
            satfinder_pic_app_show = 1;
        }
    }

    if(satfind_pic_download_ok[1] && satfinder_pic_server_show == 0)
    {
        GUI_SetProperty("img_qr", "state", "show");
        if(GXCORE_SUCCESS == GUI_SetProperty("img_qr", "load_scal_img", SERVER_PIC_PATH))
        {
            app_satfinder_tip_hide();
            satfinder_pic_server_show = 1;
        }
    }

    if(satfinder_pic_app_show && satfinder_pic_server_show)
    {
        s_find_updating = 0;
        satfinder_update_timer_stop();
    }
    else if(s_find_updating == 0)
    {
        app_satfinder_update_fail();
        satfinder_update_timer_stop();
    }
    return 0;
}

static void satfinder_update_timer_reset(void)
{
	APP_TIMER_ADD(satfinder_picupdate_timer, satfinder_pic_update_timer_timeout, 300, TIMER_REPEAT);
}

static void app_satfind_pic_download_stop(void)
{
    int i = 0;

	satfinder_pic_app_show = 0;
	satfinder_pic_server_show = 0;
	satfinder_update_timer_stop();
    memset(satfind_pic_download_ok,0,sizeof(satfind_pic_download_ok));
    satfind_pic_download_abort=1;
    for(i=0; i<SAT_FINDER_MAX_PIC_ITEM; i++)
    {
        if(satfind_pic_download_handle[i]!=0)
        {
            gxapps_curl_async_abort(satfind_pic_download_handle[i]);
            satfind_pic_download_handle[i] = 0;
        }
    }
    satfind_pic_download_abort=0;
}


static void app_satfind_pic_cb(int message, int body)
{
	gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
	int i = 0;

	if(message == GXCURL_STATE_COMPLETE)
	{
		if(callback_data->handle>0)
		{
            if(message == GXCURL_STATE_COMPLETE)
            {
                if(satfind_pic_download_abort)
                  return;
                if(callback_data->handle>0)
                {
                    for(i=0;i<SAT_FINDER_MAX_PIC_ITEM;i++)
                    {
                        if(satfind_pic_download_handle[i] ==callback_data->handle)
                        {
                            satfind_pic_download_ok[i]=1;
                            break;
                        }
                    }
                }
            }
        }
    }
}


int http_download_file(const char *url,const char *patch, int mode)
{
	int num = 0;

	if(mode == DVB_SAT_FINDE_SERVER_MODE)
	{
		num = 1;
	}
	else
	{
		num = 0;
	}
	satfind_pic_download_handle[num] = gxapps_curl_async_download(url,patch,app_satfind_pic_cb);

    return 0;
}

void download_qr(char *apk,char *server)
{
    if(apk==NULL || server==NULL)
    {
		s_find_updating = 0;
        app_log_error("gen qr failed,check network\n");
        return;
    }
    http_download_file(apk,APK_PIC_PATH,DVB_SAT_FINDE_APK_MODE);
    http_download_file(server,SERVER_PIC_PATH,DVB_SAT_FINDE_SERVER_MODE);
}


static void app_satfinder_refresh(void)
{
	app_satfind_pic_download_stop();
	app_satfinder_tip_show("Updating...", 10000);
    s_find_updating = 1;
    satfinder_update_timer_reset();
	dream_dvbfinder_start(download_qr);
}

SIGNAL_HANDLER int app_satfinder_create(GuiWidget *widget, void *usrdata)
{
    db_sat_init();
    dream_dvbfinder_init();
    app_satfinder_refresh();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satfinder_destroy(GuiWidget *widget, void *usrdata)
{
	app_satfinder_tip_hide();
	app_satfind_pic_download_stop();
	db_sat_release();
	GxCore_FileDelete(APK_PIC_PATH);
	GxCore_FileDelete(SERVER_PIC_PATH);
	app_send_msg_exec(GXMSG_SEARCH_SCAN_STOP, (void *)NULL);
	dream_dvbfinder_stop();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satfinder_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
    int key = 0;
    GUI_Event *event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_KEYDOWN:
            key = find_virtualkey_ex(event->key.scancode,event->key.sym);
            switch(key)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_0:
                    app_log_debug("current focus %s\n",GUI_GetFocusWidget());
                    break;
				case STBK_RED:
					app_satfinder_refresh();
					break;
                case STBK_MENU:
                case STBK_EXIT:
                    GUI_EndDialog(WND_SATFINDER);
                    break;
                 default:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
            }
            break;
        default:
            ret = EVENT_TRANSFER_KEEPON;
            break;
	}
    return ret;
}

void app_satfinder_menu_exec(void)
{
    SatFinderCheckStatus ret=CHECK_OK;

    ret = app_satfind_check();
    if(ret == SAT_EXCESS_MAX)
    {
        app_mainmenu_tip_exec(MM_MAX_SAT);
    }
    else if(ret == TP_EXCESS_MAX)
    {
        app_mainmenu_tip_exec(MM_MAX_TP);
    }
    else
    {
        GUI_CreateDialog(WND_SATFINDER);
    }
}

#endif

