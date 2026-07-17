#ifndef __NET_DEFAULT_PARAM__
#define __NET_DEFAULT_PARAM__

/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef ECOS_OS

#define WIFI_STATUS_KEY "network>status"
#define WIFI_STATUS_DEFAULT "none"
#define DEV_WIFI_MODE_KEY "network>wifimode"
#define IP_TYPE_WIFI_KEY  "network>wifiiptype"
#define WIFI_IP_ADDR_KEY  "network>wifiipaddr"
#define WIFI_NET_MASK_KEY "network>wifinetmask"
#define WIFI_GATE_WAY_KEY "network>wifigateway"
#define WIFI_DNS1_KEY     "network>wifidns1"
#define WIFI_DNS2_KEY     "network>wifidns2"

#define DEV_3G_MODE_KEY "network>3gmode"
#define IP_TYPE_3G_KEY "network>3giptype"
#define USB3G_IP_ADDR_KEY  "network>3gipaddr"
#define USB3G_NET_MASK_KEY "network>3gnetmask"
#define USB3G_GATE_WAY_KEY "network>3ggateway"
#define USB3G_DNS1_KEY     "network>3gdns1"
#define USB3G_DNS2_KEY     "network>3gdns2"

#define DEV_GPRS_MODE_KEY "network>gprsmode"
#define IP_TYPE_GPRS_KEY "network>gprsiptype"
#define GPRS_IP_ADDR_KEY  "network>gprsipaddr"
#define GPRS_NET_MASK_KEY "network>gprsnetmask"
#define GPRS_GATE_WAY_KEY "network>gprsgateway"
#define GPRS_DNS1_KEY     "network>gprsdns1"
#define GPRS_DNS2_KEY     "network>gprsdns2"

#define WIRED_STATUS_KEY  WIFI_STATUS_KEY //"network>status"
#define WIRED_STATUE_DEFAULT "none"
#define DEV_WIRED_MODE_KEY "network>wiredmode"
#define IP_TYPE_WIRED_KEY "network>wirediptype"
#define WIRED_IP_ADDR_KEY  "network>wiredipaddr"
#define WIRED_NET_MASK_KEY "network>wirednetmask"
#define WIRED_GATE_WAY_KEY "network>wiredgateway"
#define WIRED_DNS1_KEY     "network>wireddns1"
#define WIRED_DNS2_KEY     "network>wireddns2"

#define RNDIS_STATUS_KEY  WIFI_STATUS_KEY//"network>rndista"
#define RNDIS_MODE_KEY    "network>rndismode"
#define RNDIS_TYPE_KEY    "network>rndisiptype"
#define RNDIS_IP_ADDR_KEY  "network>rndisipaddrkey"
#define RNDIS_NET_MASK_KEY "network>rndisnetmaskkey"
#define RNDIS_GATE_WAY_KEY "network>rndisgateway"
#define RNDIS_DNS1_KEY     "network>rndisdns1key"
#define RNDIS_DNS2_KEY     "network>rndisdns2key"
#define DEV_USBETH_MODE_KEY "network>devusbwiredmodekey"
#define IP_TYPE_USBETH_KEY  "network>iptypeusbnetkey"
#define USBETH_IP_ADDR_KEY  "network>usbnetipaddrkey"
#define USBETH_NET_MASK_KEY "network>usbnetnetmaskkey"
#define USBETH_GATE_WAY_KEY "network>usbnetgatewaykey"
#define USBETH_DNS1_KEY     "network>usbnetdns1key"
#define USBETH_DNS2_KEY     "network>usbnetdns2key"

#define DEV_MODE_DEFALT 1   //1:open 0:close
#define IP_TYPE_DEFALT  1   //1:dhcp 0:static

#define DEV_MODE_DEFALT 1   //1:open 0:close
#define IP_TYPE_DEFALT  1   //1:dhcp 0:static
#define WIFI_IP_ADDR_DEFALT  "0.0.0.0"
#define WIFI_NET_MASK_DEFALT "0.0.0.0"
#define WIFI_GATE_WAY_DEFALT "0.0.0.0"
#define WIFI_DNS1_DEFALT     "0.0.0.0"
#define WIFI_DNS2_DEFALT     "0.0.0.0"
#define USB3G_IP_ADDR_DEFALT  "0.0.0.0"
#define USB3G_NET_MASK_DEFALT "0.0.0.0"
#define USB3G_GATE_WAY_DEFALT "0.0.0.0"
#define USB3G_DNS1_DEFALT     "0.0.0.0"
#define USB3G_DNS2_DEFALT     "0.0.0.0"
#define GPRS_IP_ADDR_DEFALT  "0.0.0.0"
#define GPRS_NET_MASK_DEFALT "0.0.0.0"
#define GPRS_GATE_WAY_DEFALT "0.0.0.0"
#define GPRS_DNS1_DEFALT     "0.0.0.0"
#define GPRS_DNS2_DEFALT     "0.0.0.0"
#define WIRED_IP_ADDR_DEFALT  "0.0.0.0"
#define WIRED_NET_MASK_DEFALT "0.0.0.0"
#define WIRED_GATE_WAY_DEFALT "0.0.0.0"
#define WIRED_DNS1_DEFALT     "0.0.0.0"
#define WIRED_DNS2_DEFALT     "0.0.0.0"

#define WIFI_IP_ADDR_DEFALT  "0.0.0.0"
#define WIFI_NET_MASK_DEFALT "0.0.0.0"
#define WIFI_GATE_WAY_DEFALT "0.0.0.0"
#define WIFI_DNS1_DEFALT     "0.0.0.0"
#define WIFI_DNS2_DEFALT     "0.0.0.0"
#define USB3G_IP_ADDR_DEFALT  "0.0.0.0"
#define USB3G_NET_MASK_DEFALT "0.0.0.0"
#define USB3G_GATE_WAY_DEFALT "0.0.0.0"
#define USB3G_DNS1_DEFALT     "0.0.0.0"
#define USB3G_DNS2_DEFALT     "0.0.0.0"
#define GPRS_IP_ADDR_DEFALT  "0.0.0.0"
#define GPRS_NET_MASK_DEFALT "0.0.0.0"
#define GPRS_GATE_WAY_DEFALT "0.0.0.0"
#define GPRS_DNS1_DEFALT     "0.0.0.0"
#define GPRS_DNS2_DEFALT     "0.0.0.0"
#define WIRED_IP_ADDR_DEFALT  "0.0.0.0"
#define WIRED_NET_MASK_DEFALT "0.0.0.0"
#define WIRED_GATE_WAY_DEFALT "0.0.0.0"
#define WIRED_DNS1_DEFALT     "0.0.0.0"
#define WIRED_DNS2_DEFALT     "0.0.0.0"
#define USBETH_IP_ADDR_DEFALT  "0.0.0.0"
#define USBETH_NET_MASK_DEFALT "0.0.0.0"
#define USBETH_GATE_WAY_DEFALT "0.0.0.0"
#define USBETH_DNS1_DEFALT     "8.8.8.8"
#define USBETH_DNS2_DEFALT     "208.67.222.222"
#define NETWORK_IP_ADDR_DEFALT  "0.0.0.0"
#define NETWORK_NET_MASK_DEFALT "0.0.0.0"
#define NETWORK_GATE_WAY_DEFALT "0.0.0.0"
#define NETWORK_DNS1_DEFALT     "0.0.0.0"
#define NETWORK_DNS2_DEFALT     "0.0.0.0"

#define AP0_DEV_KEY "network>ap0dev"
#define AP0_SSID_KEY "network>ap0ssid"
#define AP0_MAC_KEY "network>ap0mac"
#define AP0_PSK_KEY "network>ap0psk"

#define AP_DEV_DEFAULT NET_IF_DEV_WIFI0
#define AP_SSID_DEFAULT "NO NAME-"
#define AP_MAC_DEFAULT "00:00:00:00:00:00"
#define AP_PSK_DEFAULT ""

#endif

#ifdef __cplusplus
}
#endif

#endif


