#include "app.h"
#include "app_msg.h"
#include "app_module.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#include "bus/service/gxusbwifiplug.h"
#include "gxusb3gplug.h"
#include "app_windows.h"
#if DVB_CLOUD_SUPPORT
#include "gx_dvbonline.h"
#endif
#if (BLUETOOTH_SUPPORT > 0)
#include "bus/service/gxusbbtplug.h"
#endif

#if NETWORK_SUPPORT
#ifdef ECOS_OS
#include <sys/socket.h>
#include <net/if.h>
#include <network.h>
#include <dhcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cyg/ns/dns/dns.h>
#if MOD_3G_SUPPORT
#include "gxusb3g.h"
#endif
#if GPRS_SUPPORT
#include "module/app_gprs.h"
#endif
#if FM_SUPPORT
#include "app_fm.h"
#endif
static ubyte32 s_static_ip_addr = {0};
extern int app_wifi_linkdown(char* dev_name);
#else
#if SAMBA_SUPPORT
#include "app_samba.h"
#endif
#include "netapps/app_netapps_service.h"
#endif

//#ifdef LINUX_OS
#define PATH_LEN  (100)
#define RESULT_LEN  (100)
#define CMD_LEN  (100)
#define CONSOLE_DELAY_MS   (800)
// LINK_CONSOLE_DELAY_MS must < CONSOLE_DELAY_MS
#define LINK_CONSOLE_DELAY_MS (500)

#define PROC_NET_DEV       "/proc/net/dev"
#define NETWORK_MONITOR_DIR "/tmp/ethernet/monitor/"
#define NETWORK_MANUAL_DIR "/tmp/ethernet/manual/"
#define PPPOE_FILE "/usr/share/app/ethernet/pppoe_cfg"

//CMD
#define WIRED_START_CMD "wired_start"
#define WIRED_STOP_CMD "wired_stop"
#define WIFI_INIT_CMD "wifi_init"
#define WIFI_START_CMD "wifi_start"
#define WIFI_STOP_CMD "wifi_stop"
#define WIFI_SCAN_CMD "wifi_scan"
#define WIFI_SAVE_AP_CMD "wifi_save_ap"
#define WIFI_DEL_AP_CMD "wifi_del_ap"
#define IF_SAVE_CONFIG_CMD "if_save_cfg"
#define IF_GET_CONFIG_CMD "if_get_cfg"
#define RM_FILE_CMD "rm -f"

//3G
#define _3G_OPERATOR_KEY    "3g>operator_id"
#define _3G_OPERATOR_DEFAULT    0

#define USB3G_START_CMD "3g_start"
#define USB3G_STOP_CMD "3g_stop"
#define ETH3G_START_CMD "eth3g_start"
#define ETH3G_STOP_CMD "eth3g_stop"
#define USB3G_SET_PARAM_CMD "tg_setParam"
#define USB3G_GET_PARAM_CMD "tg_getParam"

#define USB3G_DEV_HEAD	"USB3G"
#define GPRS_DEV_HEAD	"GPRS"

#define SAVE_PPPOE_CMD "pppoe_save_cfg"

enum
{
    MIN_PRIORITY = 0,
#if (BLUETOOTH_SUPPORT > 0)
    BLUETOOTH_PRIORITY = 10,
    WIRED_PRIORITY = 20,
    USB_WIRED_PRIORITY = 30,
    WIFI_PRIORITY = 40,
    TG_PRIORITY = 50,
    RNDIS_PRIORITY = 60,
#ifdef LINUX_OS
	MAX_PRIORITY = 70
#else
	GPRS_PRIORITY = 70,
    MAX_PRIORITY = 80,
#endif
#else
    WIRED_PRIORITY = 10,
    USB_WIRED_PRIORITY = 20,
    WIFI_PRIORITY = 30,
    TG_PRIORITY = 40,
    RNDIS_PRIORITY = 50,
#ifdef LINUX_OS
	MAX_PRIORITY = 60
#else
	GPRS_PRIORITY = 60,
    MAX_PRIORITY = 70,
#endif
#endif
};

typedef struct
{
    char dev_mode;
    char ip_type;
    ubyte32 ip_addr;
    ubyte32 net_mask;
    ubyte32 gate_way;
    ubyte32 dns1;
    ubyte32 dns2;
    ubyte32 mac;
}IpCfgFileData;

typedef struct
{
    ubyte64 essid;
    ubyte32 mac;
    ubyte32 psk;
    int32_t hidden;
}ApFileData;

typedef struct
{
    char mode;
    ubyte32 username;
    ubyte32 passwd;
    ubyte32 dns1;
    ubyte32 dns2;
}PppoeFileData;

typedef struct if_device IfDevice;
typedef struct
{
    status_t (*if_start)(IfDevice*);
    status_t (*if_stop)(IfDevice*);
    IfFileRet (*if_monitor_connect)(IfDevice*); //ret: 1,ok; -1,failed; 0,connecting
}IfOps;

struct if_device
{
    ubyte64 dev_name;
    IfState dev_state;
    IfType dev_type;
	int id;
    int dev_priority;
    bool user_flag;
    bool main_flag;
    bool cur_flag;
    bool scanning_flag;
    IfOps *dev_ops;
    handle_t start_thread_handle;
    handle_t scan_thread_handle;
    handle_t dev_mutex;
};

typedef struct if_device_list IfDevList;
struct if_device_list
{
    IfDevice net_dev;
    IfDevList *next;
};


#if ETH_SUPPORT
static status_t _wired_dev_start(IfDevice *wired_dev);
static status_t _wired_dev_stop(IfDevice *wired_dev);
static IfFileRet _wired_dev_connect_monitor(IfDevice *wired_dev);
#endif

#if USB_ETH_SUPPORT
//bool thiz_usbeth_hotplug_status = false;
static status_t _usb_wired_dev_start(IfDevice *wired_dev);
static status_t _usb_wired_dev_stop(IfDevice *wired_dev);
static IfFileRet _usb_wired_dev_connect_monitor(IfDevice *wired_dev);
#endif

#ifdef LINUX_OS
#else
#if (ETH_SUPPORT || USB_ETH_SUPPORT)
int wired_cable_link;//0:unlink 1:link
int usb_wired_cable_link;//0:unlink 1:link
//static int dhcp_ok;
#endif

#if WIFI_SUPPORT || USB_ETH_SUPPORT || ETH_SUPPORT
static IfFileRet s_network_status = IF_FILE_NONE;
#endif

#endif

#if (WIFI_SUPPORT > 0)
static int s_wifi_init_status = -1;
static int s_wifi_status = -1;
static int s_wifi_pre_status = -1;
#ifdef LINUX_OS
static int s_wifi_plug_change = 0 ;
#endif

static status_t _wifi_dev_start(IfDevice *wifi_dev);
static status_t _wifi_dev_stop(IfDevice *wifi_dev);
static IfFileRet _wifi_dev_connect_monitor(IfDevice *wifi_dev);

#define WIFI_MAX_DEV_NUM (4)

#define WIFI_MAX_DEV_MEMORY_NUM (4)

#if ( WIFI_MAX_DEV_MEMORY_NUM < WIFI_MAX_DEV_NUM )
error !!!!
#endif

#define APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
//#define APP_NET_WIFI_DEBUG_MSG_STATUS_SUPPORT
//#define APP_NET_WIFI_DEBUG_PROCESS_SUPPORT

#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
#define app_wifi_debug printf
#else
#define app_wifi_debug app_log_debug
#endif

#ifdef APP_NET_WIFI_DEBUG_MSG_STATUS_SUPPORT
#define app_wifi_msg_debug printf
#else
#define app_wifi_msg_debug app_log_debug
#endif

#ifdef APP_NET_WIFI_DEBUG_PROCESS_SUPPORT
#define app_wifi_info printf
#endif

#define GXCORE_NAME_NOT_MATCH_ERROR   (-10)

static int   s_wifi_dev_active_id = 0 ;
static int   s_wifi_dev_port[WIFI_MAX_DEV_MEMORY_NUM] = {0,0};
static int   s_wifi_dev_plug[WIFI_MAX_DEV_MEMORY_NUM] = {0,0};
static char  s_wifi_dev_name[WIFI_MAX_DEV_MEMORY_NUM][8] = {{0},{0}};

int app_network_wifi_check_dev_is_active(char *dev_name)
{
	int i = 0 ;
	if ( dev_name == NULL )
	{
		app_log_error("#### wifi check msg is active dev, null \n");
		return 0 ;
	}

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( strncmp(s_wifi_dev_name[i],dev_name,NET_IF_DEV_WIFI_NLEN) == 0 )
		{
			if ( s_wifi_dev_active_id == i )
			{
				return 1 ;
			}
			else
			{
				return 0 ;
			}
		}
	}
	return 0 ;
}

static int app_network_wifi_plug_out_to_switch_dev(char *dev_name)
{
	int i = 0 ;
	if ( dev_name == NULL )
	{
		app_log_error("#### wifi plug out switch dev, null \n");
		return 0;
	}

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( strncmp(s_wifi_dev_name[i],dev_name,NET_IF_DEV_WIFI_NLEN) == 0 )
		{
			continue ;
		}
		else
		{
			if ( (s_wifi_dev_plug[i]) && ( s_wifi_dev_active_id != i ))
			{
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
				app_wifi_debug("[WIFI] plug out switch dev, old=%d,now=%d \n",s_wifi_dev_active_id,i);
#endif
				s_wifi_dev_active_id = i ;
				s_wifi_status = 0 ;
				s_wifi_pre_status = -1;
			}
			return 1;
		}
	}
	return 0 ;
}

static void app_network_wifi_add_dev_name(int port,char *dev_name)
{
	int i = 0 ;
	int first = WIFI_MAX_DEV_NUM ;

	first = dev_name[NET_IF_DEV_WIFI_NLEN-1] - 0x30;

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( (s_wifi_dev_plug[i] == 0) && (i == first) )
		{
			memset(s_wifi_dev_name[i],0,8);
			memcpy(s_wifi_dev_name[i],dev_name,NET_IF_DEV_WIFI_NLEN);
			s_wifi_dev_plug[i] = 1 ;
			s_wifi_dev_port[i] = port ;
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
			app_wifi_debug("[WIFI] add = %d \n",i);
#endif
			return ;
		}
	}

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( s_wifi_dev_plug[i] == 0 )
		{
			memset(s_wifi_dev_name[i],0,8);
			memcpy(s_wifi_dev_name[i],dev_name,NET_IF_DEV_WIFI_NLEN);
			s_wifi_dev_plug[i] = 1 ;
			s_wifi_dev_port[i] = port ;
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
			app_wifi_debug("[WIFI] add2 = %d,%d \n",i,first);
#endif
			return ;
		}
	}

	first = 0 ;
	app_log_error("#### wifi add dev name, para:%s \n",dev_name);
	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( s_wifi_dev_port[i] == port )
		{
			s_wifi_dev_plug[i] = 0 ;
			app_log_error("#### wifi modify plug, force:%s \n",s_wifi_dev_name[i]);
			if ( s_wifi_dev_active_id == i )
			{
				first = 1 ;
			}
			break;
		}
	}
	if ( first )
	{
		for(i=0;i<WIFI_MAX_DEV_NUM;i++)
		{
			if ( s_wifi_dev_plug[i] )
			{
				app_log_error("#### wifi modify active, force:%d \n",i);
				s_wifi_dev_active_id = i ;
				break;
			}
		}
	}
}

static int app_network_wifi_del_dev_name(char *dev_name)
{
	int i = 0 ;
	if ( dev_name == NULL )
	{
		app_log_error("#### wifi del dev name, null \n");
		return 1;
	}

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( strncmp(s_wifi_dev_name[i],dev_name,NET_IF_DEV_WIFI_NLEN) == 0 )
		{
			s_wifi_dev_plug[i] = 0 ;
			break;
		}
	}

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( s_wifi_dev_plug[i] )
		{
			break;
		}
	}
	if ( i == WIFI_MAX_DEV_NUM )
	{
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
		app_wifi_debug("[WIFI] no dev \n");
#endif
		s_wifi_dev_active_id = 0 ;
	}
	return 1 ;
}

int app_network_wifi_check_dev_plug_status(char *dev_name)
{
	int i = 0 ;
	if ( dev_name == NULL )
	{
		app_log_error("#### wifi check dev plug, null \n");
		return 0;
	}

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( strncmp(s_wifi_dev_name[i],dev_name,NET_IF_DEV_WIFI_NLEN) == 0 )
		{
			return s_wifi_dev_plug[i] ;
		}
	}
	return 0 ;
}

static int app_network_wifi_get_dev_id(char *dev_name)
{
	int i = 0 ;
	if ( dev_name == NULL )
	{
		app_log_error("#### wifi get dev id, null \n");
		return 0;
	}

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( strncmp(s_wifi_dev_name[i],dev_name,NET_IF_DEV_WIFI_NLEN) == 0 )
		{
			return i ;
		}
	}
	return 0 ;
}

int app_network_wifi_get_all_dev_plug_count(void)
{
	int i = 0 ;
	int count = 0 ;

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( s_wifi_dev_plug[i] )
		{
			count++ ;
		}
	}
	return count ;
}

int app_network_wifi_switch_dev_active_by_name(char *dev_name)
{
	int i = 0 ;

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( (strncmp(s_wifi_dev_name[i],dev_name,NET_IF_DEV_WIFI_NLEN) == 0) &&
		     (s_wifi_dev_plug[i]))
		{
			if ( s_wifi_dev_active_id != i )
			{
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
				app_wifi_debug("[WIFI] switch dev, old=%d, now=%d \n",s_wifi_dev_active_id,i);
#endif
				s_wifi_dev_active_id = i ;
				return 1 ;
			}
			break;
		}
	}
	return 0 ;
}

#ifdef LINUX_OS
static int app_network_wifi_save_file_data(char *dev_name)
{

#define WIFI_CONNECT_FILE1 "/usr/share/app/ethernet/wifi_connect_record"
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = '\t';
    char *p = NULL;
    char *q = NULL;
    int len = 0 ;
    int rewrite = 0 ;

    if(dev_name == NULL)
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(WIFI_CONNECT_FILE1))
        return GXCORE_ERROR;

    if((ret_fp = fopen(WIFI_CONNECT_FILE1, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL)
    {
        p = ret_buf;
        q = strchr(p, seq_char);
        if((q != NULL) && (strncmp(dev_name, p, q-p-1) == 0))
        {
            if ( strncmp(dev_name, p, q-p) != 0 )
            {
                len = q-p-1 ;
                p[len] = dev_name[len] ;
                rewrite = 1 ;
            }
            break;
        }
    }
    fclose(ret_fp);
    if ( !rewrite )
        return 0 ;

    if((ret_fp = fopen(WIFI_CONNECT_FILE1, "w")) == NULL)
        return GXCORE_ERROR;

    fputs(ret_buf,ret_fp);
    fclose(ret_fp);

    return 1;
}
#endif

static int app_network_wifi_change_dev_name_file_date(char *dev_name)
{
	int i = 0 ;
	int count = 0 ;
	int id = 0 ;
	int check_id[WIFI_MAX_DEV_NUM];

	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( (s_wifi_dev_plug[i]))
		{
			check_id[count] = i ;
			count++ ;
		}
	}
	if ( (count >= 1) && (WIFI_MAX_DEV_NUM >1) )
	{
		for(i=0;i<count;i++)
		{
			id = check_id[i];
			if ( strncmp(dev_name,s_wifi_dev_name[id],NET_IF_DEV_WIFI_NLEN) == 0 )
			{
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
				app_wifi_debug("[WIFI] link name = %s \n",s_wifi_dev_name[id]);
#endif
#ifdef LINUX_OS
				app_network_wifi_save_file_data(s_wifi_dev_name[id]);
#else
				GxBus_ConfigSet(AP0_DEV_KEY, s_wifi_dev_name[id]);
#endif
				return 1;
			}
		}
		return -1;

	}
	return -2 ;
}

static void app_network_wifi_init_active_id(void)
{
	char ret_buf[RESULT_LEN] = {0};
	int i = 0 ;
	int count = 0 ;
	static int s_poweron_first = 0 ;

	count = app_network_wifi_get_all_dev_plug_count();

	if ( (count>0) && (s_poweron_first <=0) )
	{
		s_poweron_first = 1 ;
		count = app_network_wifi_get_all_dev_plug_count();
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
		app_wifi_debug("[WIFI] first nums = %d \n",count);
#endif
		if ( count > 1 )
		{
			GxBus_ConfigGet(AP0_DEV_KEY, ret_buf, sizeof(ret_buf), AP_DEV_DEFAULT);

			for(i=0;i<WIFI_MAX_DEV_NUM;i++)
			{
				if ( strncmp(s_wifi_dev_name[i],ret_buf,NET_IF_DEV_WIFI_NLEN) == 0 )
				{
					if ( s_wifi_dev_active_id != i )
					{
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
						app_wifi_debug("[WIFI] first, old=%d, now=%d \n",s_wifi_dev_active_id,i);
#endif
						s_wifi_dev_active_id = i ;
					}
					break;
				}
			}
		}
    }
}

#endif
#if (BLUETOOTH_SUPPORT > 0)
#define MAX_BT_NO_CONNECT_CNT 10
typedef struct {
    int count;
    int plug_id;
    int user_id;
    int init_st;
    int hotplug_status;
}UsbBluetoothHotPlugStatus;
static UsbBluetoothHotPlugStatus this_bt_device = {0,-1,-1,-1,-1};
BtApInfo thiz_bt_ap_list[MAX_AP_NUM];
handle_t s_bt_aplist_mutex = -1;
handle_t s_bt_display_mutex = -1;
handle_t s_bt_scan_mutex = -1;
static unsigned int s_bt_connect_failer_count = 0 ;
static unsigned int s_bt_connect_towait_count = 0 ;

static status_t _bluetooth_dev_start(IfDevice *bt_dev);
static status_t _bluetooth_dev_stop(IfDevice *bt_dev);
static IfFileRet _bluetooth_dev_connect_monitor(IfDevice *bt_dev);
extern int app_bluez_source_link_stop(void);
extern void app_bluetooth_msg_proc(GxMessage *msg);
extern int app_check_bluetooth_status(void);
extern status_t app_bluetoot_start(char *bt_dev,char *mac);
extern status_t app_get_bt_security(char *mac);
#endif

static int app_network_wifi_dev_name_strcmp(const char *dev_name)
{
#if (WIFI_SUPPORT > 0)
	int i = 0 ;
	if ( dev_name == NULL )
	{
		return -1;
	}
	for(i=0;i<WIFI_MAX_DEV_NUM;i++)
	{
		if ( strcmp(s_wifi_dev_name[i],dev_name) == 0 )
		{
			return 0 ;
		}
	}
	return 1 ;
#else
	return 1 ;
#endif
}

#if (MOD_3G_SUPPORT > 0)
bool thiz_usb3g_hotplug_status = false;
static status_t _3g_dev_start(IfDevice *_3g_dev);
static status_t _3g_dev_stop(IfDevice *_3g_dev);
static IfFileRet _3g_dev_connect_monitor(IfDevice *_3g_dev);
extern IfState usb3g_getstate(void);
extern int usb3g_disconnect(void);
#ifdef LINUX_OS
#define _3G_CONNNECT_RECORD "/usr/share/app/ethernet/3g_connect_record"
static status_t _eth3g_dev_start(IfDevice *_eth3g_dev);
static status_t _eth3g_dev_stop(IfDevice *_eth3g_dev);
static IfFileRet _eth3g_dev_connect_monitor(IfDevice *_eth3g_dev);
#else
#define _3G_CONNNECT_RECORD "/home/gx/3g_connect_record"
#endif
#endif

#if (RNDIS_SUPPORT > 0)
#ifdef LINUX_OS
static status_t _rndis_dev_start(IfDevice *rndis_dev);
static status_t _rndis_dev_stop(IfDevice *rndis_dev);
static IfFileRet _rndis_dev_connect_monitor(IfDevice *rndis_dev);
#endif
#endif

#if (GPRS_SUPPORT > 0)
#ifdef LINUX_OS
    //add linux gprs
#else
#define _GPRS_CONNNECT_RECORD "/home/gx/gprs_connect_record"
bool thiz_gprs_startup = false;
static status_t _gprs_dev_start(IfDevice *_gprs_dev);
static status_t _gprs_dev_stop(IfDevice *_gprs_dev);
static IfFileRet _gprs_dev_connect_monitor(IfDevice *_gprs_dev);
#endif
#endif

static IfOps s_net_ops_set[IF_TYPE_UNKOWN] =
{
#if ETH_SUPPORT
    {_wired_dev_start, _wired_dev_stop, _wired_dev_connect_monitor},
#endif
#if USB_ETH_SUPPORT
    {_usb_wired_dev_start, _usb_wired_dev_stop, _usb_wired_dev_connect_monitor},
#endif
#if (WIFI_SUPPORT > 0)
    {_wifi_dev_start,  _wifi_dev_stop,  _wifi_dev_connect_monitor},
#endif
#if (BLUETOOTH_SUPPORT > 0)
    {_bluetooth_dev_start,  _bluetooth_dev_stop,  _bluetooth_dev_connect_monitor},
#endif
#if (MOD_3G_SUPPORT > 0)
    {_3g_dev_start,    _3g_dev_stop,    _3g_dev_connect_monitor},
#ifdef LINUX_OS
    {_eth3g_dev_start,    _eth3g_dev_stop,    _eth3g_dev_connect_monitor},
#endif
#endif
#if (RNDIS_SUPPORT > 0)
#ifdef LINUX_OS
    {_rndis_dev_start,  _rndis_dev_stop,  _rndis_dev_connect_monitor},
#endif
#endif
#if (GPRS_SUPPORT > 0)
#ifdef LINUX_OS
#else
    {_gprs_dev_start,    _gprs_dev_stop,    _gprs_dev_connect_monitor},
#endif
#endif
};

static IfState s_global_net_state = IF_STATE_IDLE;
static IfDevList *if_dev_list = NULL;
static handle_t s_dev_list_mutex = -1;
static int s_dev_type_num[IF_TYPE_UNKOWN+1] = {0};
static struct
{
    int dev_num;
    ubyte64 net_dev[MAX_NET_DEV_NUM];
}s_prev_init_dev;

static int s_network_dev_priority = MAX_PRIORITY;
static bool s_network_service_enable = true;
static handle_t s_network_service_mutex = -1;
static IfDevice* _if_dev_list_get(const char *dev_name);
static bool _check_prev_init_dev(const char *dev_name);


#ifdef ECOS_OS
#if WIFI_SUPPORT || USB_ETH_SUPPORT || ETH_SUPPORT
void app_network_status_set(IfFileRet status)
{
    s_network_status = status;
}

IfFileRet app_network_status_get(void)
{
    return s_network_status;
}
#endif

static status_t _set_idns_server_data(const char *dev_name, IpCfgFileData *cfg_data)
{
	ubyte32 dns = {0};
    if(cfg_data == NULL)
        return GXCORE_ERROR;

    //dns1
#if (WIFI_SUPPORT > 0)
	if(app_network_wifi_dev_name_strcmp(dev_name) == 0)
		GxBus_ConfigGet(WIFI_DNS1_KEY, dns, sizeof(cfg_data->dns1), WIFI_DNS1_DEFALT);
#endif


#if (MOD_3G_SUPPORT > 0)
	if(strcmp("USB3G", dev_name) == 0)
		GxBus_ConfigGet(USB3G_DNS1_KEY, dns, sizeof(cfg_data->dns1), USB3G_DNS1_DEFALT);
#endif

#if (GPRS_SUPPORT > 0)
	if(strcmp(GPRS_DEV_HEAD, dev_name) == 0)
		GxBus_ConfigGet(GPRS_DNS1_KEY, dns, sizeof(cfg_data->dns1), GPRS_DNS1_DEFALT);
#endif

#if (ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH0, dev_name) == 0)
		GxBus_ConfigGet(WIRED_DNS1_KEY, dns, sizeof(cfg_data->dns1), WIRED_DNS1_DEFALT);
#endif
#if (USB_ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
		GxBus_ConfigGet(USBETH_DNS1_KEY, dns, sizeof(cfg_data->dns1), USBETH_DNS1_DEFALT);
#endif
    if(strncmp(cfg_data->dns1, dns,sizeof(cfg_data->dns1)) != 0)
    {
        cyg_dns_server_del(dns);
        cyg_dns_server_add(cfg_data->dns1);
    }

    //dns2
#if (WIFI_SUPPORT > 0)
	if(app_network_wifi_dev_name_strcmp(dev_name) == 0)
		GxBus_ConfigGet(WIFI_DNS2_KEY, dns, sizeof(cfg_data->dns2), WIFI_DNS2_DEFALT);
#endif

#if (MOD_3G_SUPPORT > 0)
	if(strcmp("USB3G", dev_name) == 0)
		GxBus_ConfigGet(USB3G_DNS2_KEY, dns, sizeof(cfg_data->dns2), USB3G_DNS2_DEFALT);
#endif

#if (GPRS_SUPPORT > 0)
	if(strcmp(GPRS_DEV_HEAD, dev_name) == 0)
		GxBus_ConfigGet(GPRS_DNS2_KEY, dns, sizeof(cfg_data->dns2), GPRS_DNS2_DEFALT);
#endif

#if(ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH0, dev_name) == 0)
		GxBus_ConfigGet(WIRED_DNS2_KEY, dns, sizeof(cfg_data->dns2), WIRED_DNS2_DEFALT);
#endif
#if(USB_ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
		GxBus_ConfigGet(USBETH_DNS2_KEY, dns, sizeof(cfg_data->dns2), USBETH_DNS2_DEFALT);
#endif
    if(strncmp(cfg_data->dns2, dns,sizeof(cfg_data->dns2)) != 0)
    {
        if(strncmp(cfg_data->dns1, dns,sizeof(cfg_data->dns1)) != 0)
        {
            cyg_dns_server_del(dns);
        }
        cyg_dns_server_add(cfg_data->dns2);
    }
    struct dns_timeout_info dns_info = {0};
    dns_info.second = 1;
    dns_info.retry_count = 1;
    cyg_dns_timeout_set(&dns_info);
    //    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
    return GXCORE_SUCCESS;
}
#endif


static status_t _set_ip_cfg_file_data(const char *dev_name, IpCfgFileData *cfg_data)
{
#ifdef LINUX_OS
    char cmd_buf[256] = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (cfg_data == NULL))
        return GXCORE_ERROR;

    sprintf(cmd_buf, "%s %s %d %d %s %s %s %s %s", IF_SAVE_CONFIG_CMD, dev_name, cfg_data->dev_mode,
            cfg_data->ip_type, cfg_data->ip_addr, cfg_data->net_mask, cfg_data->gate_way, cfg_data->dns1, cfg_data->dns2);
    //app_log_debug("-----[%s]%d %s-----\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);

    return GXCORE_SUCCESS;
#else
 if((dev_name == NULL) || (strlen(dev_name) == 0) || (cfg_data == NULL))
        return GXCORE_ERROR;

#if (ETH_SUPPORT > 0)
    //dev mode
	if(strcmp(NET_IF_DEV_ETH0, dev_name) == 0)
	{
		GxBus_ConfigSetInt(DEV_WIRED_MODE_KEY, cfg_data->dev_mode);
	}
#endif
#if (USB_ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
	{
		GxBus_ConfigSetInt(DEV_USBETH_MODE_KEY, cfg_data->dev_mode);
	}
#endif
#if (WIFI_SUPPORT > 0)
    if(app_network_wifi_dev_name_strcmp(dev_name) == 0)
    {
        GxBus_ConfigSetInt(DEV_WIFI_MODE_KEY, cfg_data->dev_mode);
    }
#endif
#if (BLUETOOTH_SUPPORT > 0)
    if(strcmp(net_if_dev_usb_bt, dev_name) == 0)
    {
        GxBus_ConfigSetInt(DEV_BT_MODE_KEY, cfg_data->dev_mode);
    }
#endif

#if (MOD_3G_SUPPORT > 0)
    if(strcmp("USB3G", dev_name) == 0)
    {
        GxBus_ConfigSetInt(DEV_3G_MODE_KEY, cfg_data->dev_mode);
    }
#endif
#if (GPRS_SUPPORT > 0)
    if(strcmp(GPRS_DEV_HEAD, dev_name) == 0)
    {
        GxBus_ConfigSetInt(DEV_GPRS_MODE_KEY, cfg_data->dev_mode);
    }
#endif
//    app_log_debug("~~~[%s]%d dev_mode: %d~~~\n", __func__, __LINE__, cfg_data->dev_mode);
	_set_idns_server_data(dev_name, cfg_data);

#if (ETH_SUPPORT > 0)
    //dev mode
	if(strcmp(NET_IF_DEV_ETH0, dev_name) == 0)
	{
		GxBus_ConfigSetInt(IP_TYPE_WIRED_KEY, cfg_data->ip_type);
	}
#endif
#if (USB_ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
	{
		GxBus_ConfigSetInt(IP_TYPE_USBETH_KEY, cfg_data->ip_type);
	}
#endif
#if (WIFI_SUPPORT > 0)
    if(app_network_wifi_dev_name_strcmp(dev_name) == 0)
    {
        GxBus_ConfigSetInt(IP_TYPE_WIFI_KEY, cfg_data->ip_type);
    }
#endif

#if (MOD_3G_SUPPORT > 0)
    if(strcmp("USB3G", dev_name) == 0)
    {
        GxBus_ConfigSetInt(IP_TYPE_3G_KEY, cfg_data->ip_type);
    }
#endif

#if (GPRS_SUPPORT > 0)
    if(strcmp(GPRS_DEV_HEAD, dev_name) == 0)
    {
        GxBus_ConfigSetInt(IP_TYPE_GPRS_KEY, cfg_data->ip_type);
    }
#endif

//    app_log_debug("~~~[%s]%d ip_type: %d~~~\n", __func__, __LINE__,  cfg_data->ip_type);

#if (WIFI_SUPPORT > 0)
	if(app_network_wifi_dev_name_strcmp(dev_name) == 0)
	{
		//ip addr
		GxBus_ConfigSet(WIFI_IP_ADDR_KEY, cfg_data->ip_addr);
		//    app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

		//net mask
		GxBus_ConfigSet(WIFI_NET_MASK_KEY, cfg_data->net_mask);
		//    app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

		//gate way
		GxBus_ConfigSet(WIFI_GATE_WAY_KEY, cfg_data->gate_way);
		//    app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

		//dns1
		GxBus_ConfigSet(WIFI_DNS1_KEY, cfg_data->dns1);
		//    app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

		//dns2
		GxBus_ConfigSet(WIFI_DNS2_KEY, cfg_data->dns2);
		//    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
	}
#endif

#if (MOD_3G_SUPPORT > 0)
	if(strcmp("USB3G", dev_name) == 0)
	{
		//ip addr
		GxBus_ConfigSet(USB3G_IP_ADDR_KEY, cfg_data->ip_addr);
		//    app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

		//net mask
		GxBus_ConfigSet(USB3G_NET_MASK_KEY, cfg_data->net_mask);
		//    app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

		//gate way
		GxBus_ConfigSet(USB3G_GATE_WAY_KEY, cfg_data->gate_way);
		//    app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

		//dns1
		GxBus_ConfigSet(USB3G_DNS1_KEY, cfg_data->dns1);
		//    app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

		//dns2
		GxBus_ConfigSet(USB3G_DNS2_KEY, cfg_data->dns2);
		//    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
	}
#endif

#if (GPRS_SUPPORT > 0)
	if(strcmp(GPRS_DEV_HEAD, dev_name) == 0)
	{
		//ip addr
		GxBus_ConfigSet(GPRS_IP_ADDR_KEY, cfg_data->ip_addr);
		//    app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

		//net mask
		GxBus_ConfigSet(GPRS_NET_MASK_KEY, cfg_data->net_mask);
		//    app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

		//gate way
		GxBus_ConfigSet(GPRS_GATE_WAY_KEY, cfg_data->gate_way);
		//    app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

		//dns1
		GxBus_ConfigSet(GPRS_DNS1_KEY, cfg_data->dns1);
		//    app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

		//dns2
		GxBus_ConfigSet(GPRS_DNS2_KEY, cfg_data->dns2);
		//    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
	}
#endif

#if (ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH0, dev_name) == 0)
	{
		//ip addr
		GxBus_ConfigSet(WIRED_IP_ADDR_KEY, cfg_data->ip_addr);
		//    app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

		//net mask
		GxBus_ConfigSet(WIRED_NET_MASK_KEY, cfg_data->net_mask);
		//    app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

		//gate way
		GxBus_ConfigSet(WIRED_GATE_WAY_KEY, cfg_data->gate_way);
		//    app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

		//dns1
		GxBus_ConfigSet(WIRED_DNS1_KEY, cfg_data->dns1);
		//    app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

		//dns2
		GxBus_ConfigSet(WIRED_DNS2_KEY, cfg_data->dns2);
		//    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
	}
#endif
#if (USB_ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
	{
	    GxBus_ConfigSet(USBETH_IP_ADDR_KEY, cfg_data->ip_addr);
	    GxBus_ConfigSet(USBETH_NET_MASK_KEY, cfg_data->net_mask);
	    GxBus_ConfigSet(USBETH_GATE_WAY_KEY, cfg_data->gate_way);
	    GxBus_ConfigSet(USBETH_DNS1_KEY, cfg_data->dns1);
	    GxBus_ConfigSet(USBETH_DNS2_KEY, cfg_data->dns2);
	}
#endif
    return GXCORE_SUCCESS;
   #endif
}

static status_t _get_ip_cfg_file_data(const char *dev_name, IpCfgFileData *cfg_data)
{
#ifdef LINUX_OS
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char result_path[PATH_LEN] = {0};
    char cmd_buf[CMD_LEN] = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (cfg_data == NULL))
        return GXCORE_ERROR;

    sprintf(result_path, "%s%s%s", NETWORK_MANUAL_DIR, dev_name, "_config");
    sprintf(cmd_buf, "%s %s %s", IF_GET_CONFIG_CMD, result_path, dev_name);
    //app_log_debug("!!!!!!!!!!----- [%s]%d %s ----- !!!!!!!!!!\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(result_path))
    {
        return GXCORE_ERROR;
    }

    if((ret_fp = fopen(result_path, "r")) == NULL)
    {
        return GXCORE_ERROR;
    }

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->mac, ret_buf);
    //app_log_debug("~~~[%s]%d mac: %s~~~\n", __func__, __LINE__, cfg_data->mac);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    cfg_data->dev_mode = atoi(ret_buf);
    //app_log_debug("~~~[%s]%d mac: %s~~~\n", __func__, __LINE__, cfg_data->mac);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    cfg_data->ip_type = atoi(ret_buf);
    //app_log_debug("~~~[%s]%d ip_type: %d~~~\n", __func__, __LINE__,  cfg_data->ip_type);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->ip_addr, ret_buf);
    //app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->net_mask, ret_buf);
    //app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->gate_way, ret_buf);
    //app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->dns1, ret_buf);
    //app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

    memset(ret_buf, 0, sizeof(ret_buf));
    fgets(ret_buf, sizeof(ret_buf), ret_fp);
    if(ret_buf[strlen(ret_buf) -1] == '\n')
		ret_buf[strlen(ret_buf) -1] = 0;
    strcpy(cfg_data->dns2, ret_buf);
    //app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);

    fclose(ret_fp);

	memset(cmd_buf, 0, CMD_LEN);
    sprintf(cmd_buf, "%s %s", RM_FILE_CMD, result_path);
    system(cmd_buf);

#else
	int32_t init_value = 0;

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (cfg_data == NULL))
        return GXCORE_ERROR;

    //mac
   // app_get_wifi_mac(cfg_data->mac);
//    app_log_debug("~~~[%s]%d mac: %s~~~\n", __func__, __LINE__, cfg_data->mac);

    //dev mode
	#if (WIFI_SUPPORT > 0)
    if(app_network_wifi_dev_name_strcmp(dev_name) == 0)
        GxBus_ConfigGetInt(DEV_WIFI_MODE_KEY, &init_value, DEV_MODE_DEFALT);
	#endif
	#if (BLUETOOTH_SUPPORT > 0)
    if(strcmp(net_if_dev_usb_bt, dev_name) == 0)
        GxBus_ConfigGetInt(DEV_BT_MODE_KEY, &init_value, DEV_MODE_DEFALT);
	#endif

	#if (MOD_3G_SUPPORT > 0)
    if(strcmp("USB3G", dev_name) == 0)
        GxBus_ConfigGetInt(DEV_3G_MODE_KEY, &init_value, DEV_MODE_DEFALT);
	#endif
	#if (ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH0, dev_name) == 0)
		GxBus_ConfigGetInt(DEV_WIRED_MODE_KEY, &init_value, DEV_MODE_DEFALT);
	#endif
#if (USB_ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
	{
		GxBus_ConfigGetInt(DEV_USBETH_MODE_KEY, &init_value, DEV_MODE_DEFALT);
	}
#endif
#if (GPRS_SUPPORT > 0)
    if(strcmp(GPRS_DEV_HEAD, dev_name) == 0)
        GxBus_ConfigGetInt(DEV_GPRS_MODE_KEY, &init_value, DEV_MODE_DEFALT);
#endif
//	GxBus_ConfigGetInt(DEV_MODE_KEY, &init_value, DEV_MODE_DEFALT);
    cfg_data->dev_mode = init_value;
//    app_log_debug("~~~[%s]%d dev_mode: %d~~~\n", __func__, __LINE__, cfg_data->dev_mode);

    //ip type
	#if (WIFI_SUPPORT > 0)
	if(app_network_wifi_dev_name_strcmp(dev_name) == 0)
        GxBus_ConfigGetInt(IP_TYPE_WIFI_KEY, &init_value, IP_TYPE_DEFALT);
	#endif
	#if (MOD_3G_SUPPORT > 0)
    if(strcmp("USB3G", dev_name) == 0)
        GxBus_ConfigGetInt(IP_TYPE_3G_KEY, &init_value, IP_TYPE_DEFALT);
	#endif
	#if (GPRS_SUPPORT > 0)
    if(strcmp(GPRS_DEV_HEAD, dev_name) == 0)
        GxBus_ConfigGetInt(IP_TYPE_GPRS_KEY, &init_value, IP_TYPE_DEFALT);
	#endif
	#if (ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH0, dev_name) == 0)
		GxBus_ConfigGetInt(IP_TYPE_WIRED_KEY, &init_value, IP_TYPE_DEFALT);
	#endif
    #if (USB_ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
		GxBus_ConfigGetInt(IP_TYPE_USBETH_KEY, &init_value, IP_TYPE_DEFALT);
	#endif

    cfg_data->ip_type = init_value;
//    app_log_debug("~~~[%s]%d ip_type: %d~~~\n", __func__, __LINE__,  cfg_data->ip_type);

#if (WIFI_SUPPORT > 0)
	if(app_network_wifi_dev_name_strcmp(dev_name) == 0)
	{
		//ip addr
		GxBus_ConfigGet(WIFI_IP_ADDR_KEY, cfg_data->ip_addr, sizeof(cfg_data->ip_addr), WIFI_IP_ADDR_DEFALT);
		//    app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

		//net mask
		GxBus_ConfigGet(WIFI_NET_MASK_KEY, cfg_data->net_mask, sizeof(cfg_data->net_mask), WIFI_NET_MASK_DEFALT);
		//    app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

		//gate way
		GxBus_ConfigGet(WIFI_GATE_WAY_KEY, cfg_data->gate_way, sizeof(cfg_data->gate_way), WIFI_GATE_WAY_DEFALT);
		//    app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

		//dns1
		GxBus_ConfigGet(WIFI_DNS1_KEY, cfg_data->dns1, sizeof(cfg_data->dns1), WIFI_DNS1_DEFALT);
		//    app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

		//dns2
		GxBus_ConfigGet(WIFI_DNS2_KEY, cfg_data->dns2, sizeof(cfg_data->dns2), WIFI_DNS2_DEFALT);
		//    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
	}
#endif

#if (MOD_3G_SUPPORT > 0)
	if(strcmp("USB3G", dev_name) == 0)
	{
		//ip addr
		GxBus_ConfigGet(USB3G_IP_ADDR_KEY, cfg_data->ip_addr, sizeof(cfg_data->ip_addr), USB3G_IP_ADDR_DEFALT);
		//    app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

		//net mask
		GxBus_ConfigGet(USB3G_NET_MASK_KEY, cfg_data->net_mask, sizeof(cfg_data->net_mask), USB3G_NET_MASK_DEFALT);
		//    app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

		//gate way
		GxBus_ConfigGet(USB3G_GATE_WAY_KEY, cfg_data->gate_way, sizeof(cfg_data->gate_way), USB3G_GATE_WAY_DEFALT);
		//    app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

		//dns1
		GxBus_ConfigGet(USB3G_DNS1_KEY, cfg_data->dns1, sizeof(cfg_data->dns1), USB3G_DNS1_DEFALT);
		//    app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

		//dns2
		GxBus_ConfigGet(USB3G_DNS2_KEY, cfg_data->dns2, sizeof(cfg_data->dns2), USB3G_DNS2_DEFALT);
		//    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
	}
#endif

#if (ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH0, dev_name) == 0)
	{
		//ip addr
		GxBus_ConfigGet(WIRED_IP_ADDR_KEY, cfg_data->ip_addr, sizeof(cfg_data->ip_addr), WIRED_IP_ADDR_DEFALT);
		//    app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

		//net mask
		GxBus_ConfigGet(WIRED_NET_MASK_KEY, cfg_data->net_mask, sizeof(cfg_data->net_mask), WIRED_NET_MASK_DEFALT);
		//    app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

		//gate way
		GxBus_ConfigGet(WIRED_GATE_WAY_KEY, cfg_data->gate_way, sizeof(cfg_data->gate_way), WIRED_GATE_WAY_DEFALT);
		//    app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

		//dns1
		GxBus_ConfigGet(WIRED_DNS1_KEY, cfg_data->dns1, sizeof(cfg_data->dns1), WIRED_DNS1_DEFALT);
		//    app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

		//dns2
		GxBus_ConfigGet(WIRED_DNS2_KEY, cfg_data->dns2, sizeof(cfg_data->dns2), WIRED_DNS2_DEFALT);
	}
#endif
#if (USB_ETH_SUPPORT > 0)
	if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
	{
		GxBus_ConfigGet(USBETH_IP_ADDR_KEY, cfg_data->ip_addr, sizeof(cfg_data->ip_addr), USBETH_IP_ADDR_DEFALT);
		GxBus_ConfigGet(USBETH_NET_MASK_KEY, cfg_data->net_mask, sizeof(cfg_data->net_mask), USBETH_NET_MASK_DEFALT);
		GxBus_ConfigGet(USBETH_GATE_WAY_KEY, cfg_data->gate_way, sizeof(cfg_data->gate_way), USBETH_GATE_WAY_DEFALT);
		GxBus_ConfigGet(USBETH_DNS1_KEY, cfg_data->dns1, sizeof(cfg_data->dns1), USBETH_DNS1_DEFALT);
		GxBus_ConfigGet(USBETH_DNS2_KEY, cfg_data->dns2, sizeof(cfg_data->dns2), USBETH_DNS2_DEFALT);
		//    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
	}
#endif

#if (GPRS_SUPPORT > 0)
	if(strcmp(GPRS_DEV_HEAD, dev_name) == 0)
	{
		//ip addr
		GxBus_ConfigGet(GPRS_IP_ADDR_KEY, cfg_data->ip_addr, sizeof(cfg_data->ip_addr), GPRS_IP_ADDR_DEFALT);
		//    app_log_debug("~~~[%s]%d ip_addr: %s~~~\n", __func__, __LINE__, cfg_data->ip_addr);

		//net mask
		GxBus_ConfigGet(GPRS_NET_MASK_KEY, cfg_data->net_mask, sizeof(cfg_data->net_mask), GPRS_NET_MASK_DEFALT);
		//    app_log_debug("~~~[%s]%d net_mask: %s~~~\n", __func__, __LINE__, cfg_data->net_mask);

		//gate way
		GxBus_ConfigGet(GPRS_GATE_WAY_KEY, cfg_data->gate_way, sizeof(cfg_data->gate_way), GPRS_GATE_WAY_DEFALT);
		//    app_log_debug("~~~[%s]%d gate_way: %s~~~\n", __func__, __LINE__, cfg_data->gate_way);

		//dns1
		GxBus_ConfigGet(GPRS_DNS1_KEY, cfg_data->dns1, sizeof(cfg_data->dns1), GPRS_DNS1_DEFALT);
		//    app_log_debug("~~~[%s]%d dns1: %s~~~\n", __func__, __LINE__, cfg_data->dns1);

		//dns2
		GxBus_ConfigGet(GPRS_DNS2_KEY, cfg_data->dns2, sizeof(cfg_data->dns2), GPRS_DNS2_DEFALT);
		//    app_log_debug("~~~[%s]%d dns2: %s~~~\n", __func__, __LINE__, cfg_data->dns2);
	}
#endif
#endif

    return GXCORE_SUCCESS;
}

static status_t _network_get_mac_cfg(const char *dev, char* mac)
{
#ifdef LINUX_OS
#define NET_DEV_MAC_DIR "/usr/share/app/ethernet/"
	//int len = 0;
	char buf[128];
    char result_path[PATH_LEN] = {0};
	FILE * fp = NULL;

	if((dev == NULL ) || (mac == NULL))
	{
		app_log_error("\nNET, wrong parameter\n");
		return GXCORE_ERROR;
	}

    sprintf(result_path, "%s%s%s", NET_DEV_MAC_DIR, dev, "_mac");
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(result_path))
    {
        return GXCORE_ERROR;
    }

	fp = fopen(result_path, "r");
	if(fp ==NULL)
	{
		app_log_error("\nNET, open file failed\n");
		return GXCORE_ERROR;
	}

	memset(buf, 0, 128);
	if(fgets(buf, 32, fp) == NULL)
    {
        fclose(fp);
        return GXCORE_ERROR;
    }

    if(buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = 0;

	fclose(fp);

	strcpy(mac, buf);
	app_log_debug("\nNET, GET MAC = %s\n", mac);

    return GXCORE_SUCCESS;
#else

	if((dev == NULL ) || (mac == NULL))
	{
		app_log_error("\nNET, wrong parameter\n");
		return GXCORE_ERROR;
	}

   // app_get_wifi_mac(mac);
	app_log_debug("\nNET, GET MAC = %s\n", mac);

    return GXCORE_SUCCESS;
#endif
}

static status_t _network_set_mac(const char *dev, const char *mac)
{// pMac = 00:AA:BB:CC:DD:44
#ifdef LINUX_OS
#define NET_DEV_MAC_DIR "/usr/share/app/ethernet/"
	FILE * fp = NULL;
    char cmd_buf[CMD_LEN] = {0};
    char result_path[PATH_LEN] = {0};

	if((dev == NULL) || (mac == NULL))
	{
		app_log_error("\nNET, wrong MAC\n");
		return GXCORE_ERROR;
	}

    sprintf(cmd_buf, "ifconfig %s hw ether %s", dev, mac);
    //app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    system(cmd_buf);

    sprintf(result_path, "%s%s%s", NET_DEV_MAC_DIR, dev, "_mac");
	// flush the record file
	fp = fopen(result_path, "wb");
	if(fp ==NULL)
	{
		app_log_error("\nNET, open file failed\n");
		return GXCORE_ERROR;
	}
	memset(cmd_buf, 0, CMD_LEN);
	sprintf(cmd_buf, "%s", mac);
	if(fwrite(cmd_buf, strlen(cmd_buf), 1, fp) != 1)
	{
		app_log_error("\nNET, write failed!\n");
		fclose(fp);
		return GXCORE_ERROR;
	}
    fflush(fp);
    fsync(fileno(fp));
	fclose(fp);

    return GXCORE_SUCCESS;
#else
//TODO:config_macaddr

    return GXCORE_SUCCESS;
#endif
}

#ifdef LINUX_OS
static IfFileRet _check_dev_result_file(IfState if_state, char *dev_name)
{
    IfFileRet ret = -1;
    char ret_buf[RESULT_LEN] = {0};
    FILE *ret_fp = NULL;
    char result_path[PATH_LEN] = {0};

    if(dev_name == NULL)
        return -1;

    sprintf(result_path, "%s%s%c%s", NETWORK_MONITOR_DIR, dev_name, '/', "state");
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(result_path))
    {
        return IF_FILE_NONE;
    }
    if((ret_fp = fopen(result_path, "r")) == NULL)
    {
        return IF_FILE_NONE;
    }

    fgets(ret_buf, RESULT_LEN - 1, ret_fp);

    if(strncmp(ret_buf, "ok", 2) == 0)
        ret = IF_FILE_OK;
    else if(strncmp(ret_buf, "start", 5) == 0)
        ret = IF_FILE_START;
    else if(strncmp(ret_buf, "wait", 4) == 0)
        ret = IF_FILE_WAIT;
    else if(strncmp(ret_buf, "fail", 4) == 0)
        ret = IF_FILE_FAIL;
    else if(strncmp(ret_buf, "reject", 6) == 0)
        ret = IF_FILE_REJ;
    else
    {
        app_log_debug("ret_bufret_bufret_buf %s\n", ret_buf);
        ret = IF_FILE_NONE;
    }

    fclose(ret_fp);

    return ret;
}

static void _net_format_shell_str(char *src, char *dest)
{
	int i = 0;
    char *temp = NULL;

    if(src && dest)
    {
        temp = dest;
        for(i = 0; i < strlen(src); i++)
        {
            if('"' == *(src + i) || '`' == *(src + i)|| '$' == *(src + i))
            {
                *temp++ = '\\';
            }
            *temp++ = *(src + i);
        }
    }

    return;
}
#endif

#ifdef ECOS_OS
#if ((ETH_SUPPORT > 0)||(USB_ETH_SUPPORT > 0))
static bool net_eth_check_is_dhcp(char *dev_name)
{
    int32_t init_value;
    if(strcmp(NET_IF_DEV_ETH1, dev_name) == 0)
    {
        GxBus_ConfigGetInt(IP_TYPE_USBETH_KEY, &init_value, IP_TYPE_DEFALT);
    }
    else
    {
        GxBus_ConfigGetInt(IP_TYPE_WIRED_KEY, &init_value, IP_TYPE_DEFALT);
    }
    return (init_value == 1) ? true : false;
}
static IfFileRet _check_wired_result_file(IfState if_state, char *dev_name)
{
	IfFileRet ret = -1;
    int cable_link = 0;

	if(dev_name == NULL)
		return -1;

    ret = app_network_status_get();

	if((dev_name != NULL)&&(strcmp(NET_IF_DEV_ETH0, dev_name) == 0))
    {
        cable_link = wired_cable_link;
    }
	if((dev_name != NULL)&&(strcmp(NET_IF_DEV_ETH1, dev_name) == 0))
    {
        cable_link = usb_wired_cable_link;
    }
	if((cable_link == 1)
		&& (ret != IF_FILE_OK))
	{
		extern int net_deladdr(const char *if_name, char *ip);

		//clear last net config
        net_deladdr(dev_name, (char *)s_static_ip_addr);
        network_down(dev_name);
		if(net_eth_check_is_dhcp(dev_name))
		{
			int dhcp_status;

			network_init(dev_name);
			dhcp_status = network_dhcp(dev_name);
			if(dhcp_status == 0)
			{
				//dhcp_ok = 1;
				IpCfgFileData cfg_data;
				unsigned int ip, mask;
				unsigned char mac[15];
				struct in_addr gateway,dnsA,dnsB;
				memset(&cfg_data, 0, sizeof(cfg_data));

				cfg_data.dev_mode = 1;
				cfg_data.ip_type = 1;
				getip(dev_name, &ip, &mask, mac);
				getdns(dev_name, &gateway, &dnsA, &dnsB);
				struct in_addr sin;
				sin.s_addr=ip;
				strcpy(cfg_data.ip_addr, inet_ntoa(sin));
				sin.s_addr=mask;
				strcpy(cfg_data.net_mask, inet_ntoa(sin));
				sin=gateway;
				strcpy(cfg_data.gate_way, inet_ntoa(sin));
				sin=dnsA;
				strcpy(cfg_data.dns1, inet_ntoa(sin));
				sin=dnsB;
				strcpy(cfg_data.dns2, inet_ntoa(sin));

				_set_ip_cfg_file_data(dev_name, &cfg_data);
				memset(s_static_ip_addr, 0, sizeof(cfg_data.ip_addr));
                app_network_status_set(IF_FILE_OK);
				ret = IF_FILE_OK;
			}
			else
			{
                app_network_status_set(IF_FILE_REJ);
				ret = IF_FILE_REJ;
			}
		}
		else
		{
            IpCfgFileData cfg_data;
            struct bootp bootp_data;
            struct in_addr addr;


            app_log_info("\033[33mDo ----STATIC ... !!!\n\033[0m");

            memset(&cfg_data, 0, sizeof(IpCfgFileData));
            if(_get_ip_cfg_file_data(dev_name, &cfg_data) == GXCORE_SUCCESS)
            {
                uint32_t ip[4] = {0};
                uint32_t mask[4] = {0};
                uint32_t gateway[4] = {0};
                uint32_t dns[4] = {0};
                unsigned char broadcast[4] = {0};
                char tmp[20] = {0};

                sscanf(cfg_data.ip_addr, "%d.%d.%d.%d", &(ip[0]), &(ip[1]), &(ip[2]), &(ip[3]));
                sscanf(cfg_data.net_mask, "%d.%d.%d.%d", &(mask[0]), &(mask[1]), &(mask[2]), &(mask[3]));
                sscanf(cfg_data.gate_way, "%d.%d.%d.%d", &(gateway[0]), &(gateway[1]), &(gateway[2]), &(gateway[3]));
                sscanf(cfg_data.dns1, "%d.%d.%d.%d", &(dns[0]), &(dns[1]), &(dns[2]), &(dns[3]));

                broadcast[0] = ((unsigned char)ip[0] & (unsigned char)mask[0]) | (unsigned char)(~(mask[0]));
                broadcast[1] = ((unsigned char)ip[1] & (unsigned char)mask[1]) | (unsigned char)(~(mask[1]));
                broadcast[2] = ((unsigned char)ip[2] & (unsigned char)mask[2]) | (unsigned char)(~(mask[2]));
                broadcast[3] = ((unsigned char)ip[3] & (unsigned char)mask[3]) | (unsigned char)(~(mask[3]));

                sprintf(cfg_data.ip_addr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
                sprintf(cfg_data.net_mask, "%d.%d.%d.%d", mask[0], mask[1], mask[2], mask[3]);
                sprintf(cfg_data.gate_way, "%d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3]);
                sprintf(cfg_data.dns1, "%d.%d.%d.%d", dns[0], dns[1], dns[2], dns[3]);
                sprintf(tmp, "%d.%d.%d.%d", broadcast[0], broadcast[1], broadcast[2], broadcast[3]);
                app_log_debug("BroadCast Addr : %s\n", tmp);

                build_bootp_record(&bootp_data,
                        dev_name,
                        cfg_data.ip_addr,
                        cfg_data.net_mask,
                        tmp,
                        cfg_data.gate_way,
                        cfg_data.gate_way);
                if(network_static(dev_name, &bootp_data))
                {
                    net_deladdr(dev_name, cfg_data.ip_addr);//NET_IF_DEV_ETH1
                    app_network_status_set(IF_FILE_FAIL);
                    app_log_info("network static failed !\n");
                }
                else
                {
                    addr.s_addr = inet_addr(cfg_data.dns1);
                    cyg_dns_res_init(&addr);
					_set_ip_cfg_file_data(dev_name, &cfg_data);
                    memcpy(s_static_ip_addr, cfg_data.ip_addr, sizeof(cfg_data.ip_addr));
                    app_network_status_set(IF_FILE_OK);
                    app_log_info("network static succed.\n");
                }
            }
            else
            {
                app_network_status_set(IF_FILE_FAIL);
                app_log_info("network static failed !\n");
            }
        }
	}
	else
	{
		if(cable_link == 0)
		{
            app_network_status_set(IF_FILE_FAIL);
			ret = IF_FILE_NONE;
		}
	}

	return ret;
}
#endif
#endif


#if (WIFI_SUPPORT > 0)
static ApInfo s_ap_list[MAX_AP_NUM];

#if WIFI_SUPER_AP_SUPPORT
static status_t _get_wifi_connect_file_data(const char *wifi_dev, ApFileData *ap_data);
static status_t _set_wifi_connect_file_data(const char *wifi_dev, ApFileData *ap_data);

typedef enum {
    AP_TYPE_NULL,
    AP_TYPE_SUPER,
    AP_TYPE_USER
} WifiApType;

typedef struct {
    WifiApType ap_type;
    ApFileData super_ap;
    ApFileData user_ap;
    bool user_ap_valid;
    ubyte64 wifi_name;
} WifiApMgr;

static WifiApMgr s_ap_mgr;

static int app_super_ap_check_start(char *wifi_dev)
{
    memset(&s_ap_mgr, 0, sizeof(s_ap_mgr));
    memset(s_ap_mgr.wifi_name, 0, sizeof(ubyte64));
    strcpy(s_ap_mgr.wifi_name, wifi_dev);
    app_send_msg_exec(GXMSG_WIFI_SCAN_AP, &s_ap_mgr.wifi_name);
    return 0;
}

static int app_super_ap_check_result(const char *wifi_dev, ApInfo *ap_info, unsigned int ap_num)
{
    int i = 0;

    for (i = 0; i < ap_num; i++)
    {
        if (strcmp(WIFI_SUPER_AP_ESSID, ap_info[i].ap_essid) == 0)
        {
            memcpy(&s_ap_mgr.super_ap.essid, &ap_info[i].ap_essid, sizeof(ubyte64));
            memcpy(&s_ap_mgr.super_ap.mac, &ap_info[i].ap_mac, sizeof(ubyte32));
            strncpy((char*)&s_ap_mgr.super_ap.psk, WIFI_SUPER_AP_PSK, sizeof(ubyte32)-1);
            if (s_ap_mgr.ap_type == AP_TYPE_NULL)
                s_ap_mgr.ap_type = AP_TYPE_SUPER;
            return 0;
        }
    }

    if (s_ap_mgr.ap_type == AP_TYPE_NULL)
        s_ap_mgr.ap_type = AP_TYPE_USER;
    return 0;
}

static int app_super_ap_set_cur_ap(const char *wifi_dev, ApFileData *ap_data)
{
    memcpy(&s_ap_mgr.user_ap, ap_data, sizeof(ApFileData));
    s_ap_mgr.ap_type = AP_TYPE_USER;
    return 0;
}

static int app_get_wifi_connect_ap_data(const char *wifi_dev, ApFileData *ap_data, bool update_flag)
{
    int ret = 0 ;
    switch (s_ap_mgr.ap_type)
    {
        case AP_TYPE_NULL:
            return -1;
        case AP_TYPE_SUPER:
            memcpy(ap_data, &s_ap_mgr.super_ap, sizeof(ApFileData));
            if (update_flag)
            {
                if ( _get_wifi_connect_file_data(wifi_dev, &s_ap_mgr.user_ap) == GXCORE_SUCCESS)
                {
                    s_ap_mgr.user_ap_valid = true;
                    if (strcmp(s_ap_mgr.super_ap.essid, s_ap_mgr.user_ap.essid) == 0
                            && strcmp(s_ap_mgr.super_ap.mac, s_ap_mgr.user_ap.mac) == 0
                            && strcmp(s_ap_mgr.super_ap.psk, s_ap_mgr.user_ap.psk) == 0)
                    {
                        s_ap_mgr.ap_type = AP_TYPE_USER;
                    }
                    else
                    {
                        _set_wifi_connect_file_data(wifi_dev, &s_ap_mgr.super_ap);
                    }
                }
                else
                {
                    _set_wifi_connect_file_data(wifi_dev, &s_ap_mgr.super_ap);
                    s_ap_mgr.user_ap_valid = false;
                }
            }
            return 0;
        case AP_TYPE_USER:
            ret = _get_wifi_connect_file_data(wifi_dev, ap_data) ;
            if ( ret != GXCORE_SUCCESS)
            {
                return ret;
            }
            return 0;
        default:
            break;
    }

    return -1;
}

static bool app_super_ap_set_next_connect(void)
{
    if (s_ap_mgr.ap_type == AP_TYPE_SUPER)
    {
        s_ap_mgr.ap_type = AP_TYPE_USER;
        if (s_ap_mgr.user_ap_valid)
        {
            _set_wifi_connect_file_data(s_ap_mgr.wifi_name, &s_ap_mgr.user_ap);
            return true;
        }
    }

    return false;
}
#endif

#ifdef ECOS_OS
static uint32_t MAX_WIFI_NO_CONNECT_CNT = 10; //超时时间 CONSOLE_DELAY_MS * MAX_WIFI_NO_CONNECT_CNT
static uint32_t s_wifi_no_connect_cnt = 0;

static bool _check_is_dhcp(void)
{
	int32_t init_value;
	GxBus_ConfigGetInt(IP_TYPE_WIFI_KEY, &init_value, IP_TYPE_DEFALT);
    return (init_value == 1) ? true : false;
}

static IfFileRet _check_wifi_result_file(IfState if_state, char *dev_name)
{
    IfFileRet ret = -1;

    if(dev_name == NULL)
        return -1;

    int nStatus = s_wifi_status;
    IfFileRet network_status = app_network_status_get();
#ifdef APP_NET_WIFI_DEBUG_PROCESS_SUPPORT
    app_wifi_info("check %d,%d \n",nStatus,network_status);
#endif
    /*
    for invalid password. drv will send status 32, then past send status 0
    but in here processing slow, so missed get status 32.   20240301
    */
#if 1
    if ( (nStatus == 0) && ( ( s_wifi_pre_status == 32 ) ) )
    {
        nStatus = s_wifi_pre_status ;
    }
    s_wifi_pre_status = nStatus + 50 ;
#endif

    if(IF_FILE_OK == network_status)
	{
		ret = IF_FILE_OK;
		if(nStatus == -1) //关掉路由器
		{
            app_network_status_set(IF_FILE_DISCONNECTED);
			int32_t init_value;
			GxBus_ConfigGetInt(IP_TYPE_WIFI_KEY, &init_value, IP_TYPE_DEFALT);
			if(init_value == 1)	//dhcp
			{
				network_down(dev_name);
			}
			else //static ip
			{
				net_ifconfig(dev_name, "clear", NULL);
			}
			ret = IF_FILE_DISCONNECTED;
		}
	}
    else if(IF_FILE_START == network_status)
        ret = IF_FILE_START;
    else if(IF_FILE_WAIT == network_status)
    {
        if(nStatus == 32) {//32 invalid password
            ret = IF_FILE_REJ;
        } else {
            ret = IF_FILE_WAIT;
        }
    }
    else if(IF_FILE_FAIL == network_status)
        ret = IF_FILE_FAIL;
    else if(IF_FILE_REJ == network_status)
        ret = IF_FILE_REJ;
    else if(IF_FILE_DISCONNECTED == network_status)
		ret = IF_FILE_DISCONNECTED;
    else
    {
        ret = IF_FILE_NONE;
    }

    //4:connected
    if(nStatus == 4 && (ret == IF_FILE_WAIT || ret == IF_FILE_NONE))
    {
        extern int net_deladdr(const char *if_name, char *ip);

        //clear last net config
        net_deladdr(dev_name, (char *)s_static_ip_addr);
        network_down(dev_name);

        if(_check_is_dhcp())
        {
            int dhcp_status;

            app_log_info("\033[33mDo DHCP ... !!!\n\033[0m");
            network_init(dev_name);
            dhcp_status = network_dhcp(dev_name);
            if(dhcp_status == 0)
            {
                IpCfgFileData cfg_data;
                unsigned int ip, mask;
                unsigned char mac[15];
                struct in_addr gateway,dnsA,dnsB;
                memset(&cfg_data, 0, sizeof(cfg_data));

                cfg_data.dev_mode = 1;
                cfg_data.ip_type = 1;
                getip(dev_name, &ip, &mask, mac);
                getdns(dev_name, &gateway, &dnsA, &dnsB);
                struct in_addr sin;
                sin.s_addr=ip;
                strcpy(cfg_data.ip_addr, inet_ntoa(sin));
                sin.s_addr=mask;
                strcpy(cfg_data.net_mask, inet_ntoa(sin));
                sin=gateway;
                strcpy(cfg_data.gate_way, inet_ntoa(sin));
                sin=dnsA;
                strcpy(cfg_data.dns1, inet_ntoa(sin));
                sin=dnsB;
                strcpy(cfg_data.dns2, inet_ntoa(sin));

                _set_ip_cfg_file_data(dev_name, &cfg_data);
                memset(s_static_ip_addr, 0, sizeof(cfg_data.ip_addr));
                app_network_status_set(IF_FILE_OK);
            }
            else
            {
                app_network_status_set(IF_FILE_FAIL);
                network_down(dev_name);
            }
        }
        else
        {
            IpCfgFileData cfg_data;
            struct bootp bootp_data;
            struct in_addr addr;

            app_log_info("\033[33mDo STATIC ... !!!\n\033[0m");

            memset(&cfg_data, 0, sizeof(IpCfgFileData));
            if(_get_ip_cfg_file_data(dev_name, &cfg_data) == GXCORE_SUCCESS)
            {
                uint32_t ip[4] = {0};
                uint32_t mask[4] = {0};
                uint32_t gateway[4] = {0};
                uint32_t dns[4] = {0};
                unsigned char broadcast[4] = {0};
                char tmp[20] = {0};

                sscanf(cfg_data.ip_addr, "%d.%d.%d.%d", &(ip[0]), &(ip[1]), &(ip[2]), &(ip[3]));
                sscanf(cfg_data.net_mask, "%d.%d.%d.%d", &(mask[0]), &(mask[1]), &(mask[2]), &(mask[3]));
                sscanf(cfg_data.gate_way, "%d.%d.%d.%d", &(gateway[0]), &(gateway[1]), &(gateway[2]), &(gateway[3]));
                sscanf(cfg_data.dns1, "%d.%d.%d.%d", &(dns[0]), &(dns[1]), &(dns[2]), &(dns[3]));

                broadcast[0] = ((unsigned char)ip[0] & (unsigned char)mask[0]) | (unsigned char)(~(mask[0]));
                broadcast[1] = ((unsigned char)ip[1] & (unsigned char)mask[1]) | (unsigned char)(~(mask[1]));
                broadcast[2] = ((unsigned char)ip[2] & (unsigned char)mask[2]) | (unsigned char)(~(mask[2]));
                broadcast[3] = ((unsigned char)ip[3] & (unsigned char)mask[3]) | (unsigned char)(~(mask[3]));

                sprintf(cfg_data.ip_addr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
                sprintf(cfg_data.net_mask, "%d.%d.%d.%d", mask[0], mask[1], mask[2], mask[3]);
                sprintf(cfg_data.gate_way, "%d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3]);
                sprintf(cfg_data.dns1, "%d.%d.%d.%d", dns[0], dns[1], dns[2], dns[3]);
                sprintf(tmp, "%d.%d.%d.%d", broadcast[0], broadcast[1], broadcast[2], broadcast[3]);
                app_log_debug("BroadCast Addr : %s\n", tmp);

                build_bootp_record(&bootp_data,
                        dev_name,
                        cfg_data.ip_addr,
                        cfg_data.net_mask,
                        tmp,
                        cfg_data.gate_way,
                        cfg_data.gate_way);

                if(network_static(dev_name, &bootp_data))
                {
                    net_deladdr(dev_name, cfg_data.ip_addr);
                    app_network_status_set(IF_FILE_FAIL);
                    app_log_info("network_staic failed !\n");
                }
                else
                {
                    addr.s_addr = inet_addr(cfg_data.dns1);
                    cyg_dns_res_init(&addr);
                    _set_ip_cfg_file_data(dev_name, &cfg_data);
                    memcpy(s_static_ip_addr, cfg_data.ip_addr, sizeof(cfg_data.ip_addr));
                    app_network_status_set(IF_FILE_OK);
                    app_log_info("network static succed.\n");
                }
            }
            else
            {
                app_network_status_set(IF_FILE_FAIL);
                app_log_info("network static failed !\n");
            }
        }
    }
    else if(nStatus == 4&&(ret == IF_FILE_DISCONNECTED))
    {
        app_network_status_set(IF_FILE_NONE);//route recover.
        ret = IF_FILE_NONE;
    }
    else
    {
        if(nStatus == 4 && ret != IF_FILE_OK)
            app_log_error("\033[32m[ERR:%s] WIFI_S: %d RET: %d\n\033[0m", __func__, nStatus, ret);
    }

    //12:Authancation fail 32:4-way handle shark fail,key fail
    if((nStatus == 12 || nStatus == 22) && ret == IF_FILE_WAIT)
    {
        //TODO: CLEAR AP RECORD
        app_network_status_set(IF_FILE_FAIL);
    }

    // 关闭了正在连接的手机热点
    if((nStatus == 0)
        && ((ret == IF_FILE_START)
            || (ret == IF_FILE_WAIT)
            || (ret == IF_FILE_OK)))
    {
        ++s_wifi_no_connect_cnt; // nStatus==0时无法区分是初始状态还是关闭路由器的状态,所以需要延时机制
        if(s_wifi_no_connect_cnt >= MAX_WIFI_NO_CONNECT_CNT || ret == IF_FILE_OK)
        {
            app_network_status_set(IF_FILE_DISCONNECTED);
            int32_t init_value;
            GxBus_ConfigGetInt(IP_TYPE_WIFI_KEY, &init_value, IP_TYPE_DEFALT);
            if(init_value == 1)	//dhcp
            {
                network_down(dev_name);
            }
            else //static ip
            {
                net_ifconfig(dev_name, "clear", NULL);
            }
            ret = IF_FILE_DISCONNECTED;
        }
    }
    else
    {
        s_wifi_no_connect_cnt = 0;
    }

    return ret;
}
#endif

static status_t _get_ap_psk_and_hidden(char *ap_essid, const char *ap_mac, char *ap_psk, uint8_t *hidden)
{
#ifdef LINUX_OS
#define WIFI_AP_REC_FILE "/usr/share/app/ethernet/wifi_ap_record"
    FILE *ret_fp = NULL;
    char ret_buf[RESULT_LEN] = {0};
    ubyte64 tmp_essid = {0};
    char seq_char = '\t';
    char *p = NULL;
    char *q = NULL;

    if((ap_essid == NULL) || (ap_mac == 0) || (ap_psk == NULL))
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(WIFI_AP_REC_FILE))
        return GXCORE_ERROR;

    if((ret_fp = fopen(WIFI_AP_REC_FILE, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, RESULT_LEN - 1, ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        p = ret_buf;
        q = strchr(p, seq_char);
        if(q != NULL)
        {
            if(strlen(ap_essid) && strncmp(ap_essid, p, q-p)) //ap essid no hidden
                continue;

            memset(tmp_essid, 0, sizeof(ubyte64));
            strncpy(tmp_essid, p, q-p);

            if((p = q + 1) != '\0')
            {
                q = strchr(p, seq_char);
                if((q != NULL) && (strncmp(ap_mac, p, q-p) == 0)) //mac
                {
                    if(0 == strlen(ap_essid)) //copy essid to hidden ap
                        strlcpy(ap_essid, tmp_essid, sizeof(ubyte64));

                    if((p = q + 1) != '\0')//psk
                    {
                        q = strchr(p, seq_char);
                        if(q != NULL)
                        {
                            strncpy(ap_psk, p, q-p);
                            if((p = q + 1) != '\0')//hidden
                                *hidden = atoi(p);
                        }
                    }
                    fclose(ret_fp);
                    return GXCORE_SUCCESS;
                }
            }
        }
        memset(ret_buf, 0, RESULT_LEN);
    }

    fclose(ret_fp);
    return GXCORE_ERROR;

#else
    extern status_t app_wifi_ap_load(char * mac, char * ssid, char * psk, int *hidden);
    ApFileData ap_data;
    memset(&ap_data, 0, sizeof(ApFileData));

    if((ap_essid == NULL) || (ap_mac == 0) || (ap_psk == NULL))
        return GXCORE_ERROR;

    if(GXCORE_SUCCESS == app_wifi_ap_load((char *)ap_mac, ap_data.essid, ap_data.psk, &ap_data.hidden))
    {
        strcpy((char *)ap_psk, ap_data.psk);
        if(0 == strlen(ap_essid))
            strncpy(ap_essid, ap_data.essid, sizeof(ubyte64));
        *hidden = ap_data.hidden;
        app_log_debug("\033[31mGet ap data from flash : Name: %s MAC: %s PSK: %s Hidden: %d\n\033[0m", ap_essid, ap_mac, ap_data.psk, ap_data.hidden);
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
#endif
}

static int _ap_quality_cmp(const void *a, const void *b)
{
    ApInfo *ap_1 = (ApInfo*)a;
    ApInfo *ap_2 = (ApInfo*)b;
    int ret = 0;

    if((ap_1 == NULL) || (ap_2 == NULL))
        return 0;

    if(ap_1->ap_quality > ap_2->ap_quality)
        ret = -1;
    else if(ap_1->ap_quality < ap_2->ap_quality)
        ret = 1;

    return ret;
}


#ifdef LINUX_OS
static unsigned int _get_ap_list_file_data(const char *aplist_file, ApInfo *ap_data, unsigned int max_ap_num)
{
#define ADD_HEAD "Address"
#define SSID_HEAD "ESSID"
#define Q_HEAD "Quality"
#define KEY_HEAD "key"

    FILE *ret_fp = NULL;
    char ret_buf[RESULT_LEN] = {0};
    unsigned int i = 0;
    unsigned int res_num = 0;

    if((aplist_file == NULL) || (ap_data == NULL))
        return 0;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(aplist_file))
        return 0;

    if((ret_fp = fopen(aplist_file, "r")) == NULL)
    {
        fclose(ret_fp);
        return 0;
    }

    if(fgets(ret_buf, RESULT_LEN - 1, ret_fp)  == NULL)//mac of wifi dev
    {
        fclose(ret_fp);
        return 0;
    }

    memset(ret_buf, 0, RESULT_LEN);
    if(fgets(ret_buf, RESULT_LEN - 1, ret_fp)  == NULL) //ap num
    {
        fclose(ret_fp);
        return 0;
    }

    memset(ret_buf, 0, RESULT_LEN);
    while((fgets(ret_buf, RESULT_LEN - 1, ret_fp) != NULL)&&(res_num<max_ap_num))
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        if(strncmp(ret_buf, ADD_HEAD, strlen(ADD_HEAD)) == 0)
        {
            res_num++;
            i = res_num - 1;
            strcpy(ap_data[i].ap_mac, ret_buf + strlen(ADD_HEAD));
        }
        else if(strncmp(ret_buf, SSID_HEAD, strlen(SSID_HEAD)) == 0)
        {
            strcpy(ap_data[i].ap_essid, ret_buf + strlen(SSID_HEAD));
        }
        else if(strncmp(ret_buf, Q_HEAD, strlen(Q_HEAD)) == 0)
        {
            ap_data[i].ap_quality = atoi(ret_buf + strlen(Q_HEAD));
        }
        else if(strncmp(ret_buf, KEY_HEAD, strlen(KEY_HEAD)) == 0)
        {
            if(strncmp(ret_buf + strlen(KEY_HEAD), "on", 2) == 0)
                ap_data[i].encryption = 1;
            else
                ap_data[i].encryption = 0;
        }
    }

    //sort by quality
    qsort(ap_data, res_num, sizeof(ApInfo), _ap_quality_cmp);

    fclose(ret_fp);
    return res_num;
}


#endif

static status_t _set_wifi_connect_file_data(const char *wifi_dev, ApFileData *ap_data)
{
#ifdef LINUX_OS
    char cmd_buf[256] = {0};
    ubyte64 psk = {0};

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0) || (ap_data == NULL))
        return GXCORE_ERROR;

    _net_format_shell_str(ap_data->psk, psk);
    sprintf(cmd_buf, "%s %s \"%s\" %s \"%s\" %d", WIFI_SAVE_AP_CMD, wifi_dev, ap_data->essid,
            ap_data->mac, psk, ap_data->hidden);
     //app_log_debug("-----[%s]%d %s-----\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);
#else
    ApFileData ap_s ;
    memset(&ap_s,0,sizeof(ApFileData));
    if(ap_data->psk && strlen(ap_data->psk) == 0)
    {
        extern status_t app_wifi_ap_load(char * mac, char * ssid, char * psk, int *hidden);
        if(GXCORE_SUCCESS == app_wifi_ap_load((char *)ap_data->mac, ap_s.essid, ap_s.psk, &ap_s.hidden))
        {
            strcpy((char *)ap_data->psk, ap_s.psk);
        }
    }

    extern status_t app_wifi_ap_save(char * mac, char * ssid, char * psk, int pskmaxlen, int hidden);
    app_wifi_ap_save(ap_data->mac, ap_data->essid, ap_data->psk, sizeof(ap_data->psk), ap_data->hidden);
    GxBus_ConfigSet(AP0_DEV_KEY, wifi_dev);
    GxBus_ConfigSet(AP0_SSID_KEY, ap_data->essid);
    GxBus_ConfigSet(AP0_MAC_KEY, ap_data->mac);
    GxBus_ConfigSet(AP0_PSK_KEY, ap_data->psk);
    GxBus_ConfigSetInt(AP0_HIDE_KEY, ap_data->hidden);
    s_wifi_no_connect_cnt = 0;
#endif

    return GXCORE_SUCCESS;
}

static status_t _get_wifi_connect_file_data(const char *wifi_dev, ApFileData *ap_data)
{
#ifdef LINUX_OS
#define WIFI_CONNECT_FILE "/usr/share/app/ethernet/wifi_connect_record"
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = '\t';
    char *p = NULL;
    char *q = NULL;

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0) || (ap_data == NULL))
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(WIFI_CONNECT_FILE))
        return GXCORE_ERROR;

    if((ret_fp = fopen(WIFI_CONNECT_FILE, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        p = ret_buf;
        q = strchr(p, seq_char);
        if((q != NULL) && (strncmp(wifi_dev, p, q-p) == 0)) //wifi name
        {
            if((p = q + 1) != '\0')
            {
                q = strchr(p, seq_char);
                if(q != NULL) //essid
                {
                    strncpy(ap_data->essid, p, q-p);
                     //app_log_debug("~~~~~~[%s]%d, wifi file essid: %s!!!!!\n", __func__, __LINE__, ap_data->essid);

                    if((p = q + 1) != '\0')
                    {
                        q = strchr(p, seq_char);
                        if(q != NULL) //mac
                        {
                            strncpy(ap_data->mac, p, q-p);
                             //app_log_debug("~~~~~~[%s]%d, wifi file mac: %s!!!!!\n", __func__, __LINE__, ap_data->mac);

                            if((p = q + 1) != '\0')
                            {
                                q = strchr(p, seq_char);
                                if(q != NULL)
                                {
                                    strncpy(ap_data->psk, p, q-p);
                                    if((p = q + 1) != '\0')
                                        ap_data->hidden = atoi(p);
                                }
                            }
                        }
                    }
                }
            }
            fclose(ret_fp);
            return GXCORE_SUCCESS;
        }
        else if( q != NULL )
        {
            if(strncmp(wifi_dev, p, q-p-1) == 0)
            {
                return GXCORE_NAME_NOT_MATCH_ERROR ;
            }
        }

        memset(ret_buf, 0, sizeof(ret_buf));
    }

    fclose(ret_fp);
    return GXCORE_ERROR;
#else
    char ret_buf[RESULT_LEN] = {0};

    GxBus_ConfigGet(AP0_DEV_KEY, ret_buf, sizeof(ret_buf), AP_DEV_DEFAULT);
    if(strcmp(wifi_dev, ret_buf) == 0) //wifi name
    {
        GxBus_ConfigGet(AP0_MAC_KEY, ap_data->mac, sizeof(ap_data->mac), AP_MAC_DEFAULT);
        if((ap_data->mac != NULL) && (strncmp(ap_data->mac, AP_MAC_DEFAULT, sizeof(AP_MAC_DEFAULT)) == 0))
            return GXCORE_ERROR;

        GxBus_ConfigGet(AP0_SSID_KEY, ap_data->essid, sizeof(ap_data->essid), AP_SSID_DEFAULT);
        GxBus_ConfigGet(AP0_PSK_KEY, ap_data->psk, sizeof(ap_data->psk), AP_PSK_DEFAULT);
        GxBus_ConfigGetInt(AP0_HIDE_KEY, &ap_data->hidden, AP_HIDE_DEFAULT);
        return GXCORE_SUCCESS;
    }
    else
    {
        GxBus_ConfigGet(AP0_MAC_KEY, ap_data->mac, sizeof(ap_data->mac), AP_MAC_DEFAULT);
        if((ap_data->mac != NULL) && (strncmp(ap_data->mac, AP_MAC_DEFAULT, sizeof(AP_MAC_DEFAULT)) == 0))
            return GXCORE_ERROR;
        return GXCORE_NAME_NOT_MATCH_ERROR;
    }

    return GXCORE_ERROR;
#endif
}
#endif

#if (PPPOE_SUPPORT > 0)
static status_t _set_pppoe_file_data(const char *dev_name, PppoeFileData *pppoe_data)
{
    char cmd_buf[256] = {0};
    ubyte64 user = {0};
    ubyte64 psk = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (pppoe_data == NULL))
        return GXCORE_ERROR;

    _net_format_shell_str(pppoe_data->username, user);
    _net_format_shell_str(pppoe_data->passwd, psk);
    sprintf(cmd_buf, "%s %s %d \"%s\" \"%s\" %s %s", SAVE_PPPOE_CMD, dev_name,
            pppoe_data->mode, user, psk, pppoe_data->dns1, pppoe_data->dns2);
    //app_log_debug("-----[%s]%d %s-----\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);
    return GXCORE_SUCCESS;
}

static status_t _get_pppoe_file_data(const char *dev_name, PppoeFileData *pppoe_data)
{
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = ',';
    char *p = NULL;
    char *q = NULL;
    char temp[10] = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (pppoe_data == NULL))
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(PPPOE_FILE))
        return GXCORE_ERROR;

    if((ret_fp = fopen(PPPOE_FILE, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        p = ret_buf;
        q = strchr(p, seq_char);
        if((q != NULL) && (strncmp(dev_name, p, q-p) == 0)) //dev name
        {
            if((p = q + 1) != '\0')
            {
                q = strchr(p, seq_char);
                if(q != NULL) //mode
                {
                    strncpy(temp, p, q-p);
                    pppoe_data->mode = atoi(temp);
                    if((p = q + 1) != '\0')
                    {
                        q = strchr(p, seq_char);
                        if(q != NULL)//username
                        {
                            strncpy(pppoe_data->username, p, q-p);
                            //app_log_debug("[%s]%d, %s\n",__func__, __LINE__, p);
                            if((p = q + 1) != '\0')
                            {
                                q = strchr(p, seq_char);
                                if(q != NULL) //passwd
                                {
                                    strncpy(pppoe_data->passwd, p, q-p);
                                    //app_log_debug("[%s]%d, %s\n", __func__, __LINE__, p);
                                    if((p = q + 1) != '\0')
                                    {
                                        q = strchr(p, seq_char);
                                        {
                                            if(q != NULL) //dns1
                                            {
                                                //app_log_debug("[%s]%d, %s\n", __func__, __LINE__, p);
                                                strncpy(pppoe_data->dns1, p, q-p);
                                                if((p = q + 1) != '\0')
                                                {
                                                //app_log_debug("[%s]%d, %s\n", __func__, __LINE__, p);
                                                    strcpy(pppoe_data->dns2, p);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            fclose(ret_fp);
            //app_log_debug("~~~~~~[%s]%d, pppoe file passwd: %s!!!!!\n", __func__, __LINE__, pppoe_data->mac);
            return GXCORE_SUCCESS;
        }
        memset(ret_buf, 0, sizeof(ret_buf));
    }

    fclose(ret_fp);
    return GXCORE_ERROR;
}
#endif

#if MOD_3G_SUPPORT
static status_t _get_3g_cfg_file_data(GxMsgProperty_3G *tgParam)
{
    FILE *ret_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = ',';
    char *p = NULL;
    char *q = NULL;

    if(tgParam == NULL)
        return GXCORE_ERROR;

    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(_3G_CONNNECT_RECORD))
        return GXCORE_ERROR;

    if((ret_fp = fopen(_3G_CONNNECT_RECORD, "r")) == NULL)
        return GXCORE_ERROR;

    while(fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL)
    {
        if(ret_buf[strlen(ret_buf) -1] == '\n')
            ret_buf[strlen(ret_buf) -1] = 0;

        p = ret_buf;
        q = strchr(p, seq_char);

        if((q == NULL) || (strncmp(tgParam->dev_name, p, q-p) != 0)) //3g name
        {
            memset(ret_buf, 0, sizeof(ret_buf));
            continue;
        }

        if((p = q + 1) == '\0')
        {
            memset(ret_buf, 0, sizeof(ret_buf));
            continue;
        }

        if((q = strchr(p, seq_char)) == NULL) //operator id
        {
            memset(ret_buf, 0, sizeof(ret_buf));
            continue;
        }

        if(tgParam->operator_id != atoi(p))
        {
            memset(ret_buf, 0, sizeof(ret_buf));
            continue;
        }

        if((p = q + 1) == '\0')
        {
            break;
        }

        q = strchr(p, seq_char);
        if(q != NULL) //apn
        {
            strncpy(tgParam->apn, p, q-p);
            if((p = q + 1) != '\0')
            {
                q = strchr(p, seq_char);
                if(q != NULL) //phone
                {
                    strncpy(tgParam->phone, p, q-p);
                    if((p = q + 1) != '\0')
                    {
                        q = strchr(p, seq_char);
                        if(q != NULL) //username
                        {
                            strncpy(tgParam->username, p, q-p);
                            if((p = q + 1) != '\0')
                            {
                                //password
                                strcpy(tgParam->password, p);
                            }
                        }
                    }
                }
            }
        }

        fclose(ret_fp);
        return GXCORE_SUCCESS;
    }

    fclose(ret_fp);
    return GXCORE_ERROR;
}

#ifdef LINUX_OS
static status_t _set_3g_cfg_file_data(GxMsgProperty_3G *_3g_param)
{
    char cmd_buf[256] = {0};
    ubyte64 apn = {0}, phone = {0}, user = {0}, psk = {0};

	if((_3g_param == NULL) || (strlen(_3g_param->dev_name) == 0))
        return GXCORE_ERROR;

    _net_format_shell_str(_3g_param->apn, apn);
    _net_format_shell_str(_3g_param->phone, phone);
    _net_format_shell_str(_3g_param->username, user);
    _net_format_shell_str(_3g_param->password, psk);
    //app_log_debug("dev_name:%s, operator_id:%d, apn:%s, phone:%s, username:%s, passwd:%s\n", _3g_param->dev_name, _3g_param->operator_id,
    //      _3g_param->apn, _3g_param->phone, _3g_param->username, _3g_param->password);

    sprintf(cmd_buf, "%s \"%s\" \"%d\" \"%s\" \"%s\" \"%s\" \"%s\"", USB3G_SET_PARAM_CMD, _3g_param->dev_name, _3g_param->operator_id,
            apn, phone, user, psk);
    //app_log_debug("-----[%s]%d %s-----\n",  __func__, __LINE__, cmd_buf);
    system(cmd_buf);
    return GXCORE_SUCCESS;
}
#endif

#ifdef ECOS_OS
static status_t _mv_3g_cfg(const char *src, const char *dest)
{
    FILE *src_fp = NULL;
    FILE *dest_fp = NULL;
    char ret_buf[RESULT_LEN] = {0};

    if((src == NULL) || (dest == NULL))
        return GXCORE_ERROR;

    if(strcmp(src, dest) == 0)
        return GXCORE_ERROR;

    if((src_fp = fopen(src, "r")) == NULL)
        return GXCORE_ERROR;

    if((dest_fp = fopen(dest, "w+")) == NULL)
    {
        fclose(src_fp);
        return GXCORE_ERROR;
    }

    while(fgets(ret_buf, RESULT_LEN - 1, src_fp) != NULL)
    {
        fwrite(ret_buf, 1, strlen(ret_buf), dest_fp);
        memset(ret_buf, 0, sizeof(ret_buf));
    }

    fclose(src_fp);
    fflush(dest_fp);
    fsync(fileno(dest_fp));
    fclose(dest_fp);

    GxCore_FileDelete(src);

    return GXCORE_SUCCESS;
}

static status_t _set_3g_cfg_file_data(GxMsgProperty_3G *_3g_param)
{
#define _3G_RECORD_TMP "/mnt/3g_connect_temp"
    FILE *ret_fp = NULL;
    FILE *temp_fp = NULL;
    char ret_buf[256] = {0};
    char seq_char = ',';
    char *p = NULL;
    char *q = NULL;
    bool replace = false;

    if(_3g_param == NULL)
        return GXCORE_ERROR;

    if((temp_fp = fopen(_3G_RECORD_TMP, "w+")) == NULL)
        return GXCORE_ERROR;

    if((ret_fp = fopen(_3G_CONNNECT_RECORD, "r")) != NULL)
    {
        while((ret_fp != NULL) && (fgets(ret_buf, sizeof(ret_buf), ret_fp) != NULL))
        {
            p = ret_buf;
            q = strchr(p, seq_char);

            if((q == NULL) || (strncmp(_3g_param->dev_name, p, q-p) == 0)) //3g name
            {
                p = q + 1;
                if((*p != '\0') && (*p != '\n'))
                {
                    if((q = strchr(p, seq_char)) != NULL) //operator id
                    {
                        if(_3g_param->operator_id == atoi(p))
                        {
                            //same & replace
                            memset(ret_buf, 0, sizeof(ret_buf));
                            sprintf(ret_buf, "%s,%d,%s,%s,%s,%s\n", _3g_param->dev_name, _3g_param->operator_id,
                                    _3g_param->apn, _3g_param->phone, _3g_param->username, _3g_param->password);
                            replace = true;
                        }
                    }
                }
            }

            fwrite(ret_buf, 1, strlen(ret_buf), temp_fp);
            memset(ret_buf, 0, sizeof(ret_buf));
        }
        fclose(ret_fp);
    }

    if(replace == false)
    {
        memset(ret_buf, 0, sizeof(ret_buf));
        sprintf(ret_buf, "%s,%d,%s,%s,%s,%s\n", _3g_param->dev_name, _3g_param->operator_id,
                _3g_param->apn, _3g_param->phone, _3g_param->username, _3g_param->password);
        fwrite(ret_buf, 1, strlen(ret_buf), temp_fp);
    }
    fclose(temp_fp);

    if(GxCore_FileExists(_3G_RECORD_TMP) == GXCORE_FILE_EXIST)
    {
        return _mv_3g_cfg(_3G_RECORD_TMP, _3G_CONNNECT_RECORD);
    }

#if 0
    if(GxCore_FileExists(_3G_RECORD_TMP) == GXCORE_FILE_EXIST)
    {
        app_log_debug("[%s]%d, _3G_RECORD_TMP EXIST!!!\n", __func__, __LINE__);
    }
    else
    {
        app_log_debug("[%s]%d, _3G_RECORD_TMP UNEXIST!!!\n", __func__, __LINE__);
    }

    if(GxCore_FileExists(_3G_CONNNECT_RECORD) == GXCORE_FILE_EXIST)
    {
        app_log_debug("[%s]%d, _3G_CONNNECT_RECORD EXIST!!!\n", __func__, __LINE__);
    }
    else
    {
        app_log_debug("[%s]%d, _3G_CONNNECT_RECORD UNEXIST!!!\n", __func__, __LINE__);
    }
#endif
    return GXCORE_ERROR;
}
static void gxusb3g_ip(char *ip_addr)
{
	struct ifreq ifr;
	struct	in_addr sin_addr;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	strcpy(ip_addr, "0.0.0.0");
	if( sock != -1 )
	{
		memset(&sin_addr,0,sizeof(sin_addr));
		strlcpy(ifr.ifr_name, "ppp0", sizeof(ifr.ifr_name));
		if( ioctl( sock, SIOCGIFADDR, &ifr ) == 0 )
			sin_addr = ((struct sockaddr_in *)&ifr.ifr_dstaddr)->sin_addr;
		strcpy(ip_addr, inet_ntoa(sin_addr));
		close(sock);
	}
}

static void gxusb3g_netmask(char *net_mask)
{
	struct ifreq ifr;
	struct	in_addr sin_addr;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	strcpy(net_mask, "0.0.0.0");
	if( sock != -1 )
	{
		memset(&sin_addr,0,sizeof(sin_addr));
		strlcpy(ifr.ifr_name, "ppp0", sizeof(ifr.ifr_name));
		if( ioctl( sock, SIOCGIFNETMASK, &ifr ) == 0 )
			sin_addr = ((struct sockaddr_in *)&ifr.ifr_dstaddr)->sin_addr;
		strcpy(net_mask, inet_ntoa(sin_addr));
		close(sock);
	}
}

static void _usb3g_set_ip_cfg_file_data(char *dev_name)
{
	IpCfgFileData cfg_data;
	uint32_t  ip = 0;
	uint32_t  gateway = 0;
	uint32_t  dnsA = 0;
	uint32_t  dnsB = 0;
	struct in_addr sin;

	memset(&cfg_data, 0, sizeof(cfg_data));
	cfg_data.dev_mode = 1;
	cfg_data.ip_type = 1;
	gxusb3g_ip(cfg_data.ip_addr);
	gxusb3g_netmask(cfg_data.net_mask);

    usb3g_ifconfig(&ip, &gateway, &dnsA, &dnsB);
    sin.s_addr = gateway;
	strlcpy(cfg_data.gate_way, inet_ntoa(sin), sizeof(cfg_data.gate_way));
    sin.s_addr = dnsA;
	strlcpy(cfg_data.dns1, inet_ntoa(sin), sizeof(cfg_data.dns1));
    sin.s_addr = dnsB;
	strlcpy(cfg_data.dns2, inet_ntoa(sin), sizeof(cfg_data.dns2));

	_set_ip_cfg_file_data(dev_name, &cfg_data);
}
static IfFileRet _check_3g_result_file(IfState if_state, char *dev_name)
{
    IfFileRet file_ret = IF_FILE_NONE;
    IfState state_3g = IF_STATE_INVALID;
    static IfState old_state_3g = IF_STATE_INVALID;

    state_3g = usb3g_getstate();
    switch(state_3g)
    {
        case IF_STATE_START:
            file_ret = IF_FILE_START;
            break;

        case IF_STATE_CONNECTING:
            file_ret = IF_FILE_WAIT;
            break;

        case IF_STATE_CONNECTED:
            file_ret = IF_FILE_OK;
            if(old_state_3g!=state_3g)
            {
                _usb3g_set_ip_cfg_file_data(dev_name);
            }
            break;

        case IF_STATE_REJ:
            file_ret = IF_FILE_REJ;
            break;

        default:
            file_ret = IF_FILE_NONE;
            break;
    }

    old_state_3g = state_3g;
    return file_ret;
}
#endif
#endif


#if (ETH_SUPPORT>0)
static void _net_thread_wired_start(void* arg)
{
    ubyte32 dev_name = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strcpy(dev_name, (char*)arg);

#ifdef LINUX_OS
    if(strlen(dev_name) != 0)
    {
        char cmd_buf[CMD_LEN] = {0};

        sprintf(cmd_buf, "%s %s", WIRED_START_CMD, dev_name);
        app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
        system(cmd_buf);
    }
#else
    if(wired_cable_link == 1)
    {
        app_network_status_set(IF_FILE_START);
    }
#endif
}
static status_t _wired_dev_start(IfDevice *wired_dev)
{
    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return GXCORE_ERROR;


    return GxCore_ThreadCreate("wired_start", &(wired_dev->start_thread_handle), _net_thread_wired_start,
            (void*)(wired_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t _wired_dev_stop(IfDevice *wired_dev)
{
    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return GXCORE_ERROR;
#ifdef LINUX_OS
    char cmd_buf[CMD_LEN] = {0};

    sprintf(cmd_buf, "%s %s", WIRED_STOP_CMD, wired_dev->dev_name);
    system(cmd_buf);
    //app_log_debug("~~~~~~[%s]%d, %s!!!wired stop !!\n", __func__, __LINE__, cmd_buf);

#else

    int32_t init_value;

	GxBus_ConfigGetInt(IP_TYPE_WIRED_KEY, &init_value, IP_TYPE_DEFALT);
	if(init_value == 1)	//dhcp
	{
		network_down(wired_dev->dev_name);
	}
	else //static ip
	{
		net_ifconfig(wired_dev->dev_name, "clear", NULL);
	}

#endif
    return GXCORE_SUCCESS;
}

static IfFileRet _wired_dev_connect_monitor(IfDevice *wired_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return IF_FILE_NONE;
#ifdef LINUX_OS
    file_ret = _check_dev_result_file(wired_dev->dev_state, wired_dev->dev_name);
#else
    file_ret = _check_wired_result_file(wired_dev->dev_state, wired_dev->dev_name);
#endif
    return file_ret;
}

#endif

#if USB_ETH_SUPPORT
static void _net_thread_usb_wired_start(void* arg)
{
    ubyte32 dev_name = {0};
    GxCore_ThreadDetach();
    if(arg == NULL)
        return;
    strcpy(dev_name, (char*)arg);
#ifdef LINUX_OS
    if(strlen(dev_name) != 0)
    {
        char cmd_buf[CMD_LEN] = {0};

        sprintf(cmd_buf, "%s %s", WIRED_START_CMD, dev_name);
        app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
        system(cmd_buf);
    }
#else
	if(usb_wired_cable_link == 1)
	{
        app_network_status_set(IF_FILE_START);
	}
#endif
}
static status_t _usb_wired_dev_start(IfDevice *wired_dev)
{
    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return GXCORE_ERROR;
#ifdef ECOS_OS
	if(usb_wired_cable_link == 1)
	{
		net_ifconfig(wired_dev->dev_name, "up", NULL);
	}
#endif
    return GxCore_ThreadCreate("usb_wired_start", &(wired_dev->start_thread_handle), _net_thread_usb_wired_start,
            (void*)(wired_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
}
static status_t _usb_wired_dev_stop(IfDevice *wired_dev)
{
	if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
		return GXCORE_ERROR;
#ifdef LINUX_OS
    char cmd_buf[CMD_LEN] = {0};

    sprintf(cmd_buf, "%s %s", WIRED_STOP_CMD, wired_dev->dev_name);
    system(cmd_buf);
    //app_log_debug("~~~~~~[%s]%d, %s!!!wired stop !!\n", __func__, __LINE__, cmd_buf);
#else
    int32_t init_value;
	GxBus_ConfigGetInt(IP_TYPE_USBETH_KEY, &init_value, IP_TYPE_DEFALT);
	if(init_value == 1)	//dhcp
	{
		network_down(wired_dev->dev_name);
	}
	else //static ip
	{
		net_ifconfig(wired_dev->dev_name, "clear", NULL);
	}
#endif
	return GXCORE_SUCCESS;
}
static IfFileRet _usb_wired_dev_connect_monitor(IfDevice *wired_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;
    if((wired_dev == NULL) || (strlen(wired_dev->dev_name) == 0))
        return IF_FILE_NONE;
#ifdef LINUX_OS
    file_ret = _check_dev_result_file(wired_dev->dev_state, wired_dev->dev_name);
#else
	file_ret = _check_wired_result_file(wired_dev->dev_state, wired_dev->dev_name);
#endif
    return file_ret;
}
#endif

#if (WIFI_SUPPORT > 0)
static bool wifi_running_state = false;
static bool wifi_start_flag = false;
static bool wifi_stop_flag  = true;
static void _net_thread_wifi_start(void* arg)
{
    ApFileData ap_file_cur_data;
    ApFileData ap_file_data;
    ubyte32 dev_name = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strcpy(dev_name, (char*)arg);

    while(1)
    {
        if ( app_network_wifi_check_dev_plug_status(dev_name) != 1)
            break;

        if(wifi_start_flag == true)
        {
            wifi_start_flag = false;
            memset(&ap_file_data, 0, sizeof(ApFileData));
#if WIFI_SUPER_AP_SUPPORT
            if(app_get_wifi_connect_ap_data(dev_name, &ap_file_data, true) < 0)
#else
            if(_get_wifi_connect_file_data(dev_name, &ap_file_data) != GXCORE_SUCCESS) //no ap cfg
#endif
            {
                app_log_debug("[%s] NO AP CONFIG FILE!\n", __func__);
                break;
            }

            if( (wifi_stop_flag == true )
                    || strcmp(ap_file_cur_data.essid,ap_file_data.essid)
                    || strcmp(ap_file_cur_data.mac,ap_file_data.mac)
                    || strcmp(ap_file_cur_data.psk,ap_file_data.psk)
                    || (ap_file_cur_data.hidden != ap_file_data.hidden))
            {
                wifi_stop_flag = false ;

#ifdef LINUX_OS
                if(strlen(dev_name) != 0)
                {
                    char cmd_buf[CMD_LEN] = {0};

                    sprintf(cmd_buf, "%s %s", WIFI_STOP_CMD, dev_name);
                    system(cmd_buf);

                    memset(cmd_buf, 0, sizeof(cmd_buf));
                    sprintf(cmd_buf, "%s %s", WIFI_START_CMD, dev_name);
                    system(cmd_buf);
                }
#else
                extern status_t _wifi_start(const char *wifi_dev, const char *essid, const char *mac, const char *psk, int32_t hidden);
#ifdef APP_NET_WIFI_DEBUG_PROCESS_SUPPORT
                app_wifi_info("thread start %s \n",dev_name);
#endif
				app_network_status_set(IF_FILE_START);
                s_wifi_no_connect_cnt = 0;
                _wifi_start(dev_name, ap_file_data.essid, ap_file_data.mac, ap_file_data.psk, ap_file_data.hidden);
#endif
#ifdef APP_NET_WIFI_DEBUG_PROCESS_SUPPORT
                app_wifi_info("thread stop %s \n",dev_name);
#endif
                memset(&ap_file_cur_data, 0, sizeof(ApFileData));
                strcpy(ap_file_cur_data.essid,ap_file_data.essid);
                strcpy(ap_file_cur_data.mac,ap_file_data.mac);
                strcpy(ap_file_cur_data.psk,ap_file_data.psk);
                ap_file_cur_data.hidden = ap_file_data.hidden;
            }
        }
        GxCore_ThreadDelay(LINK_CONSOLE_DELAY_MS);
    }
    wifi_running_state = false;
#ifdef APP_NET_WIFI_DEBUG_PROCESS_SUPPORT
    app_wifi_info("thread exit %s \n",dev_name);
#endif
    return;
}

static status_t _wifi_dev_start(IfDevice *wifi_dev)
{
    int ret = 0 ;
    ApFileData ap_file_data;
    if((wifi_dev == NULL) || (strlen(wifi_dev->dev_name) == 0))
        return GXCORE_ERROR;

    app_if_up(wifi_dev->dev_name);
    memset(&ap_file_data, 0, sizeof(ApFileData));
#if WIFI_SUPER_AP_SUPPORT
    ret = app_get_wifi_connect_ap_data(wifi_dev->dev_name, &ap_file_data, false);
#else
    ret = _get_wifi_connect_file_data(wifi_dev->dev_name, &ap_file_data) ; // no ap cfg
#endif
    if (ret != GXCORE_SUCCESS )
    {
        if ( ret  == GXCORE_NAME_NOT_MATCH_ERROR )
        {
            app_network_wifi_change_dev_name_file_date(wifi_dev->dev_name);
        }
        return GXCORE_ERROR;
    }

#ifdef ECOS_OS
    if ( app_network_status_get() == IF_FILE_REJ )
    {
        app_network_status_set(IF_FILE_NONE);
    }
#endif
#ifdef APP_NET_WIFI_DEBUG_PROCESS_SUPPORT
    app_wifi_info("wifi start %d %s \n",wifi_running_state,wifi_dev->dev_name);
#endif
    wifi_start_flag = true;
    if(wifi_running_state == false)
    {
        wifi_running_state = true;
        return GxCore_ThreadCreate("wifi_start", &(wifi_dev->start_thread_handle), _net_thread_wifi_start,(void*)(wifi_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
    }
    return GXCORE_SUCCESS;
}

static status_t _wifi_dev_stop(IfDevice *wifi_dev)
{
#ifdef APP_NET_WIFI_DEBUG_PROCESS_SUPPORT
    app_wifi_info("wifi stop %s \n",wifi_dev->dev_name);
#endif
#ifdef LINUX_OS
    char cmd_buf[CMD_LEN] = {0};

    if((wifi_dev == NULL) || (strlen(wifi_dev->dev_name) == 0))
        return GXCORE_ERROR;

    sprintf(cmd_buf, "%s %s", WIFI_STOP_CMD, wifi_dev->dev_name);
    system(cmd_buf);
    //app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
#else
    //TODO: STOP WIFI

	int32_t init_value;
    if((wifi_dev == NULL) || (strlen(wifi_dev->dev_name) == 0))
        return GXCORE_ERROR;

	GxBus_ConfigGetInt(IP_TYPE_WIFI_KEY, &init_value, IP_TYPE_DEFALT);
    app_wifi_linkdown(wifi_dev->dev_name);
	if(init_value == 1)	//dhcp
	{
		network_down(wifi_dev->dev_name);
	}
	else //static ip
	{
		net_ifconfig(wifi_dev->dev_name, "clear", NULL);
	}
#endif
	wifi_stop_flag = true ;

    return GXCORE_SUCCESS;
}

static IfFileRet _wifi_dev_connect_monitor(IfDevice *wifi_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((wifi_dev == NULL) || (strlen(wifi_dev->dev_name) == 0))
        return IF_FILE_NONE;

#ifdef LINUX_OS
    extern int app_check_ap_state(const char *ifname);
    int ret = 0 ;
    file_ret = _check_dev_result_file(wifi_dev->dev_state, wifi_dev->dev_name);
    if(IF_FILE_OK == file_ret) {
        static unsigned int counter = 0;
        s_wifi_plug_change = 0 ;
        ret = app_check_ap_state(wifi_dev->dev_name);
        if(0 == ret) {
            if(counter++ == 10) {
                file_ret = IF_FILE_DISCONNECTED;
                counter = 0;
            }
        }
        else {
            counter = 0;
            if ( (-2 == ret) && s_wifi_plug_change) {
                file_ret = IF_FILE_DISCONNECTED;
            }
        }
    }
#endif

#ifdef ECOS_OS
    file_ret = _check_wifi_result_file(wifi_dev->dev_state, wifi_dev->dev_name);
#endif
    return file_ret;
}
#endif

#if (BLUETOOTH_SUPPORT > 0)
static IfFileRet _check_bluetooth_result_file(IfState if_state, char *dev_name)
{
    IfFileRet ret = -1;

    if(dev_name == NULL)
        return -1;

    int status_bt = app_check_bluetooth_status();

    if ( BT_A2DP_SOURCE_CONNECT_FAILED == status_bt)
    {
        s_bt_connect_failer_count ++  ;
        s_bt_connect_towait_count = 0 ;
    }
    else
    {
        if ( BT_A2DP_SOURCE_CONNETING == status_bt )
        {
            s_bt_connect_towait_count ++ ;
        }
        else
        {
            s_bt_connect_failer_count = 0 ;
            s_bt_connect_towait_count = 0 ;
        }
    }

    if ( (s_bt_connect_failer_count >= 3) || (s_bt_connect_towait_count>=50))
    {
        s_bt_connect_failer_count = 0 ;
        s_bt_connect_towait_count = 0 ;
        return IF_FILE_REJ ;
    }

    switch(status_bt)
    {
        case BT_A2DP_INIT_SUCCESSFUL:
        case BT_A2DP_SCAN_FINISH:
            ret = IF_FILE_START;
            break;
        case BT_A2DP_SOURCE_WAIT:
        case BT_A2DP_SOURCE_CONNETING:
            ret = IF_FILE_WAIT;
            break;
        case BT_A2DP_SOURCE_CONNECT:
        case BT_A2DP_SINK_CONNECT:
            ret = IF_FILE_OK;
            break;
        case BT_A2DP_SOURCE_CONNECT_FAILED:
            ret = IF_FILE_FAIL;
            break;
        case BT_A2DP_SOURCE_DISCONNECT:
        case BT_A2DP_SOURCE_DISCONNECTED:
            ret = IF_FILE_DISCONNECTED;
            break;
        case BT_A2DP_SOURCE_RECONNECT:
            ret = IF_FILE_REJ;
            break;
        default:
            ret = IF_FILE_NONE;
            break;
    }

    return ret;
}

static status_t _set_bluetooth_connect_file_data(const char *bt_dev, ApFileData *ap_data)
{
    //extern int app_bluez_source_connect(ubyte32 mac);
    if(ap_data == NULL)
    {
        return GXCORE_ERROR;
    }
    //app_bluez_source_connect(ap_data->mac);
    GxBus_ConfigSet(BT0_DEV_KEY, bt_dev);
    GxBus_ConfigSet(BT0_SSID_KEY, ap_data->essid);
    GxBus_ConfigSet(BT0_MAC_KEY, ap_data->mac);

    return GXCORE_SUCCESS;
}


static status_t _get_bluetooth_connect_file_data(const char *bt_dev, ApFileData *ap_data)
{
    char ret_buf[RESULT_LEN] = {0};

    GxBus_ConfigGet(BT0_DEV_KEY, ret_buf, sizeof(ret_buf), BT_DEV_DEFAULT);
    if(strcmp(bt_dev, ret_buf) == 0) //bt name
    {
        GxBus_ConfigGet(BT0_MAC_KEY, ap_data->mac, sizeof(ap_data->mac), BT_MAC_DEFAULT);
        if((ap_data->mac != NULL) && (strncmp(ap_data->mac, BT_MAC_DEFAULT, sizeof(BT_MAC_DEFAULT)) == 0))
        {
            return GXCORE_ERROR;
        }
        else
        {
            GxBus_ConfigGet(BT0_SSID_KEY, ap_data->essid, sizeof(ap_data->essid), AP_SSID_DEFAULT);
            return GXCORE_SUCCESS;
        }
    }

    return GXCORE_ERROR;
}

status_t _bluetooth_start(char *bt_dev, char *mac)
{
    char cmd[50] = {0};
    if(bt_dev == NULL || mac == NULL)
        return GXCORE_ERROR;

    app_log_debug("\033[35m[%s]Name  MAC : %s \n\033[0m", bt_dev, mac);

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "BSSID=%s", mac);

    app_bluetoot_start(bt_dev,mac);
    return GXCORE_SUCCESS;
}

static void _net_thread_bluetooth_start(void* arg)
{
    ApFileData ap_file_data;
    ubyte32 dev_name = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strcpy(dev_name, (char*)arg);

    memset(&ap_file_data, 0, sizeof(ApFileData));
    if(_get_bluetooth_connect_file_data(dev_name, &ap_file_data) == GXCORE_ERROR) //no ap cfg
    {
        app_log_debug("[%s] NO BT CONFIG FILE!\n", __func__);
        return;
    }

#ifdef LINUX_OS

#else

    _bluetooth_start(dev_name, ap_file_data.mac);
#endif
}

static status_t _bluetooth_dev_start(IfDevice *bt_dev)
{
    ApFileData ap_file_data;
    if((bt_dev == NULL) || (strlen(bt_dev->dev_name) == 0))
        return GXCORE_ERROR;

   // app_if_up(bt_dev->dev_name);
    memset(&ap_file_data, 0, sizeof(ApFileData));
    if(_get_bluetooth_connect_file_data(bt_dev->dev_name, &ap_file_data) == GXCORE_ERROR)
        return GXCORE_ERROR;

    if ( app_get_bt_security(ap_file_data.mac) == GXCORE_ERROR )
        return GXCORE_ERROR;

    return GxCore_ThreadCreate("bluetooth_start", &(bt_dev->start_thread_handle), _net_thread_bluetooth_start,
                            (void*)(bt_dev->dev_name), 30 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t _bluetooth_dev_stop(IfDevice *bt_dev)
{
#ifdef LINUX_OS
    char cmd_buf[CMD_LEN] = {0};

    if((bt_dev == NULL) || (strlen(bt_dev->dev_name) == 0))
        return GXCORE_ERROR;

    //sprintf(cmd_buf, "%s %s", WIFI_STOP_CMD, bt_dev->dev_name);
    //system(cmd_buf);
    //app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
#else

    if((bt_dev == NULL) || (strlen(bt_dev->dev_name) == 0))
        return GXCORE_ERROR;
    app_bluez_source_link_stop();
#endif
    return GXCORE_SUCCESS;
}

static IfFileRet _bluetooth_dev_connect_monitor(IfDevice *bt_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((bt_dev == NULL) || (strlen(bt_dev->dev_name) == 0))
        return IF_FILE_NONE;

    file_ret = _check_bluetooth_result_file(bt_dev->dev_state, bt_dev->dev_name);
    return file_ret;
}
#endif

#if (MOD_3G_SUPPORT > 0)
static void _net_thread_3g_start(void* arg)
{
    GxCore_ThreadDetach();
	if(arg == NULL)
        return;
#ifdef LINUX_OS
    ubyte32 dev_name = {0};

    strcpy(dev_name, (char*)arg);

    if(strlen(dev_name) != 0)
    {
        char cmd_buf[CMD_LEN] = {0};

        sprintf(cmd_buf, "%s %s", USB3G_START_CMD, dev_name);
        system(cmd_buf);
        //app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    }
#else
    usb3g_usrparm param;
    GxMsgProperty_3G tg_param;
    int id;
    status_t status = GXCORE_ERROR;
    int type_cmb_sel = -1;

    memset(&tg_param, 0, sizeof(GxMsgProperty_3G));
    strlcpy(tg_param.dev_name, (char*)arg, sizeof(tg_param.dev_name));
    GxBus_ConfigGetInt(_3G_OPERATOR_KEY, &id, _3G_OPERATOR_DEFAULT);
    GxBus_ConfigGetInt(_3G_TYPE_KEY, &type_cmb_sel, _3G_TYPE_DEFAULT);
    tg_param.operator_id = id;

	if(type_cmb_sel == 0)
	{
		app_log_flow("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
		usb3g_connect(NULL);
		app_log_flow("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
	}
	else
	{
		status = _get_3g_cfg_file_data(&tg_param);
		if(status == GXCORE_SUCCESS)
		{
			app_log_flow("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
			memset(&param, 0, sizeof(usb3g_usrparm));
			strlcpy(param.APN, tg_param.apn, sizeof(param.APN));
			strlcpy(param.phone, tg_param.phone, sizeof(param.phone));
			strlcpy(param.username, tg_param.username, sizeof(param.username));
			strlcpy(param.password, tg_param.password, sizeof(param.password));
			usb3g_connect(&param);
			app_log_flow("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
		}
	}
#endif
}

static status_t _3g_dev_start(IfDevice *_3g_dev)
{
    if((_3g_dev == NULL) ||(strlen(_3g_dev->dev_name) == 0))
        return GXCORE_ERROR;

#ifdef ECOS_OS
    int type_cmb_sel = -1;

    GxBus_ConfigGetInt(_3G_TYPE_KEY, &type_cmb_sel, _3G_TYPE_DEFAULT);

    if((type_cmb_sel>0)&&(GXCORE_FILE_UNEXIST == GxCore_FileExists(_3G_CONNNECT_RECORD)))
        return GXCORE_ERROR;

#endif
    if((_3g_dev->dev_state == IF_STATE_IDLE) || (_3g_dev->dev_state == IF_STATE_REJ))
        GxCore_ThreadCreate("tg_start", &(_3g_dev->start_thread_handle), _net_thread_3g_start,
                (void*)(_3g_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
    //GxCore_ThreadJoin(_3g_dev->start_thread_handle);
    return GXCORE_SUCCESS;
}

static IfFileRet _3g_dev_connect_monitor(IfDevice *_3g_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((_3g_dev == NULL) || (strlen(_3g_dev->dev_name) == 0))
        return IF_FILE_NONE;

#ifdef  LINUX_OS
    file_ret = _check_dev_result_file(_3g_dev->dev_state, _3g_dev->dev_name);
#else
    file_ret = _check_3g_result_file(_3g_dev->dev_state, _3g_dev->dev_name);
#endif
    return file_ret;
}

static status_t _3g_dev_stop(IfDevice *_3g_dev)
{
    if((_3g_dev == NULL) || (strlen(_3g_dev->dev_name) == 0))
        return GXCORE_ERROR;
#ifdef LINUX_OS
    char cmd_buf[CMD_LEN] = {0};

	sprintf(cmd_buf, "%s %s", USB3G_STOP_CMD, _3g_dev->dev_name);
	//app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
	system(cmd_buf);

#else
    if(thiz_usb3g_hotplug_status == true)
    {
        app_log_flow("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
        usb3g_disconnect();
        app_log_flow("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
    }
#endif
    return GXCORE_SUCCESS;
}
#ifdef ECOS_OS
int app_network_3g_stop(void)
{
	usb3g_disconnect();
    return GXCORE_SUCCESS;
}
#endif

#ifdef LINUX_OS
static void _net_thread_eth3g_start(void* arg)
{
    ubyte32 dev_name = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strcpy(dev_name, (char*)arg);

    if(strlen(dev_name) != 0)
    {
        char cmd_buf[CMD_LEN] = {0};

        sprintf(cmd_buf, "%s %s", ETH3G_START_CMD, dev_name);
        system(cmd_buf);
        //app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    }
}

static status_t _eth3g_dev_start(IfDevice *_eth3g_dev)
{
    if((_eth3g_dev == NULL) ||(strlen(_eth3g_dev->dev_name) == 0))
        return GXCORE_ERROR;

    return GxCore_ThreadCreate("etg_start", &(_eth3g_dev->start_thread_handle), _net_thread_eth3g_start,
                            (void*)(_eth3g_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t _eth3g_dev_stop(IfDevice *_eth3g_dev)
{
	char cmd_buf[CMD_LEN] = {0};

	if((_eth3g_dev == NULL) || (strlen(_eth3g_dev->dev_name) == 0))
		return GXCORE_ERROR;

	sprintf(cmd_buf, "%s %s", ETH3G_STOP_CMD, _eth3g_dev->dev_name);
	//app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
	system(cmd_buf);

    return GXCORE_SUCCESS;
}

static IfFileRet _eth3g_dev_connect_monitor(IfDevice *_eth3g_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((_eth3g_dev == NULL) || (strlen(_eth3g_dev->dev_name) == 0))
        return IF_FILE_NONE;

    file_ret = _check_dev_result_file(_eth3g_dev->dev_state, _eth3g_dev->dev_name);

    return file_ret;
}
#endif
#endif

#if (RNDIS_SUPPORT > 0)
#ifdef LINUX_OS
static void _net_thread_rndis_start(void* arg)
{
    ubyte32 dev_name = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strcpy(dev_name, (char*)arg);

    if(strlen(dev_name) != 0)
    {
        char cmd_buf[CMD_LEN] = {0};

        sprintf(cmd_buf, "%s %s", "rndis_start", "usb0");
        system(cmd_buf);
        //app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    }
}

static status_t _rndis_dev_start(IfDevice *_rndis_dev)
{
    if((_rndis_dev == NULL) ||(strlen(_rndis_dev->dev_name) == 0))
        return GXCORE_ERROR;

    return GxCore_ThreadCreate("rndis_start", &(_rndis_dev->start_thread_handle), _net_thread_rndis_start,
                            (void*)(_rndis_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
}

static status_t _rndis_dev_stop(IfDevice *_rndis_dev)
{
	char cmd_buf[CMD_LEN] = {0};

	if((_rndis_dev == NULL) || (strlen(_rndis_dev->dev_name) == 0))
		return GXCORE_ERROR;

	sprintf(cmd_buf, "%s %s", "rndis_stop", "usb0");
	//app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
	system(cmd_buf);

    return GXCORE_SUCCESS;
}

static IfFileRet _rndis_dev_connect_monitor(IfDevice *_rndis_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((_rndis_dev == NULL) || (strlen(_rndis_dev->dev_name) == 0))
        return IF_FILE_NONE;

    file_ret = _check_dev_result_file(_rndis_dev->dev_state, _rndis_dev->dev_name);
    return file_ret;
}
#endif
#endif

#if  GPRS_SUPPORT
static IfFileRet _check_gprs_result_file(IfState if_state, char *dev_name)
{
    IfFileRet file_ret = IF_FILE_NONE;
    IfState state_gprs = IF_STATE_INVALID;
    static IfState old_state_gprs = IF_STATE_INVALID;

    state_gprs = gprs_getstate();
    switch(state_gprs)
    {
        case IF_STATE_START:
            file_ret = IF_FILE_START;
            break;

        case IF_STATE_CONNECTING:
            file_ret = IF_FILE_WAIT;
            break;

        case IF_STATE_CONNECTED:
            file_ret = IF_FILE_OK;
            if(old_state_gprs!=state_gprs)
            {
#if MOD_3G_SUPPORT
                _usb3g_set_ip_cfg_file_data(dev_name);
#endif
            }
            break;

        case IF_STATE_REJ:
            file_ret = IF_FILE_REJ;
            break;

        default:
            file_ret = IF_FILE_NONE;
            break;
    }

    old_state_gprs = state_gprs;
    return file_ret;
}

static void _net_thread_gprs_start(void* arg)
{
	GxCore_ThreadDetach();

	gprs_connect();
}

static status_t _gprs_dev_start(IfDevice *_gprs_dev)
{
    if((_gprs_dev == NULL) ||(strlen(_gprs_dev->dev_name) == 0))
        return GXCORE_ERROR;

    //if(GXCORE_FILE_UNEXIST == GxCore_FileExists(_GPRS_CONNNECT_RECORD))
        //return GXCORE_ERROR;

    if((_gprs_dev->dev_state == IF_STATE_IDLE) || (_gprs_dev->dev_state == IF_STATE_REJ))
        GxCore_ThreadCreate("gprs_start", &(_gprs_dev->start_thread_handle), _net_thread_gprs_start,
                (void*)(_gprs_dev->dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);
    //GxCore_ThreadJoin(_3g_dev->start_thread_handle);

    return GXCORE_SUCCESS;
}

static status_t _gprs_dev_stop(IfDevice *_gprs_dev)
{
    if((_gprs_dev == NULL) || (strlen(_gprs_dev->dev_name) == 0))
	  return GXCORE_ERROR;

	if(thiz_gprs_startup == true)
	{
		app_log_flow("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
		gprs_disconnect();
		app_log_flow("!!!!!!!!![%s]%d!!!!!!!!\n", __func__, __LINE__);
	}

	return GXCORE_SUCCESS;
}

static IfFileRet _gprs_dev_connect_monitor(IfDevice *_gprs_dev)
{
    IfFileRet file_ret = IF_FILE_NONE;

    if((_gprs_dev == NULL) || (strlen(_gprs_dev->dev_name) == 0))
        return IF_FILE_NONE;

    file_ret = _check_gprs_result_file(_gprs_dev->dev_state, _gprs_dev->dev_name);

    return file_ret;
}
#endif

static bool _if_dev_list_check(const char *dev_name)
{
    IfDevList *p = if_dev_list;
    if((dev_name == NULL) || (p == NULL))
        return false;

    GxCore_MutexLock(s_dev_list_mutex);
    while(p != NULL)
    {
        if(strcmp(p->net_dev.dev_name, dev_name) == 0)
        {
            GxCore_MutexUnlock(s_dev_list_mutex);
            return true;
        }
        p = p->next;
    }
    GxCore_MutexUnlock(s_dev_list_mutex);

    return false;
}

static status_t _if_dev_list_add(IfDevice *new_dev)
{
    IfDevList *p = if_dev_list;
    IfDevList *p_dev_node = NULL;

    if(new_dev == NULL)
    {
        return GXCORE_ERROR;
    }

    GxCore_MutexLock(s_dev_list_mutex);

    if(if_dev_list == NULL)
    {
        if_dev_list = (IfDevList*)GxCore_Malloc(sizeof(IfDevList));
        if(if_dev_list == NULL)
        {
            GxCore_MutexUnlock(s_dev_list_mutex);
            return GXCORE_ERROR;
        }
        memcpy(&(if_dev_list->net_dev), new_dev, sizeof(IfDevice));
        if_dev_list->next = NULL;
    }
    else
    {
        while(p->next != NULL)
        {
            p = p->next;
        }

        p_dev_node = (IfDevList*)GxCore_Malloc(sizeof(IfDevList));
        if(p_dev_node == NULL)
        {
            GxCore_MutexUnlock(s_dev_list_mutex);
            return GXCORE_ERROR;
        }
        memcpy(&(p_dev_node->net_dev), new_dev, sizeof(IfDevice));
        p_dev_node->next = NULL;
        p->next = p_dev_node;
    }

    GxCore_MutexUnlock(s_dev_list_mutex);
    return GXCORE_SUCCESS;
}

static status_t _if_dev_list_del(const char *dev_name)
{
    IfDevList *p = if_dev_list;
    IfDevList *p_dev = NULL;

    if((dev_name == NULL) || (p == NULL))
    {
        return GXCORE_ERROR;
    }

    GxCore_MutexLock(s_dev_list_mutex);
    if(strcmp(if_dev_list->net_dev.dev_name, dev_name) == 0)
    {
        p_dev = if_dev_list;
        if_dev_list = if_dev_list->next;
        GxCore_Free(p_dev);
    }
    else
    {
        while(p->next != NULL)
        {
            if(strcmp(p->next->net_dev.dev_name, dev_name) == 0)
            {
                p_dev = p->next;
                p->next = p->next->next;
                GxCore_Free(p_dev);
                break;
            }
            p = p->next;
        }
    }

    GxCore_MutexUnlock(s_dev_list_mutex);
    return GXCORE_SUCCESS;
}

//plz GxCore_Free result if != NULL
static IfDevice* _if_dev_list_get(const char *dev_name)
{
    IfDevList *p = if_dev_list;
    IfDevice *p_ret = NULL;

    if((dev_name == NULL) || (p == NULL))
    {
        return NULL;
    }

    GxCore_MutexLock(s_dev_list_mutex);

    while(p != NULL)
    {
        if(strcmp(p->net_dev.dev_name, dev_name) == 0)
        {
            p_ret = (IfDevice*)GxCore_Malloc(sizeof(IfDevice));
            if(p_ret != NULL)
            {
                memcpy(p_ret, &(p->net_dev), sizeof(IfDevice));
            }
            break;
        }
        p = p->next;
    }

    GxCore_MutexUnlock(s_dev_list_mutex);

    return p_ret;
}

static status_t _if_dev_list_modify(IfDevice *new_dev)
{
    IfDevList *p = if_dev_list;
    status_t ret = GXCORE_ERROR;

    if((new_dev == NULL) || (p == NULL))
    {
        return GXCORE_ERROR;
    }

    GxCore_MutexLock(s_dev_list_mutex);
    while(p != NULL)
    {
        if(strcmp(p->net_dev.dev_name, new_dev->dev_name) == 0)
        {
            memcpy(&(p->net_dev), new_dev, sizeof(IfDevice));
            ret = GXCORE_SUCCESS;
            break;
        }
        p = p->next;
    }
    GxCore_MutexUnlock(s_dev_list_mutex);

    return ret;
}

static status_t _lock_if_dev(const char *dev_name)
{
    IfDevice *p_dev = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if((p_dev = _if_dev_list_get(dev_name)) == NULL)
        return GXCORE_ERROR;

    GxCore_MutexLock(p_dev->dev_mutex);
    GxCore_Free(p_dev);
    p_dev = NULL;
    return GXCORE_SUCCESS;
}

static status_t _unlock_if_dev(const char *dev_name)
{
    IfDevice *p_dev = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if((p_dev = _if_dev_list_get(dev_name)) == NULL)
        return GXCORE_ERROR;

    GxCore_MutexUnlock(p_dev->dev_mutex);
    GxCore_Free(p_dev);
    p_dev = NULL;
    return GXCORE_SUCCESS;
}

#ifdef LINUX_OS
static char *_get_ifname(char *    name,   /* Where to store the name */
          int   nsize,  /* Size of name buffer */
          char *    buf)    /* Current position in buffer */
{
  char *    end;

  /* Skip leading spaces */
  while(isspace(*buf))
    buf++;

//#ifndef IW_RESTRIC_ENUM
  /* Get name up to the last ':'. Aliases may contain ':' in them,
   * but the last one should be the separator */
  end = strrchr(buf, ':');
//#else
  /* Get name up to ": "
   * Note : we compare to ": " to make sure to process aliased interfaces
   * properly. Doesn't work on /proc/net/dev, because it doesn't guarantee
   * a ' ' after the ':'*/
  //end = strstr(buf, ": ");
//#endif

  /* Not found ??? To big ??? */
  if((end == NULL) || (((end - buf) + 1) > nsize))
    return(NULL);

  /* Copy */
  memcpy(name, buf, (end - buf));
  name[end - buf] = '\0';

  /* Return value currently unused, just make sure it's non-NULL */
  return(end);
}
#endif

static int _get_prev_init_dev(ubyte64 *net_dev)
{
#ifdef LINUX_OS
    FILE *read_fp;
    char cmdline[60] = {0};
    ubyte64 dev_buf = {0};
    int nfs_dev_num = 0;
    char *p = NULL;

    if(net_dev == NULL)
    {
        return 0;
    }

    strcpy(cmdline, "route -n | grep -w U | awk '{print $8}'");
    read_fp = popen(cmdline, "r");
    if (read_fp != NULL)
    {
        while(fgets(dev_buf, sizeof(dev_buf), read_fp))
        {
            p = strchr(dev_buf, '\n');
            if(p != NULL)
                *p = '\0';
            strcpy(net_dev[nfs_dev_num], dev_buf);
            //app_log_debug("~~~~~~~~~[%s]%d nfs_dev = %s~~~~~~~~~\n", __func__, __LINE__, net_dev[nfs_dev_num]);
            nfs_dev_num++;
        }
    }

    pclose(read_fp);

    //app_log_debug("~~~~~~~~~[%s]%d nfs_dev_num = %d ~~~~~~~~~\n", __func__, __LINE__, nfs_dev_num);
    return nfs_dev_num;
#else
    return 0;
#endif
}

#if 0
static void _delete_prev_init_dev(const char *dev_name)
{
    ubyte64 *temp_dev = NULL;
    int i = 0;

    if((dev_name == NULL) || (strlen(dev_name) == 0) || (s_prev_init_dev.dev_num == 0))
        return;

    temp_dev = GxCore_Calloc(s_prev_init_dev.dev_num, sizeof(ubyte64));
    if(temp_dev != NULL)
    {
        memcpy(temp_dev, s_prev_init_dev.net_dev, s_prev_init_dev.dev_num * sizeof(ubyte64));
        memset(s_prev_init_dev.net_dev, 0, s_prev_init_dev.dev_num * sizeof(ubyte64));
        s_prev_init_dev.dev_num = 0;

        for(i = 0; i < s_prev_init_dev.dev_num; i++)
        {
            if(strcmp(temp_dev[i], dev_name) != 0)
            {
                strcpy(s_prev_init_dev.net_dev[s_prev_init_dev.dev_num++], temp_dev[i]);
            }
        }
    }

    return;
}
#endif

static bool _check_prev_init_dev(const char *dev_name)
{
    int i = 0;
    bool ret = false;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
        return false;

    for(i = 0; i < s_prev_init_dev.dev_num; i++)
    {
        if(strcmp(dev_name, s_prev_init_dev.net_dev[i]) == 0)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

IfType app_if_dev_check_type(const char *dev_name)
{
    IfType ret = IF_TYPE_UNKOWN;
    if(dev_name == NULL)
        return ret;

    if(strncmp(dev_name, NET_IF_DEV_ETH0, 4) == 0)
    {
#if (ETH_SUPPORT > 0)
        ret = IF_TYPE_WIRED;
#endif
    }
    else if(strncmp(dev_name, NET_IF_DEV_ETH1, 4) == 0)
    {
#if (USB_ETH_SUPPORT > 0)
        ret = IF_TYPE_USB_WIRED;
#endif
    }

#if (WIFI_SUPPORT > 0)
    else if(strncmp(dev_name, "ra", 2) == 0)
    {
        ret = IF_TYPE_WIFI;
    }
    else if(strncmp(dev_name, "wlan", 4) == 0)
    {
        ret = IF_TYPE_WIFI;
    }
#endif
#if (MOD_3G_SUPPORT > 0)
    else if(strncmp(dev_name, USB3G_DEV_HEAD, strlen(USB3G_DEV_HEAD)) == 0)
    {
        ret = IF_TYPE_3G;
    }
#endif
#if (RNDIS_SUPPORT > 0)
#ifdef  LINUX_OS
    else if(strncmp(dev_name, NET_IF_DEV_USB0, strlen(NET_IF_DEV_USB0)) == 0)
    {
        ret = IF_TYPE_RNDIS;
    }
#endif
#endif
#if (BLUETOOTH_SUPPORT > 0)
    else if(strncmp(dev_name, net_if_dev_usb_bt, strlen(net_if_dev_usb_bt)) == 0)
    {
        ret = IF_TYPE_BLUETOOTH;
    }
#endif
#if (GPRS_SUPPORT > 0)
#ifdef  LINUX_OS
#else
	else if(strncmp(dev_name, GPRS_DEV_HEAD, strlen(GPRS_DEV_HEAD)) == 0)
	{
		ret = IF_TYPE_GPRS;
	}
#endif
#endif

    return ret;
}

static bool _check_dev_user_flag(const char *dev_name)
{
    IpCfgFileData cfg_data;
    bool ret = false;
    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return false;
    }

    memset(&cfg_data, 0, sizeof(IpCfgFileData));
    if(_get_ip_cfg_file_data(dev_name, &cfg_data) == GXCORE_SUCCESS)
    {
        (cfg_data.dev_mode == 0) ? (ret = false) : (ret = true);
    }
    return ret;
}

static bool _check_dev_main_flag(const char *dev_name)
{
    if(dev_name == NULL)
    {
        return false;
    }

    if(strcmp(dev_name, NET_IF_DEV_ETH0) == 0)
        return true;

    return false;
}

#ifdef ECOS_OS
#if (WIFI_SUPPORT > 0)

static void _network_check_wifi_dev(char *name)
{
    if (name == NULL )
        return ;

    if ( app_network_wifi_check_dev_plug_status(name) != 1 )
        return ;

    //TODO:Get Dev List From ECOS
    if(_if_dev_list_check(name) == false)
    {
        IfType dev_type = app_if_dev_check_type(name);
        if(dev_type != IF_TYPE_UNKOWN)
        {
            IfDevice dev;
            ubyte32 mac = {0};
            char gateway[20] = {0};

            memset(&dev, 0, sizeof(IfDevice));
            strlcpy(dev.dev_name, name, IFNAMSIZ);
            dev.dev_type = dev_type;
            dev.dev_priority = WIFI_PRIORITY;
			#if (USB_ETH_SUPPORT > 0)
            if(dev_type == IF_TYPE_USB_WIRED)
                dev.dev_priority = USB_WIRED_PRIORITY;
            #endif
            //if(dev_type == IF_TYPE_3G)
            //    dev.dev_priority = TG_PRIORITY;

            dev.user_flag = _check_dev_user_flag(name);
            dev.main_flag = _check_dev_main_flag(name);
            dev.dev_ops = &s_net_ops_set[dev_type];
            dev.start_thread_handle = -1;
            dev.scan_thread_handle = -1;
            dev.cur_flag = false;
            dev.dev_state = IF_STATE_INVALID;
            dev.id = app_network_wifi_get_dev_id(name);

            if(app_get_gateway(name, gateway) == GXCORE_SUCCESS)
            {
                if(s_global_net_state != IF_STATE_CONNECTED)
                {
                    dev.dev_state = IF_STATE_CONNECTED;
                    s_global_net_state = IF_STATE_CONNECTED;
                    dev.dev_priority = MIN_PRIORITY;
                    s_network_dev_priority = MIN_PRIORITY;
                    dev.cur_flag = true;
                }
            }
            else
            {
                if(_check_prev_init_dev(dev.dev_name) == true)
                    dev.dev_state = IF_STATE_CONNECTING;
            }

            GxCore_MutexCreate(&dev.dev_mutex);

            if(_network_get_mac_cfg(name, mac) == GXCORE_SUCCESS)
            {
                //app_log_debug("~~~~~~[%s]%d, %s mac: %s !!!!!\n", __func__, __LINE__, name, mac);
                _network_set_mac(name, mac);
            }

            _if_dev_list_add(&dev);
        }
    }
    else
    {
        bool main_flag = _check_dev_main_flag(name);
        IfDevice *temp_dev = _if_dev_list_get(name);

        if(temp_dev->main_flag != main_flag)
        {
            temp_dev->main_flag = main_flag;
            _if_dev_list_modify(temp_dev);
        }
        GxCore_Free(temp_dev);
        temp_dev = NULL;
    }
}

static void _network_check_all_wifi_dev(void)
{
    int i = 0 ;
    int id = 0 ;
    id = s_wifi_dev_active_id ;
    if ( (id >= 0) && (id < WIFI_MAX_DEV_NUM) && (s_wifi_dev_plug[id]) )
    {
        _network_check_wifi_dev(s_wifi_dev_name[id]);
    }

    for(i=0;i<WIFI_MAX_DEV_NUM;i++)
    {
        if ( s_wifi_dev_active_id == i )
            continue ;
        if ( s_wifi_dev_plug[i] == 0 )
            continue ;

        _network_check_wifi_dev(s_wifi_dev_name[i]);
    }
}
#endif

#if (BLUETOOTH_SUPPORT > 0)
static void _network_check_bluetooth_dev(void)
{
    ubyte64 name = {0};
    if(this_bt_device.init_st == -1)
        return;
    //TODO:Get Dev List From ECOS
    snprintf(name, sizeof(ubyte64), "%s", net_if_dev_usb_bt);
    if(_if_dev_list_check(name) == false)
    {
        IfType dev_type = app_if_dev_check_type(name);
        if(dev_type != IF_TYPE_UNKOWN)
        {
            IfDevice dev;

            memset(&dev, 0, sizeof(IfDevice));
            strlcpy(dev.dev_name, name, IFNAMSIZ);
            dev.dev_type = dev_type;
            dev.dev_priority = BLUETOOTH_PRIORITY;
            dev.user_flag = _check_dev_user_flag(name);
            dev.main_flag = _check_dev_main_flag(name);
            dev.dev_ops = &s_net_ops_set[dev_type];
            dev.start_thread_handle = -1;
            dev.scan_thread_handle = -1;
            dev.cur_flag = false;
            dev.dev_state = IF_STATE_INVALID;
            dev.id = this_bt_device.plug_id;


            GxCore_MutexCreate(&dev.dev_mutex);

            _if_dev_list_add(&dev);
        }
    }
    else
    {
        bool main_flag = _check_dev_main_flag(name);
        IfDevice *temp_dev = _if_dev_list_get(name);

        if(temp_dev->main_flag != main_flag)
        {
            temp_dev->main_flag = main_flag;
            _if_dev_list_modify(temp_dev);
        }
        GxCore_Free(temp_dev);
        temp_dev = NULL;
    }

}
#endif

#if ((USB_ETH_SUPPORT > 0)||(ETH_SUPPORT > 0))
extern unsigned int synopGMAC_get_link(void);
extern int usbnet_get_linkstatus(void);
static int _network_wired_check_link(void)
{
	int i = 5,b = 0,c = 0;
	while(i--)
	{
		if(0 == synopGMAC_get_link())
		{
			b++;
		}
		else
		{
			c++;
		}
	}
	if(b >= c)
		b = 0 ;
	else
		b = 1;
	return b;// 0:unlink; 1:link
}
static int network_check_cable_link(const char *dev_name)
{
	int i = 5,b = 0,c = 0;
	if((dev_name != NULL)&&(strcmp(NET_IF_DEV_ETH0, dev_name) == 0))
	{
		b = _network_wired_check_link();
	}
	else
	{
		while(i--)
		{
			if(0 == usbnet_get_linkstatus())
			{
				b++;
			}
			else
			{
				c++;
			}
		}
		if(b >= c)
			b = 0 ;
		else
			b = 1;
	}
	return b;// 0:unlink; 1:link
}
static void _network_check_wired_dev(const char *dev_name)
{
    ubyte64 name = {0};

	if((dev_name != NULL)&&(strcmp(NET_IF_DEV_ETH0, dev_name) == 0))
    {
        wired_cable_link = network_check_cable_link(dev_name);
        if(wired_cable_link == 0)
	    {
            network_down((char *)dev_name);
            return;
	    }
    }
	if((dev_name != NULL)&&(strcmp(NET_IF_DEV_ETH1, dev_name) == 0))
    {
	    usb_wired_cable_link = network_check_cable_link(dev_name);
        if(usb_wired_cable_link == 0)
	    {
            network_down((char *)dev_name);
            return;
	    }

    }
    //TODO:Get Dev List From ECOS
    snprintf(name, sizeof(ubyte64), "%s", dev_name);
    if(_if_dev_list_check(name) == false)
    {
        IfType dev_type = app_if_dev_check_type(name);
        if(dev_type != IF_TYPE_UNKOWN)
        {
            IfDevice dev;
            ubyte32 mac = {0};
            char gateway[20] = {0};

            memset(&dev, 0, sizeof(IfDevice));
            strlcpy(dev.dev_name, name, IFNAMSIZ);
            dev.dev_type = dev_type;

#if (ETH_SUPPORT > 0)
            if(dev_type == IF_TYPE_WIRED)
                dev.dev_priority = WIRED_PRIORITY;
#endif
#if (USB_ETH_SUPPORT > 0)
            if(dev_type == IF_TYPE_USB_WIRED)
                dev.dev_priority = USB_WIRED_PRIORITY;
#endif

            dev.user_flag = _check_dev_user_flag(name);
            dev.main_flag = _check_dev_main_flag(name);
            dev.dev_ops = &s_net_ops_set[dev_type];
            dev.start_thread_handle = -1;
            dev.scan_thread_handle = -1;
            dev.cur_flag = false;
            dev.dev_state = IF_STATE_INVALID;

            if(app_get_gateway(name, gateway) == GXCORE_SUCCESS)
            {
                if(s_global_net_state != IF_STATE_CONNECTED)
                {
                    dev.dev_state = IF_STATE_CONNECTED;
                    s_global_net_state = IF_STATE_CONNECTED;
                    dev.dev_priority = MIN_PRIORITY;
                    s_network_dev_priority = MIN_PRIORITY;
                    dev.cur_flag = true;
                }
            }
            else
            {
                if(_check_prev_init_dev(dev.dev_name) == true)
                    dev.dev_state = IF_STATE_CONNECTING;
            }

            GxCore_MutexCreate(&dev.dev_mutex);

            if(_network_get_mac_cfg(name, mac) == GXCORE_SUCCESS)
            {
                //app_log_debug("~~~~~~[%s]%d, %s mac: %s !!!!!\n", __func__, __LINE__, name, mac);
                _network_set_mac(name, mac);
            }

            _if_dev_list_add(&dev);
        }
    }
    else
    {
        bool main_flag = _check_dev_main_flag(name);
        IfDevice *temp_dev = _if_dev_list_get(name);

        if(temp_dev->main_flag != main_flag)
        {
            temp_dev->main_flag = main_flag;
            _if_dev_list_modify(temp_dev);
        }
        GxCore_Free(temp_dev);
        temp_dev = NULL;
    }
}
#endif
#endif

#if (MOD_3G_SUPPORT > 0)
static bool _network_usb3g_lookup(char *dev_name, int nsize)
{
    bool ret = false;
    if(thiz_usb3g_hotplug_status == true)
    {
        if(dev_name != NULL)
        {
            strlcpy(dev_name, USB3G_DEV_HEAD, nsize);
        }
        ret = true;
    }
    return ret;
}

static void _network_check_3g_dev(void)
{
	IfDevice dev;

	memset(&dev, 0, sizeof(IfDevice));
	if (_network_usb3g_lookup(dev.dev_name, sizeof(dev.dev_name)))
	{
		if(_if_dev_list_check(dev.dev_name) == false)
		{
			dev.dev_state = IF_STATE_INVALID;
			dev.dev_type = IF_TYPE_3G;
            dev.dev_priority = TG_PRIORITY;
			dev.user_flag = _check_dev_user_flag(dev.dev_name);
			dev.main_flag = false;//_check_dev_main_flag(name);
			dev.cur_flag = false;
			dev.dev_ops = &s_net_ops_set[IF_TYPE_3G];
			dev.start_thread_handle = -1;
			dev.scan_thread_handle = -1;
			GxCore_MutexCreate(&dev.dev_mutex);

			_if_dev_list_add(&dev);
		}
		else
		{
			//do nothing
		}
	}
}
#endif

#if (GPRS_SUPPORT > 0)
#ifdef LINUX_OS
#else
static bool _network_gprs_lookup(char *dev_name, int nsize)
{
	gprs_fun func_list[] = {
		g_AppGprs.at,
		g_AppGprs.cpin,
		g_AppGprs.creg,
		//g_AppGprs.cgdcont,
		//g_AppGprs.atd,
		//g_AppGprs.switchtocmd,
		//g_AppGprs.at,
	};
	int i = 0;
	int func_num = sizeof(func_list)/sizeof(gprs_fun);

	if(thiz_gprs_startup== false)
	{
		g_AppGprs.init(115200);

		for(i = 0; i < func_num; i++)
		{
			if(func_list[i]() != APP_GPRS_OK)
				break;
		}

		if(i >= func_num)
		{
			g_AppGprs.force_exit();
			thiz_gprs_startup = true;
			if(dev_name != NULL)
			{
				strlcpy(dev_name, GPRS_DEV_HEAD, nsize);
			}
		}
	}
	return thiz_gprs_startup;
}
static void _network_check_gprs_dev(void)
{
	IfDevice dev;

	memset(&dev, 0, sizeof(IfDevice));
	if (_network_gprs_lookup(dev.dev_name, sizeof(dev.dev_name)))
	{
		strlcpy(dev.dev_name, GPRS_DEV_HEAD, sizeof(dev.dev_name));
		if(_if_dev_list_check(dev.dev_name) == false)
		{
			dev.dev_state = IF_STATE_INVALID;
			dev.dev_type = IF_TYPE_GPRS;
            		dev.dev_priority = GPRS_PRIORITY;
			dev.user_flag = _check_dev_user_flag(dev.dev_name);
			dev.main_flag = false;//_check_dev_main_flag(name);
			dev.cur_flag = false;
			dev.dev_ops = &s_net_ops_set[IF_TYPE_GPRS];
			dev.start_thread_handle = -1;
			dev.scan_thread_handle = -1;
			GxCore_MutexCreate(&dev.dev_mutex);

			_if_dev_list_add(&dev);
		}
		else
		{
			//do nothing
		}
	}
}
#endif
#endif


static void _network_check_all_dev(void)
{
#ifdef LINUX_OS
    char      buff[128] = {0};
    FILE *    fh = NULL;

    fh = fopen(PROC_NET_DEV, "r");

    if(fh != NULL)
    {
        fgets(buff, sizeof(buff), fh);
        fgets(buff, sizeof(buff), fh);

        /* Read each device line */
        while(fgets(buff, sizeof(buff), fh))
        {
            ubyte64  name = {0};
            char *s = NULL;

            /* Skip empty or almost empty lines. It seems that in some
             * * cases fgets return a line with only a newline. */

            if((buff[0] == '\0') || (buff[1] == '\0'))
                continue;

            s = _get_ifname(name, sizeof(name), buff);
            if(!s)
            {
                //app_log_debug("Cannot parse PROC_NET_DEV \n");
            }
            else
            {
                if(strcmp(name, NET_IF_DEV_LOOP) == 0 || _check_prev_init_dev(name))
                {
                    continue;
                }

                if((strncmp(name, NET_IF_DEV_ETH0, 3) == 0) && (!app_check_netlink(name)))
                {
                    continue;
                }
                if((_if_dev_list_check(name) == false))
                {
                    IfType dev_type = app_if_dev_check_type(name);
                    if(dev_type != IF_TYPE_UNKOWN)
                    {
                        IfDevice dev;
                        ubyte32 mac = {0};
                        char gateway[20] = {0};

                        memset(&dev, 0, sizeof(IfDevice));
                        //strncpy(dev.dev_name, name, IFNAMSIZ);
                        strlcpy(dev.dev_name, name, sizeof(ubyte64));
                        dev.dev_type = dev_type;

#if (ETH_SUPPORT > 0)
                        if(dev_type == IF_TYPE_WIRED)
                            dev.dev_priority = WIRED_PRIORITY;
#endif
#if (WIFI_SUPPORT > 0)
                        if(dev_type == IF_TYPE_WIFI)
                        {
                            dev.dev_priority = WIFI_PRIORITY;
                            dev.id = app_network_wifi_get_dev_id(name);
                        }
#endif
#if (USB_ETH_SUPPORT > 0)
                        if(dev_type == IF_TYPE_USB_WIRED)
                            dev.dev_priority = USB_WIRED_PRIORITY;
#endif
#if (MOD_3G_SUPPORT > 0)
                        if(dev_type == IF_TYPE_ETH3G)
                            dev.dev_priority = TG_PRIORITY;
#endif
                        //if(dev_type == IF_TYPE_3G)
                        //    dev.dev_priority = TG_PRIORITY;
#if (RNDIS_SUPPORT > 0)
#ifdef LINUX_OS
                        if(dev_type == IF_TYPE_RNDIS)
                            dev.dev_priority = RNDIS_PRIORITY;
#endif
#endif
                        dev.user_flag = _check_dev_user_flag(name);
                        dev.main_flag = _check_dev_main_flag(name);
                        dev.dev_ops = &s_net_ops_set[dev_type];
                        dev.start_thread_handle = -1;
                        dev.scan_thread_handle = -1;
                        dev.cur_flag = false;
                        dev.dev_state = IF_STATE_INVALID;

                        if(app_get_gateway(name, gateway) == GXCORE_SUCCESS)
                        {
                            if(s_global_net_state != IF_STATE_CONNECTED)
                            {
                                dev.dev_state = IF_STATE_CONNECTED;
                                s_global_net_state = IF_STATE_CONNECTED;
                                dev.dev_priority = MIN_PRIORITY;
                                s_network_dev_priority = MIN_PRIORITY;
                                dev.cur_flag = true;
                            }
                        }
                        else
                        {
                            if(_check_prev_init_dev(dev.dev_name) == true)
                                dev.dev_state = IF_STATE_CONNECTING;
                        }

                        GxCore_MutexCreate(&dev.dev_mutex);

                        if(_network_get_mac_cfg(name, mac) == GXCORE_SUCCESS)
                        {
                            //app_log_debug("~~~~~~[%s]%d, %s mac: %s !!!!!\n", __func__, __LINE__, name, mac);
                            _network_set_mac(name, mac);
                        }

                        _if_dev_list_add(&dev);
                    }
                }
                else
                {
                    bool main_flag = _check_dev_main_flag(name);
                    IfDevice *temp_dev = _if_dev_list_get(name);

                    if(temp_dev->main_flag != main_flag)
                    {
                        temp_dev->main_flag = main_flag;
                        _if_dev_list_modify(temp_dev);
                    }
                    GxCore_Free(temp_dev);
                    temp_dev = NULL;
                }
            }
        }
        fclose(fh);
    }

#else //ECOS_OS
#if (WIFI_SUPPORT > 0)
    _network_check_all_wifi_dev();
#endif
#if (BLUETOOTH_SUPPORT > 0)
    _network_check_bluetooth_dev();
#endif
#if (defined DYNAMIC_DRIVER_CONTROL)
    {
#if ((USB_ETH_SUPPORT > 0)||(ETH_SUPPORT > 0))
#if ETH_SUPPORT
        extern int get_enable_eth(void);
        if(get_enable_eth())
        {
            _network_check_wired_dev(NET_IF_DEV_ETH0);
        }
        else
#endif
        {
            _network_check_wired_dev(NET_IF_DEV_ETH1);
        }
#endif
    }
#else
#if (ETH_SUPPORT > 0)
    _network_check_wired_dev(NET_IF_DEV_ETH0);
#endif

#if (USB_ETH_SUPPORT > 0)
    _network_check_wired_dev(NET_IF_DEV_ETH1);
#endif
#endif

#if 0//(RNDIS_SUPPORT > 0)
	_network_check_rndis_dev(NET_IF_DEV_USB0);
#endif
#endif

#if (MOD_3G_SUPPORT > 0)
    //check 3g dev
	_network_check_3g_dev();
#endif
#if (GPRS_SUPPORT > 0)
#ifdef LINUX_OS
#else
    //check gprs dev
	_network_check_gprs_dev();
#endif
#endif
}

#if 0
static status_t _network_set_dev_state(const char *dev_name, IfState state)
{
    IfDevice *temp_dev = NULL;
    status_t ret = GXCORE_ERROR;

    if((dev_name == NULL))
    {
        return GXCORE_ERROR;
    }

    if((temp_dev = _if_dev_list_get(dev_name)) != NULL)
    {
        temp_dev->dev_state = state;
        ret = _if_dev_list_modify(temp_dev);
        GxCore_Free(temp_dev);
        temp_dev = NULL;
    }
    return ret;
}
#endif

#if 0
static status_t _network_get_main_dev(IfDevice *ret)
{
    IfDevice *temp = _if_dev_list_get(NET_IF_DEV_ETH0);

    if(temp != NULL)
    {
        memcpy(ret, temp, sizeof(IfDevice));
        GxCore_Free(temp);
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}
#endif

static status_t app_network_dev_list_get(GxMsgProperty_NetDevList *dev_list, unsigned char state_flag)
{
    unsigned int num = 0;

    IfDevList *p_dev_list = NULL;
    IfDevice *p_dev = NULL;

    GxCore_MutexLock(s_dev_list_mutex);

    p_dev_list = if_dev_list;
    while(p_dev_list != NULL)
    {
        p_dev = &(p_dev_list->net_dev);
        if((p_dev->dev_state & state_flag) != 0)
        {
            strcpy(dev_list->dev_name[num++], p_dev->dev_name);
        }
        p_dev_list = p_dev_list->next;
    }

    GxCore_MutexUnlock(s_dev_list_mutex);

    dev_list->dev_num = num;

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_get_state(GxMsgProperty_NetDevState *dev_state)
{
    IfDevice *p_dev = NULL;

    if((dev_state == NULL) || (strlen(dev_state->dev_name) == 0))
    {
        dev_state->dev_state = IF_STATE_INVALID;
        return GXCORE_ERROR;
    }

    if((p_dev = _if_dev_list_get(dev_state->dev_name)) == NULL)
        return GXCORE_ERROR;

    dev_state->dev_state = p_dev->dev_state;

    GxCore_Free(p_dev);
    p_dev = NULL;

    return GXCORE_SUCCESS;
}

#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
static bool app_network_check_sat2ip_netdev(const char *dev_name)
{
#if (ETH_SUPPORT > 0)
		if(strncmp(dev_name, NET_IF_DEV_ETH0, 4) == 0)
			return true;
#endif
#if (USB_ETH_SUPPORT > 0)
		if(strncmp(dev_name, NET_IF_DEV_ETH1, 4) == 0)
			return true;
#endif
#if (WIFI_SUPPORT > 0)
		if(strncmp(dev_name, "ra", 2) == 0)
			return true;
		if(strncmp(dev_name, "wlan", 4) == 0)
			return true;
#endif

	return false;
}
#endif

static status_t app_network_state_report(const char *dev_name, IfState dev_state)
{
    GxMessage *new_msg = NULL;
    GxMsgProperty_NetDevState *state = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
        return GXCORE_ERROR;

#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
    if (app_network_check_sat2ip_netdev(dev_name))
    {
        if (dev_state & IF_STATE_CONNECTED)
        {
            IpCfgFileData cfg_data;
            memset(&cfg_data, 0, sizeof(IpCfgFileData));
            if(_get_ip_cfg_file_data(dev_name, &cfg_data) == GXCORE_SUCCESS)
            {
#if SAT2IP_SERVER_SUPPORT
                app_sat2ip_set_net_ok((char*)cfg_data.ip_addr);
#endif
#if DVB2IP_SERVER_SUPPORT
                app_dvb2ip_net_flag_set(1, (char*)cfg_data.ip_addr);
                {
                    /* Network is up WITH an IP -- the earliest point a DNS lookup
                     * can succeed, so start the recovery API client here. It is
                     * idempotent, spawns its own worker (never blocks this
                     * thread) and retries internally until the resolver settles.
                     * Failure leaves the cache empty -> every channel stays on
                     * the factory decode path. */
                    void app_rist_api_start(void);
                    app_rist_api_start();
                }
#endif
            }
        }
        else
        {
#if SAT2IP_SERVER_SUPPORT
            app_sat2ip_unset_net_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
            app_dvb2ip_net_flag_set(0, NULL);
#endif
        }
    }
#endif

    new_msg = GxBus_MessageNew(GXMSG_NETWORK_STATE_REPORT);
    state= (GxMsgProperty_NetDevState*)GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_NetDevState);
    if(state != NULL)
    {
        strcpy(state->dev_name, dev_name);
        state->dev_state = dev_state;
        GxBus_MessageSend(new_msg);

        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static status_t app_network_plug_in_report(char *dev_name)
{
    GxMessage *new_msg = NULL;
    ubyte64 *dev = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
        return GXCORE_ERROR;
#if WIFI_SUPER_AP_SUPPORT
    if ( app_network_wifi_check_dev_is_active(dev_name) )
    {
        app_super_ap_check_start(dev_name);
    }
#endif
    new_msg = GxBus_MessageNew(GXMSG_NETWORK_PLUG_IN);
    dev = (ubyte64*)GxBus_GetMsgPropertyPtr(new_msg, ubyte64);
    if(dev != NULL)
    {
        strcpy(*dev, dev_name);
        GxBus_MessageSend(new_msg);

        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static status_t app_network_plug_out_report(const char *dev_name)
{
    GxMessage *new_msg = NULL;
    ubyte64 *dev = NULL;

    if((dev_name == NULL) || (strlen(dev_name) == 0))
        return GXCORE_ERROR;

    new_msg = GxBus_MessageNew(GXMSG_NETWORK_PLUG_OUT);
    dev = (ubyte64*)GxBus_GetMsgPropertyPtr(new_msg, ubyte64);
    if(dev != NULL)
    {
        strcpy(*dev, dev_name);
        GxBus_MessageSend(new_msg);

        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static status_t app_network_get_cur_dev(ubyte64 *dev_name)
{
    IfDevList *p = if_dev_list;
    if((dev_name == NULL) || (p == NULL))
        return GXCORE_ERROR;

    GxCore_MutexLock(s_dev_list_mutex);
    while(p != NULL)
    {
        if(p->net_dev.cur_flag == true)
        {
            strcpy(*dev_name, p->net_dev.dev_name);
            GxCore_MutexUnlock(s_dev_list_mutex);
            return GXCORE_SUCCESS;
        }
        p = p->next;
    }
    GxCore_MutexUnlock(s_dev_list_mutex);

    return GXCORE_ERROR;
}

IfState app_network_get_cur_dev_and_state(ubyte64 *get_dev_name)
{
    ubyte64 cur_dev;
    GxMsgProperty_NetDevState state;

    memset(cur_dev, 0, sizeof(ubyte64));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, cur_dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    if (get_dev_name)
    {
        strcpy(*get_dev_name, cur_dev);
    }
    return state.dev_state;
}

static status_t app_network_dev_set_mac(GxMsgProperty_NetDevMac *dev_mac)
{
    IfDevice *p_dev = NULL;

    if((dev_mac == NULL) || (strlen(dev_mac->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(dev_mac->dev_name) == true)
        return GXCORE_ERROR;

    _lock_if_dev(dev_mac->dev_name);

    if((p_dev = _if_dev_list_get(dev_mac->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_mac->dev_name);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {
        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        p_dev->dev_state = IF_STATE_IDLE;
        app_log_info("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        _if_dev_list_modify(p_dev);
    }

    _network_set_mac(dev_mac->dev_name, dev_mac->dev_mac);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_mac->dev_name);
    return GXCORE_SUCCESS;
}

static status_t app_network_dev_get_mac(GxMsgProperty_NetDevMac *dev_mac)
{
    IfDevice *p_dev = NULL;
    IpCfgFileData cfg_data;

    if((dev_mac == NULL) || (strlen(dev_mac->dev_name) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(dev_mac->dev_name);
    if((p_dev = _if_dev_list_get(dev_mac->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_mac->dev_name);
        return GXCORE_ERROR;
    }

    memset(&cfg_data, 0, sizeof(IpCfgFileData));
    if(_get_ip_cfg_file_data(dev_mac->dev_name, &cfg_data) == GXCORE_ERROR)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_mac->dev_name);
        return GXCORE_ERROR;
    }

    strcpy(dev_mac->dev_mac, cfg_data.mac);

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(dev_mac->dev_name);
    return GXCORE_SUCCESS;
}


static int app_network_set_priority(IfDevice *cur_dev)
{
    int ret = 0;

    if(strcmp(cur_dev->dev_name, "eth0") == 0)
        ret = WIRED_PRIORITY;
    else if(app_network_wifi_dev_name_strcmp(cur_dev->dev_name) == 0)
        ret = WIFI_PRIORITY;
    else if(strcmp(cur_dev->dev_name, "usb0") == 0)
        ret = RNDIS_PRIORITY;
#ifdef LINUX_OS
#else
	else if(strcmp(cur_dev->dev_name, GPRS_DEV_HEAD) == 0)
		ret = GPRS_PRIORITY;
#endif
else
        ret = TG_PRIORITY;
	return ret;
}

static int app_network_change_connect_dev(char *dev_name)
{
    IfDevice *p_dev = NULL;
    IfDevList *p_dev_list = NULL;

    if(dev_name == NULL)
    {
        return GXCORE_ERROR;
    }

    GxCore_MutexLock(s_network_service_mutex);

    p_dev_list = if_dev_list;
    while(p_dev_list != NULL)
    {
        p_dev = &(p_dev_list->net_dev);
        if(strcmp(p_dev->dev_name, dev_name) != 0)
        {
            p_dev->dev_priority = app_network_set_priority(p_dev);
            if((p_dev->cur_flag == true)
                || (p_dev->dev_state == IF_STATE_START)
                || (p_dev->dev_state == IF_STATE_CONNECTING))
            {
                if(p_dev->dev_ops->if_stop != NULL)
                    p_dev->dev_ops->if_stop(p_dev);
                p_dev->dev_state = IF_STATE_IDLE;
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = WIRED_PRIORITY;
                app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        else
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);
            if(p_dev->cur_flag == true)
                p_dev->cur_flag = false;

            s_network_dev_priority = WIRED_PRIORITY;
            p_dev->dev_state = IF_STATE_IDLE;
            p_dev->dev_priority = MIN_PRIORITY;
        }
        p_dev_list = p_dev_list->next;
    }
    GxCore_MutexUnlock(s_network_service_mutex);

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_set_ipcfg(GxMsgProperty_NetDevIpcfg *dev_config)
{
    IfDevice *p_dev = NULL;
    unsigned char mode = 0;
    IpCfgFileData cfg_data;

    if((dev_config == NULL) || (strlen(dev_config->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(dev_config->dev_name) == true)
        return GXCORE_ERROR;

    app_network_change_connect_dev(dev_config->dev_name);

    _lock_if_dev(dev_config->dev_name);
    if((p_dev = _if_dev_list_get(dev_config->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_config->dev_name);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        p_dev->dev_state = IF_STATE_IDLE;
        app_log_info("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        _if_dev_list_modify(p_dev);
    }

    (p_dev->user_flag == false) ? (mode = 0) : (mode = 1);

    cfg_data.dev_mode = mode;
    cfg_data.ip_type = dev_config->ip_type;

    strcpy(cfg_data.ip_addr, dev_config->ip_addr);
    strcpy(cfg_data.net_mask, dev_config->net_mask);
    strcpy(cfg_data.gate_way, dev_config->gate_way);
    strcpy(cfg_data.dns1, dev_config->dns1);
    strcpy(cfg_data.dns2, dev_config->dns2);

    _set_ip_cfg_file_data(dev_config->dev_name, &cfg_data);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_config->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_get_ipcfg(GxMsgProperty_NetDevIpcfg *dev_config)
{
    IfDevice *p_dev = NULL;
    IpCfgFileData cfg_data;

    if((dev_config == NULL) || (strlen(dev_config->dev_name) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(dev_config->dev_name);

    if((p_dev = _if_dev_list_get(dev_config->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_config->dev_name);
        return GXCORE_ERROR;
    }

    memset(&cfg_data, 0, sizeof(IpCfgFileData));
    if(_get_ip_cfg_file_data(dev_config->dev_name, &cfg_data) == GXCORE_ERROR)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_config->dev_name);
        return GXCORE_ERROR;
    }

    dev_config->ip_type = cfg_data.ip_type;
    strcpy(dev_config->ip_addr, cfg_data.ip_addr);
    strcpy(dev_config->net_mask, cfg_data.net_mask);
    strcpy(dev_config->gate_way, cfg_data.gate_way);
    strcpy(dev_config->dns1, cfg_data.dns1);
    strcpy(dev_config->dns2, cfg_data.dns2);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_config->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_set_mode(GxMsgProperty_NetDevMode *dev_mode)
{
    IfDevice *p_dev = NULL;
    IpCfgFileData cfg_data;
    IfDevList *p = NULL ;
    int type = 0 ;

    if((dev_mode == NULL) || (strlen(dev_mode->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(dev_mode->dev_name) == true)
        return GXCORE_ERROR;

    _lock_if_dev(dev_mode->dev_name);

    if((p_dev = _if_dev_list_get(dev_mode->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_mode->dev_name);
        return GXCORE_ERROR;
    }

    memset(&cfg_data, 0, sizeof(IpCfgFileData));
    if(_get_ip_cfg_file_data(dev_mode->dev_name, &cfg_data) == GXCORE_ERROR)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_mode->dev_name);
        return GXCORE_ERROR;
    }

    type = p_dev->dev_type ;
	//if(dev_mode->mode == 0)
	{
		p_dev->dev_priority = app_network_set_priority(p_dev);
		if((p_dev->cur_flag == true)
				|| (p_dev->dev_state == IF_STATE_START)
				|| (p_dev->dev_state == IF_STATE_CONNECTING)
				|| (p_dev->dev_state == IF_STATE_REJ)
				|| (p_dev->dev_state == IF_STATE_DISCONNECTED))
		{
			if(p_dev->dev_ops->if_stop != NULL)
				p_dev->dev_ops->if_stop(p_dev);

			p_dev->dev_state = IF_STATE_IDLE;
			p_dev->cur_flag = false;
			s_global_net_state = IF_STATE_IDLE;
			s_network_dev_priority = MAX_PRIORITY;
			app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
		}
		app_network_state_report(p_dev->dev_name, p_dev->dev_state);
	}

    (dev_mode->mode == 0) ? (p_dev->user_flag = false) : (p_dev->user_flag = true);
    _if_dev_list_modify(p_dev);

    cfg_data.dev_mode = dev_mode->mode;
    _set_ip_cfg_file_data(dev_mode->dev_name, &cfg_data);

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(dev_mode->dev_name);

    //all wifi dev, mode is same
    if ( (type != IF_TYPE_WIFI) || (app_network_wifi_get_all_dev_plug_count() <= 1) )
        return GXCORE_SUCCESS;

    GxCore_MutexLock(s_dev_list_mutex);
    p = if_dev_list;
    while(p != NULL)
    {
        if ( p->net_dev.dev_type == IF_TYPE_WIFI )
        {
            if ( dev_mode->mode == 0 )
            {
                if (p->net_dev.user_flag != false)
                {
                    p->net_dev.user_flag = false ;
                }
            }
            else
            {
                if (p->net_dev.user_flag != true)
                {
                    p->net_dev.user_flag = true ;
                }
            }
        }
        p = p->next;
    }
    GxCore_MutexUnlock(s_dev_list_mutex);

    return GXCORE_SUCCESS;
}

static status_t app_network_dev_get_mode(GxMsgProperty_NetDevMode *dev_mode)
{
    IfDevice *p_dev = NULL;

    if((dev_mode == NULL) || (strlen(dev_mode->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    _lock_if_dev(dev_mode->dev_name);
    if((p_dev = _if_dev_list_get(dev_mode->dev_name)) == NULL)
    {
        _unlock_if_dev(dev_mode->dev_name);
        return GXCORE_ERROR;
    }

    (p_dev->user_flag == false) ? (dev_mode->mode = 0) : (dev_mode->mode = 1);
    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(dev_mode->dev_name);

    return GXCORE_SUCCESS;
}

#if (WIFI_SUPPORT > 0)

static status_t app_wifi_aplist_report(const char *wifi_dev, ApInfo *ap_info, unsigned int ap_num, GxMsgID msg_id)
{
    GxMessage *new_msg = NULL;
    GxMsgProperty_ApList *ap_list = NULL;

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0))
        return GXCORE_ERROR;

#if WIFI_SUPER_AP_SUPPORT
    app_super_ap_check_result(wifi_dev, ap_info, ap_num);
#endif

    if((ap_num > 0) && (ap_info == NULL))
        return GXCORE_ERROR;

    if((new_msg = GxBus_MessageNew(msg_id)) != NULL)
    {
        ap_list= (GxMsgProperty_ApList*)GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_ApList);
        if(ap_list != NULL)
        {
            strcpy(ap_list->wifi_dev, wifi_dev);

            (ap_num <= MAX_AP_NUM) ? (ap_list->ap_num = ap_num) : (ap_list->ap_num = MAX_AP_NUM);
            ap_list->ap_info = ap_info;
            GxBus_MessageSend(new_msg);

            return GXCORE_SUCCESS;
        }
    }
    return GXCORE_ERROR;
}

static int _hide_ap_handle(ApInfo *ap_data, int ap_num)
{
    int i, j;
    int cnt = 0;
    bool flag = false;
    ApInfo *ap_tmp = NULL;

    if((ap_tmp = GxCore_Calloc(ap_num, sizeof(ApInfo))) == NULL)
    {
        app_log_error("%s:%d malloc err!", __func__, __LINE__);
        return 0;
    }

    for(i = 0; i < ap_num; i++)
    {
        flag = false;
        for(j = 0; j < cnt; j++)
        {
            if(strcmp(ap_data[i].ap_mac, ap_tmp[j].ap_mac) == 0)
            {
                flag = true;
                ap_tmp[j].hidden = 1;
                if(strlen(ap_data[i].ap_essid) > 0)
                    strcpy(ap_tmp[j].ap_essid, ap_data[i].ap_essid);
                break;
            }
        }
        if(flag)
            continue;
        memcpy(&ap_tmp[cnt], &ap_data[i], sizeof(ApInfo));
        if(strlen(ap_tmp[cnt].ap_essid) == 0)
            ap_tmp[cnt].hidden = 1;
        ++cnt;
    }
    memset(ap_data, 0, sizeof(ApInfo) * ap_num);
    memcpy(ap_data, ap_tmp, sizeof(ApInfo) * cnt);
    GxCore_Free(ap_tmp);

    return cnt;
}

static int32_t _wifi_scan_flag_set(char *wifi_dev, bool flag)
{
    IfDevice *p_dev = NULL;

    _lock_if_dev(wifi_dev);
    if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
        goto err;

    if(p_dev->dev_state == IF_STATE_INVALID) //plug out
        goto err;

    p_dev->scanning_flag = flag;
    _if_dev_list_modify(p_dev);
    _unlock_if_dev(wifi_dev);
    GxCore_Free(p_dev);
    return 0;
err:
    _unlock_if_dev(wifi_dev);
    GxCore_Free(p_dev);
    return -1;
}

static int32_t _wifi_start_finish(char *wifi_dev)
{
    IfDevice *p_dev = NULL;

    if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
        goto err;

    while(p_dev->dev_state == IF_STATE_START)
    {
        GxCore_ThreadDelay(100);

        GxCore_Free(p_dev);
        p_dev = NULL;
        if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
            goto err;
    }

    GxCore_Free(p_dev);
    return 0;
err:
    GxCore_Free(p_dev);
    return -1;
}

static void _wifi_ap_list_check(uint32_t ap_num)
{
    uint32_t i = 0;
    uint8_t record_hidden = 0;

    for(i = 0; i < ap_num; i++)
    {
        if(0 == strlen(s_ap_list[i].ap_essid))
            s_ap_list[i].hidden = 1;

        record_hidden = 0 ;
        _get_ap_psk_and_hidden(s_ap_list[i].ap_essid, s_ap_list[i].ap_mac, s_ap_list[i].ap_psk, &record_hidden);

        if(0 == s_ap_list[i].hidden)
            s_ap_list[i].hidden = record_hidden;

        if(s_ap_list[i].encryption == 0) //open ap
        {
            memset(s_ap_list[i].ap_psk,0,sizeof(ubyte32));
            continue;
        }
    }
}

#ifdef LINUX_OS
static uint32_t _wifi_scan_ap_list(char *wifi_dev, char *essid)
{
    uint32_t ap_num = 0;
    char cmd_buf[CMD_LEN] = {0};
    char result_path[PATH_LEN] = {0};

    sprintf(result_path, "%s%s%s", NETWORK_MANUAL_DIR, wifi_dev, "_aplist");
    if(essid)
        sprintf(cmd_buf, "%s %s %s manual %s", WIFI_SCAN_CMD, result_path, wifi_dev, essid);
    else
        sprintf(cmd_buf, "%s %s %s manual", WIFI_SCAN_CMD, result_path, wifi_dev);
    system(cmd_buf);

    ap_num = _get_ap_list_file_data(result_path, s_ap_list, MAX_AP_NUM);

    memset(cmd_buf, 0, CMD_LEN);
    sprintf(cmd_buf, "%s %s", RM_FILE_CMD, result_path);
    system(cmd_buf);


    return ap_num;
}
#else
extern unsigned int app_get_aplist(char *dev_name,ApInfo *ap_data, unsigned int max_ap_num, char *essid);

static uint32_t _wifi_scan_ap_list(char *wifi_dev, char *essid)
{
    static unsigned int nTryCount;
    uint32_t ap_num = 0;

    nTryCount = 5;

    while(nTryCount > 0 && ap_num == 0)
    {
        if((ap_num = app_get_aplist(wifi_dev,s_ap_list, MAX_AP_NUM, essid)) > 0)
            break;

        GxCore_ThreadDelay(300);
        nTryCount--;
        app_log_info("\033[31mTry to get ap list again!!\n\033[0m");
    }
    qsort(s_ap_list, ap_num, sizeof(ApInfo), _ap_quality_cmp);

    return ap_num;
}
#endif


static void _wifi_thread_scan_ap(void* arg)
{
    uint32_t ap_num = 0;
    ubyte64 wifi_dev = {0};

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    strlcpy(wifi_dev, *(ubyte64*)arg, sizeof(ubyte64));

    if(0 == strlen(wifi_dev))
        return;

    memset(s_ap_list, 0, sizeof(ApInfo) * MAX_AP_NUM);
    app_if_up(wifi_dev);

    if(_wifi_start_finish(wifi_dev) < 0)
        goto end;

    if(_wifi_scan_flag_set(wifi_dev, true) < 0)
        goto end;

    ap_num = _wifi_scan_ap_list(wifi_dev, NULL);
    _wifi_ap_list_check(ap_num);

    if(_wifi_scan_flag_set(wifi_dev, false) < 0)
        goto end;
end:
    ap_num = _hide_ap_handle(s_ap_list, ap_num);
    app_wifi_aplist_report(wifi_dev, s_ap_list, ap_num, GXMSG_WIFI_SCAN_AP_OVER);
}

static void _wifi_thread_scan_hidden_ap(void* arg)
{
    uint32_t ap_num = 0;
    ubyte64 wifi_dev = {0};
    GxMsgProperty_ScanHideAp params;

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

    memset(&params, 0, sizeof(GxMsgProperty_ScanHideAp));
    memcpy(&params, (GxMsgProperty_ScanHideAp *)arg, sizeof(GxMsgProperty_ScanHideAp));
    strlcpy(wifi_dev, params.dev_name, sizeof(wifi_dev));

    if(0 == strlen(wifi_dev))
        return;

    memset(s_ap_list, 0, sizeof(ApInfo) * MAX_AP_NUM);
    app_if_up(wifi_dev);

    if(_wifi_start_finish(wifi_dev))
        goto end;

    if(_wifi_scan_flag_set(wifi_dev, true) < 0)
        goto end;

    ap_num = _wifi_scan_ap_list(wifi_dev, params.ap_essid);
    _wifi_ap_list_check(ap_num);

    if(_wifi_scan_flag_set(wifi_dev, false) < 0)
        goto end;
end:
    ap_num = _hide_ap_handle(s_ap_list, ap_num);
    app_wifi_aplist_report(wifi_dev, s_ap_list, ap_num, GXMSG_WIFI_SCAN_HIDDEN_AP_OVER);
}

static status_t app_wifi_scan_ap(const char *dev_name)
{
    IfDevice *p_dev = NULL;
    status_t ret = GXCORE_ERROR;
    static ubyte64 _dev_name = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    _lock_if_dev(dev_name);
    if((p_dev = _if_dev_list_get(dev_name)) == NULL)
    {
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_state == IF_STATE_INVALID)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->scanning_flag == true)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    strlcpy(_dev_name, p_dev->dev_name, sizeof(ubyte64));
    ret = GxCore_ThreadCreate("wifi_scan", &(p_dev->scan_thread_handle), _wifi_thread_scan_ap,
            (void*)(&_dev_name), 10 * 1024, GXOS_DEFAULT_PRIORITY);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_name);
    return ret;
}

static status_t app_wifi_scan_hide_ap(GxMsgProperty_ScanHideAp *scan_hide_ap)
{
    IfDevice *p_dev = NULL;
    status_t ret = GXCORE_ERROR;
    static GxMsgProperty_ScanHideAp params = {{0}, {0}};

    if(NULL == scan_hide_ap || 0 == strlen(scan_hide_ap->dev_name))
        return GXCORE_ERROR;

    _lock_if_dev(scan_hide_ap->dev_name);
    if((p_dev = _if_dev_list_get(scan_hide_ap->dev_name)) == NULL)
    {
        _unlock_if_dev(scan_hide_ap->dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_state == IF_STATE_INVALID)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(scan_hide_ap->dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->scanning_flag == true)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(scan_hide_ap->dev_name);
        return GXCORE_ERROR;
    }

    strlcpy(params.dev_name, p_dev->dev_name, sizeof(ubyte64));
    strlcpy(params.ap_essid, scan_hide_ap->ap_essid, sizeof(params.ap_essid));

    ret = GxCore_ThreadCreate("wifi_scan", &(p_dev->scan_thread_handle), _wifi_thread_scan_hidden_ap,
            (void*)(&params), 10 * 1024, GXOS_DEFAULT_PRIORITY);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(scan_hide_ap->dev_name);
    return ret;
}

static status_t app_wifi_get_cur_ap(GxMsgProperty_CurAp *cur_ap)
{
    //app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
    IfDevice *p_dev = NULL;
    ApFileData ap_file_data;

    if((cur_ap == NULL) || (strlen(cur_ap->wifi_dev) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(cur_ap->wifi_dev);
    if((p_dev = _if_dev_list_get(cur_ap->wifi_dev)) == NULL)
    {
        _unlock_if_dev(cur_ap->wifi_dev);
        return GXCORE_ERROR;
    }
#if (WIFI_SUPPORT > 0)
    if(p_dev->dev_type != IF_TYPE_WIFI)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(cur_ap->wifi_dev);
        return GXCORE_ERROR;
    }
#endif
    memset(&ap_file_data, 0, sizeof(ApFileData));
#if WIFI_SUPER_AP_SUPPORT
    if(app_get_wifi_connect_ap_data(cur_ap->wifi_dev, &ap_file_data, false) == 0)
#else
    if(_get_wifi_connect_file_data(cur_ap->wifi_dev, &ap_file_data) == GXCORE_SUCCESS)
#endif
    {
        strcpy(cur_ap->ap_mac, ap_file_data.mac);
        strcpy(cur_ap->ap_essid, ap_file_data.essid);
        strcpy(cur_ap->ap_psk, ap_file_data.psk);
        //app_log_debug("~~~~~~[%s]%d, file: %s, %s, cur_ap: %s, %s!!!!!\n", __func__, __LINE__,
        //ap_file_data.essid, ap_file_data.mac, cur_ap->ap_essid, cur_ap->ap_mac);
    }
    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(cur_ap->wifi_dev);

    return GXCORE_SUCCESS;
}

static status_t app_wifi_set_cur_ap(GxMsgProperty_CurAp *cur_ap)
{
    IfDevice *p_dev = NULL;
    ApFileData ap_data;

    if((cur_ap == NULL) || (strlen(cur_ap->wifi_dev) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(cur_ap->wifi_dev) == true)
        return GXCORE_ERROR;
#ifdef ECOS_OS
    network_down(cur_ap->wifi_dev);
#endif
    app_network_change_connect_dev(cur_ap->wifi_dev);

    _lock_if_dev(cur_ap->wifi_dev);
    if((p_dev = _if_dev_list_get(cur_ap->wifi_dev)) == NULL)
    {
        _unlock_if_dev(cur_ap->wifi_dev);
        return GXCORE_ERROR;
    }
#if (WIFI_SUPPORT > 0)
    if(p_dev->dev_type != IF_TYPE_WIFI)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(cur_ap->wifi_dev);
        return GXCORE_ERROR;
    }
#endif
    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        p_dev->dev_state = IF_STATE_IDLE;
        app_log_info("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);
        _if_dev_list_modify(p_dev);
    }
    memset(&ap_data, 0, sizeof(ApFileData));
    strcpy(ap_data.essid, cur_ap->ap_essid);
    strcpy(ap_data.mac, cur_ap->ap_mac);
    strcpy(ap_data.psk, cur_ap->ap_psk);
    ap_data.hidden = cur_ap->hidden;
    _set_wifi_connect_file_data(p_dev->dev_name, &ap_data);
#if WIFI_SUPER_AP_SUPPORT
    app_super_ap_set_cur_ap(p_dev->dev_name, &ap_data);
#endif

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(cur_ap->wifi_dev);

    return GXCORE_SUCCESS;
}

static status_t app_wifi_del_cur_ap(const char *wifi_dev)
{
#ifdef LINUX_OS
    char cmd_buf[CMD_LEN] = {0};
#endif
    IfDevice *p_dev = NULL;

    if((wifi_dev == NULL) || (strlen(wifi_dev) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(wifi_dev) == true)
        return GXCORE_ERROR;

    _lock_if_dev(wifi_dev);
    if((p_dev = _if_dev_list_get(wifi_dev)) == NULL)
    {
        _unlock_if_dev(wifi_dev);
        return GXCORE_ERROR;
    }
#if WIFI_SUPER_AP_SUPPORT
    if (p_dev->dev_state == IF_STATE_REJ)
    {
        p_dev->dev_state = IF_STATE_IDLE;
        _unlock_if_dev(wifi_dev);
        return GXCORE_ERROR;
    }
#endif
#if (WIFI_SUPPORT > 0)
    if(p_dev->dev_type != IF_TYPE_WIFI)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(wifi_dev);
        return GXCORE_ERROR;
    }
#endif
    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
            }
            s_global_net_state = IF_STATE_IDLE;
            p_dev->dev_state = IF_STATE_IDLE;//for bug 146547
            s_network_dev_priority = MAX_PRIORITY;
            app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);

            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        p_dev->dev_state = IF_STATE_IDLE;
        app_log_info("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);
        _if_dev_list_modify(p_dev);
    }

#ifdef LINUX_OS
    sprintf(cmd_buf, "%s %s", WIFI_DEL_AP_CMD, wifi_dev);
    //app_log_debug("~~~~~~[%s]%d, %s!!!!!\n", __func__, __LINE__, cmd_buf);
    system(cmd_buf);
#else
    //clear ap connect record
    GxBus_ConfigSet(AP0_DEV_KEY, AP_DEV_DEFAULT);
    GxBus_ConfigSet(AP0_SSID_KEY, AP_SSID_DEFAULT);
    GxBus_ConfigSet(AP0_MAC_KEY, AP_MAC_DEFAULT);
    GxBus_ConfigSet(AP0_PSK_KEY, AP_PSK_DEFAULT);
#endif

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(wifi_dev);

    return GXCORE_SUCCESS;
}
#endif
/*******************scan bluetooth*****************/
#if (BLUETOOTH_SUPPORT > 0)

static status_t app_bluetooth_aplist_report(const char *bt_dev, BtApInfo *ap_info, unsigned char ap_num)
{
    GxMessage *new_msg = NULL;
    GxMsgProperty_ApList *ap_list = NULL;

    if((bt_dev == NULL) || (strlen(bt_dev) == 0))
        return GXCORE_ERROR;

    if((ap_num > 0) && (ap_info == NULL))
        return GXCORE_ERROR;

    if((new_msg = GxBus_MessageNew(GXMSG_BLUETOOTH_SCAN_AP_OVER)) != NULL)
    {
        ap_list= (GxMsgProperty_ApList*)GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_ApList);
        if(ap_list != NULL)
        {
            strcpy(ap_list->bluetooth_dev, bt_dev);

            (ap_num <= MAX_AP_NUM) ? (ap_list->ap_num = ap_num) : (ap_list->ap_num = MAX_AP_NUM);
            ap_list->bt_ap_info = ap_info;
            GxBus_MessageSend(new_msg);

            return GXCORE_SUCCESS;
        }
    }
    return GXCORE_ERROR;
}

static void _bluetooth_thread_scan_ap(void* arg)
{
    ubyte64 bt_dev = {0};
    IfDevice *p_dev = NULL;
    unsigned char ap_num = 0;
    unsigned char try_count = 0;

    GxCore_ThreadDetach();

    if(arg == NULL)
        return;

BT_START:
    strlcpy(bt_dev, *(ubyte64*)arg, sizeof(ubyte64));
    if(strlen(bt_dev) != 0)
    {
        //app_if_up(bt_dev);
        if((p_dev = _if_dev_list_get(bt_dev)) == NULL)
        {
            app_bluetooth_aplist_report(bt_dev, thiz_bt_ap_list, ap_num);
            return;
        }

        while(p_dev->dev_state == IF_STATE_START)
        {
            GxCore_ThreadDelay(800);
            try_count ++;
            GxCore_Free(p_dev);
            p_dev = NULL;
            if((p_dev = _if_dev_list_get(bt_dev)) == NULL)
            {
                app_bluetooth_aplist_report(bt_dev, thiz_bt_ap_list, ap_num);
                return;
            }
            else if((thiz_bt_ap_list != NULL)||(try_count == 5))
            {
                app_log_info("*_____***TryCount=%d****\n",try_count);
                p_dev->dev_state = IF_STATE_CONNECTING;
            }
        }

        _lock_if_dev(bt_dev);
        GxCore_Free(p_dev);
        p_dev = NULL;
        if((p_dev = _if_dev_list_get(bt_dev)) == NULL)
        {
            _unlock_if_dev(bt_dev);
            app_bluetooth_aplist_report(bt_dev, thiz_bt_ap_list, ap_num);
            return;
        }

        if(p_dev->dev_state == IF_STATE_INVALID) //plug out
        {
            GxCore_Free(p_dev);
            p_dev = NULL;
            _unlock_if_dev(bt_dev);
            return;
        }
        p_dev->scanning_flag = true;
        _if_dev_list_modify(p_dev);
        _unlock_if_dev(bt_dev);

        {
            static unsigned int nTryCount;
            nTryCount = 3;

            extern int app_get_bluetooth_apnum(unsigned char *ap_num);
            while(nTryCount > 0)
            {
                app_get_bluetooth_apnum(&ap_num);
                if(ap_num > 0)
                    break;
                GxCore_ThreadDelay(100);
                nTryCount--;
                app_log_info("\033[31mTry to get ap list again!!\n\033[0m");
            }
            try_count++;
            //qsort(thiz_bt_ap_list, ap_num, sizeof(ApInfo), _ap_quality_cmp);
        }

        _lock_if_dev(bt_dev);
        GxCore_Free(p_dev);
        p_dev = NULL;
        if((p_dev = _if_dev_list_get(bt_dev)) == NULL)
        {
            _unlock_if_dev(bt_dev);
            app_bluetooth_aplist_report(bt_dev,thiz_bt_ap_list, ap_num);
            return;
        }
        if(p_dev->dev_state == IF_STATE_INVALID) //plug out
        {
            GxCore_Free(p_dev);
            p_dev = NULL;
            _unlock_if_dev(bt_dev);
            return;
        }

        p_dev->scanning_flag = false;
        _if_dev_list_modify(p_dev);
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(bt_dev);
    }

    if((ap_num == 0)&&(try_count<6))
    {
        extern int app_reset_scan_ap(int mode);
        app_reset_scan_ap(1);
        GxCore_ThreadDelay(1000);
        app_log_info("\033[31mTry to get ap count  = %d \n\033[0m",try_count);
        goto BT_START;
    }
    app_bluetooth_aplist_report(bt_dev,thiz_bt_ap_list, ap_num);
}

static status_t app_bluetooth_scan_ap(const char *dev_name)
{
    IfDevice *p_dev = NULL;
    status_t ret = GXCORE_ERROR;
    static ubyte64 _dev_name = {0};

    if((dev_name == NULL) || (strlen(dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    _lock_if_dev(dev_name);
    if((p_dev = _if_dev_list_get(dev_name)) == NULL)
    {
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_state == IF_STATE_INVALID)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->scanning_flag == true)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(dev_name);
        return GXCORE_ERROR;
    }

    strlcpy(_dev_name, p_dev->dev_name, sizeof(ubyte64));
    //app_log_flow("~~~~~~[%s]%d!!!!!\n", __func__, __LINE__);
    ret = GxCore_ThreadCreate("bluetooth_scan", &(p_dev->scan_thread_handle), _bluetooth_thread_scan_ap,
            (void*)(&_dev_name), 30 * 1024, GXOS_DEFAULT_PRIORITY);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(dev_name);
    return ret;
}

static status_t app_bluetooth_get_cur_ap(GxMsgProperty_CurAp *cur_ap)
{
    //app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
    IfDevice *p_dev = NULL;
    ApFileData ap_file_data;

    if((cur_ap == NULL) || (strlen(cur_ap->bluetooth_dev) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(cur_ap->bluetooth_dev);
    if((p_dev = _if_dev_list_get(cur_ap->bluetooth_dev)) == NULL)
    {
        _unlock_if_dev(cur_ap->bluetooth_dev);
        return GXCORE_ERROR;
    }
    if(p_dev->dev_type != IF_TYPE_BLUETOOTH)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(cur_ap->bluetooth_dev);
        return GXCORE_ERROR;
    }
    memset(&ap_file_data, 0, sizeof(ApFileData));
    if(_get_bluetooth_connect_file_data(cur_ap->bluetooth_dev, &ap_file_data) == GXCORE_SUCCESS)
    {
        strcpy(cur_ap->ap_mac, ap_file_data.mac);
        strcpy(cur_ap->ap_essid, ap_file_data.essid);
    }
    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(cur_ap->bluetooth_dev);

    return GXCORE_SUCCESS;
}

static status_t app_bluetooth_set_cur_ap(GxMsgProperty_CurAp *cur_ap)
{
    IfDevice *p_dev = NULL;
    ApFileData ap_data;

    if((cur_ap == NULL) || (strlen(cur_ap->bluetooth_dev) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(cur_ap->bluetooth_dev) == true)
        return GXCORE_ERROR;
    app_network_change_connect_dev(net_if_dev_usb_bt);

    _lock_if_dev(cur_ap->bluetooth_dev);
    if((p_dev = _if_dev_list_get(cur_ap->bluetooth_dev)) == NULL)
    {
        _unlock_if_dev(cur_ap->bluetooth_dev);
        return GXCORE_ERROR;
    }
    if(p_dev->dev_type != IF_TYPE_BLUETOOTH)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(cur_ap->bluetooth_dev);
        return GXCORE_ERROR;
    }
    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        p_dev->dev_state = IF_STATE_IDLE;
        app_log_info("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);
        _if_dev_list_modify(p_dev);
    }
    memset(&ap_data, 0, sizeof(ApFileData));
    strcpy(ap_data.essid, cur_ap->ap_essid);
    strcpy(ap_data.mac, cur_ap->ap_mac);
    _set_bluetooth_connect_file_data(p_dev->dev_name, &ap_data);

    GxCore_Free(p_dev);
    p_dev = NULL;

    s_bt_connect_failer_count = 0 ;

    _unlock_if_dev(cur_ap->bluetooth_dev);

    return GXCORE_SUCCESS;
}

static status_t app_bluetooth_del_cur_ap(const char *bt_dev)
{
    IfDevice *p_dev = NULL;

    if((bt_dev == NULL) || (strlen(bt_dev) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(bt_dev) == true)
        return GXCORE_ERROR;

    _lock_if_dev(bt_dev);
    if((p_dev = _if_dev_list_get(bt_dev)) == NULL)
    {
        _unlock_if_dev(bt_dev);
        return GXCORE_ERROR;
    }
    if(p_dev->dev_type != IF_TYPE_BLUETOOTH)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(bt_dev);
        return GXCORE_ERROR;
    }
    if((p_dev->dev_state == IF_STATE_START)
        || (p_dev->dev_state == IF_STATE_CONNECTING)
        || (p_dev->dev_state == IF_STATE_CONNECTED)
        || (p_dev->dev_state == IF_STATE_REJ))
    {

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                p_dev->dev_state = IF_STATE_IDLE;//for bug 146547
                s_network_dev_priority = MAX_PRIORITY;
                app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        p_dev->dev_state = IF_STATE_IDLE;
        app_log_info("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);
        _if_dev_list_modify(p_dev);
    }

    //clear ap connect record
    GxBus_ConfigSet(BT0_DEV_KEY, BT_DEV_DEFAULT);
    GxBus_ConfigSet(BT0_SSID_KEY, AP_SSID_DEFAULT);
    GxBus_ConfigSet(BT0_MAC_KEY, BT_MAC_DEFAULT);

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(bt_dev);

    return GXCORE_SUCCESS;
}
#endif
//-----------
#if (MOD_3G_SUPPORT > 0)
static status_t app_network_3g_set_param(GxMsgProperty_3G *_3g_param)
{
    IfDevice *p_dev = NULL;
    if((_3g_param == NULL) || (strlen(_3g_param->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(_3g_param->dev_name) == true)
        return GXCORE_ERROR;

    app_network_change_connect_dev(_3g_param->dev_name);

    _lock_if_dev(_3g_param->dev_name);
    if((p_dev = _if_dev_list_get(_3g_param->dev_name)) == NULL)
    {
        _unlock_if_dev(_3g_param->dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_3G)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(_3g_param->dev_name);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
            || (p_dev->dev_state == IF_STATE_CONNECTING)
            || (p_dev->dev_state == IF_STATE_CONNECTED)
            || (p_dev->dev_state == IF_STATE_REJ))
    {
        if(p_dev->dev_ops->if_stop != NULL)
            p_dev->dev_ops->if_stop(p_dev);

        p_dev->dev_state = IF_STATE_IDLE;
        app_log_info("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);

        if(p_dev->cur_flag == true)
        {
            p_dev->cur_flag = false;
            s_global_net_state = IF_STATE_IDLE;
            s_network_dev_priority = MAX_PRIORITY;
            app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
        }
        app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        _if_dev_list_modify(p_dev);
    }

    if(_set_3g_cfg_file_data(_3g_param) == GXCORE_SUCCESS)
        GxBus_ConfigSetInt(_3G_OPERATOR_KEY, _3g_param->operator_id);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(_3g_param->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_network_3g_get_param(GxMsgProperty_3G *tgParam)
{
    return _get_3g_cfg_file_data(tgParam);
}
#endif

#if (PPPOE_SUPPORT > 0)
static status_t app_pppoe_set_param(GxMsgProperty_PppoeCfg *pppoe_cfg)
{
    IfDevice *p_dev = NULL;
    PppoeFileData pppoe_data;

    if((pppoe_cfg == NULL) || (strlen(pppoe_cfg->dev_name) == 0))
    {
        return GXCORE_ERROR;
    }

    if(_check_prev_init_dev(pppoe_cfg->dev_name) == true)
        return GXCORE_ERROR;

    _lock_if_dev(pppoe_cfg->dev_name);
    if((p_dev = _if_dev_list_get(pppoe_cfg->dev_name)) == NULL)
    {
        _unlock_if_dev(pppoe_cfg->dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_WIRED)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(pppoe_cfg->dev_name);
        return GXCORE_ERROR;
    }

    if((p_dev->dev_state == IF_STATE_START)
            || (p_dev->dev_state == IF_STATE_CONNECTING)
            || (p_dev->dev_state == IF_STATE_CONNECTED)
            || (p_dev->dev_state == IF_STATE_REJ))
    {

        if(p_dev->dev_state != IF_STATE_REJ)
        {
            if(p_dev->dev_ops->if_stop != NULL)
                p_dev->dev_ops->if_stop(p_dev);

            if(p_dev->cur_flag == true)
            {
                p_dev->cur_flag = false;
                s_global_net_state = IF_STATE_IDLE;
                s_network_dev_priority = MAX_PRIORITY;
                app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
            }
            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
        }
        p_dev->dev_state = IF_STATE_IDLE;
        app_log_info("~~~~~~[%s]%d, %s down!!!!!\n", __func__, __LINE__, p_dev->dev_name);
        _if_dev_list_modify(p_dev);
    }

    memset(&pppoe_data, 0, sizeof(PppoeFileData));
    pppoe_data.mode = pppoe_cfg->mode;
    strcpy(pppoe_data.username, pppoe_cfg->username);
    strcpy(pppoe_data.passwd, pppoe_cfg->passwd);
    strcpy(pppoe_data.dns1, pppoe_cfg->dns1);
    strcpy(pppoe_data.dns2, pppoe_cfg->dns2);
    _set_pppoe_file_data(p_dev->dev_name, &pppoe_data);

    GxCore_Free(p_dev);
    p_dev = NULL;

    _unlock_if_dev(pppoe_cfg->dev_name);

    return GXCORE_SUCCESS;
}

static status_t app_pppoe_get_param(GxMsgProperty_PppoeCfg *pppoe_cfg)
{
    IfDevice *p_dev = NULL;
    PppoeFileData pppoe_file_data;

    if((pppoe_cfg == NULL) || (strlen(pppoe_cfg->dev_name) == 0))
        return GXCORE_ERROR;

    _lock_if_dev(pppoe_cfg->dev_name);
    if((p_dev = _if_dev_list_get(pppoe_cfg->dev_name)) == NULL)
    {
        _unlock_if_dev(pppoe_cfg->dev_name);
        return GXCORE_ERROR;
    }

    if(p_dev->dev_type != IF_TYPE_WIRED)
    {
        GxCore_Free(p_dev);
        p_dev = NULL;
        _unlock_if_dev(pppoe_cfg->dev_name);
        return GXCORE_ERROR;
    }

    memset(&pppoe_file_data, 0, sizeof(PppoeFileData));
    if(_get_pppoe_file_data(pppoe_cfg->dev_name, &pppoe_file_data) == GXCORE_SUCCESS)
    {
        pppoe_cfg->mode = pppoe_file_data.mode;
        strcpy(pppoe_cfg->username, pppoe_file_data.username);
        strcpy(pppoe_cfg->passwd, pppoe_file_data.passwd);
        strcpy(pppoe_cfg->dns1, pppoe_file_data.dns1);
        strcpy(pppoe_cfg->dns2, pppoe_file_data.dns2);
        //app_log_debug("~~~~~~[%s]%d, mode: %d, user: %s, pwd: %s, dns1: %s, dns2: %s!!!!!\n", __func__, __LINE__,
        //        pppoe_cfg->mode, pppoe_cfg->username, pppoe_cfg->passwd, pppoe_cfg->dns1, pppoe_cfg->dns2);
    }

    GxCore_Free(p_dev);
    p_dev = NULL;
    _unlock_if_dev(pppoe_cfg->dev_name);

    return GXCORE_SUCCESS;
}
#endif

static bool app_network_dev_compete(IfDevice *p_dev)
{
    bool ret = true;
    if(p_dev->main_flag == true)
        return true;

    if(p_dev->dev_priority > s_network_dev_priority)
        ret = false;
    else if(p_dev->dev_priority < s_network_dev_priority)
        ret = true;
    else // ==
    {
        if(p_dev->cur_flag == false)
            ret = false;
        else
            ret = true;
    }

    return ret;
}


static bool _if_dev_check_min_priority(IfDevice *cur_dev)
{
	bool ret = true;
	IfDevList *p_dev_list = NULL;
	IfDevice *p_dev = NULL;
	ApFileData ap_file_data;
	memset(&ap_file_data, 0, sizeof(ApFileData));
	p_dev_list = if_dev_list;
    while(p_dev_list != NULL)
    {
        p_dev = &(p_dev_list->net_dev);
        if(strcmp(p_dev->dev_name,cur_dev->dev_name)==0)
        {
            p_dev_list = p_dev_list->next;
        }
        else
        {
            _lock_if_dev(p_dev->dev_name);
            if((p_dev->dev_priority<cur_dev->dev_priority)&&(p_dev->user_flag))
            {
                if((strncmp(p_dev->dev_name, "eth",3) == 0)||(strcmp(p_dev->dev_name, "usb0") == 0))
                {
                    ret = false;
                    _unlock_if_dev(p_dev->dev_name);
                    return ret;
                }
                if((app_network_wifi_dev_name_strcmp(p_dev->dev_name) == 0)
#if (WIFI_SUPPORT > 0)
#if WIFI_SUPER_AP_SUPPORT
                        && (app_get_wifi_connect_ap_data(p_dev->dev_name, &ap_file_data, false) == 0)
#else
                        && (_get_wifi_connect_file_data(p_dev->dev_name, &ap_file_data) == GXCORE_SUCCESS)
#endif
#endif
                  )
                    {
                        ret = false;
                        _unlock_if_dev(p_dev->dev_name);
                        return ret;
                    }
                if((strncmp(p_dev->dev_name, "USB3G", 5) == 0)
#if (MOD_3G_SUPPORT > 0)
                        && (_network_usb3g_lookup(NULL, 0))
#endif
                )
                {
                    ret = false;
                    _unlock_if_dev(p_dev->dev_name);
                    return ret;
                }
            }
            _unlock_if_dev(p_dev->dev_name);
            p_dev_list = p_dev_list->next;
        }
    }
    return ret;
}

static status_t _network_stop_service(void)
{
    IfDevList *p_dev_list = NULL;
    IfDevice *p_dev = NULL;

    GxCore_MutexLock(s_network_service_mutex);

    if(s_network_service_enable == true)
    {
        p_dev_list = if_dev_list;
        while(p_dev_list != NULL)
        {
            p_dev = &(p_dev_list->net_dev);

            GxCore_MutexLock(p_dev->dev_mutex);

            if(_check_prev_init_dev(p_dev->dev_name) == false)
            {
#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
                if (p_dev->cur_flag && app_network_check_sat2ip_netdev(p_dev->dev_name))
                {
#if SAT2IP_SERVER_SUPPORT
                    app_sat2ip_unset_net_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
                    app_dvb2ip_net_flag_set(0, NULL);
#endif
                }
#endif

                if(p_dev->dev_ops->if_stop != NULL)
                    p_dev->dev_ops->if_stop(p_dev);
                p_dev->dev_state = IF_STATE_IDLE;
                p_dev->scanning_flag = false;
                p_dev->cur_flag = false;

                app_network_plug_out_report(p_dev->dev_name);
            }
            GxCore_MutexUnlock(p_dev->dev_mutex);
            p_dev_list = p_dev_list->next;
        }

        s_global_net_state = IF_STATE_IDLE;
        s_network_dev_priority = MAX_PRIORITY;
        app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);

        s_network_service_enable = false;
    }

    GxCore_MutexUnlock(s_network_service_mutex);

    return GXCORE_SUCCESS;
}

static status_t _network_start_service(void)
{
    GxCore_MutexLock(s_network_service_mutex);

    s_network_service_enable = true;

    GxCore_MutexUnlock(s_network_service_mutex);

    return GXCORE_SUCCESS;
}

#if NETAPPS_SUPPORT
#include <gx_apps.h>
#include <app_netapps_init.h>
#include <app_netapps_common.h>
static void _netapps_init(void)
{
	app_netapps_event_init();
	gxapps_init(PLATFORM_LIB_VER, NETAPPS_VER);
	gxapps_curl_enable_compress(1);
    app_player_open(PLAYER_FOR_IPTV);
	//app_send_msg_exec(GXMSG_NETWORK_START, NULL);
#if SAMBA_SUPPORT
	app_samba_init();
#endif
#if VPN_SUPPORT
    app_vpn_service_init();
#endif
#if DVB_CLOUD_SUPPORT
	gx_dvbonline_init();
#endif
#if PLUGINSTORE_SUPPORT || PLUGINONLINE_SUPPORT
	app_netapps_service_init();
#endif
#if SAT2IP_SERVER_SUPPORT
    app_sat2ip_arg_init();
    app_sat2ip_update_prog_total();
    app_sat2ip_set_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_arg_init();
#if FM_SUPPORT
    app_fm_data_init();
#endif
    app_dvb2ip_prog_num_update();
    app_dvb2ip_gui_flag_set(1);
#endif
}
#endif//NETAPPS_SUPPORT

status_t AppNetworkServiceInit(handle_t self, int priority_offset)
{
    handle_t sch;

#if NETAPPS_SUPPORT
    _netapps_init();
#endif

#ifdef LINUX_OS
#if (PPPOE_SUPPORT == 0)
    char cmd_buf[CMD_LEN] = {0};
    if(GXCORE_FILE_EXIST == GxCore_FileExists(PPPOE_FILE))
    {
        memset(cmd_buf, 0, CMD_LEN);
        sprintf(cmd_buf, "%s %s", RM_FILE_CMD, PPPOE_FILE);
        system(cmd_buf);
    }
#endif
#endif

    GxBus_MessageRegister(GXMSG_NETWORK_START, 0);
    GxBus_MessageRegister(GXMSG_NETWORK_STOP, 0);

    GxBus_MessageRegister(GXMSG_NETWORK_DEV_LIST, sizeof(GxMsgProperty_NetDevList));

    GxBus_MessageRegister(GXMSG_NETWORK_GET_STATE, sizeof(GxMsgProperty_NetDevState));
    GxBus_MessageRegister(GXMSG_NETWORK_STATE_REPORT, sizeof(GxMsgProperty_NetDevState));

    GxBus_MessageRegister(GXMSG_NETWORK_GET_TYPE, sizeof(GxMsgProperty_NetDevType));

    GxBus_MessageRegister(GXMSG_NETWORK_GET_CUR_DEV, sizeof(ubyte64));

    GxBus_MessageRegister(GXMSG_NETWORK_PLUG_IN, sizeof(ubyte64));
    GxBus_MessageRegister(GXMSG_NETWORK_PLUG_OUT, sizeof(ubyte64));

    GxBus_MessageRegister(GXMSG_NETWORK_SET_MAC, sizeof(GxMsgProperty_NetDevMac));
    GxBus_MessageRegister(GXMSG_NETWORK_GET_MAC, sizeof(GxMsgProperty_NetDevMac));

    GxBus_MessageRegister(GXMSG_NETWORK_SET_IPCFG, sizeof(GxMsgProperty_NetDevIpcfg));
    GxBus_MessageRegister(GXMSG_NETWORK_GET_IPCFG, sizeof(GxMsgProperty_NetDevIpcfg));

    GxBus_MessageRegister(GXMSG_NETWORK_SET_MODE, sizeof(GxMsgProperty_NetDevMode));
    GxBus_MessageRegister(GXMSG_NETWORK_GET_MODE, sizeof(GxMsgProperty_NetDevMode));

    GxBus_MessageRegister(GXMSG_WIFI_SCAN_AP, sizeof(ubyte64));
    GxBus_MessageRegister(GXMSG_WIFI_SCAN_AP_OVER, sizeof(GxMsgProperty_ApList));

    GxBus_MessageRegister(GXMSG_WIFI_SCAN_HIDDEN_AP, sizeof(GxMsgProperty_ScanHideAp));
    GxBus_MessageRegister(GXMSG_WIFI_SCAN_HIDDEN_AP_OVER, sizeof(GxMsgProperty_ApList));

    GxBus_MessageRegister(GXMSG_WIFI_GET_CUR_AP, sizeof(GxMsgProperty_CurAp));
    GxBus_MessageRegister(GXMSG_WIFI_SET_CUR_AP, sizeof(GxMsgProperty_CurAp));
    GxBus_MessageRegister(GXMSG_WIFI_DEL_CUR_AP, sizeof(ubyte64));

	//GxBus_MessageRegister(GXMSG_3G_CONNECT, sizeof(GxMsgProperty_3G));
	//GxBus_MessageRegister(GXMSG_3G_DISCONNECT, sizeof(ubyte64));
	GxBus_MessageRegister(GXMSG_3G_SET_PARAM, sizeof(GxMsgProperty_3G));
	GxBus_MessageRegister(GXMSG_3G_GET_PARAM, sizeof(GxMsgProperty_3G));
    GxBus_MessageRegister(GXMSG_3G_GET_CUR_OPERATOR, sizeof(GxMsgProperty_3G_Operator));

    GxBus_MessageRegister(GXMSG_PPPOE_SET_PARAM, sizeof(GxMsgProperty_PppoeCfg));
	GxBus_MessageRegister(GXMSG_PPPOE_GET_PARAM, sizeof(GxMsgProperty_PppoeCfg));

    GxBus_MessageListen(self,GXMSG_NETWORK_START);
    GxBus_MessageListen(self,GXMSG_NETWORK_STOP);
    GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
    GxBus_MessageListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
    GxBus_MessageListen(self, GXMSG_USBWIFI_STATUS);
	#if BLUETOOTH_SUPPORT
    GxBus_MessageRegister(GXMSG_BLUETOOTH_SCAN_AP, sizeof(ubyte64));
    GxBus_MessageRegister(GXMSG_BLUETOOTH_SCAN_AP_OVER, sizeof(GxMsgProperty_ApList));
    GxBus_MessageRegister(GXMSG_BLUETOOTH_GET_CUR_AP, sizeof(GxMsgProperty_CurAp));
    GxBus_MessageRegister(GXMSG_BLUETOOTH_SET_CUR_AP, sizeof(GxMsgProperty_CurAp));
    GxBus_MessageRegister(GXMSG_BLUETOOTH_DEL_CUR_AP, sizeof(ubyte64));

    GxBus_MessageListen(self,GXMSG_BLUETOOTH_SCAN_AP);
    GxBus_MessageListen(self,GXMSG_BLUETOOTH_SCAN_AP_OVER);
    GxBus_MessageListen(self,GXMSG_BLUETOOTH_GET_CUR_AP);
    GxBus_MessageListen(self,GXMSG_BLUETOOTH_SET_CUR_AP);
    GxBus_MessageListen(self,GXMSG_BLUETOOTH_DEL_CUR_AP);

	GxBus_MessageListen(self, GXMSG_USBBT_HOTPLUG_IN);
    GxBus_MessageListen(self, GXMSG_USBBT_HOTPLUG_OUT);

    GxBus_MessageListen(self, GXMSG_USBBT_INIT_SUCCESS);
    GxBus_MessageListen(self, GXMSG_USBBT_SCAN_RESULT);
    GxBus_MessageListen(self, GXMSG_USBBT_SCAN_FINISH);
    GxBus_MessageListen(self, GXMSG_USBBT_LINK_KEY_NOTIFY);
    GxBus_MessageListen(self, GXMSG_USBBT_SINK_CONNECT);
    GxBus_MessageListen(self, GXMSG_USBBT_SINK_DISCONNECT);
    GxBus_MessageListen(self, GXMSG_USBBT_SINK_CONNECT_FAILED);
    GxBus_MessageListen(self, GXMSG_USBBT_SINK_PLAY);
    GxBus_MessageListen(self, GXMSG_USBBT_SINK_PAUSE);
    GxBus_MessageListen(self, GXMSG_USBBT_SOURCE_CONNECT);
    GxBus_MessageListen(self, GXMSG_USBBT_SOURCE_CONNECT_FAILED);
    GxBus_MessageListen(self, GXMSG_USBBT_SOURCE_DISCONNECT);
    GxBus_MessageListen(self, GXMSG_USBBT_SOURCE_PLAY);
    GxBus_MessageListen(self, GXMSG_USBBT_SOURCE_PAUSE);
    GxBus_MessageListen(self, GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED);
    GxBus_MessageListen(self, GXMSG_USBBT_SOURCE_STREAM_RELEASED);
	#endif
	#if USB_ETH_SUPPORT
	GxBus_MessageListen(self, GXMSG_USBNET_HOTPLUG_IN);
    GxBus_MessageListen(self, GXMSG_USBNET_HOTPLUG_OUT);
	#endif

    GxBus_MessageListen(self,GXMSG_NETWORK_DEV_LIST);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_STATE);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_TYPE);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_CUR_DEV);
    GxBus_MessageListen(self,GXMSG_NETWORK_SET_MAC);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_MAC);
    GxBus_MessageListen(self,GXMSG_NETWORK_SET_IPCFG);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_IPCFG);
    GxBus_MessageListen(self,GXMSG_NETWORK_SET_MODE);
    GxBus_MessageListen(self,GXMSG_NETWORK_GET_MODE);
    GxBus_MessageListen(self,GXMSG_WIFI_SCAN_AP);
    GxBus_MessageListen(self,GXMSG_WIFI_SCAN_HIDDEN_AP);
    GxBus_MessageListen(self,GXMSG_WIFI_GET_CUR_AP);
    GxBus_MessageListen(self,GXMSG_WIFI_SET_CUR_AP);
    GxBus_MessageListen(self,GXMSG_WIFI_DEL_CUR_AP);

	//usrGxBus_MessageListen(self,GXMSG_3G_CONNECT);
	//GxBus_MessageListen(self,GXMSG_3G_DISCONNECT);
	GxBus_MessageListen(self,GXMSG_3G_SET_PARAM);
	GxBus_MessageListen(self,GXMSG_3G_GET_PARAM);
	GxBus_MessageListen(self,GXMSG_3G_GET_CUR_OPERATOR);
	GxBus_MessageListen(self,GXMSG_USB3G_HOTPLUG_IN);
	GxBus_MessageListen(self,GXMSG_USB3G_HOTPLUG_OUT);

	GxBus_MessageListen(self, GXMSG_ETH_HOTPLUG_IN);
	GxBus_MessageListen(self, GXMSG_ETH_HOTPLUG_OUT);
#ifdef ECOS_OS
    GxBus_MessageListen(self, GXMSG_USBWIFI_SCAN_END);
#endif
    GxBus_MessageListen(self,GXMSG_PPPOE_SET_PARAM);
	GxBus_MessageListen(self,GXMSG_PPPOE_GET_PARAM);

    GxCore_MutexCreate(&s_dev_list_mutex);

    s_network_dev_priority = MAX_PRIORITY;

    GxCore_MutexCreate(&s_network_service_mutex);

#if BLUETOOTH_SUPPORT
    GxCore_MutexCreate(&s_bt_aplist_mutex);
    GxCore_MutexCreate(&s_bt_display_mutex);
    GxCore_MutexCreate(&s_bt_scan_mutex);
#endif

    s_prev_init_dev.dev_num = _get_prev_init_dev(s_prev_init_dev.net_dev);

    sch = GxBus_SchedulerCreate("NetworkMsgScheduler", GXBUS_SCHED_MSG,
                                    1024 * 16, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    sch = GxBus_SchedulerCreate("NetworkConsoleScheduler", GXBUS_SCHED_CONSOLE,
                                    1024 * 10, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    //_network_start_service();

    return GXCORE_SUCCESS;
}

void AppNetworkServiceDestroy(handle_t self)
{
#if (WIFI_SUPPORT > 0)
    s_wifi_init_status = -1;
    s_wifi_status = -1;
#endif
    GxBus_MessageUnListen(self, GXMSG_NETWORK_START);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_STOP);
    GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_IN);
    GxBus_MessageUnListen(self, GXMSG_USBWIFI_HOTPLUG_OUT);
    GxBus_MessageUnListen(self, GXMSG_USB3G_HOTPLUG_IN);
    GxBus_MessageUnListen(self, GXMSG_USB3G_HOTPLUG_OUT);
	#if BLUETOOTH_SUPPORT
    this_bt_device.init_st = -1;
    this_bt_device.hotplug_status = -1;
	GxBus_MessageUnListen(self, GXMSG_USBBT_HOTPLUG_IN);
    GxBus_MessageUnListen(self, GXMSG_USBBT_HOTPLUG_OUT);

    GxBus_MessageUnListen(self, GXMSG_USBBT_INIT_SUCCESS);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SCAN_RESULT);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SCAN_FINISH);
    GxBus_MessageUnListen(self, GXMSG_USBBT_LINK_KEY_NOTIFY);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_CONNECT);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_DISCONNECT);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_CONNECT_FAILED);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_PLAY);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SINK_PAUSE);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SOURCE_CONNECT);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SOURCE_CONNECT_FAILED);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SOURCE_DISCONNECT);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SOURCE_PLAY);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SOURCE_PAUSE);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED);
    GxBus_MessageUnListen(self, GXMSG_USBBT_SOURCE_STREAM_RELEASED);

    GxBus_MessageUnListen(self,GXMSG_BLUETOOTH_SCAN_AP);
    GxBus_MessageUnListen(self,GXMSG_BLUETOOTH_SCAN_AP_OVER);
    GxBus_MessageUnListen(self,GXMSG_BLUETOOTH_GET_CUR_AP);
    GxBus_MessageUnListen(self,GXMSG_BLUETOOTH_SET_CUR_AP);
    GxBus_MessageUnListen(self,GXMSG_BLUETOOTH_DEL_CUR_AP);

    GxBus_MessageUnregister(GXMSG_BLUETOOTH_SCAN_AP);
    GxBus_MessageUnregister(GXMSG_BLUETOOTH_SCAN_AP_OVER);
    GxBus_MessageUnregister(GXMSG_BLUETOOTH_GET_CUR_AP);
    GxBus_MessageUnregister(GXMSG_BLUETOOTH_SET_CUR_AP);
    GxBus_MessageUnregister(GXMSG_BLUETOOTH_DEL_CUR_AP);
	#endif
	#if USB_ETH_SUPPORT
	GxBus_MessageUnListen(self, GXMSG_USBNET_HOTPLUG_IN);
    GxBus_MessageUnListen(self, GXMSG_USBNET_HOTPLUG_OUT);
	#endif
#if ETH_SUPPORT
    GxBus_MessageUnListen(self, GXMSG_ETH_HOTPLUG_IN);
    GxBus_MessageUnListen(self, GXMSG_ETH_HOTPLUG_OUT);
#endif
    GxBus_MessageUnListen(self, GXMSG_NETWORK_DEV_LIST);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_STATE);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_TYPE);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_CUR_DEV);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_SET_MAC);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_MAC);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_SET_IPCFG);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_IPCFG);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_SET_MODE);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_GET_MODE);
    GxBus_MessageUnListen(self, GXMSG_WIFI_SCAN_AP);
    GxBus_MessageUnListen(self, GXMSG_WIFI_GET_CUR_AP);
    GxBus_MessageUnListen(self, GXMSG_WIFI_SET_CUR_AP);
    GxBus_MessageUnListen(self, GXMSG_WIFI_DEL_CUR_AP);
	//GxBus_MessageUnListen(self, GXMSG_3G_CONNECT);
	//GxBus_MessageUnListen(self, GXMSG_3G_DISCONNECT);
	GxBus_MessageUnListen(self, GXMSG_3G_SET_PARAM);
	GxBus_MessageUnListen(self, GXMSG_3G_GET_PARAM);
	GxBus_MessageUnListen(self,GXMSG_3G_GET_CUR_OPERATOR);

    GxBus_MessageUnListen(self, GXMSG_PPPOE_SET_PARAM);
	GxBus_MessageUnListen(self, GXMSG_PPPOE_GET_PARAM);

    GxBus_MessageUnregister(GXMSG_NETWORK_START);
    GxBus_MessageUnregister(GXMSG_NETWORK_STOP);
    GxBus_MessageUnregister(GXMSG_USBWIFI_HOTPLUG_IN);
    GxBus_MessageUnregister(GXMSG_USBWIFI_HOTPLUG_OUT);
    GxBus_MessageUnregister(GXMSG_USBWIFI_STATUS);
#ifdef ECOS_OS
    GxBus_MessageUnregister(GXMSG_USBWIFI_SCAN_END);
#endif
    GxBus_MessageUnregister(GXMSG_USB3G_HOTPLUG_IN);
    GxBus_MessageUnregister(GXMSG_USB3G_HOTPLUG_OUT);
    GxBus_MessageUnregister(GXMSG_NETWORK_DEV_LIST);

    GxBus_MessageUnregister(GXMSG_NETWORK_GET_STATE);
    GxBus_MessageUnregister(GXMSG_NETWORK_STATE_REPORT);

    GxBus_MessageUnregister(GXMSG_NETWORK_GET_TYPE);

    GxBus_MessageUnregister(GXMSG_NETWORK_GET_CUR_DEV);

    GxBus_MessageUnregister(GXMSG_NETWORK_PLUG_IN);
    GxBus_MessageUnregister(GXMSG_NETWORK_PLUG_OUT);

    GxBus_MessageUnregister(GXMSG_NETWORK_SET_MAC);
    GxBus_MessageUnregister(GXMSG_NETWORK_GET_MAC);

    GxBus_MessageUnregister(GXMSG_NETWORK_SET_IPCFG);
    GxBus_MessageUnregister(GXMSG_NETWORK_GET_IPCFG);

    GxBus_MessageUnregister(GXMSG_NETWORK_SET_MODE);
    GxBus_MessageUnregister(GXMSG_NETWORK_GET_MODE);

    GxBus_MessageUnregister(GXMSG_WIFI_SCAN_AP);
    GxBus_MessageUnregister(GXMSG_WIFI_SCAN_AP_OVER);
    GxBus_MessageUnregister(GXMSG_WIFI_SCAN_HIDDEN_AP_OVER);
    GxBus_MessageUnregister(GXMSG_WIFI_GET_CUR_AP);
    GxBus_MessageUnregister(GXMSG_WIFI_SET_CUR_AP);
    GxBus_MessageUnregister(GXMSG_WIFI_DEL_CUR_AP);
	//GxBus_MessageUnregister(GXMSG_3G_CONNECT);
	//GxBus_MessageUnregister(GXMSG_3G_DISCONNECT);
	GxBus_MessageUnregister(GXMSG_3G_SET_PARAM);
	GxBus_MessageUnregister(GXMSG_3G_GET_PARAM);
	GxBus_MessageUnregister(GXMSG_3G_GET_CUR_OPERATOR);

    GxBus_MessageUnregister(GXMSG_PPPOE_SET_PARAM);
	GxBus_MessageUnregister(GXMSG_PPPOE_GET_PARAM);

    GxBus_ServiceUnlink(self);

    return;
}

GxMsgStatus AppNetworkServiceRecvMsg(handle_t self, GxMessage *msg)
{
    GxMsgStatus ret = GXCORE_SUCCESS;

    if(msg == NULL)
        return GXCORE_ERROR;

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_START:
            _network_start_service();
            break;

        case GXMSG_NETWORK_STOP:
            _network_stop_service();
            break;

#if (WIFI_SUPPORT > 0)
        case GXMSG_USBWIFI_HOTPLUG_OUT:
        {
            GxMsgProperty_UsbwifiHotPlug *plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);
#ifdef LINUX_OS
            s_wifi_plug_change = 1 ;
#endif
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
            app_wifi_debug("[WIFI] out [%d,%s] \n",plug->port,plug->dev_name);
#endif
            if ( app_network_wifi_check_dev_is_active(plug->dev_name))
            {
#ifdef ECOS_OS
                network_down(plug->dev_name);
#endif
            }
            app_network_wifi_del_dev_name(plug->dev_name);
            break;
        }

        case GXMSG_USBWIFI_HOTPLUG_IN:
        {
            GxMsgProperty_UsbwifiHotPlug *plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
			app_wifi_debug("[WIFI] in [%d,%s] \n",plug->port,plug->dev_name);
#endif
            app_network_wifi_add_dev_name(plug->port,plug->dev_name);
            break;
        }

        case GXMSG_USBWIFI_STATUS:
        {
            GxMsgProperty_WifiStatus *usbwifi_state = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_WifiStatus);
            if( app_network_wifi_check_dev_is_active(usbwifi_state->dev_name) )
            {
                if ( s_wifi_status != usbwifi_state->status )
                {
#ifdef APP_NET_WIFI_DEBUG_MSG_STATUS_SUPPORT
                    app_wifi_msg_debug("\n[WIFI] status %d,[%d,%s]\n",usbwifi_state->status,s_wifi_pre_status,usbwifi_state->dev_name);
#endif
                    s_wifi_pre_status = s_wifi_status ;
                    s_wifi_status = usbwifi_state->status;
                }
            }
            break;
        }
#ifdef ECOS_OS
        case GXMSG_USBWIFI_SCAN_END:
        {
            GxMsgProperty_WifiStatus* plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_WifiStatus);
extern void app_wifi_set_scanover_flag(bool flag);
            if( app_network_wifi_check_dev_is_active(plug->dev_name) )
            {
                app_wifi_set_scanover_flag(true);
            }
            break;
        }
#endif
#endif
        case GXMSG_NETWORK_DEV_LIST:
        {
            GxMsgProperty_NetDevList *dev_list = NULL;
            unsigned char state_flag = (IF_STATE_IDLE
                                        | IF_STATE_START
                                        | IF_STATE_CONNECTING
                                        | IF_STATE_CONNECTED
                                        | IF_STATE_REJ
                                        | IF_STATE_DISCONNECTED);

            dev_list = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevList);
            app_network_dev_list_get(dev_list, state_flag);
            break;
        }

        case GXMSG_NETWORK_GET_STATE:
        {
            GxMsgProperty_NetDevState *dev_state = NULL;
            dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
            app_network_dev_get_state(dev_state);
            break;
        }

        case GXMSG_NETWORK_GET_TYPE:
        {
            GxMsgProperty_NetDevType *dev_type = NULL;
            dev_type = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevType);
            dev_type->dev_type = app_if_dev_check_type(dev_type->dev_name);
            break;
        }

        case GXMSG_NETWORK_GET_CUR_DEV:
        {
            ubyte64 *cur_dev = NULL;
            cur_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            app_network_get_cur_dev(cur_dev);
            break;
        }

        case GXMSG_NETWORK_SET_MAC:
        {
            GxMsgProperty_NetDevMac *dev_mac = NULL;
            dev_mac = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevMac);
            app_network_dev_set_mac(dev_mac);
            break;
        }

        case GXMSG_NETWORK_GET_MAC:
        {
            GxMsgProperty_NetDevMac *dev_mac = NULL;
            dev_mac = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevMac);
            app_network_dev_get_mac(dev_mac);
            break;
        }

        case GXMSG_NETWORK_SET_IPCFG:
        {
            GxMsgProperty_NetDevIpcfg *dev_config = NULL;
            dev_config = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevIpcfg);
            app_network_dev_set_ipcfg(dev_config);
            break;
        }

        case GXMSG_NETWORK_GET_IPCFG:
        {
            GxMsgProperty_NetDevIpcfg *dev_config = NULL;
            dev_config = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevIpcfg);
            app_network_dev_get_ipcfg(dev_config);
            break;
        }

        case GXMSG_NETWORK_SET_MODE:
        {
            GxMsgProperty_NetDevMode *dev_mode = NULL;
            dev_mode = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevMode);
            app_network_dev_set_mode(dev_mode);
            break;
        }

        case GXMSG_NETWORK_GET_MODE:
        {
            GxMsgProperty_NetDevMode *dev_mode = NULL;
            dev_mode = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevMode);
            app_network_dev_get_mode(dev_mode);
            break;
        }

#if (WIFI_SUPPORT > 0)
        case GXMSG_WIFI_SCAN_AP:
        {
            //app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            ubyte64 *dev_name = NULL;
            dev_name = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            app_wifi_scan_ap(*dev_name);

            break;
        }

        case GXMSG_WIFI_SCAN_HIDDEN_AP:
        {
            GxMsgProperty_ScanHideAp *scan_hide_ap = NULL;
            scan_hide_ap = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_ScanHideAp);
            app_wifi_scan_hide_ap(scan_hide_ap);

            break;
        }

        case GXMSG_WIFI_GET_CUR_AP:
        {
            //app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_CurAp *cur_ap = NULL;
            cur_ap = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_CurAp);
            app_wifi_get_cur_ap(cur_ap);
            break;
        }

        case GXMSG_WIFI_SET_CUR_AP:
        {
            //app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_CurAp *cur_ap = NULL;
            cur_ap = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_CurAp);
            app_wifi_set_cur_ap(cur_ap);
            break;
        }

        case GXMSG_WIFI_DEL_CUR_AP:
        {
            //app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            ubyte64 *wifi_dev = NULL;
            wifi_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            app_wifi_del_cur_ap(*wifi_dev);
#ifdef ECOS_OS
			int32_t init_value;
			GxBus_ConfigGetInt(IP_TYPE_WIFI_KEY, &init_value, IP_TYPE_DEFALT);
			if(init_value == 1)	//dhcp
			{
				network_down(*wifi_dev);
			}
			else //static ip
			{
				net_ifconfig(*wifi_dev, "clear", NULL);
			}
#endif
            break;
        }
#endif

#if (BLUETOOTH_SUPPORT > 0)
        case GXMSG_USBBT_HOTPLUG_IN:
        {
            GxMsgProperty_UsbBtHotPlug *bt_plug = NULL;
            bt_plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtHotPlug);
            if(bt_plug != NULL)
            {
                app_log_info("[%s][%d], GXMSG_USBBT_HOTPLUG_IN, id = %d, err = %d\n", __func__, __LINE__, bt_plug->id, bt_plug->error);
                this_bt_device.plug_id = bt_plug->id;
                this_bt_device.count++;
                this_bt_device.hotplug_status = 1;
                app_bluetooth_msg_proc(msg);
            }
            break;
        }

        case GXMSG_USBBT_HOTPLUG_OUT:
        {
            GxMsgProperty_UsbBtHotPlug *bt_plug = NULL;
            bt_plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtHotPlug);
            if(bt_plug != NULL)
            {
                s_bt_connect_failer_count = 0 ;
                s_bt_connect_towait_count = 0 ;
                app_log_info("[%s][%d], GXMSG_USB_BT_HOTPLUG_OUT, id = %d, err = %d\n", __func__, __LINE__, bt_plug->id, bt_plug->error);
                this_bt_device.count--;
                app_bluetooth_msg_proc(msg);
                if(this_bt_device.count == 0)
                {
                    this_bt_device.hotplug_status = false;
                }
                else
                {
                    GxMsgProperty_UsbwifiHotPlug *plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbwifiHotPlug);

					if(this_bt_device.user_id == plug->id)
					{
						IfDevice *p_dev = NULL;

						_lock_if_dev(net_if_dev_usb_bt);
						if((p_dev = _if_dev_list_get(net_if_dev_usb_bt)) == NULL)
						{
							_unlock_if_dev(net_if_dev_usb_bt);
							break;
						}
						if(p_dev->dev_ops->if_stop != NULL)
							p_dev->dev_ops->if_stop(p_dev);

						p_dev->dev_state = IF_STATE_IDLE;
						p_dev->id = (p_dev->id == 0) ? 1 : 0;
						p_dev->cur_flag = false;
						s_global_net_state = IF_STATE_IDLE;
						s_network_dev_priority = MAX_PRIORITY;
						app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);

						app_network_state_report(p_dev->dev_name, p_dev->dev_state);
						_if_dev_list_modify(p_dev);

						p_dev = NULL;
						_unlock_if_dev(NET_IF_DEV_WIFI0);
					}

                }
            }
            break;
        }
		case GXMSG_BLUETOOTH_SCAN_AP:
		{
            ubyte64 *dev_name = NULL;
            dev_name = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            app_bluetooth_scan_ap(*dev_name);

            break;
        }
		case GXMSG_BLUETOOTH_GET_CUR_AP:
		{
            GxMsgProperty_CurAp *cur_ap = NULL;
            cur_ap = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_CurAp);
            app_bluetooth_get_cur_ap(cur_ap);
            break;
        }
		case GXMSG_BLUETOOTH_SET_CUR_AP:
		{
            GxMsgProperty_CurAp *cur_ap = NULL;
            cur_ap = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_CurAp);
            app_bluetooth_set_cur_ap(cur_ap);
            break;
        }
		case GXMSG_BLUETOOTH_DEL_CUR_AP:
        {
            ubyte64 *wifi_dev = NULL;
            wifi_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            app_bluetooth_del_cur_ap(*wifi_dev);
            break;
        }
        case GXMSG_USBBT_INIT_SUCCESS:
        case GXMSG_USBBT_SCAN_RESULT:
        case GXMSG_USBBT_SCAN_FINISH:
        case GXMSG_USBBT_LINK_KEY_NOTIFY:
        case GXMSG_USBBT_SINK_CONNECT:
        case GXMSG_USBBT_SINK_DISCONNECT:
        case GXMSG_USBBT_SINK_CONNECT_FAILED:
        case GXMSG_USBBT_SINK_PLAY:
        case GXMSG_USBBT_SINK_PAUSE:
        case GXMSG_USBBT_SOURCE_CONNECT:
        case GXMSG_USBBT_SOURCE_CONNECT_FAILED:
        case GXMSG_USBBT_SOURCE_DISCONNECT:
        case GXMSG_USBBT_SOURCE_PLAY:
        case GXMSG_USBBT_SOURCE_PAUSE:
        case GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED:
        case GXMSG_USBBT_SOURCE_STREAM_RELEASED:
             app_bluetooth_msg_proc(msg);
             break;
#endif

#if (MOD_3G_SUPPORT > 0)
		case GXMSG_3G_SET_PARAM:
        {
            app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_3G *tgParam = NULL;
            tgParam = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_3G);
			app_network_3g_set_param(tgParam);
            break;
        }

		case GXMSG_3G_GET_PARAM:
        {
            app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_3G *tgParam = NULL;
            tgParam = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_3G);
			app_network_3g_get_param(tgParam);
            break;
        }

        case GXMSG_3G_GET_CUR_OPERATOR:
        {
            app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_3G_Operator *operator = NULL;
            int value = 0;
            operator = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_3G_Operator);
            GxBus_ConfigGetInt(_3G_OPERATOR_KEY, &value, _3G_OPERATOR_DEFAULT);
            operator->operator_id = value;
            break;
        }

        case GXMSG_USB3G_HOTPLUG_IN:
        {
            GxMsgProperty_Usb3gHotPlug *tg_plug = NULL;
            tg_plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_Usb3gHotPlug);
            if(tg_plug != NULL)
            {
                app_log_debug("[%s][%d], GXMSG_USB3G_HOTPLUG_IN, id = %d, err = %d\n", __func__, __LINE__, tg_plug->id, tg_plug->error);
                thiz_usb3g_hotplug_status = true;
            }
            break;
        }

        case GXMSG_USB3G_HOTPLUG_OUT:
        {
            GxMsgProperty_Usb3gHotPlug *tg_plug = NULL;
            tg_plug = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_Usb3gHotPlug);
            if(tg_plug != NULL)
            {
                app_log_debug("[%s][%d], GXMSG_USB3G_HOTPLUG_OUT, id = %d, err = %d\n", __func__, __LINE__, tg_plug->id, tg_plug->error);
                thiz_usb3g_hotplug_status = false;
            }
            break;
        }
#endif

#if (ETH_SUPPORT > 0)
#ifdef ECOS_OS
		case GXMSG_ETH_HOTPLUG_IN:
		{
			wired_cable_link = 1;
			break;
		}

		case GXMSG_ETH_HOTPLUG_OUT:
		{
			wired_cable_link = 0;
			break;
		}
#endif
#endif
#if USB_ETH_SUPPORT
#ifdef ECOS_OS
		case GXMSG_USBNET_HOTPLUG_IN:
            //thiz_usbeth_hotplug_status = true;
			usb_wired_cable_link = 1;
            break;
        case GXMSG_USBNET_HOTPLUG_OUT:
            //thiz_usbeth_hotplug_status = false;
			usb_wired_cable_link = 0;
			network_down(NET_IF_DEV_ETH1);
			down_net(NET_IF_DEV_ETH1);
            break;
#endif
#endif
#if (PPPOE_SUPPORT > 0)
        case GXMSG_PPPOE_SET_PARAM:
        {
            app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_PppoeCfg *pppoe_cfg = NULL;
            pppoe_cfg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PppoeCfg);
			app_pppoe_set_param(pppoe_cfg);
            break;
        }

		case GXMSG_PPPOE_GET_PARAM:
        {
            app_log_flow("-----[%s]%d-----\n", __func__, __LINE__);
            GxMsgProperty_PppoeCfg *pppoe_cfg = NULL;
            pppoe_cfg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_PppoeCfg);
			app_pppoe_get_param(pppoe_cfg);
            break;
        }
#endif

        default:
            ret = GXMSG_OK;
            break;
    }

    return ret;
}

static void AppNetworkServiceConsole(handle_t self)
{
    bool valid_flag = false;
    IfDevList *p_dev_list = NULL;
    IfDevice *p_dev = NULL;

    if(GxCore_MutexTrylock(s_network_service_mutex) != GXCORE_SUCCESS)
    {
        GxCore_ThreadDelay(CONSOLE_DELAY_MS);
        return;
    }

#if ((WIFI_SUPPORT > 0)||(BLUETOOTH_SUPPORT > 0))
#ifdef ECOS_OS
/*
    static int sWifi = -1;
    static int sInit = -1;
    static int sPlug = -1;

    if(s_wifi_status != sWifi || s_wifi_init_status != sInit || g_wifi_hotplug_status != sPlug) {
	    app_log_debug("@@@@@@@@@@ >> wifi status : %d init status : %d  hotplug : %d\n",
			    s_wifi_status,
			    s_wifi_init_status,
			    g_wifi_hotplug_status);
	    sWifi = s_wifi_status;
	    sInit = s_wifi_init_status;
	    sPlug = g_wifi_hotplug_status;
    }
*/
    if(s_wifi_init_status == -1 && 0 == s_wifi_status)
    {
        s_wifi_init_status = 1;
    }
    /*
	if((app_network_wifi_get_all_dev_plug_count() <=0) && (0 == s_wifi_status))
    {
        s_wifi_init_status = -1;
        app_network_status_set(IF_FILE_FAIL);
    }*/
#endif
#if BLUETOOTH_SUPPORT
    if(this_bt_device.init_st == -1 && 0 != app_check_bluetooth_status())
    {
        this_bt_device.init_st = 1;
    }

    if(this_bt_device.hotplug_status == 0 && 0 == app_check_bluetooth_status())
    {
        this_bt_device.init_st = -1;
        //GxBus_ConfigSet(BT_STATUS_KEY, "fail");
    }
#endif

    if(s_network_service_enable == false) //|| s_wifi_init_status == -1)
    {
        GxCore_MutexUnlock(s_network_service_mutex);
        GxCore_ThreadDelay(CONSOLE_DELAY_MS);
        return;
    }

    app_network_wifi_init_active_id();
#endif
    _network_check_all_dev();

    p_dev_list = if_dev_list;
    while(p_dev_list != NULL)
    {
        p_dev = &(p_dev_list->net_dev);
        if(GxCore_MutexTrylock(p_dev->dev_mutex) != GXCORE_SUCCESS)
        {
            p_dev_list = p_dev_list->next;
            continue;
        }
#if (GPRS_SUPPORT > 0)
#ifdef LINUX_OS
#else
        if(app_if_dev_check_type(p_dev->dev_name) == IF_TYPE_GPRS)
        {
            valid_flag = thiz_gprs_startup;
        }
#endif
#endif

#if (MOD_3G_SUPPORT > 0)
        if(app_if_dev_check_type(p_dev->dev_name) == IF_TYPE_3G)
        {
            valid_flag = _network_usb3g_lookup(NULL, 0);
        }
        else
#endif
        {
//#ifdef ECOS_OS
#if (WIFI_SUPPORT > 0)
            if(app_if_dev_check_type(p_dev->dev_name) == IF_TYPE_WIFI)
            {
                valid_flag = ((app_network_wifi_check_dev_plug_status(p_dev->dev_name) == 1) ? true : false);
            }
            else
#endif
//#endif
                valid_flag = app_check_netlink(p_dev->dev_name);
#ifdef ECOS_OS
#if (ETH_SUPPORT > 0)
			if(app_if_dev_check_type(p_dev->dev_name) == IF_TYPE_WIRED)
			{
				valid_flag = ((wired_cable_link == 1) ? true : false);
			}
#endif

#if (USB_ETH_SUPPORT > 0)
			if(app_if_dev_check_type(p_dev->dev_name) == IF_TYPE_USB_WIRED)
			{
				valid_flag = ((usb_wired_cable_link == 1) ? true : false);
			}
#endif
#if (BLUETOOTH_SUPPORT > 0)
            if(app_if_dev_check_type(p_dev->dev_name) == IF_TYPE_BLUETOOTH)
            {
                valid_flag = ((this_bt_device.hotplug_status == 1) ? true : false);
            }
#endif

#endif
        }

        if(valid_flag == false) //plug out
        {
#if (WIFI_SUPPORT > 0)
            if(app_if_dev_check_type(p_dev->dev_name) == IF_TYPE_WIFI)
            {
                if ( app_network_wifi_get_all_dev_plug_count() <=0 )
                    s_wifi_init_status = -1;
            }
#endif
#if (BLUETOOTH_SUPPORT > 0)
            if(app_if_dev_check_type(p_dev->dev_name) == IF_TYPE_BLUETOOTH)
                this_bt_device.init_st = -1;
#endif

            if((p_dev->dev_state != IF_STATE_INVALID) && (_check_prev_init_dev(p_dev->dev_name) == false))
            {
                APP_DBG(APP_NETWORK, "device %s stop!!!", p_dev->dev_name);
                IfDevice *p_dev_bak = NULL;

                if((p_dev_bak = _if_dev_list_get(p_dev->dev_name)) != NULL)
                {
                    p_dev_list = p_dev_list->next;
                    _if_dev_list_del(p_dev->dev_name);

                    app_network_plug_out_report(p_dev_bak->dev_name);

#if (WIFI_SUPPORT > 0)
                    if ( (p_dev_bak->dev_type == IF_TYPE_WIFI)&& (!app_network_wifi_check_dev_is_active(p_dev_bak->dev_name)) )
                    {
#ifdef APP_NET_WIFI_DEBUG_PRINTF_SUPPORT
                        app_wifi_debug("[WIFI] not active \n");
#endif
                    }
                    else
#endif
                    {
                        if(p_dev_bak->dev_ops->if_stop != NULL)
                            p_dev_bak->dev_ops->if_stop(p_dev_bak);
                    }
#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
                    if (p_dev_bak->cur_flag && app_network_check_sat2ip_netdev(p_dev_bak->dev_name))
                    {
#if SAT2IP_SERVER_SUPPORT
                        app_sat2ip_unset_net_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
                        app_dvb2ip_net_flag_set(0, NULL);
#endif
                    }
#endif
                    if(p_dev_bak->cur_flag == true)
                    {
                        s_global_net_state = IF_STATE_IDLE;
                        s_network_dev_priority = MAX_PRIORITY;
                        app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
                    }
                    s_dev_type_num[p_dev_bak->dev_type]--;
#if (WIFI_SUPPORT > 0)
                    if (p_dev_bak->dev_type == IF_TYPE_WIFI)
                    {
                        app_network_wifi_plug_out_to_switch_dev(p_dev_bak->dev_name);
                    }
#endif
                    GxCore_MutexUnlock(p_dev_bak->dev_mutex);
                    GxCore_Free(p_dev_bak);
                    continue;
                }
            }
        }
        else
        {
            if(p_dev->dev_state == IF_STATE_INVALID) //plug in
            {
                if(s_dev_type_num[p_dev->dev_type] > 0)
                {
                    if ( p_dev->dev_type != IF_TYPE_WIFI )
                    {
                        GxCore_MutexUnlock(p_dev->dev_mutex);
                        p_dev_list = p_dev_list->next;

                        continue;
                    }
                }

                s_dev_type_num[p_dev->dev_type]++;
                p_dev->dev_state = IF_STATE_IDLE;
                p_dev->user_flag = _check_dev_user_flag(p_dev->dev_name);
                app_network_plug_in_report(p_dev->dev_name);
            }
            if(p_dev->dev_state == IF_STATE_IDLE)
            {
                if(p_dev->cur_flag == true)
                {
                    p_dev->cur_flag = false;
                }
#if (WIFI_SUPPORT > 0)
                if ( (p_dev->dev_type == IF_TYPE_WIFI)&& (!app_network_wifi_check_dev_is_active(p_dev->dev_name)) )
                {
                    GxCore_MutexUnlock(p_dev->dev_mutex);
                    p_dev_list = p_dev_list->next;
                    continue;
                }
#endif
                if((p_dev->user_flag == true)
                        &&(p_dev->scanning_flag == false)
                   //     && (app_network_dev_compete(p_dev) == true)
                        && (_if_dev_check_min_priority(p_dev) == true))
                {
                    ubyte64 cur_dev = {0};
                    app_network_get_cur_dev(&cur_dev);
                    if(_check_prev_init_dev(cur_dev) == false)
                    {
                        if((p_dev->dev_ops->if_start != NULL)
#if WIFI_SUPER_AP_SUPPORT
                                && ((p_dev->dev_type == IF_TYPE_WIFI && s_ap_mgr.ap_type != AP_TYPE_NULL)
                                    || (p_dev->dev_type != IF_TYPE_WIFI))
#endif
                                && (p_dev->dev_ops->if_start(p_dev) == GXCORE_SUCCESS))
                        {
                            APP_DBG(APP_NETWORK, "device %s start priority %d!!!!!", p_dev->dev_name,p_dev->dev_priority);
                            p_dev->dev_priority = MIN_PRIORITY;
                            s_network_dev_priority = WIRED_PRIORITY;
                            p_dev->dev_state = IF_STATE_START;
#if BLUETOOTH_SUPPORT
                            if(p_dev->dev_type==IF_TYPE_BLUETOOTH)
                                this_bt_device.user_id = p_dev->id;
#endif

                            app_network_state_report(p_dev->dev_name, p_dev->dev_state);
                        }
                    }
                }
            }
            else //active dev
            {
                //user disable
                if((_check_prev_init_dev(p_dev->dev_name) == false)
                        && ((p_dev->user_flag == false) && (p_dev->scanning_flag == false)))
                {
                    APP_DBG(APP_NETWORK, "device %s stop!!!", p_dev->dev_name);
                    if(p_dev->dev_ops->if_stop != NULL)
                        p_dev->dev_ops->if_stop(p_dev);
                    p_dev->dev_state = IF_STATE_IDLE;

                    if(p_dev->cur_flag == true)
                    {
                        p_dev->cur_flag = false;
                        s_global_net_state = IF_STATE_IDLE;
                        s_network_dev_priority = MAX_PRIORITY;
                        app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
                    }

                    GxCore_MutexUnlock(p_dev->dev_mutex);
                    p_dev_list = p_dev_list->next;

                    app_network_state_report(p_dev->dev_name, p_dev->dev_state);

                    continue;
                }

                //useless dev
                if((p_dev->scanning_flag == false) && (app_network_dev_compete(p_dev) == false))
                {
                    APP_DBG(APP_NETWORK, "device %s stop!!!", p_dev->dev_name);
                    if(p_dev->dev_ops->if_stop != NULL)
                        p_dev->dev_ops->if_stop(p_dev);
                    p_dev->dev_state = IF_STATE_IDLE;
                    p_dev->cur_flag = false;

                    GxCore_MutexUnlock(p_dev->dev_mutex);
                    p_dev_list = p_dev_list->next;

                    app_network_state_report(p_dev->dev_name, p_dev->dev_state);

                    continue;
                }

                if((p_dev->dev_state == IF_STATE_START)
                        || (p_dev->dev_state == IF_STATE_CONNECTING)
                        || (p_dev->dev_state == IF_STATE_CONNECTED)
                        || (p_dev->dev_state == IF_STATE_DISCONNECTED))
                {
                    if(p_dev->dev_ops->if_monitor_connect != NULL)
                    {
                        IfFileRet ret = p_dev->dev_ops->if_monitor_connect(p_dev);
                        if((ret == IF_FILE_FAIL) || (ret == IF_FILE_REJ)) //failed
                        {
                            if((p_dev->scanning_flag == false) && (_check_prev_init_dev(p_dev->dev_name) == false))
                            {
                                APP_DBG(APP_NETWORK, "device %s stop!!!", p_dev->dev_name);
                                if(p_dev->dev_ops->if_stop != NULL)
                                    p_dev->dev_ops->if_stop(p_dev);

                                if(ret == IF_FILE_REJ)
                                    p_dev->dev_state = IF_STATE_REJ;
                                else
                                    p_dev->dev_state = IF_STATE_IDLE;
#if WIFI_SUPER_AP_SUPPORT
                                bool next_ap_flag = false;
                                if (p_dev->dev_type == IF_TYPE_WIFI)
                                {
                                    if ((next_ap_flag = app_super_ap_set_next_connect()))
                                    {
                                        p_dev->dev_state = IF_STATE_IDLE;
                                    }
                                }
                                if(p_dev->cur_flag && !next_ap_flag)
                                {
                                    p_dev->cur_flag = false;
                                    s_global_net_state = IF_STATE_IDLE;
                                    s_network_dev_priority = MAX_PRIORITY;
                                    app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
                                }
#else
                                if(p_dev->cur_flag == true)
                                {
                                    p_dev->cur_flag = false;
                                    s_global_net_state = IF_STATE_IDLE;
                                    s_network_dev_priority = MAX_PRIORITY;
                                    app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
                                }
#endif
                                app_network_state_report(p_dev->dev_name, p_dev->dev_state);
                            }
                        }
                        else if(ret == IF_FILE_DISCONNECTED) // wifi shutdown router
                        {
                            if(p_dev->dev_state != IF_STATE_DISCONNECTED)
                            {
                                if((p_dev->scanning_flag == false) && (_check_prev_init_dev(p_dev->dev_name) == false))
                                {
                                    APP_DBG(APP_NETWORK, "device %s stop!!!", p_dev->dev_name);
                                    if(p_dev->dev_ops->if_stop != NULL)
                                        p_dev->dev_ops->if_stop(p_dev);

                                    p_dev->dev_state = IF_STATE_IDLE;
                                    if(p_dev->cur_flag == true)
                                    {
                                        p_dev->cur_flag = false;
                                        s_global_net_state = IF_STATE_IDLE;
                                        s_network_dev_priority = MAX_PRIORITY;
                                        app_log_info("~~~~~~[%s]%d, s_global_net_state down!!!!!\n", __func__, __LINE__);
                                    }
                                    app_network_state_report(p_dev->dev_name, IF_STATE_DISCONNECTED);
                                }
                            }
                        }
                        else if(ret == IF_FILE_OK) //ok
                        {
                            if(p_dev->dev_state != IF_STATE_CONNECTED)
                            {
                                app_log_info("~~~~~~[%s]%d, %s ok!!!!!\n", __func__, __LINE__, p_dev->dev_name);
                                p_dev->dev_state = IF_STATE_CONNECTED;
                                p_dev->cur_flag = true;
                                s_global_net_state = IF_STATE_CONNECTED;
                                s_network_dev_priority = p_dev->dev_priority;
#if IPTV_LITE_SUPPORT
                                {
                                    extern int app_iptv_poweron_play(void);
                                    int poweron_play = 0;
                                    GxBus_ConfigGetInt(POWERON_PLAY_KEY, &poweron_play, POWERON_PLAY_VALUE);
                                    if((poweron_play == 1) && (GXCORE_ERROR == GUI_CheckDialog(WND_MAIN_MENU)) &&
                                                            (GXCORE_SUCCESS == GUI_CheckDialog(WND_FULL_SCREEN)))
                                    {
                                        app_iptv_poweron_play();
                                    }
                                }
#endif

                                app_network_state_report(p_dev->dev_name, p_dev->dev_state);
                                app_log_info("~~~~~~[%s]%d, s_global_net_state up!!!!!\n", __func__, __LINE__);
                            }
                        }
                        else if(ret == IF_FILE_WAIT)
                        {
                            if(p_dev->dev_state == IF_STATE_START)
                            {
                                app_log_info("~~~~~~[%s]%d, %s connect!!!!!\n", __func__, __LINE__, p_dev->dev_name);
                                p_dev->dev_state = IF_STATE_CONNECTING;
                                app_network_state_report(p_dev->dev_name, p_dev->dev_state);
                            }
                        }
                        else//monitoring...
                        {

                        }
                    }
                }
            }
        }

        GxCore_MutexUnlock(p_dev->dev_mutex);
        p_dev_list = p_dev_list->next;
    }

    GxCore_MutexUnlock(s_network_service_mutex);
    GxCore_ThreadDelay(CONSOLE_DELAY_MS);
}

GxServiceClass app_network_service =
{
    "app network service",
    AppNetworkServiceInit,
    AppNetworkServiceDestroy,
    AppNetworkServiceRecvMsg,
    AppNetworkServiceConsole,
};

#endif
//#endif
