#ifndef __APP_NETWORK_H__
#define __APP_NETWORK_H__

//#ifdef LINUX_OS
#if NETWORK_SUPPORT

#include "app.h"

#define NET_IF_DEV_LOOP    "lo"
#define NET_IF_DEV_ETH0    "eth0"
#define NET_IF_DEV_USB0    "usb0"
#define NET_IF_DEV_ETH1    "eth1"
#ifdef ECOS_OS
#define NET_IF_DEV_WIFI0 "ra0"
#define NET_IF_DEV_WIFI_NLEN 3
#else
#define NET_IF_DEV_WIFI0 "wlan0"
#define NET_IF_DEV_WIFI_NLEN 5
#endif
#define NET_IF_DEV_USBBT    "USBBT0"

#define MAX_NET_DEV_NUM (10)
#define MAX_AP_NUM (80)

// Get from wlantools wireless.10.h macro:IW_ESSID_MAX_SIZE
#define ESSID_MAX_SIZE (31)

typedef enum
{
#if (ETH_SUPPORT > 0)
    IF_TYPE_WIRED = 0,
#endif
#if USB_ETH_SUPPORT
    IF_TYPE_USB_WIRED,
#endif
#if (WIFI_SUPPORT > 0)
    IF_TYPE_WIFI,
#endif
#if (BLUETOOTH_SUPPORT > 0)
    IF_TYPE_BLUETOOTH,
#endif
#if (MOD_3G_SUPPORT > 0)
    IF_TYPE_3G,
#ifdef LINUX_OS
    IF_TYPE_ETH3G,
#endif
#endif
#if (GPRS_SUPPORT > 0)
	IF_TYPE_GPRS,
#endif
#if (RNDIS_SUPPORT > 0)
#ifdef LINUX_OS
    IF_TYPE_RNDIS,
#endif
#endif
    IF_TYPE_UNKOWN
}IfType;

typedef enum
{
    BT_A2DP_NONE = 0,
    BT_A2DP_INIT_SUCCESSFUL,
    BT_A2DP_SCAN_FINISH,
    BT_A2DP_SINK_INFO,
    BT_A2DP_SINK_CONNECT,
    BT_A2DP_SINK_DISCONNECT,
    BT_A2DP_SINK_CONNECT_FAILED,
    BT_A2DP_SINK_PLAY,
    BT_A2DP_SINK_PAUSE,
    BT_SCAN_RESULT,
    BT_A2DP_SOURCE_CONNECT,
    BT_A2DP_SOURCE_RECONNECT,
    BT_A2DP_SOURCE_CONNECT_FAILED,
    BT_A2DP_SOURCE_DISCONNECT,
    BT_A2DP_SOURCE_PLAY,
    BT_A2DP_SOURCE_PAUSE,
    BT_A2DP_SOURCE_WAIT,
    BT_A2DP_SOURCE_REJ,
    BT_A2DP_SOURCE_DISCONNECTED,
    BT_A2DP_SOURCE_SBC_PARAM,
    BT_A2DP_SOURCE_CONNETING,
    BT_A2DP_SINK_CONNETING,
    BT_A2DP_SINK_DISCONNECT_NO_REP
}BlueToothStatus;

typedef enum
{
    IF_STATE_INVALID = (1 << 0),
    IF_STATE_IDLE = (1 << 1),
    IF_STATE_START = (1 << 2),
    IF_STATE_CONNECTING = (1 << 3),
    IF_STATE_CONNECTED = (1 << 4),
    IF_STATE_REJ = (1 << 5),
    IF_STATE_DISCONNECTED = (1 << 6),
}IfState;

typedef enum
{
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_SHARED = (1<<0),
    WIFI_AUTH_WPA1PSK = (1<<1),
    WIFI_AUTH_WPA2PSK = (1<<2),
    WIFI_AUTH_WPA1 = (1<<3),
    WIFI_AUTH_WPA2 = (1<<4),
}WifiAuthType;

typedef enum
{
    WIFI_ENCRYP_NONE = 0,
    WIFI_ENCRYP_WEP = (1<<0),
    WIFI_ENCRYP_TKIP = (1<<1),
    WIFI_ENCRYP_AES = (1<<2),
}WifiEncrypType;

typedef enum
{
    IF_FILE_NONE = 0,
    IF_FILE_START,
    IF_FILE_WAIT,
    IF_FILE_OK,
    IF_FILE_FAIL,
    IF_FILE_REJ,
    IF_FILE_DISCONNECTED
}IfFileRet;

typedef struct
{
    ubyte64 ap_essid;
    ubyte32 ap_mac;
    ubyte32 ap_psk;
    unsigned char ap_quality;
    unsigned short encryption;
    uint8_t hidden;
}ApInfo;

typedef struct
{
    ubyte64 bt_name;
    ubyte32 ap_mac;
}BtApInfo;

typedef struct
{
    unsigned int dev_num;
    ubyte64 dev_name[MAX_NET_DEV_NUM];
}GxMsgProperty_NetDevList;

typedef struct
{
    ubyte64 dev_name;
    IfState dev_state;
}GxMsgProperty_NetDevState;

typedef struct
{
    ubyte64 dev_name;
    IfType dev_type;
}GxMsgProperty_NetDevType;

typedef struct
{
    ubyte64 dev_name;
    char ip_type;
    ubyte32 ip_addr;
    ubyte32 net_mask;
    ubyte32 gate_way;
    ubyte32 dns1;
    ubyte32 dns2;
}GxMsgProperty_NetDevIpcfg;

typedef struct
{
    ubyte64 dev_name;
    unsigned char mode;
}GxMsgProperty_NetDevMode;

typedef struct
{
    ubyte64 dev_name;
    ubyte32 dev_mac;
}GxMsgProperty_NetDevMac;

typedef struct
{
    ubyte64 wifi_dev;
    unsigned int ap_num;
    ApInfo *ap_info;
#if (BLUETOOTH_SUPPORT > 0)
    ubyte64 bluetooth_dev; //
    BtApInfo *bt_ap_info;
#endif
}GxMsgProperty_ApList;

typedef struct
{
    ubyte64 dev_name;
    char ap_essid[ESSID_MAX_SIZE + 1];
}GxMsgProperty_ScanHideAp;

typedef struct
{
    ubyte64 wifi_dev;
#if (BLUETOOTH_SUPPORT > 0)
  ubyte64 bluetooth_dev;//--
#endif
    char ap_essid[ESSID_MAX_SIZE + 1];
    ubyte32 ap_mac;
    ubyte32 ap_psk;
    int hidden;
}GxMsgProperty_CurAp;

typedef struct
{
	ubyte64 dev_name;
    unsigned char operator_id; //operator
    ubyte32 apn;
    ubyte32 phone;
    ubyte32 username;
    ubyte32 password;
}GxMsgProperty_3G;

typedef struct
{
    unsigned char operator_id;
}GxMsgProperty_3G_Operator;

typedef struct
{
	ubyte64 dev_name;
    unsigned char mode;
    ubyte32 username;
    ubyte32 passwd;
    ubyte32 dns1;
    ubyte32 dns2;
}GxMsgProperty_PppoeCfg;

IfState app_network_get_cur_dev_and_state(ubyte64 *get_dev_name);
IfType app_if_dev_check_type(const char *dev_name);
#if (BLUETOOTH_SUPPORT > 0)
extern char net_if_dev_usb_bt[16];
#endif
// user select wifi, then set active
int  app_network_wifi_switch_dev_active_by_name(char *dev_name);
// wifi dev count
int  app_network_wifi_get_all_dev_plug_count(void);
// wifi active
int app_network_wifi_check_dev_is_active(char *dev_name);
// wifi plug
int app_network_wifi_check_dev_plug_status(char *dev_name);
#endif
//#endif
#endif
