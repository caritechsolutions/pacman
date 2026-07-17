#ifndef SYNOP_GMAC_NETWORK_INTERFACE_H
#define SYNOP_GMAC_NETWORK_INTERFACE_H

#include "synopGMAC_plat.h"
#include "synopGMAC_Dev.h"
#include "net_priv.h"


#define MTU_LEN 1500

#define HANDLE_RX 0
#define HANDLE_TX 1

typedef struct synopGMACAdapterStruct{
	synopGMACdevice * synopGMACdev;
	struct net_device *synopGMACnetdev;
	struct net_device_stats synopGMACNetStats;
} synopGMACPciNetworkAdapter;

extern unsigned int synop_mac_fb;
s32   synopGMAC_init_network_interface(struct net_device *netdev);
void  synopGMAC_exit_network_interface(struct net_device *netdev);

s32 synopGMAC_linux_open(struct net_device *);
s32 synopGMAC_linux_close(struct net_device *);
s32 synopGMAC_linux_xmit_frames(struct sk_buff *skb, struct net_device *netdev, int async);
int synopGMAC_intr_handler(s32 intr_num, void * dev_id);
struct net_device_stats * synopGMAC_linux_get_stats(struct net_device *);

#endif /* End of file */
