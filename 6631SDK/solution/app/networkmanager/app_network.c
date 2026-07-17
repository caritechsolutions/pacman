#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_pop.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_windows.h"

#if NETWORK_SUPPORT
#define IMG_SCAN_GREEN				"img_network_green"
#define TXT_SCAN_GREEN				"text_network_green"
#define IMG_SCAN_RED				"img_network_red"
#define TXT_SCAN_RED				"text_network_red"
#define IMG_OPT_HALF_FOCUS_R		"s_bar_half_focus_r"
#define IMG_OPT_HALF_UNFOCUS		"s_bar_half_unfocus"
#define BOX_NETWORK_OPTIONS			"box_network_options"
#define MODE_CONTENT				"[Off,On]"
#define TYPE_CONTENT				"[Static IP,DHCP]"
#define NO_DEV_CONTENT				"[None]"

#define INVALIDE_IP "000.000.000.000"

#ifdef ECOS_OS
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

extern int network_dhcp(char *if_name);
extern int network_down(char *if_name);

#if WIFI_SUPPORT
extern bool s_wifi_set_connecting;
#endif
#endif

typedef enum
{
    NET_ITEM_DEV,
    NET_ITEM_MODE,
    NET_ITEM_TYPE,
    NET_ITEM_IPADDR,
    NET_ITEM_SUB_MASK,
    NET_ITEM_GATEWAY,
    NET_ITEM_DNS1,
    NET_ITEM_DNS2,
    NET_ITEM_SAVE,
    NET_ITEM_STATE,
    NET_ITEM_TOTAL,
}NetWorkItem;

typedef enum
{
    NET_MODE_OFF = 0,
    NET_MODE_ON,
}NetWorkMode;

typedef enum
{
    NET_TYPE_STATIC = 0,
    NET_TYPE_DHCP,
}NetWorkType;

typedef struct
{
    uint32_t		dev;
    uint32_t		mode;
	uint32_t		ip_type;
	uint32_t		dev_total;
	IfType			dev_type;
	IfState			connection_state;
	ubyte64			*network_dev;
	GxMsgProperty_NetDevIpcfg ip_cfg;
	bool			pppoe_flag;
	bool			pop_flag;
	uint32_t			net_sel;
}NetworkUICtrl;

static NetworkUICtrl s_network_ctrl = {0};
static event_list *s_network_error_timer = NULL;
static event_list *s_network_pop_timer = NULL;
static event_list *s_network_hide_pop_timer = NULL;
static event_list *sp_GetIpCfg = NULL;

#if VPN_SUPPORT
extern void app_vpn_menu_create(void);
#endif

#if WIFI_SUPPORT
extern status_t app_create_wifi_menu(ubyte64 *wifi_dev);
#endif

#if MOD_3G_SUPPORT
extern status_t app_create_3g_menu(ubyte64 *dev_3g);
#endif

#if PPPOE_SUPPORT
#define PPPOE_CONTENT	"[PPPoE]"
extern status_t app_create_pppoe_menu(ubyte64 *pppoe_dev);
#endif

static char *s_network_opt[NET_ITEM_TOTAL] = {
	"cmbox_network_device",
	"cmbox_network_mode",
    "cmbox_network_type",
	"edit_network_ip_address",
	"edit_network_mask",
	"edit_network_gate_way",
	"edit_network_dns1",
	"edit_network_dns2",
	"btn_network_save",
	"text_network_opt1",
};

static char * s_network_boxitem[] =
{
    "boxitem_network_device",
    "boxitem_network_mode",
    "boxitem_network_type",
    "boxitem_network_ipaddress",
    "boxitem_network_mask",
    "boxitem_network_gate_way",
    "boxitem_network_dns1",
    "boxitem_network_dns2",
	"boxitem_network_save"
};

static char *s_network_text[] = {
       "text_network_device",
       "text_network_mode",
       "text_network_type",
       "text_network_ip_address",
       "text_network_mask",
       "text_network_gate_way",
       "text_network_dns1",
       "text_network_dns2",
       "text_network_save",
};

char* app_network_get_state_str(IfState state)
{
    static char *str = NULL;

    switch(state)
    {
        case IF_STATE_INVALID:
            str=STR_ID_INVALID;
            break;

        case IF_STATE_IDLE:
            str=STR_ID_NO_CONNECT;
            break;

        case IF_STATE_START:
        case IF_STATE_CONNECTING:
            str=STR_ID_CONNECTTING;
            break;

        case IF_STATE_CONNECTED:
            str=STR_ID_CONNECTED;
            break;

        case IF_STATE_REJ:
#if (WIFI_SUPPORT > 0)
            if(s_network_ctrl.dev_type == IF_TYPE_WIFI)
                str = STR_ID_ERR_PASSWD;
            else
#endif
                str=STR_ID_NO_CONNECT;
            break;

        case IF_STATE_DISCONNECTED:
			str=STR_ID_NO_CONNECT;
            break;

        default:
            break;
    }

    return str;
}

static void app_set_lost_focus_forecolor(void)
{
#define NETWORK_LOST_FOCUS_FORECOLOR	"[text_golden_color,text_golden_color,text_golden_color]"

	GUI_GetProperty(BOX_NETWORK_OPTIONS, "select", &s_network_ctrl.net_sel);
	if(s_network_ctrl.net_sel > NET_ITEM_SAVE)
		return;
	GUI_SetProperty(s_network_opt[s_network_ctrl.net_sel], "forecolor", NETWORK_LOST_FOCUS_FORECOLOR);
	GUI_SetProperty(s_network_text[s_network_ctrl.net_sel], "forecolor", NETWORK_LOST_FOCUS_FORECOLOR);
}

static void app_retset_focus_forecolor(void)
{
#define NETWORK_TEXT_GOT_FOCUS_FORECOLOR		"[text_color,text_color,text_disable_color]"
#define NETWORK_BOX_EDIT_GOT_FOCUS_FORECOLOR		"[text_color,text_focus_color,text_disable_color]"

	if(s_network_ctrl.net_sel > NET_ITEM_SAVE)
		return;
	GUI_SetProperty(s_network_opt[s_network_ctrl.net_sel], "forecolor", NETWORK_BOX_EDIT_GOT_FOCUS_FORECOLOR);
	GUI_SetProperty(s_network_text[s_network_ctrl.net_sel], "forecolor", NETWORK_TEXT_GOT_FOCUS_FORECOLOR);
}

static void app_network_box_set_select(NetWorkItem sel_item)
{
	int32_t box_sel = 0;

	box_sel = sel_item;
	GUI_SetProperty(BOX_NETWORK_OPTIONS, "select", &box_sel);
}

static int32_t app_network_cmbox_get_select(NetWorkItem boxitem)
{
	int32_t cmb_sel = 0;

	if(boxitem < NET_ITEM_DEV || boxitem > NET_ITEM_TYPE)
		return -1;
	GUI_GetProperty(s_network_opt[boxitem], "select", &cmb_sel);
	return cmb_sel;
}

static void app_set_network_state_string(IfState state)
{
    char *state_str = NULL;

	state_str = app_network_get_state_str(state);
    GUI_SetProperty(s_network_opt[NET_ITEM_STATE], "string", state_str);
}

static void app_network_ip_update(void)
{
    app_network_ip_completion(s_network_ctrl.ip_cfg.ip_addr);
    GUI_SetProperty(s_network_opt[NET_ITEM_IPADDR], "string", s_network_ctrl.ip_cfg.ip_addr);

    app_network_ip_completion(s_network_ctrl.ip_cfg.net_mask);
    GUI_SetProperty(s_network_opt[NET_ITEM_SUB_MASK], "string", s_network_ctrl.ip_cfg.net_mask);

    app_network_ip_completion(s_network_ctrl.ip_cfg.gate_way);
    GUI_SetProperty(s_network_opt[NET_ITEM_GATEWAY], "string", s_network_ctrl.ip_cfg.gate_way);

    app_network_ip_completion(s_network_ctrl.ip_cfg.dns1);
    GUI_SetProperty(s_network_opt[NET_ITEM_DNS1], "string", s_network_ctrl.ip_cfg.dns1);

    app_network_ip_completion(s_network_ctrl.ip_cfg.dns2);
    GUI_SetProperty(s_network_opt[NET_ITEM_DNS2], "string", s_network_ctrl.ip_cfg.dns2);
}

void app_network_ip_clear(void)
{
    uint8_t i = 0;

    for(i = NET_ITEM_IPADDR; i <= NET_ITEM_DNS2; i++)
    {
		GUI_SetProperty(s_network_opt[i], "string", INVALIDE_IP);
    }
}

static int app_network_show_ip(void)
{
    GxMsgProperty_NetDevMode dev_mode;
    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));

    if(s_network_ctrl.network_dev == NULL)
        return -1;
    strcpy(dev_mode.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);

    if(dev_mode.mode == NET_MODE_ON)
    {
        if(s_network_ctrl.ip_cfg.ip_type == NET_TYPE_STATIC)
        {
            app_network_ip_update();
        }
        else
        {
            if(s_network_ctrl.connection_state == IF_STATE_CONNECTED)
            {
                app_network_ip_update();
            }
            else
            {
                app_network_ip_clear();
            }
        }
    }
    else
    {
        app_network_ip_clear();
    }
    return 0;
}

static void app_set_boxitem_state(NetWorkMode mode,NetWorkType type)
{
        int i = 0;

        if ( mode == NET_MODE_OFF )
        {
            return ;
        }

        GUI_SetProperty(s_network_boxitem[NET_ITEM_TYPE], "enable", NULL);

        if((type == NET_TYPE_DHCP) || (s_network_ctrl.pppoe_flag == true))
        {
                for(i = NET_ITEM_IPADDR; i < NET_ITEM_SAVE; i++)
                {
                        GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
                }
        }
        else
        {
                for(i = NET_ITEM_IPADDR; i < NET_ITEM_SAVE; i++)
                {
                        GUI_SetProperty(s_network_boxitem[i], "enable", NULL);
                }
        }
}

static void app_set_address_state_by_type(NetWorkType type)
{
	int i = 0;
	if((type == NET_TYPE_DHCP) || (s_network_ctrl.pppoe_flag == true))
	{
		for(i = NET_ITEM_IPADDR; i < NET_ITEM_SAVE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
		}
	}
	else
	{
		for(i = NET_ITEM_IPADDR; i < NET_ITEM_SAVE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "enable", NULL);
		}
	}
	app_network_show_ip();
    app_update_pop_dlg(WND_POP_BOOK);
}

void app_network_iptype_set(void)
{
    int cmb_sel = 0;

    if(s_network_ctrl.pppoe_flag == true)
	{
	    cmb_sel = 0;
	}
	else
	{
		cmb_sel = s_network_ctrl.ip_type;
	}
    GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "select", &cmb_sel);
}

void app_network_mode_set(void)
{
	int32_t mode_sel = 0;
	int i = 0;
	mode_sel = s_network_ctrl.mode;
	GUI_SetProperty(s_network_opt[NET_ITEM_MODE], "select", &mode_sel);

	if(s_network_ctrl.mode == NET_MODE_OFF)
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_SAVE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
		}
		app_network_show_ip();

	}
    if(0 == s_network_ctrl.dev_total)
    {
        GUI_SetProperty(s_network_boxitem[NET_ITEM_MODE], "disable", NULL);
    }
    else if(s_network_ctrl.dev_total > 0)
    {
        GUI_SetProperty(s_network_boxitem[NET_ITEM_MODE], "enable", NULL);
    }
}

static int _network_get_current_dev(void)
{
    int dev_index = 0;
    int i = 0;
    ubyte64 cur_dev = {0};

    if(s_network_ctrl.network_dev != NULL)
    {
        app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
        for(i = 0; i < s_network_ctrl.dev_total; i++)
        {
            if(strcmp(cur_dev, s_network_ctrl.network_dev[i]) == 0)
            {
                dev_index = i;
                break;
            }
        }
    }

    return dev_index;
}

#ifdef LINUX_OS

static int _network_is_item_invalid(int sel)
{
	int invalid = 0;

	if(s_network_ctrl.mode == NET_MODE_OFF)
	{
		if((sel >= NET_ITEM_TYPE) && (sel < NET_ITEM_SAVE))
			invalid = 1;
	}
	else if((s_network_ctrl.ip_type == NET_TYPE_DHCP) || (s_network_ctrl.pppoe_flag == true))
	{
		if((sel > NET_ITEM_TYPE) && (sel < NET_ITEM_SAVE))
			invalid = 1;
	}

	return invalid;
}
#endif

void app_network_dev_update(void)
{
    GxMsgProperty_NetDevMode dev_mode;
    GxMsgProperty_NetDevState state;
    GxMsgProperty_NetDevType type;
	int32_t dev_sel = 0;

    GUI_GetProperty(s_network_opt[NET_ITEM_DEV], "select", &dev_sel);
    memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
    strcpy(type.dev_name, s_network_ctrl.network_dev[dev_sel]);
    app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
    s_network_ctrl.dev_type = type.dev_type;

#ifdef ECOS_OS
#if WIFI_SUPPORT
	if(s_wifi_set_connecting)
	{
		if(s_network_ctrl.dev_type == IF_TYPE_WIFI)
		{
			int32_t init_value;
			GxBus_ConfigGetInt(IP_TYPE_WIFI_KEY, &init_value, IP_TYPE_DEFALT);
			s_network_ctrl.ip_type = init_value;
			s_network_ctrl.mode = NET_MODE_ON;//On
		}
		s_wifi_set_connecting = false;
	}
	else
#endif
#endif
	{
	    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
	    strcpy(dev_mode.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]);
	    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
	    s_network_ctrl.mode = dev_mode.mode;
	}
#if PPPOE_SUPPORT
    GxMsgProperty_PppoeCfg pppoe_cfg;
    memset(&pppoe_cfg, 0, sizeof(GxMsgProperty_PppoeCfg));
    strcpy(pppoe_cfg.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]);
    app_send_msg_exec(GXMSG_PPPOE_GET_PARAM, &pppoe_cfg);
    if(pppoe_cfg.mode > 0)
    {
        s_network_ctrl.pppoe_flag = true;
        GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "content", PPPOE_CONTENT);
        s_network_ctrl.ip_type = s_network_ctrl.ip_cfg.ip_type;
    }
    else
    {
        s_network_ctrl.pppoe_flag = false;
        GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "content", TYPE_CONTENT);
    }
#endif

    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    s_network_ctrl.connection_state = state.dev_state;

    memset(&s_network_ctrl.ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
    if(s_network_ctrl.pppoe_flag)
    {
        s_network_ctrl.ip_cfg.ip_type = s_network_ctrl.ip_type;
    }
    else
    {
        strcpy(s_network_ctrl.ip_cfg.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]);
        app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_network_ctrl.ip_cfg);
    }
#if RNDIS_SUPPORT
#ifdef LINUX_OS
    if(s_network_ctrl.dev_type == IF_TYPE_RNDIS)
        s_network_ctrl.ip_type = NET_TYPE_DHCP;
    else
#endif
#endif
#if (MOD_3G_SUPPORT > 0)
    if(s_network_ctrl.dev_type == IF_TYPE_3G)
        s_network_ctrl.ip_type = NET_TYPE_DHCP;//dhcp
#ifdef LINUX_OS
    else if(s_network_ctrl.dev_type == IF_TYPE_ETH3G)
        s_network_ctrl.ip_type = NET_TYPE_DHCP;//dhcp
#endif
    else
#endif
    {
        if(false == s_network_ctrl.pppoe_flag)
            s_network_ctrl.ip_type = s_network_ctrl.ip_cfg.ip_type;
    }

    app_network_iptype_set();
    app_network_mode_set();
    app_set_network_state_string(s_network_ctrl.connection_state);

#if (WIFI_SUPPORT > 0)
    if(s_network_ctrl.dev_type == IF_TYPE_WIFI)
    {
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "show");
        GUI_SetProperty(TXT_SCAN_GREEN, "string", STR_ID_SCAN);
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "show");

        app_network_wifi_switch_dev_active_by_name(type.dev_name);
    }
    else
#endif
#if (MOD_3G_SUPPORT > 0)
    if(s_network_ctrl.dev_type == IF_TYPE_3G) //3g
    {
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "show");
        GUI_SetProperty(TXT_SCAN_GREEN, "string", STR_ID_SETUP);
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "show");
    }
    else
#endif
#if (PPPOE_SUPPORT > 0)
    if(s_network_ctrl.dev_type == IF_TYPE_WIRED) //pppoe
    {
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "show");
        GUI_SetProperty(TXT_SCAN_GREEN, "string", STR_ID_PPPOE);
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "show");
    }
    else
#endif
    {
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "hide");
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "hide");
    }
}

void app_network_update(void)
{
	uint8_t i = 0;
    char *dev_content = NULL;
    ubyte64 display_name = {0};
    GxMsgProperty_NetDevList dev_list;

    memset(&dev_list, 0, sizeof(GxMsgProperty_NetDevList));
    app_send_msg_exec(GXMSG_NETWORK_DEV_LIST, &dev_list);

    if(s_network_ctrl.network_dev != NULL)
	{
		GxCore_Free(s_network_ctrl.network_dev);
		s_network_ctrl.network_dev = NULL;
	}

    s_network_ctrl.dev_total = dev_list.dev_num;
    if(s_network_ctrl.dev_total == 0)
    {
        s_network_ctrl.dev = 0;
        s_network_ctrl.mode = NET_MODE_OFF;
        s_network_ctrl.ip_type = NET_TYPE_STATIC;
        s_network_ctrl.pppoe_flag = false;
		s_network_ctrl.dev_type = IF_TYPE_UNKOWN;
		s_network_ctrl.connection_state = IF_STATE_INVALID;

        GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "content", TYPE_CONTENT);
        GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "content", NO_DEV_CONTENT);
        app_network_ip_clear();
        app_network_iptype_set();
        app_network_mode_set();
		app_set_network_state_string(s_network_ctrl.connection_state);
    }
    else
    {
        if((s_network_ctrl.network_dev = (ubyte64*)GxCore_Malloc(s_network_ctrl.dev_total * sizeof(ubyte64))) != NULL)
        {
            memset(s_network_ctrl.network_dev, 0, s_network_ctrl.dev_total * sizeof(ubyte64));
            if((dev_content = (char*)GxCore_Malloc(s_network_ctrl.dev_total * sizeof(ubyte64) + 5)) != NULL)
            {
                memset(dev_content, 0, s_network_ctrl.dev_total * sizeof(ubyte64) + 5);
                for(i = 0; i < s_network_ctrl.dev_total; i++)
                {
                    strcpy(s_network_ctrl.network_dev[i], dev_list.dev_name[i]);
                    memset(display_name, 0, sizeof(ubyte64));
                    if(i == 0)
                    {
                        strcat(dev_content, "[");
                        app_if_name_convert(s_network_ctrl.network_dev[i], display_name);
                        strcat(dev_content, display_name);
                    }
                    else
                    {
                        strcat(dev_content, ",");
                        app_if_name_convert(s_network_ctrl.network_dev[i], display_name);
                        strcat(dev_content, display_name);
                    }
                }
                strcat(dev_content, "]");
                GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "content", dev_content);
                GxCore_Free(dev_content);
                dev_content = NULL;
            }
        }
    }
}

void app_network_set_mac(char *dev, char *mac)
{
    GxMsgProperty_NetDevMac dev_mac;

    if((dev == NULL) || (strlen(dev) == 0) || (mac == NULL) || (strlen(mac) == 0))
        return;
    memset(&dev_mac, 0, sizeof(GxMsgProperty_NetDevMac));
    strcpy(dev_mac.dev_name, dev);
    strcpy(dev_mac.dev_mac, mac);
    app_send_msg_exec(GXMSG_NETWORK_SET_MAC, &dev_mac);
}

void app_network_get_mac(char *dev, char *mac)
{
    GxMsgProperty_NetDevMac dev_mac;

    if((dev == NULL) || (strlen(dev) == 0) || (mac == NULL))
        return;
    memset(&dev_mac, 0, sizeof(GxMsgProperty_NetDevMac));
    strcpy(dev_mac.dev_name, dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_MAC, &dev_mac);
    strcpy(mac, dev_mac.dev_mac);
}

static void app_network_pop_info_state_show(void)
{
    GUI_SetProperty("img_network_pop_bg", "state", "show");
    GUI_SetProperty("txt_network_pop_tip", "state", "show");
    GUI_SetProperty("txt_network_pop_info", "state", "show");
    GUI_SetProperty("img_network_pop_bg", "draw_now", NULL);//for bug 145219
    GUI_SetProperty("txt_network_pop_tip", "draw_now", NULL);//for bug 145219
    s_network_ctrl.pop_flag = true;
}

static void app_network_pop_info_show(const char *info_str)
{
    app_network_pop_info_state_show();
    GUI_SetProperty("txt_network_pop_info", "string", (void*)info_str);
    GUI_SetProperty("txt_network_pop_info", "draw_now", NULL);//for bug 145219
    app_update_pop_dlg(WND_POP_BOOK);
}

static void app_network_pop_info_hide(void)
{
    GUI_SetProperty("img_network_pop_bg", "state", "hide");
    GUI_SetProperty("txt_network_pop_tip", "state", "hide");
    GUI_SetProperty("txt_network_pop_info", "state", "hide");
    GUI_SetProperty("txt_network_pop_info", "draw_now", NULL);
    s_network_ctrl.pop_flag = false;
}

static int _get_ipcfg_cb(void *data)
{
	int32_t box_sel = 0;
    IfState dev_state = s_network_ctrl.connection_state;
    sp_GetIpCfg = NULL;

	GUI_GetProperty(BOX_NETWORK_OPTIONS, "select", &box_sel);
    if(dev_state == IF_STATE_CONNECTED)
    {
        memset(&s_network_ctrl.ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
        app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &(s_network_ctrl.ip_cfg.dev_name));
        app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_network_ctrl.ip_cfg);

        GxMsgProperty_NetDevMode dev_mode;
        memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
        strcpy(dev_mode.dev_name, s_network_ctrl.ip_cfg.dev_name);
        app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
        if(s_network_ctrl.mode != dev_mode.mode)
            s_network_ctrl.mode = dev_mode.mode;

        if(s_network_ctrl.ip_type != s_network_ctrl.ip_cfg.ip_type && 1 == s_network_ctrl.ip_cfg.ip_type && box_sel > NET_ITEM_TYPE && box_sel < NET_ITEM_SAVE)
        {
            app_network_box_set_select(NET_ITEM_DEV);
        }
#if RNDIS_SUPPORT
#ifdef LINUX_OS
    if(s_network_ctrl.dev_type == IF_TYPE_RNDIS)
        s_network_ctrl.ip_type = NET_TYPE_DHCP;
    else
#endif
#endif
#if (MOD_3G_SUPPORT > 0)
        if(s_network_ctrl.dev_type == IF_TYPE_3G)
            s_network_ctrl.ip_type = NET_TYPE_DHCP; //dhcp
#ifdef LINUX_OS
        else if(s_network_ctrl.dev_type == IF_TYPE_ETH3G)
            s_network_ctrl.ip_type = NET_TYPE_DHCP;//dhcp
#endif
        else
#endif
        {
            if(false == s_network_ctrl.pppoe_flag)
                s_network_ctrl.ip_type = s_network_ctrl.ip_cfg.ip_type;
        }
        app_network_iptype_set();
        app_network_mode_set();
    }
    app_set_boxitem_state(s_network_ctrl.mode,s_network_ctrl.ip_type);
    app_network_show_ip();
   if(s_network_ctrl.pop_flag)
    {
        app_network_pop_info_state_show();
    }

    //update wnd
    GUI_SetProperty(WND_NETWORK, "update", NULL);

    return 0;
}

static int app_network_state_msg_proc(void)
{
    if (reset_timer(sp_GetIpCfg) != 0)
    {
        sp_GetIpCfg = create_timer(_get_ipcfg_cb, 0, NULL, TIMER_ONCE);
    }

    return 0;
}

static void _network_state_msg_proc(GxMsgProperty_NetDevState *state)
{
    IfState old_state;
    if((state == NULL)||(s_network_ctrl.network_dev==NULL))
        return;

	s_network_ctrl.dev = app_network_cmbox_get_select(NET_ITEM_DEV);
    if(strcmp(state->dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]) != 0)
        return;

    //app_log_debug("~~~~~~[%s]%d, %s state change %d!!!!!\n", __func__, __LINE__, state->dev_name,  state->dev_state);

    old_state = s_network_ctrl.connection_state;
    if(state->dev_state == IF_STATE_INVALID)
    {
        //rebuild
    }
    else
    {
        s_network_ctrl.connection_state = state->dev_state;
#if (WIFI_SUPPORT > 0)
        GxMsgProperty_NetDevType devtype;
        memset(&devtype, 0, sizeof(GxMsgProperty_NetDevType));
        strcpy(devtype.dev_name, state->dev_name);
        app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &devtype);

        if(s_network_ctrl.connection_state == IF_STATE_REJ && devtype.dev_type == IF_TYPE_WIFI)
        {
            ubyte64 wifi_name;
            memset(wifi_name, 0, sizeof(ubyte64));
            strcpy(wifi_name, state->dev_name);
            app_send_msg_exec(GXMSG_WIFI_DEL_CUR_AP, &wifi_name);
        }
#endif
        if(s_network_ctrl.pop_flag)
        {
            int32_t box_sel = 0;
            GUI_GetProperty(BOX_NETWORK_OPTIONS, "select", &box_sel);
            if ( (state->dev_state == IF_STATE_IDLE ) && (box_sel == NET_ITEM_SAVE) )
            {
            }
            else
            {
                app_network_pop_info_hide();
            }
            //s_network_ctrl.pop_flag = true;
        }

        if(state->dev_state == IF_STATE_CONNECTED ||
                (state->dev_state == IF_STATE_DISCONNECTED &&  old_state != state->dev_state))
            app_network_state_msg_proc();
    }
	app_set_network_state_string(state->dev_state);
}

static int app_pop_info_timer_cb(void *use)
{
    uint32_t i = 0;
    ubyte64 cur_dev = {0};
    APP_TIMER_REMOVE(s_network_pop_timer);

    app_network_pop_info_hide();

    strcpy(cur_dev, s_network_ctrl.network_dev[s_network_ctrl.dev]);
    app_network_update();
    if(s_network_ctrl.dev_total > 0)
    {
        s_network_ctrl.dev = 0;
        for(i = 0; i < s_network_ctrl.dev_total; i++)
        {
            if(strcmp(cur_dev, s_network_ctrl.network_dev[i]) == 0)
            {
                s_network_ctrl.dev = i;
                break;
            }
        }
		for(i =  NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			if(s_network_ctrl.mode == NET_MODE_ON && s_network_ctrl.ip_type == NET_TYPE_STATIC)
				GUI_SetProperty(s_network_boxitem[i], "enable", NULL);
		}
		GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "select", &s_network_ctrl.dev);
        app_network_dev_update();
        app_network_show_ip();
    }
	else
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
		}

	}
    app_update_pop_dlg(WND_POP_BOOK);
    return 0;
}

static int app_pop_info_timer_invalid_cb(void *use)
{
	APP_TIMER_REMOVE(s_network_error_timer);
	app_network_pop_info_hide();

#ifdef ECOS_OS
	int32_t box_sel = 0;
	uint32_t dev_id   = 0 ;
	IfType   dev_type = IF_TYPE_WIFI;
	uint32_t mode = 0 ;
	uint32_t ip_type = 0;
	bool	 pppoe_flag = false;
	uint32_t i = 0;
	ubyte64 cur_dev = {0};

	GUI_GetProperty(BOX_NETWORK_OPTIONS, "select", &box_sel);

	dev_id   = s_network_ctrl.dev ;
	dev_type = s_network_ctrl.dev_type ;
	mode    = s_network_ctrl.mode ;
	ip_type = s_network_ctrl.ip_type ;
	pppoe_flag = s_network_ctrl.pppoe_flag ;

	strcpy(cur_dev, s_network_ctrl.network_dev[s_network_ctrl.dev]);
	app_network_update();
	if(s_network_ctrl.dev_total > 0)
	{
		s_network_ctrl.dev = 0;
		for(i = 0; i < s_network_ctrl.dev_total; i++)
		{
			if(strcmp(cur_dev, s_network_ctrl.network_dev[i]) == 0)
			{
				s_network_ctrl.dev = i;
				break;
			}
		}
		for(i =  NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "enable", NULL);
		}
		GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "select", &s_network_ctrl.dev);
	}
	else
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
		}

	}

	if( (s_network_ctrl.dev_total > 0) && (dev_type == s_network_ctrl.dev_type) &&
		(dev_id == s_network_ctrl.dev) )
	{
		if ( mode != s_network_ctrl.mode )
		{
			s_network_ctrl.mode = mode ;
			app_network_mode_set();
		}
		if ( (ip_type != s_network_ctrl.ip_type) || (pppoe_flag != s_network_ctrl.pppoe_flag) )
		{
			s_network_ctrl.ip_type = ip_type ;
			s_network_ctrl.pppoe_flag = pppoe_flag ;
			app_network_iptype_set();
		}
	}

	if(s_network_ctrl.mode == NET_MODE_OFF)
	{
		if(box_sel == NET_ITEM_SAVE)
		{
			if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
			{
				app_update_pop_dlg(WND_POP_BOOK);
			}
			else
			{
				app_network_box_set_select(NET_ITEM_DEV);
			}
		}
	}
#endif

	app_network_show_ip();
	return 0;
}

static int app_hide_pop_timer_cb(void *use)
{
    APP_TIMER_REMOVE(s_network_hide_pop_timer);
    app_network_pop_info_hide();
    return 0;
}

static bool app_network_check_status(void)
{
    GxMsgProperty_NetDevState state;

    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    s_network_ctrl.connection_state = state.dev_state;

    return ((state.dev_state == IF_STATE_CONNECTED) ? true : false);
}

static bool app_network_check_config(void)
{
    uint8_t i = 0;
    char *p[5] = {NULL};
    char buf[5][16];
	int32_t type_sel = 0;
	int32_t mode_sel = 0;
    GxMsgProperty_NetDevMode dev_mode;

    GUI_GetProperty(s_network_opt[NET_ITEM_TYPE], "select", &type_sel);
    GUI_GetProperty(s_network_opt[NET_ITEM_MODE], "select", &mode_sel);
    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));

    for(i = 0; i < 5; i++)
    {
        GUI_GetProperty(s_network_opt[i+NET_ITEM_IPADDR], "string", &p[i]);
        app_network_ip_simplify(buf[i], p[i]);
    }

    strcpy(dev_mode.dev_name, s_network_ctrl.ip_cfg.dev_name);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);

    if(strcmp(s_network_ctrl.ip_cfg.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]) != 0
            ||(dev_mode.mode != mode_sel)
            ||(s_network_ctrl.ip_cfg.ip_type != type_sel)
            ||strncmp(s_network_ctrl.ip_cfg.ip_addr, buf[0], strlen(s_network_ctrl.ip_cfg.ip_addr))
            ||strncmp(s_network_ctrl.ip_cfg.net_mask, buf[1], strlen(s_network_ctrl.ip_cfg.net_mask))
            ||strncmp(s_network_ctrl.ip_cfg.gate_way, buf[2], strlen(s_network_ctrl.ip_cfg.gate_way))
            ||strncmp(s_network_ctrl.ip_cfg.dns1, buf[3], strlen(s_network_ctrl.ip_cfg.dns1))
            ||strncmp(s_network_ctrl.ip_cfg.dns2, buf[4], strlen(s_network_ctrl.ip_cfg.dns2)))
    {
        return true;
    }

    return false;
}

static void app_network_set_config(void)
{
    uint8_t i = 0;
	char *p[5] = {NULL};
	char buf[5][16];
    GxMsgProperty_NetDevMode dev_mode;
    GxMsgProperty_NetDevIpcfg dev_ipcfg;

    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
    memset(&dev_ipcfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));

    for(i = 0; i < 5; i++)
    {
        GUI_GetProperty(s_network_opt[i+NET_ITEM_IPADDR], "string", &p[i]);
        app_network_ip_simplify(buf[i], p[i]);
    }

    strcpy(dev_ipcfg.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]);
    dev_ipcfg.ip_type = s_network_ctrl.ip_type;
    strcpy(dev_ipcfg.ip_addr, buf[0]);
    strcpy(dev_ipcfg.net_mask, buf[1]);
    strcpy(dev_ipcfg.gate_way, buf[2]);
    strcpy(dev_ipcfg.dns1, buf[3]);
    strcpy(dev_ipcfg.dns2, buf[4]);

	strcpy(dev_mode.dev_name, s_network_ctrl.network_dev[s_network_ctrl.dev]);
    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
    if(dev_mode.mode != s_network_ctrl.mode)
    {
        if(s_network_ctrl.mode == NET_MODE_OFF)
        {
            dev_mode.mode = 0;
            app_send_msg_exec(GXMSG_NETWORK_SET_MODE, &dev_mode);
        }
        else
        {
            dev_mode.mode = 1;
            app_send_msg_exec(GXMSG_NETWORK_SET_MODE, &dev_mode);
            app_send_msg_exec(GXMSG_NETWORK_SET_IPCFG, &dev_ipcfg);
        }
    }
    else
    {
        if(s_network_ctrl.mode == NET_MODE_ON)
        {
            app_send_msg_exec(GXMSG_NETWORK_SET_IPCFG, &dev_ipcfg);
        }
    }
}

static int app_network_mask_valid(unsigned int mask)
{
	unsigned int temp;
	if((mask>0)&&(mask<0xffffffff))
	{
		temp = ntohl(mask);
		if((temp|(temp-1))==0xffffffff)
		{
			return 1;
		}
	}
	return 0;
}

#if 0
static int app_network_ip_mask_valid(unsigned int ip, unsigned int mask)
{
	int a=0, b=0, c=0;

	if((!app_network_ip_valid(ip))||(!app_network_mask_valid(mask)))
	{
		return 0;
	}

	a = ip&0x000000ff;
	b = ntohl(mask);

	if((a>0)&&(a<127))
	{
		if(mask<0x000000ff)
		{
			return 0;
		}
		if(mask>0x000000ff)
		{
			b-=0xff000000;
		}
	}
	else if((a>=128)&&(a<=191))
	{
		if(mask<0x0000ffff)
		{
			return 0;
		}
		if(mask>0x0000ffff)
		{
			b-=0xffff0000;
		}
	}
	else if((a>=192)&&(a<=223))
	{
		if(mask<0x00ffffff)
		{
			return 0;
		}
		if(mask>0x00ffffff)
		{
			b-=0xffffff00;
		}
	}

	c = ~ntohl(mask)&ntohl(ip);
	if(c==0||c==~ntohl(mask))
	{
		return 0;
	}

	if(b>0)
	{
		c = b&(ntohl(ip));
		if(c==0||c==b)
		{
			return 0;
		}
	}

	return 1;
}
#endif

static int app_network_parameter_valid(unsigned int ip, unsigned int mask, unsigned int gateway)
{
    if(!app_network_ip_valid(ip))
    {
        return IP_ADDR_ERROR;
    }
    else if(!app_network_mask_valid(mask))
    {
        return SUB_MASK_ERROR;
    }
    else if(!app_network_ip_valid(gateway))
    {
        return GATE_WAY_ERROR;
    }

    if((ip&mask)!=(gateway&mask))
    {
        return IP_OR_MASK_ERROR;
    }

    return NO_ERROR;
}

static int app_network_check_parameter(void)
{
    int i = 0;
    char *p[5] = {NULL};
    uint32_t ip[4] = {0};
    uint32_t mask[4] = {0};
    uint32_t gateway[4] = {0};
    uint32_t dns1[4] = {0};
    uint32_t dns2[4] = {0};
    ubyte32 u_ip;
    ubyte32 u_mask;
    ubyte32 u_gateway;
    ubyte32 u_dns1;
    ubyte32 u_dns2;
    memset(u_ip, 0, sizeof(ubyte32));
    memset(u_mask, 0, sizeof(ubyte32));
    memset(u_gateway, 0, sizeof(ubyte32));
    memset(u_dns1, 0, sizeof(ubyte32));
    memset(u_dns2, 0, sizeof(ubyte32));

    NetWorkParamsError error_type = NO_ERROR;

    for(i = 0; i < 5; i++)
    {
        GUI_GetProperty(s_network_opt[i+NET_ITEM_IPADDR], "string", &p[i]);
    }
    sscanf(p[0], "%d.%d.%d.%d", &(ip[0]), &(ip[1]), &(ip[2]), &(ip[3]));
    sscanf(p[1], "%d.%d.%d.%d", &(mask[0]), &(mask[1]), &(mask[2]), &(mask[3]));
    sscanf(p[2], "%d.%d.%d.%d", &(gateway[0]), &(gateway[1]), &(gateway[2]), &(gateway[3]));
    sscanf(p[3], "%d.%d.%d.%d", &(dns1[0]), &(dns1[1]), &(dns1[2]), &(dns1[3]));
    sscanf(p[4], "%d.%d.%d.%d", &(dns2[0]), &(dns2[1]), &(dns2[2]), &(dns2[3]));

    sprintf(u_ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    sprintf(u_mask, "%d.%d.%d.%d", mask[0], mask[1], mask[2], mask[3]);
    sprintf(u_gateway, "%d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3]);
    sprintf(u_dns1, "%d.%d.%d.%d", dns1[0], dns1[1], dns1[2], dns1[3]);
    sprintf(u_dns2, "%d.%d.%d.%d", dns2[0], dns2[1], dns2[2], dns2[3]);

    error_type = app_network_parameter_valid(inet_addr(u_ip), inet_addr(u_mask), inet_addr(u_gateway));
    if(error_type != NO_ERROR)
    {
        return error_type;
    }
    if(!app_network_ip_valid(inet_addr(u_dns1)))
    {
        if(strcmp(p[3],"000.000.000.000") != 0)
        {
            error_type = DNS1_ERROR;
            return error_type;
        }
    }
    if(!app_network_ip_valid(inet_addr(u_dns2)))
    {
        if(strcmp(p[4],"000.000.000.000") != 0)
        {
            error_type = DNS2_ERROR;
            return error_type;
        }
        else
        {
            if(strcmp(p[3],"000.000.000.000") == 0)
            {
                error_type = DNS1_ERROR;
                return error_type;
            }
        }
    }

    if(strcmp(p[3], p[4])==0)
    {
        error_type = DNS_COMMON_ERROR;
        return error_type;

    }

    return error_type;
}

#if (WIFI_SUPPORT > 0)
static int _app_pop_dlg_disconnect_wifi_ret(PopDlgRet ret)
{
    int32_t cur_sel = 0;
    if(POP_VAL_OK == ret)
    {
        GUI_GetProperty(s_network_opt[NET_ITEM_DEV], "select", &cur_sel);
        app_send_msg_exec(GXMSG_WIFI_DEL_CUR_AP, s_network_ctrl.network_dev[cur_sel]);
        s_network_ctrl.connection_state = IF_STATE_IDLE ;
        app_network_state_msg_proc();
        app_set_network_state_string(IF_STATE_IDLE);
    }
    return 0;
}

static void _app_check_wifi_pop_dlg(void)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.format = POP_FORMAT_DLG;
    pop.type = POP_TYPE_YES_NO;
    pop.str = "Current wifi is working status, Switch next wifi must stop, Do you want to stop?";
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = _app_pop_dlg_disconnect_wifi_ret;
    popdlg_create(&pop);
}

int  _app_check_wifi_is_change_item(int32_t key)
{
    int32_t cur_sel = 0;
    int32_t next_sel = 0;
    GxMsgProperty_NetDevType  cur_type;
    GxMsgProperty_NetDevState cur_state;
    GxMsgProperty_NetDevType  next_type;
    GxMsgProperty_CurAp cur_ap;

    if ( (s_network_ctrl.dev_total>1) && (app_network_wifi_get_all_dev_plug_count() > 1) )
    {
        GUI_GetProperty(s_network_opt[NET_ITEM_DEV], "select", &cur_sel);
        if ( key == STBK_RIGHT )
        {
            next_sel = (cur_sel + 1)%s_network_ctrl.dev_total ;
        }
        else
        {
            next_sel = (cur_sel + s_network_ctrl.dev_total - 1)%s_network_ctrl.dev_total ;
        }

        memset(&cur_type, 0, sizeof(GxMsgProperty_NetDevType));
        strcpy(cur_type.dev_name, s_network_ctrl.network_dev[cur_sel]);
        app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &cur_type);

        memset(&next_type, 0, sizeof(GxMsgProperty_NetDevType));
        strcpy(next_type.dev_name, s_network_ctrl.network_dev[next_sel]);
        app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &next_type);

        memset(&cur_state, 0, sizeof(GxMsgProperty_NetDevState));
        strcpy(cur_state.dev_name, s_network_ctrl.network_dev[cur_sel]);
        app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &cur_state);

        if( (cur_type.dev_type == IF_TYPE_WIFI) && (next_type.dev_type == IF_TYPE_WIFI) )
        {
            if ( (cur_state.dev_state > IF_STATE_IDLE) && (cur_state.dev_state != IF_STATE_REJ) )
            {
                _app_check_wifi_pop_dlg();
                return 0;
            }
            else if (cur_state.dev_state == IF_STATE_IDLE)
            {
                memset(&cur_ap, 0, sizeof(GxMsgProperty_CurAp));
                strcpy(cur_ap.wifi_dev, s_network_ctrl.network_dev[cur_sel]);
                app_send_msg_exec(GXMSG_WIFI_GET_CUR_AP, &cur_ap);
                if( (0 != strlen(cur_ap.ap_mac)) && (0 != strncmp(cur_ap.ap_mac, AP_MAC_DEFAULT, sizeof(AP_MAC_DEFAULT))) )
                {
                    _app_check_wifi_pop_dlg();
                    return 0 ;
                }
            }
        }
    }
    return 1 ;
}
#endif

SIGNAL_HANDLER int app_network_box_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int32_t box_sel = 0;

	event = (GUI_Event *)usrdata;
	GUI_GetProperty(BOX_NETWORK_OPTIONS, "select", &box_sel);
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_RIGHT:
			case STBK_LEFT:
				if(box_sel == NET_ITEM_TYPE)
				{
					if((s_network_ctrl.pop_flag == true)||(s_network_ctrl.pppoe_flag == true))
						return EVENT_TRANSFER_STOP;
#if (MOD_3G_SUPPORT > 0)
					if(s_network_ctrl.dev_type == IF_TYPE_3G)
					{
						s_network_ctrl.ip_type = NET_TYPE_DHCP;
						GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "select", &s_network_ctrl.ip_type);
						return  EVENT_TRANSFER_STOP;
					}
#ifdef LINUX_OS
					else if(s_network_ctrl.dev_type == IF_TYPE_ETH3G)
					{
						s_network_ctrl.ip_type = NET_TYPE_DHCP;
						GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "select", &s_network_ctrl.ip_type);
						return  EVENT_TRANSFER_STOP;
					}
#endif
#endif
				}
				break;
			case STBK_DOWN:
                if(s_network_ctrl.pop_flag == true)
                    return  EVENT_TRANSFER_STOP;
				if(box_sel == NET_ITEM_SAVE)
				{
					app_network_box_set_select(NET_ITEM_DEV);
					return  EVENT_TRANSFER_STOP;
				}
				break;
			case STBK_UP:
				if(s_network_ctrl.pop_flag == true)
                    return  EVENT_TRANSFER_STOP;
				if(box_sel == NET_ITEM_DEV)
				{
					app_network_box_set_select(NET_ITEM_SAVE);
					return  EVENT_TRANSFER_STOP;
				}
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;

}

SIGNAL_HANDLER int app_network_cmb_device_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_RIGHT:
			case STBK_LEFT:
#if (WIFI_SUPPORT > 0)
				if( _app_check_wifi_is_change_item(event->key.sym) == 0)
				{
					return EVENT_TRANSFER_STOP ;
				}
#endif
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_KEEPON;

}

SIGNAL_HANDLER int app_network_cmb_device_change(const char* widgetname, void *usrdata)
{
	int32_t dev_sel = 0;

    GUI_GetProperty(s_network_opt[NET_ITEM_DEV], "select", &dev_sel);
	s_network_ctrl.dev = dev_sel;
	if((s_network_ctrl.pop_flag == true) || (s_network_ctrl.dev_total == 1) || (s_network_ctrl.dev_total == 0))
		return EVENT_TRANSFER_STOP;

	app_network_dev_update();
	app_network_show_ip();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_cmb_mode_change(const char* widgetname, void *usrdata)
{
	int32_t mode_sel = 0;
	uint8_t i = 0;

	GUI_GetProperty(s_network_opt[NET_ITEM_MODE], "select", &mode_sel);
	s_network_ctrl.mode = mode_sel;
	if(s_network_ctrl.pop_flag == true || s_network_ctrl.dev_total == 0)
	    return EVENT_TRANSFER_STOP;

	if(0 == s_network_ctrl.dev_total)
    {
        GUI_SetProperty(s_network_boxitem[NET_ITEM_MODE], "disable", NULL);
    }
    else if(s_network_ctrl.dev_total > 0)
    {
        GUI_SetProperty(s_network_boxitem[NET_ITEM_MODE], "enable", NULL);
    }
	if(mode_sel == NET_MODE_OFF)
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_SAVE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
		}
		app_network_show_ip();
	}
	else
	{
        GUI_SetProperty(s_network_boxitem[NET_ITEM_TYPE], "enable", NULL);
        app_network_iptype_set();
		app_set_address_state_by_type(s_network_ctrl.ip_type);
    }
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_network_ip_address_reach_end(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_NETWORK_IP_ADDRESS_LEN 15
    GUI_GetProperty("edit_network_ip_address", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_NETWORK_IP_ADDRESS_LEN - 1 : 0);
    GUI_SetProperty("edit_network_ip_address", "select", &sel);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_network_mask_reach_end(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_NETWORK_MASK_LEN 15
    GUI_GetProperty("edit_network_mask", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_NETWORK_MASK_LEN - 1 : 0);
    GUI_SetProperty("edit_network_mask", "select", &sel);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_network_gate_way_reach_end(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_NETWORK_GATE_WAY_LEN 15
    GUI_GetProperty("edit_network_gate_way", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_NETWORK_GATE_WAY_LEN - 1 : 0);
    GUI_SetProperty("edit_network_gate_way", "select", &sel);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_network_dns1_reach_end(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_NETWORK_DNS1_LEN 15
    GUI_GetProperty("edit_network_dns1", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_NETWORK_DNS1_LEN - 1 : 0);
    GUI_SetProperty("edit_network_dns1", "select", &sel);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_network_dns2_reach_end(const char* widgetname, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_NETWORK_DNS2_LEN 15
    GUI_GetProperty("edit_network_dns2", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_NETWORK_DNS2_LEN - 1 : 0);
    GUI_SetProperty("edit_network_dns2", "select", &sel);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_cmb_type_change(const char* widgetname, void *usrdata)
{
	int32_t cmb_sel = 0;
    GUI_GetProperty(s_network_opt[NET_ITEM_TYPE], "select", &cmb_sel);
	s_network_ctrl.ip_type = cmb_sel;
	app_set_address_state_by_type(cmb_sel);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_btn_save_keypress(const char* widgetname, void *usrdata)
{
    NetWorkParamsError error_type = NO_ERROR;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
            case STBK_OK:
                if(s_network_ctrl.pop_flag == true)
                    return EVENT_TRANSFER_STOP;

                if((!app_network_check_config()) && (app_network_check_status()))
                    return EVENT_TRANSFER_STOP;
                if(s_network_ctrl.mode > NET_MODE_OFF)    //mode on
                {
                    if((s_network_ctrl.ip_type == 0) && (s_network_ctrl.pppoe_flag != true))  //only static ip
                    {
                        error_type = app_network_check_parameter();
                        if(error_type != NO_ERROR)
                        {
                            /*#389317 Static IP参数设置为错误的值后统一恢复*/
                            if (error_type == DNS1_ERROR)
                            {
                                GUI_SetProperty(s_network_opt[NET_ITEM_DNS1], "string", s_network_ctrl.ip_cfg.dns1);
                            }
                            else if(error_type == DNS2_ERROR)
                            {
                                GUI_SetProperty(s_network_opt[NET_ITEM_DNS2], "string", s_network_ctrl.ip_cfg.dns2);
                            }
                            else if(error_type == GATE_WAY_ERROR || error_type == SUB_MASK_ERROR || error_type == IP_ADDR_ERROR)
                            {
                                GUI_SetProperty(s_network_opt[NET_ITEM_GATEWAY], "string", s_network_ctrl.ip_cfg.gate_way);
                                GUI_SetProperty(s_network_opt[NET_ITEM_SUB_MASK], "string", s_network_ctrl.ip_cfg.net_mask);
                                GUI_SetProperty(s_network_opt[NET_ITEM_IPADDR], "string", s_network_ctrl.ip_cfg.ip_addr);
                            }
                            //这个组件和pop重合，和pop一起提交刷新，gui有可能最后刷新这个组件导致pop异常。
                            GUI_SetInterface("flush",NULL);
                            app_network_pop_info_show(app_get_network_para_err_tip(error_type));
                            if(0 != reset_timer(s_network_hide_pop_timer))
                            {
                                s_network_hide_pop_timer = create_timer(app_hide_pop_timer_cb, 1000, NULL, TIMER_REPEAT);
                            }
                            return EVENT_TRANSFER_STOP;
                        }
                    }
                }
                if(s_network_ctrl.dev_total > 0)
                {
                    app_network_set_config();
                    app_network_pop_info_show(STR_ID_WAITING);
                    if(0 != reset_timer(s_network_pop_timer))
                    {
                        s_network_pop_timer = create_timer(app_pop_info_timer_cb, 1000, NULL, TIMER_REPEAT);
                    }
                }
                break;
			case STBK_EXIT:
			case STBK_MENU:
                if(true == s_network_ctrl.pop_flag)
                {
                    if(s_network_error_timer != NULL)
                    {
                        app_pop_info_timer_invalid_cb(NULL);
                    }
                    else if(s_network_pop_timer != NULL)
                    {
                        app_pop_info_timer_cb(NULL);
                    }
                    else if (s_network_hide_pop_timer != NULL )
                    {
                        app_hide_pop_timer_cb(NULL);
                    }
                    return EVENT_TRANSFER_STOP;
                }
                return EVENT_TRANSFER_KEEPON;
            default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_create(const char* widgetname, void *usrdata)
{
	int i = 0;

    #ifdef ECOS_OS
	#if WIFI_SUPPORT
	s_wifi_set_connecting = false;
	#endif
	#endif

	app_retset_focus_forecolor();
	GUI_SetProperty(s_network_opt[NET_ITEM_MODE], "content", MODE_CONTENT);
	GUI_SetProperty(s_network_opt[NET_ITEM_TYPE], "content", TYPE_CONTENT);

	app_network_update();
    s_network_ctrl.dev = _network_get_current_dev();
    if(s_network_ctrl.dev_total > 0)
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "enable", NULL);
		}
		GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "select", &s_network_ctrl.dev);
		app_network_dev_update();
        app_network_show_ip();
    }
	else
	{
		for(i = NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
		}
	}
    #if VPN_SUPPORT
    GUI_SetProperty(IMG_SCAN_RED, "state", "show");
    GUI_SetProperty(TXT_SCAN_RED, "state", "show");
    #endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_destroy(const char* widgetname, void *usrdata)
{
    s_network_ctrl.pop_flag = false;
    #ifdef ECOS_OS
	#if WIFI_SUPPORT
	s_wifi_set_connecting = false;
	#endif
	#endif
    APP_TIMER_REMOVE(s_network_error_timer);
    APP_TIMER_REMOVE(s_network_pop_timer);
    APP_TIMER_REMOVE(s_network_hide_pop_timer);

	if(s_network_ctrl.network_dev != NULL)
	{
		GxCore_Free(s_network_ctrl.network_dev);
		s_network_ctrl.network_dev = NULL;
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_lost_focus(GuiWidget *widget, void *usrdata)
{
	app_set_lost_focus_forecolor();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_got_focus(GuiWidget *widget, void *usrdata)
{

    ubyte64 cur_dev = {0};
    uint32_t i = 0;
    uint32_t box_sel = 0;

	GUI_GetProperty(BOX_NETWORK_OPTIONS, "select", &box_sel);
	app_retset_focus_forecolor();
    if(s_network_ctrl.network_dev != NULL)
    {
        strcpy(cur_dev, s_network_ctrl.network_dev[s_network_ctrl.dev]);
    }
    app_network_update();
    if(s_network_ctrl.dev_total > 0)
    {
        s_network_ctrl.dev = 0;
		for(i = 0; i < s_network_ctrl.dev_total; i++)
        {
            if(strcmp(cur_dev, s_network_ctrl.network_dev[i]) == 0)
            {
                s_network_ctrl.dev = i;
                break;
            }
        }
		GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "select", &s_network_ctrl.dev);
        app_network_dev_update();
        app_network_show_ip();
#ifdef LINUX_OS
		if((i >= s_network_ctrl.dev_total)||(_network_is_item_invalid(box_sel)))
		{
			app_network_box_set_select(NET_ITEM_DEV);
		}
#else
		if(s_network_ctrl.mode == NET_MODE_OFF)
		{
			if(box_sel != NET_ITEM_MODE)
			{
				app_network_box_set_select(NET_ITEM_DEV);
			}
		}

		if((s_network_ctrl.ip_type == 1) || (s_network_ctrl.pppoe_flag == true))
		{
			if((box_sel > NET_ITEM_TYPE) && (box_sel < NET_ITEM_SAVE))
			{
				app_network_box_set_select(NET_ITEM_DEV);
			}
		}
        if(strcmp(cur_dev, s_network_ctrl.network_dev[s_network_ctrl.dev]) != 0)
        {
			app_network_box_set_select(NET_ITEM_DEV);
        }

#endif
    }
	else
	{
		for(i =  NET_ITEM_TYPE; i < NET_ITEM_STATE; i++)
		{
			GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
		}

		GUI_SetProperty(IMG_SCAN_GREEN, "state", "hide");
		GUI_SetProperty(TXT_SCAN_GREEN, "state", "hide");
		app_network_box_set_select(NET_ITEM_DEV);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_network_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
            case STBK_GREEN:
            {
                if(true == s_network_ctrl.pop_flag)
                  break;
                APP_TIMER_REMOVE(sp_GetIpCfg);
#if (WIFI_SUPPORT > 0)
                if(s_network_ctrl.dev_type == IF_TYPE_WIFI)
                {
                    ubyte64 *wifi_dev = &s_network_ctrl.network_dev[s_network_ctrl.dev];
                    app_create_wifi_menu(wifi_dev);
                }
#endif

#if (MOD_3G_SUPPORT > 0)
                if(s_network_ctrl.dev_type == IF_TYPE_3G)
                {
                    ubyte64 *_3g_dev = &s_network_ctrl.network_dev[s_network_ctrl.dev];
                    app_create_3g_menu(_3g_dev);
                }
#endif

#if (PPPOE_SUPPORT > 0)
                if(s_network_ctrl.dev_type == IF_TYPE_WIRED) //pppoe
                {
                    ubyte64 *pppoe_dev = &s_network_ctrl.network_dev[s_network_ctrl.dev];
                    app_create_pppoe_menu(pppoe_dev);
                }
#endif
            }
            break;
            case STBK_RED:
                #if VPN_SUPPORT
                if(true == s_network_ctrl.pop_flag)
                    break;
                app_vpn_menu_create();
                #endif
                break;
			case STBK_MENU:
			case STBK_EXIT:
				GUI_EndDialog(WND_NETWORK);
				break;
			case VK_BOOK_TRIGGER:
				GUI_EndDialog("after wnd_full_screen");
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

static void _network_plug_proc(ubyte64 *net_dev)
{

    uint8_t sel = 0;
    ubyte64 cur_dev = {0};
    uint32_t i = 0;

    if((net_dev == NULL))
        return;

	GUI_GetProperty(BOX_NETWORK_OPTIONS, "select", &sel);
    if(s_network_ctrl.network_dev != NULL)
    {
        strcpy(cur_dev, s_network_ctrl.network_dev[s_network_ctrl.dev]);
        if(strcmp((char*)net_dev, s_network_ctrl.network_dev[s_network_ctrl.dev]) != 0)
        {
            app_network_update();
            if(s_network_ctrl.dev_total > 0)
            {
                s_network_ctrl.dev = 0;
                for(i = 0; i < s_network_ctrl.dev_total; i++)
                {
                    if(strcmp(cur_dev, s_network_ctrl.network_dev[i]) == 0)
                    {
                        s_network_ctrl.dev = i;
                        break;
                    }
                }
                GUI_SetProperty(s_network_opt[NET_ITEM_DEV], "select", &s_network_ctrl.dev);
            }
            return;
        }
    }
    if(s_network_ctrl.pop_flag == true)
    {
        APP_TIMER_REMOVE(s_network_error_timer);
        APP_TIMER_REMOVE(s_network_pop_timer);
        APP_TIMER_REMOVE(s_network_hide_pop_timer);
        app_network_pop_info_hide();
    }
    app_network_update();
    if(s_network_ctrl.dev_total > 0)
    {
        s_network_ctrl.dev = 0;
        ubyte64 cur_dev_bak = {0};
        memset(&cur_dev_bak, 0, sizeof(ubyte64));
        app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev_bak);
        if((cur_dev_bak != NULL) && (strlen(cur_dev_bak) != 0))
        {
            s_network_ctrl.dev = _network_get_current_dev();
        }
        else
        {
            for(i = 0; i < s_network_ctrl.dev_total; i++)
            {
                if(strcmp(cur_dev, s_network_ctrl.network_dev[i]) == 0)
                {
                    s_network_ctrl.dev = i;
                    break;
                }
            }
        }

        GUI_SetProperty(s_network_boxitem[NET_ITEM_SAVE], "enable", NULL);
		GUI_GetProperty(s_network_opt[NET_ITEM_DEV], "select", &s_network_ctrl.dev);
        app_network_dev_update();
        app_network_show_ip();
#ifdef ECOS_OS
		if(s_network_ctrl.mode == 0)
		{
			if(sel >= NET_ITEM_TYPE && sel <= NET_ITEM_SAVE)
            {
				app_network_box_set_select(NET_ITEM_DEV);
			}
		}

		if((s_network_ctrl.ip_type == NET_TYPE_DHCP) || (s_network_ctrl.pppoe_flag == true))
		{
			if((sel > NET_ITEM_TYPE) && (sel < NET_ITEM_SAVE))
			{
				app_network_box_set_select(NET_ITEM_DEV);
			}
		}
#endif
#ifdef LINUX_OS
        if((i >= s_network_ctrl.dev_total)||(_network_is_item_invalid(sel)))
#endif
#ifdef ECOS_OS
        if(strcmp(cur_dev, s_network_ctrl.network_dev[s_network_ctrl.dev]) != 0)
#endif
        {
			app_network_box_set_select(NET_ITEM_DEV);
        }
    }
    else
    {
        for(i = NET_ITEM_MODE; i < NET_ITEM_STATE; i++)
        {
            GUI_SetProperty(s_network_boxitem[i], "disable", NULL);
        }
		app_network_box_set_select(NET_ITEM_DEV);
        GUI_SetProperty(IMG_SCAN_GREEN, "state", "hide");
        GUI_SetProperty(TXT_SCAN_GREEN, "state", "hide");
    }

	GUI_SetInterface("flush", 0); // 问题72292device会闪一下wifi

}

void app_network_msg_proc(GxMessage *msg)
{
    if(msg == NULL)
        return;

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_STATE_REPORT:
        {
            GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
            _network_state_msg_proc(dev_state);
            break;
        }

        case GXMSG_NETWORK_PLUG_IN:
        {
            ubyte64 *net_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            //app_log_info("~~~~~~[%s]%d, %s plug in!!!!!\n", __func__, __LINE__, *net_dev);
            _network_plug_proc(net_dev);
            break;
        }

        case GXMSG_NETWORK_PLUG_OUT:
        {
            ubyte64 *net_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            //app_log_info("~~~~~~[%s]%d, %s plug out!!!!!\n", __func__, __LINE__, *net_dev);
            _network_plug_proc(net_dev);
            break;
        }

        default:
            break;
    }
}

void app_network_menu_exec(void)
{
    GUI_CreateDialog(WND_NETWORK);
}

#endif //NETWORK_SUPPORT
