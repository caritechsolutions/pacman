/* This file is part of the boot loader */

/*
 * net.h
 *
 * network layer
 *
 * first revision by Ho Lee 11/06/2002
 */

#ifndef _NET_H_
#define _NET_H_

#include <net_ipv4.h>
#include <stdio.h>
#include <util.h>
#include <string.h>

/*
 *	Network device statistics. Akin to the 2.0 ether stats but
 *	with byte counters.
 */

struct sk_buff {
    unsigned char *data;    // pointer to packet data
    unsigned int len;       // size of packet
    unsigned int buflen;    // size of allocated buffer
    int valid;              // data is available (set by skb_put())
    int owned;              // owned by a process (set by skb_get())
    int timestamp;          // sequence number (set by skb_put() and verified by skb_get())
    struct sk_buff *next;   // next socket buffer with data

    // after packet parsing
    struct ethhdr *eth;
    unsigned char *ethdata;
    int ethdata_len;

    struct arprequest *arp;
    struct iphdr *ip;
    unsigned char *ipdata;
    int ipdata_len;

    struct icmphdr *icmp;
    struct udphdr *udp;
    unsigned char *udpdata;
    int udpdata_len;

    struct bootphdr *bootp;
    struct tftphdr *tftp;

	//
	struct net_device *dev;
};

#define PKTSIZE         1522

/* ARP hardware address length */
#define ARP_HLEN 6
/**
 * struct eth_pdata - Platform data for Ethernet MAC controllers
 *
 * @iobase: The base address of the hardware registers
 * @enetaddr: The Ethernet MAC address that is loaded from EEPROM or env
 * @phy_interface: PHY interface to use - see PHY_INTERFACE_MODE_...
 * @max_speed: Maximum speed of Ethernet connection supported by MAC
 * @priv_pdata: device specific plat
 */
struct eth_pdata {
	phys_addr_t iobase;
	unsigned char enetaddr[ARP_HLEN];
	int phy_interface;
	int max_speed;
	void *priv_pdata;
};

enum eth_recv_flags {
	/*
	 * Check hardware device for new packets (otherwise only return those
	 * which are already in the memory buffer ready to process)
	 */
	ETH_RECV_CHECK_DEVICE       = 1 << 0,
};

enum { NETDEV_NODEV, NETDEV_DOWN, NETDEV_UP };

//
// function prototypes
//

int net_init(void);
int net_found(void);
int net_arp_setup(void);
int net_dev_up(void);
int net_dev_down(void);
struct sk_buff *net_getpacket(int timeout);
int net_sendpacket(struct sk_buff *skb, int async);
int net_defaultaction(struct sk_buff *skb);
void net_device_status(void);

struct sk_buff *skb_alloc(int len);
void skb_free(struct sk_buff *skb);
struct sk_buff *skb_dup(struct sk_buff *skb);
void skb_put(struct sk_buff *skb);
struct sk_buff *skb_get(void);

struct sk_buff *skb_alloc_tx(int len);

#endif
