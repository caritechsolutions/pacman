#ifndef APP_NETWORK_API_H
#define APP_NETWORK_API_H

#include "app_config.h"
#include <gxcore.h>

#ifdef LINUX_OS
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <errno.h>
#include <net/if_arp.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "net/if.h"
#include <net/if_var.h>
#include <net/if_arp.h>
#endif

#if NETWORK_SUPPORT
status_t app_if_name_convert(const char *if_name, char *display_name);
void app_network_ip_completion(char *ip);
void app_network_ip_simplify(char *buf, char *ip);
typedef enum
{
    NO_ERROR,
    IP_ADDR_ERROR,
    SUB_MASK_ERROR,
    GATE_WAY_ERROR,
    DNS1_ERROR,
    DNS2_ERROR,
    DNS_COMMON_ERROR,
    IP_OR_MASK_ERROR,
    OTHER_ERROR,
    ERROR_TYPE_TOTAL,
}NetWorkParamsError;

int app_network_ip_valid(unsigned int ip);
char *app_get_network_para_err_tip(NetWorkParamsError error_type);
status_t app_get_mac_addr(const char *if_name, char *mac);
status_t app_set_mac_addr(const char *if_name, const char *mac);

char *app_inet_ntoa(struct in_addr in);
bool app_check_netlink(const char *if_name); //if_name: eth0,ra0... etc.
status_t app_get_ip_addr(const char *if_name, char *ipaddr);
status_t app_set_ip_addr(const char *if_name, const char *ipaddr);
status_t app_get_bcast_addr(const char *if_name, char *bcast);
status_t app_set_bcast_addr(const char *if_name, const char *bcast);
status_t app_get_gateway(const char *if_name, char *gateway);
status_t app_if_up(const char *if_name);
status_t app_if_down(const char *if_name);
#endif
#endif
