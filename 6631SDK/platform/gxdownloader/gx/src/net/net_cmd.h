#ifndef NET_CMD_H
#define NET_CMD_H

#define NETWORK_STATIC 0x02
#define NETWORK_DHCP 0x01
#define DHCPSTATE_FAILED  12 // Net is down

struct network_info{
	const  char *ipaddress;
	const  char  *netmask;
	const  char  *broadcast;
	const  char  *gateway;
	const  char  *serverip;
};

#endif
