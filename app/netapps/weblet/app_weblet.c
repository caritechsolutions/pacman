/*
 * =====================================================================================
 *
 *       Filename:  app_server.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011??12??09?? 09ʱ34??49??
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_send_msg.h"
#include "app_module.h"
#include "app_pop.h"
#include "app_windows.h"
#include "app_channel_info.h"

#if WEBLET_SUPPORT
#include "app_qrencode.h"
#include "app_pop_qr.h"
#include "app_weblet.h"
#include <sys/socket.h>
#ifdef LINUX_OS
#include <sys/un.h>
#endif

#define QRCODE_NAME  "txt_weblet_qrcode_name"
#define QR_PAGE1 "img_weblet_page_ico1"
#define QR_PAGE2 "img_weblet_page_ico2"
#define PAGE_FOCUS_COLOR   "[#FFFF00,#FFFF00,#FFFF00]"
#define PAGE_UNFOCUS_COLOR "[#808080,#808080,#808080]"


typedef enum {
	WEB_REMOTE = 0,
	WEB_PROG,
	//WEB_FEEDBACK,
	WEB_TOTAL
}WEBLET_ITEM;
static WEBLET_ITEM selected = 0;
static GxBusPmViewInfo view_info_old = {0};
static event_list* sTimerQcode = NULL;
static char ipaddr[64] = {0};
#if QR_POP_SUPPORT
static char s_qr_url[128] = {0};
#endif
static char s_weblet_server_forbid_host_ip[20]={0};
static unsigned int weblet_qr_handle = 0;

static int _timer_out(void *userdata)
{
#if QR_POP_SUPPORT
	if(weblet_qr_handle>0)
	{
		qren_encode_show(weblet_qr_handle, "img_qrcode");
	}
#endif
	GUI_SetInterface("flush", NULL);
	sTimerQcode = NULL;
	return 0;
}

static void _timer_stop(void)
{
    if(sTimerQcode!= NULL)
        timer_stop(sTimerQcode);
}

static void _timer_reset(void)
{
    if(sTimerQcode != NULL)
        reset_timer(sTimerQcode);
}

static void show_qrencode(void)
{
	char url[128];

	switch (selected)
	{
		case WEB_REMOTE:
		default:
			sprintf(url, "http://%s", ipaddr);
			GUI_SetProperty(QRCODE_NAME, "string", STR_ID_WEB_REMOTE);
			GUI_SetProperty(QR_PAGE1, "backcolor", PAGE_FOCUS_COLOR);
			GUI_SetProperty(QR_PAGE2, "backcolor", PAGE_UNFOCUS_COLOR);
			break;

		case WEB_PROG:
			sprintf(url, "http://%s/programmes.html", ipaddr);
			GUI_SetProperty(QRCODE_NAME, "string", STR_ID_WEB_FINDER);
			GUI_SetProperty(QR_PAGE1, "backcolor", PAGE_UNFOCUS_COLOR);
			GUI_SetProperty(QR_PAGE2, "backcolor", PAGE_FOCUS_COLOR);
			break;
#if 0
		case WEB_FEEDBACK:
			sprintf(url, "http://%s/feedback.html", ipaddr);
			GUI_SetProperty(QRCODE_NAME, "string", "Feedback");
			break;
#endif
	}

	/* don't show here, create one timer instead */
#if QR_POP_SUPPORT
	weblet_qr_handle = qren_encode_init(url, 16);
#endif
	sTimerQcode = create_timer(_timer_out, 100, NULL, TIMER_ONCE);
}

static int _weblet_pop_exit_cb(PopDlgRet ret)
{
    if(!strcmp(GUI_GetFocusWindow(),WND_FULL_SCREEN)&&(g_AppPlayOps.normal_play.play_total != 0))
        app_channel_info_create_dialog();

    return 0;
}

static int app_start_weblet_server(char *ip_get)
{

    if(!app_check_weblet_server_state())
    {
        memset(s_weblet_server_forbid_host_ip,0,sizeof(s_weblet_server_forbid_host_ip));
        strncpy(s_weblet_server_forbid_host_ip,ip_get,strlen(ip_get));
        app_weblet_init(s_weblet_server_forbid_host_ip);
    }
    else
    {
        if(strcmp(ip_get,s_weblet_server_forbid_host_ip))
        {
            app_weblet_uninit();
            memset(s_weblet_server_forbid_host_ip,0,sizeof(s_weblet_server_forbid_host_ip));
            strncpy(s_weblet_server_forbid_host_ip,ip_get,strlen(ip_get));
            app_weblet_init(s_weblet_server_forbid_host_ip);
        }
    }

    return 0;
}

int app_qr_ip_addr_get(char *ip_get)
{
	struct ifreq ifr;
	int ret = 0;
	struct sockaddr_in* s_in;
	int fd;
    ubyte64 cur_dev;
	char* ip_addr = 0;
    PopDlg pop;
    char *default_server_port = NULL;

	if(ip_get == NULL) return -1;

    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = _weblet_pop_exit_cb;
    pop.timeout_sec = 3;
    pop.pos.x =400;
    pop.pos.y =130;

    GxMsgProperty_NetDevType type;
    GxMsgProperty_NetDevState state;
    memset(cur_dev, 0, sizeof(ubyte64));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, cur_dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);

	memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
	strcpy(type.dev_name, cur_dev);
	app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
	if(
#if (ETH_SUPPORT > 0)
		(type.dev_type != IF_TYPE_WIRED) &&
#endif
#if WIFI_SUPPORT
		(type.dev_type != IF_TYPE_WIFI) &&
#endif
        1)
	{
		pop.str = STR_ID_LINK_NETWORK;
		popdlg_create(&pop);
		return -1;
	}

    if(state.dev_state == IF_STATE_CONNECTED)
    {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd<0) return -1;
        strcpy(ifr.ifr_name, cur_dev);
        ifr.ifr_addr.sa_family = AF_INET;
        ret = ioctl(fd, SIOCGIFADDR, &ifr);
        if (ret<0)
        {
            close(fd);
            return -1;
        }
        s_in = (struct sockaddr_in*)&ifr.ifr_addr;
        ip_addr = strdup(inet_ntoa(s_in->sin_addr));

		strncpy(ip_get, ip_addr, strlen(ip_addr));

        close(fd);
		if (ip_addr)
		{
			free(ip_addr);
			ip_addr = 0;
		}
	}
	else
	{
		pop.str = STR_ID_SERVER_FAIL;
		popdlg_create(&pop);
		return -1;
	}

    app_start_weblet_server(ip_get);

    strcat(ip_get,":");
    default_server_port = app_get_weblet_server_default_port();
    strcat(ip_get,default_server_port);

    return 0;
}

void app_weblet_init_dialog(void)
{
	int ret = 0;

	memset(ipaddr, 0, sizeof(ipaddr));
	if((ret = app_qr_ip_addr_get(ipaddr)) < 0)
		return;

	GUI_CreateDialog(WND_WEBLET);
	return;
}

int app_weblet_pop_rc_qr(void)
{
#if QR_POP_SUPPORT
	int ret = 0;
	char ip_addr[64] = {0};
	PopQrParas paras;

	memset(ip_addr, 0, sizeof(ip_addr));
	if((ret = app_qr_ip_addr_get(ip_addr)) < 0)
		return -1;

	sprintf(s_qr_url, "http://%s", ip_addr);
	memset(&paras, 0, sizeof(PopQrParas));
	paras.str_content = s_qr_url;
	paras.str_title = STR_ID_WEB_LOGIN_TIP;
	app_pop_create_qr(&paras);
#endif
	return 0;
}

SIGNAL_HANDLER int app_weblet_create(GuiWidget *widget, void *usrdata)
{
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_old));
	show_qrencode();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_weblet_destroy(GuiWidget *widget, void *usrdata)
{
	GxBusPmViewInfo view_info = {0};
	selected = 0;
	APP_TIMER_REMOVE(sTimerQcode);
	if(weblet_qr_handle>0)
	{
#if QR_POP_SUPPORT
		qren_encode_close(weblet_qr_handle);
#endif
		weblet_qr_handle = 0;
	}
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	if(view_info.skip_view_switch != view_info_old.skip_view_switch)
	{
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)&view_info_old);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_weblet_got_focus(GuiWidget *widget, void *usrdata)
{
	_timer_reset();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_weblet_lost_focus(GuiWidget *widget, void *usrdata)
{
	_timer_stop();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_weblet_keypress(GuiWidget *widget, void *usrdata)
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
					break;

                case STBK_EXIT:
                case STBK_MENU:
					{
						GUI_EndDialog(WND_WEBLET);
					}
                    break;

                case STBK_UP:
                case STBK_DOWN:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                case STBK_LEFT:
					selected = selected?selected-1:(WEB_TOTAL-1);
					show_qrencode();
					break;

				case STBK_RIGHT:
					selected = (selected+1)%WEB_TOTAL;
					show_qrencode();
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
#endif
