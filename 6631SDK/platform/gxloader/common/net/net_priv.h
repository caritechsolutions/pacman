/* This file is part of the boot loader */

/*
 * net.h
 *
 * network layer
 *
 * first revision by Ho Lee 11/06/2002
 */

#ifndef __BOOT_LOADER_NET_H
#define __BOOT_LOADER_NET_H

#include <net_ipv4.h>
#include <stdio.h>
#include <util.h>
#include <string.h>

/*
 *	Network device statistics. Akin to the 2.0 ether stats but
 *	with byte counters.
 */
 
struct net_device_stats
{
	unsigned long	rx_packets;		/* total packets received	*/
	unsigned long	tx_packets;		/* total packets transmitted	*/
	unsigned long	rx_bytes;		/* total bytes received 	*/
	unsigned long	tx_bytes;		/* total bytes transmitted	*/
	unsigned long	rx_errors;		/* bad packets received		*/
	unsigned long	tx_errors;		/* packet transmit problems	*/
	unsigned long	rx_dropped;		/* no space in linux buffers	*/
	unsigned long	tx_dropped;		/* no space available in linux	*/
	unsigned long	multicast;		/* multicast packets received	*/
	unsigned long	collisions;

	/* detailed rx_errors: */
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;		/* receiver ring buff overflow	*/
	unsigned long	rx_crc_errors;		/* recved pkt with crc error	*/
	unsigned long	rx_frame_errors;	/* recv'd frame alignment error */
	unsigned long	rx_fifo_errors;		/* recv'r fifo overrun		*/
	unsigned long	rx_missed_errors;	/* receiver missed packet	*/

	/* detailed tx_errors */
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;
	
	/* for cslip etc */
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
};

enum { ETHIRQ_RX = 0x01, ETHIRQ_TX = 0x02 };

#define MACADDR_LEN             6
#define	IFNAMSIZ	16

struct net_device {
    int state;
    void *priv;
    int (*init)(struct net_device *dev);
    int (*open)(struct net_device *dev);
    int (*close)(struct net_device *dev);
    int (*send_packet)(struct sk_buff *skbuff, struct net_device *dev, int async);
    int (*receive_packet)(struct net_device *dev);
    void (*print_status)(struct net_device *dev);
    unsigned char dev_addr[MACADDR_LEN];

    // EEPROM
    int eeprom_size;
    unsigned short (*eeprom_readw)(struct net_device *dev, int reg);
    void (*eeprom_writew)(struct net_device *dev, int reg, int data);

	//for em86xx_eth.h
	char			name[IFNAMSIZ];
	/*	I/O specific fields */
	unsigned long		rmem_end;	/* shmem "recv" end	*/
	unsigned long		rmem_start;	/* shmem "recv" start	*/
	unsigned long		mem_end;	/* shared mem end	*/
	unsigned long		mem_start;	/* shared mem start	*/
//	unsigned long		base_addr;	/* device I/O address	*/
	unsigned int		irq;		/* device IRQ number	*/

	unsigned char		dma;		/* DMA channel		*/

	struct net_device_stats* (*get_stats)(struct net_device *dev);

	unsigned long		trans_start;	/* Time (in jiffies) of last Tx	*/
	unsigned long		last_rx;	/* Time of last Rx	*/

	unsigned			mtu;	/* interface MTU value, Maximum Transmission Unit*/
	unsigned short		hard_header_len;	/* hardware hdr length	*/

	/* Interface address info. */
	unsigned char		broadcast[MACADDR_LEN];	/* hw bcast add	*/
	unsigned char		addr_len;	/* hardware address length	*/

#define HAVE_NETDEV_POLL
	int			(*poll) (struct net_device *dev, int *quota);

};

extern struct net_device g_netdev;

#endif
