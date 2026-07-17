#include "config.h"
#include "net_priv.h"
#include "net_ipv4.h"
#include "time.h"

#include <net.h>

#if 0
#define DPRINTF(x...)       printf(x)
#else
#define DPRINTF(x...)
#endif

//
// global variables
//

// ethernet
static unsigned char g_eth_broadcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// ARP
arptable_t g_arptable[MAX_ARPTABLE];

//
// Init
//

int ipv4_init(void)
{
	return 0;
}

void ipv4_setup_arptable(in_addr_t ipaddr, in_addr_t netmask, unsigned char *node, in_addr_t gateway, in_addr_t dns)
{
	arp_setaddr_index(ARP_CLIENT, ipaddr, netmask, node);
	arp_setaddr_index(ARP_GATEWAY, gateway, INADDR_ANY, NULL);
	arp_setaddr_index(ARP_DNS, dns, INADDR_ANY, NULL);
}

//
// TCP/IP Basic
//

// return
//   0 : A class
//   1 : B class
//   2 : C class
//   < 0 : invalid
int ipv4_get_ipaddr_class(in_addr_t ipaddr)
{
	if (IN_CLASSA(ipaddr))
		return 0;
	else if (IN_CLASSB(ipaddr))
		return 1;
	else if (IN_CLASSC(ipaddr))
		return 2;
	else
		return -1;
}

// check IP address class and return appropriate default network mask address
// return INADDR_INVALID if the address is not valid
in_addr_t ipv4_get_default_netmask(in_addr_t ipaddr)
{
	switch (ipv4_get_ipaddr_class(ipaddr)) {
		case 0 : return IN_CLASSA_NET;
		case 1 : return IN_CLASSB_NET;
		case 2 : return IN_CLASSC_NET;
		default : return INADDR_INVALID;
	}
}

// check if the IP address is valid
// IP address should not be zero or broadcast address in the subclass
int ipv4_ipaddr_valid(in_addr_t ipaddr)
{
	int netmask;

	// zero of broadcast
	if (ipaddr == INADDR_ANY || ipaddr == INADDR_BROADCAST)
		return 0;

	// check if the address is in the valid region
	if (ipv4_get_ipaddr_class(ipaddr) < 0)
		return 0;

	// check if the address is zero or broadcast in the subclass
	netmask = ipv4_get_default_netmask(ipaddr);
	ipaddr &= ~netmask;
	if (ipaddr == 0 || ipaddr == (~netmask))
		return 0;

	return 1;
}

// assign not assigned user port
int ipv4_alloc_port(void)
{
	static int s_lastport = IPPORT_RESERVED;

	if (++s_lastport == IPPORT_USERRESERVED)
		s_lastport = IPPORT_RESERVED;

	return s_lastport;
}

//
// Packet processing
//

// decide whether to accept the packet or discard it.
// called before decapsulating the packet
int ipv4_check_for_me(struct sk_buff *skb)
{
	DPRINTF("RX : %d, To = %02x:%02x:%02x:%02x:%02x:%02x, From = %02x:%02x:%02x:%02x:%02x:%02x, Proto = %02x%02x\n",
			skb->len,
			skb->data[0], skb->data[1], skb->data[2], skb->data[3], skb->data[4], skb->data[5],
			skb->data[6], skb->data[7], skb->data[8], skb->data[9], skb->data[10], skb->data[11],
			skb->data[12], skb->data[13]
	       );

	// if the destination is my MAC address, it's for me
	if (memcmp(skb->data, g_arptable[ARP_CLIENT].node, ETH_ALEN) == 0)
		return 1;

	// accept just a few of broadcast message
	// 1. BOOTP/DHCP response
	// 2. ARP request
	if (memcmp(skb->data, g_eth_broadcast, ETH_ALEN) == 0) {
		struct ethhdr *eth;
		struct iphdr *ip;
		struct udphdr *udp;
		struct arprequest *arp;

		eth = (struct ethhdr *) skb->data;

		switch (ntohs(eth->h_proto)) {
			case ETH_P_IP :
				ip = (struct iphdr *) (skb->data + sizeof(struct ethhdr));

				switch (ip->protocol) {
					case IPPROTO_UDP :
						udp = (struct udphdr *) (skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr));
						if (ntohs(udp->dest) == IPPORT_BOOTP_CLIENT)
							return 1;
						break;
				}
				break;

			case ETH_P_ARP :
				arp = (struct arprequest *) (skb->data + sizeof(struct ethhdr));
				// printf("ARP for %d, %s\n", ntohs(arp->ar_op), ipaddr_to_str(ntohl(arp->ar_tip)));
				if (ntohl(arp->ar_tip) == g_arptable[ARP_CLIENT].ipaddr)
					return 1;
				break;
		}
	}

	return 0;
}

// parse the ethernet packet
// fix the byte order
int ipv4_parsepacket(struct sk_buff *skb)
{
	int len = skb->len;

	// parse ethernet header
	skb->eth = (struct ethhdr *) skb->data;
	skb->eth->h_proto = ntohs(skb->eth->h_proto);

	skb->ethdata = skb->data + ETH_HLEN;
	len -= ETH_HLEN;
	skb->ethdata_len = len;

	// parse ethernet data
	skb->arp = NULL;
	skb->ip = NULL;
	skb->ipdata = NULL;
	skb->icmp = NULL;
	skb->udp = NULL;
	skb->udpdata = NULL;
	skb->bootp = NULL;
	skb->tftp = NULL;

	// parse each protocol header (especially byte order)
	switch (skb->eth->h_proto) {
		case ETH_P_IP :
			DPRINTF("IP : ");
			skb->ip = (struct iphdr *) skb->ethdata;

			if (ipv4_calc_checksum(skb->ip, sizeof(struct iphdr)) != 0) {
				printf("IP packet checksum error.\n");
				return 1;
			}

			skb->ip->tot_len = ntohs(skb->ip->tot_len);
			skb->ip->id = ntohs(skb->ip->id);
			skb->ip->frag_off = ntohs(skb->ip->frag_off);
			skb->ip->saddr = ntohl(skb->ip->saddr);
			skb->ip->daddr = ntohl(skb->ip->daddr);

			skb->ipdata = skb->ethdata + sizeof(struct iphdr);
			if (len < skb->ip->tot_len) {
				printf("Received IP packet is small. Received = %d, supposed to be %d\n", len, skb->ip->tot_len);
				return 1;
			}
			if (len > skb->ip->tot_len)
				len = skb->ip->tot_len;
			len -= sizeof(struct iphdr);
			skb->ipdata_len = len;

			switch (skb->ip->protocol) {
				case IPPROTO_IP :
					DPRINTF("IP\n");
					break;
				case IPPROTO_ICMP :
					DPRINTF("ICMP\n");
					skb->icmp = (struct icmphdr *) skb->ipdata;

					if (ipv4_calc_checksum(skb->icmp, len) != 0) {
						printf("ICMP packet checksum error.\n");
						return 1;
					}

					if (skb->icmp->type == ICMP_ECHOREPLY || skb->icmp->type == ICMP_ECHO) {
						skb->icmp->un.echo.id = ntohs(skb->icmp->un.echo.id);
						skb->icmp->un.echo.sequence = ntohs(skb->icmp->un.echo.sequence);
					}

					// ipv4_dump_packet(skb, 0, 0);
					break;
				case IPPROTO_UDP :
					DPRINTF("UDP\n");
					skb->udp = (struct udphdr *) skb->ipdata;

					if (ipv4_calc_tcpudp_checksum(skb->udp, len, skb->ip->saddr, skb->ip->daddr) != 0) {
						printf("UDP checksum error.\n");
						return 1;
					}

					skb->udp->src = ntohs(skb->udp->src);
					skb->udp->dest = ntohs(skb->udp->dest);
					skb->udp->len = ntohs(skb->udp->len);

					if (skb->udp->len != len) {
						printf("UDP packet size mismatch\n");
						return 1;
					}

					skb->udpdata = skb->ipdata + sizeof(struct udphdr);
					len -= sizeof(struct udphdr);
					skb->udpdata_len = len;
			}
			break;

		case ETH_P_ARP :
			skb->arp = (struct arprequest *) skb->ethdata;
			skb->arp->ar_hrd = ntohs(skb->arp->ar_hrd);
			skb->arp->ar_pro = ntohs(skb->arp->ar_pro);
			skb->arp->ar_op = ntohs(skb->arp->ar_op);
			skb->arp->ar_sip = ntohl(skb->arp->ar_sip);
			skb->arp->ar_tip = ntohl(skb->arp->ar_tip);
			DPRINTF("ARP : %d, from = %s, to = %s\n", skb->arp->ar_op,
					ipaddr_to_str(skb->arp->ar_sip),
					ipaddr_to_str(skb->arp->ar_tip));
			break;

		case ETH_P_RARP :
			DPRINTF("RARP\n");
			break;
		default :
			return 1;
	}

	return 0;
}

// return
//   0 : packet is not processed
//   1 : packet is processed
int ipv4_processpacket(struct sk_buff *skb)
{
	switch (skb->eth->h_proto) {
		case ETH_P_IP :
			if (skb->icmp && skb->icmp->type == ICMP_ECHO) {
				// reply to echo request
				int len = skb->ipdata_len;
				DPRINTF("Received ECHO request\n");
				skb->icmp->type = ICMP_ECHOREPLY;
				skb->icmp->un.echo.id = htons(skb->icmp->un.echo.id);
				skb->icmp->un.echo.sequence = htons(skb->icmp->un.echo.sequence);
				skb->icmp->checksum = 0;
				skb->icmp->checksum = ipv4_calc_checksum(skb->icmp, len);
				len += sizeof(struct iphdr);
				// transmit ICMP packet asynchronously
				ip_transmit(skb->ip->saddr, IPPROTO_ICMP, len, skb->ip, 1);
				return 1;
			}
			break;
		case ETH_P_ARP :
			// reply to ARP request
			if (skb->arp->ar_op == ARPOP_REQUEST) {
				arptable_t *arp;
				in_addr_t sipaddr, tipaddr;

				// Add or update ARP table
				tipaddr = skb->arp->ar_tip;
				if (tipaddr == g_arptable[ARP_CLIENT].ipaddr) {
					sipaddr = skb->arp->ar_sip;
					if ((arp = arp_lookup(sipaddr)) == NULL)
						arp = arp_alloc();
					if (arp) 
						arp_setaddr(arp, sipaddr, INADDR_ANY, skb->arp->ar_sha);

					// transmit ARP reply packet
					DPRINTF("Received ARP request\n");
					arp_transmit(ARPOP_REPLY, sipaddr, skb->arp->ar_sha, 1);
					return 1;
				}
			}
			break;
		case ETH_P_RARP :
			break;
	}

	return 0;
}

//
// Ethernet
//

// send ethernet packet to specified ethernet address
// dest : ethernet address
// type : ethernet protocol ID (ETH_P_xxx)
int eth_transmit(unsigned char *dest, uint16_t type, unsigned int len, const void *buf, int async)
{
	struct sk_buff *skb = skb_alloc_tx(len + ETH_HLEN);
	ethhdr_t *pethhdr = (ethhdr_t *) skb->data;

	// fill ethernet header
	memcpy(pethhdr->h_dest, dest, ETH_ALEN);    // Destination address
	memcpy(pethhdr->h_source, g_arptable[ARP_CLIENT].node, ETH_ALEN);   // Source address
	pethhdr->h_proto = htons(type);             // Frame type
	memcpy(skb->data + ETH_HLEN, buf, len);     // Frame data

	// fill sk_buff structure
	skb->len = len + ETH_HLEN;
	skb->valid = 1;

	// send it!
	return net_sendpacket(skb, async);
}

struct sk_buff *eth_receive(int type, int timeout)
{
	struct sk_buff *skb;
	unsigned int startticks = gx_time_get_ms();

#if 0 //TODO
	if (!em86xx_irqenabled()) {
		printf("Warning from eth_receive() : interrupt disabled, not able to receive new packet\n");
		return NULL;
	}
#endif
	while ((skb = net_getpacket(timeout)) != NULL) {
		// ipv4_dump_packet(skb, 0, 0);
		switch (type) {
			case WAIT_NOTHING :
				break;
			case WAIT_ARP_REPLY :
				if (skb->arp && skb->arp->ar_op == ARPOP_REPLY)
					return skb;
				break;
			case WAIT_ICMP_REPLY :
				if (skb->icmp && skb->icmp->type == ICMP_ECHOREPLY)
					return skb;
				break;
			case WAIT_UDP :
				if (skb->udp)
					return skb;
				break;
		}

		ipv4_processpacket(skb);

		skb_free(skb);

		if (timer_timeout(startticks, timeout))
			break;
	}

	return NULL;
}

//
// ARP table
//

arptable_t *arp_lookup(unsigned long ipaddr)
{
	int i;

	for (i = 0; i < MAX_ARPTABLE; ++i)
		if (g_arptable[i].ipaddr == ipaddr)
			return g_arptable + i;

	return NULL;
}

arptable_t *arp_alloc(void)
{
	int i;

	for (i = ARP_USER; i < MAX_ARPTABLE; ++i)
		if (g_arptable[i].ipaddr == INADDR_ANY)
			break;
	if (i == MAX_ARPTABLE)
		return NULL;

	return g_arptable + i;
}

int arp_valid(arptable_t *arp)
{
	int i;

	if (!ipv4_ipaddr_valid(arp->ipaddr))
		return 0;

	for (i = 0; i < ETH_ALEN; ++i)
		if (arp->node[i] != 0)
			return 1;

	return 0;
}

void arp_setaddr(arptable_t *arp, in_addr_t ipaddr, in_addr_t netmask, unsigned char *node)
{
	arp->ipaddr = ipaddr;
	arp->netmask = (netmask == INADDR_ANY) ? ipv4_get_default_netmask(ipaddr) : netmask;
	if (node)
		memcpy(arp->node, node, ETH_ALEN);
	else
		memset(arp->node, 0, ETH_ALEN);
}

void arp_setaddr_index(int index, in_addr_t ipaddr, in_addr_t netmask, unsigned char *node)
{
	arp_setaddr(g_arptable + index, ipaddr, netmask, node);
}

void arp_listtable(arptable_t *arp)
{
	int i;
	unsigned char *cp;

	for (i = 0; i < MAX_ARPTABLE; ++i) {
		if ((arp == NULL || arp == g_arptable + i) && arp_valid(g_arptable + i)) {
			cp = g_arptable[i].node;
			printf("%-20s %02X:%02X:%02X:%02X:%02X:%02X\n",
					ipaddr_to_str(g_arptable[i].ipaddr),
					cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
		}
	}
}

//
// ARP protocol
//
// Refer to RFC 826 "An Ethernet Address Resolution Protocol"
//

// send ARP packet : request or reply
// opcode : ARPOP_REQUEST or ARPOP_REPLY
// ipaddr : address to reply
// node : hardware address to reply
int arp_transmit(int opcode, in_addr_t ipaddr, unsigned char *node, int async)
{
	arprequest_t arpreq;

	// fill ARP header
	arpreq.ar_hrd = htons(ARPHRD_ETHER);
	arpreq.ar_pro = htons(ETH_P_IP);
	arpreq.ar_hln = ETH_ALEN;
	arpreq.ar_pln = 4;
	arpreq.ar_op = htons(opcode);

	// fill ARP body
	memcpy(arpreq.ar_sha, g_arptable[ARP_CLIENT].node, ETH_ALEN);
	arpreq.ar_sip = htonl(g_arptable[ARP_CLIENT].ipaddr);

	if (node)
		memcpy(arpreq.ar_tha, node, ETH_ALEN);
	else
		memset(arpreq.ar_tha, 0, ETH_ALEN);
	arpreq.ar_tip = htonl(ipaddr);

	// send ARP request (broadcast) or reply packet
	if (opcode == ARPOP_REQUEST)
		return eth_transmit(g_eth_broadcast, ETH_P_ARP, sizeof(arpreq), &arpreq, async);
	else
		return eth_transmit(node, ETH_P_ARP, sizeof(arpreq), &arpreq, async);
}

arptable_t *arp_dorequest(unsigned long ipaddr, int async)
{
	int retry;
	arptable_t *arp = arp_lookup(ipaddr);
	struct sk_buff *skb;

	// look for available ARP slot
	if (arp == NULL)
		if ((arp = arp_alloc()) == NULL)
			return NULL;

	// send ARP request packet and wait for reply
	for (retry = 1; retry <= MAX_ARP_RETRIES; retry++) {
		arp_transmit(ARPOP_REQUEST, ipaddr, NULL, async);
		skb = eth_receive(WAIT_ARP_REPLY, ARP_REQUEST_TIMEOUT);

		if (skb) {
			if (skb->arp->ar_op == ARPOP_REPLY && skb->arp->ar_sip == ipaddr) {
				arp_setaddr(arp, ipaddr, INADDR_ANY, skb->arp->ar_sha);
				skb_free(skb);
				break;
			}
		} else if (retry < MAX_ARP_RETRIES)
			printf("ARP request timeout. Retry.\n");
		else {
			printf("ARP request failed.\n");
			return NULL;
		}
	}

	return arp;
}

//
// send a IP packet
//
// Refer to RFC 791 "Internet Protocol" about IP header
//

int ip_transmit(in_addr_t destip, int protocol, int len, const void *buf, int async)
{
	struct iphdr *ip;

	len += sizeof(struct iphdr);

	// fill in the IP header
	ip = (struct iphdr *) buf;
	ip->version = 0x04;
	ip->ihl = 0x05;
	ip->tos = 0;
	ip->tot_len = htons(len);
	ip->id = 0;
	ip->frag_off = 0;
	ip->ttl = 60;
	ip->protocol = (unsigned char) protocol;
	ip->check = 0;
	ip->saddr = htonl(g_arptable[ARP_CLIENT].ipaddr);
	ip->daddr = htonl(destip);
	ip->check = ipv4_calc_checksum(ip, sizeof(struct iphdr));

	// transmit the packet
	if (destip == INADDR_BROADCAST) {
		eth_transmit(g_eth_broadcast, ETH_P_IP, len, buf, async);
	} else {
		arptable_t *arp;
		in_addr_t netmask = g_arptable[ARP_CLIENT].netmask;

		if (ipv4_ipaddr_valid(g_arptable[ARP_CLIENT].ipaddr)) {
			if ((destip & netmask) != (g_arptable[ARP_CLIENT].ipaddr & netmask)) {
				// not in the same class
				// send packet over gateway
				if (!ipv4_ipaddr_valid(g_arptable[ARP_GATEWAY].ipaddr)) {
					// now the gateway is not specified
					return 1;
				}
				destip = g_arptable[ARP_GATEWAY].ipaddr;
			}
		}

		if ((arp = arp_lookup(destip)) == NULL || !arp_valid(arp))
			if ((arp = arp_dorequest(destip, async)) == NULL)
				return 1;

		eth_transmit(arp->node, ETH_P_IP, len, buf, async);
	}

	return 0;
}

unsigned int ipv4_calc_sum(const void *ip, int len)
{
	uint16_t *_ip = (uint16_t *) ip;
	unsigned int sum = 0;
	int odd = len & 1;

	for (len >>= 1; len > 0; --len)
		sum += *(_ip++);

	// if the length is odd, assume that the next byte is padded by 0
	if (odd)
		sum += (*(uint8_t *) _ip);

	return sum;
}

unsigned short ipv4_calc_checksum(const void *ip, int len)
{
	unsigned int sum = ipv4_calc_sum(ip, len);

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ((~sum) & 0x0000FFFF);
}

//
// send a ping ICMP packet
//

int icmp_ping(in_addr_t destip, int count)
{
	static unsigned short s_id = 0, s_seq = 0;
	static unsigned char s_pingmsg[] = "FLYDUCK, SIGMADESIGNS";

	int i, nreply;
	icmpip_t packet;
	struct sk_buff *skb;

	for (i = 0, nreply = 0; i < count; ++i) {
		packet.icmp.type = ICMP_ECHO;
		packet.icmp.code = 0;
		packet.icmp.checksum = 0;
		packet.icmp.un.echo.id = htons(++s_id);
		packet.icmp.un.echo.sequence = htons(++s_seq);

		memset(packet.icmp.data, ' ', MAX_ICMP_DATA);
		memcpy(packet.icmp.data, s_pingmsg, sizeof s_pingmsg);

		packet.icmp.checksum = ipv4_calc_checksum(&packet.icmp, sizeof packet.icmp);

		// receive unused packages, prevent buf overflow
		eth_receive(WAIT_ICMP_REPLY, 10);

		if (ip_transmit(destip, IPPROTO_ICMP, sizeof packet.icmp, &packet, 0) != 0)
			break;

		if ((skb = eth_receive(WAIT_ICMP_REPLY, TIMEOUT)) != NULL) {
			printf("Reply from %s : icmp_seq = %d\n",
					ipaddr_to_str(skb->ip->saddr), skb->icmp->un.echo.sequence);
			++nreply;
		} else {
			printf("Unreachable to %s : icmp_seq = %d\n",
					ipaddr_to_str(destip), s_seq);
		}

		skb_free(skb);
	}

	return nreply;
}

//
// send a UDP datagram
//
// destip : destination IP address
// srcsock : source port number
// destsock : destination socket number
// len : size of UDP data (exept IP, UDP header)
// buf : packet buffer
//
// buffer is consisted of
//   struct iphdr ip;
//   struct udphdr udp;
//   protocol specific body;
//
// return 0 : success
// return non-zero : error
//
// Refer to RFC 768 "User Datagram Protocol" about header and checksum
//

int udp_transmit(in_addr_t destip, unsigned int srcsock, unsigned int destsock, int len, const void *buf, int async)
{
	struct udphdr *udp;

	len += sizeof(struct udphdr);

	// fill in the UDP header
	udp = (struct udphdr *)((char *)buf + sizeof(struct iphdr));
	udp->src = htons(srcsock);
	udp->dest = htons(destsock);
	udp->len = htons(len);
	udp->checksum = 0;

	udp->checksum = ipv4_calc_tcpudp_checksum(udp, len, g_arptable[ARP_CLIENT].ipaddr, destip);

	return ip_transmit(destip, IPPROTO_UDP, len, buf, async);
}

unsigned short ipv4_calc_tcpudp_checksum(const void *data, int len, in_addr_t src, in_addr_t dest)
{
	unsigned int sum;
	tcpudp_pseudohdr_t phdr;

	phdr.src = htonl(src);
	phdr.dest = htonl(dest);
	phdr.zero = 0;
	phdr.protocol = IPPROTO_UDP;
	phdr.length = htons(len);

	sum = ipv4_calc_sum(data, len);
	sum += ipv4_calc_sum((void *)&phdr, sizeof phdr);

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ((~sum) & 0x0000FFFF);
}

struct sk_buff *udp_receive(in_addr_t fromip, unsigned int srcsock, unsigned int destsock, int timeout)
{
	struct sk_buff *skb;
	unsigned int startticks = gx_time_get_ms();

	while ((skb = eth_receive(WAIT_UDP, timeout)) != NULL) {
		int match = 1;

		// printf("from = %08x, %08x, src = %04x %04x, des = %04x %04x\n",
		//   fromip, skb->ip->saddr, srcsock, skb->udp->src, destsock, skb->udp->dest);

		if (fromip) {
			if (skb->ip->saddr != fromip)
				match = 0;
		}
		if (srcsock) {
			if (skb->udp->src != srcsock)
				match = 0;
		}
		if (destsock) {
			if (skb->udp->dest != destsock)
				match = 0;
		}

		if (match) {
			return skb;
		}

		skb_free(skb);

		if (timer_timeout(startticks, timeout))
			break;
	}

	return NULL;
}

//
// Miscellaneous
//

enum {
	DUMP_PACKET_DEFAULT,
	DUMP_PACKET_BOOTP,
	DUMP_PACKET_TFTP,
};

void dump_memory(void *memptr, unsigned int addr, int num, int unit);

/* for dump_memory, add later */
/*static char *ipv4_ethproto_str(int proto)
{
	switch (proto) {
		case ETH_P_LOOP :
			return "LOOP";
		case ETH_P_IP :
			return "IP";
		case ETH_P_ARP :
			return "ARP";
		case ETH_P_RARP :
			return "RARP";
		default :
			return "Unknown";
	}
}

static char *ipv4_ipproto_str(int proto)
{
	switch (proto) {
		case IPPROTO_IP :
			return "IP";
		case IPPROTO_ICMP :
			return "ICMP";
		case IPPROTO_IGMP :
			return "IGMP";
		case IPPROTO_TCP :
			return "TCP";
		case IPPROTO_UDP :
			return "UDP";
		case IPPROTO_RAW :
			return "RAW";
		default :
			return "Other";
	}
}

static char *ipv4_ipport_str(int port)
{
	static struct {
		int port;
		char *name;
	} s_ipport_str_list[] = {
		{ IPPORT_ECHO, "echo" },
		{ IPPORT_FTP, "ftp" },
		{ IPPORT_TELNET, "telnet" },
		{ IPPORT_SMTP, "smtp" },
		{ IPPORT_DOMAINSERVER, "nameserver" },
		{ IPPORT_BOOTP_SERVER, "bootp server" },
		{ IPPORT_BOOTP_CLIENT, "bootp client" },
		{ IPPORT_TFTP, "tftp" },
		{ IPPORT_SUNRPC, "RPC" },
		{ 0, "Other" },
	};

	int i;

	for (i = 0; s_ipport_str_list[i].port != 0 && s_ipport_str_list[i].port != port; ++i)
		;

	return s_ipport_str_list[i].name;
}*/

#if 0
void ipv4_dump_packet(struct sk_buff *skb, int type, int need_parsing)
{
	if (need_parsing)
		ipv4_parsepacket(skb);

	printf("Ethernet : 0x%x (%d)\n", skb->len, skb->len);
	printf("  Target   : %02X:%02X:%02X:%02X:%02X:%02X\n",
			skb->eth->h_dest[0], skb->eth->h_dest[1],
			skb->eth->h_dest[2], skb->eth->h_dest[3],
			skb->eth->h_dest[4], skb->eth->h_dest[5]);
	printf("  Source   : %02X:%02X:%02X:%02X:%02X:%02X\n",
			skb->eth->h_source[0], skb->eth->h_source[1],
			skb->eth->h_source[2], skb->eth->h_source[3],
			skb->eth->h_source[4], skb->eth->h_source[5]);
	printf("  Protocol : %04x (%s)\n",
			skb->eth->h_proto,
			ipv4_ethproto_str(skb->eth->h_proto));
	printf("  data len : 0x%x (%d)\n", skb->ethdata_len, skb->ethdata_len);

	switch (skb->eth->h_proto) {
		case ETH_P_IP :
			printf("IP :\n");
			printf("  Version + IHL : %02x\n", skb->ethdata[0]);
			printf("  TOS           : %02x\n", skb->ip->tos);
			printf("  Length        : %04x\n", skb->ip->tot_len);
			printf("  ID            : %04x\n", skb->ip->id);
			printf("  Frag          : %04x\n", skb->ip->frag_off);
			printf("  TTL           : %02x\n", skb->ip->ttl);
			printf("  Protocol      : %02x (%s)\n", skb->ip->protocol, ipv4_ipproto_str(skb->ip->protocol));
			printf("  Checksum      : %04x\n", skb->ip->check);
			printf("  Source        : %s (%08x)\n", ipaddr_to_str(skb->ip->saddr), skb->ip->saddr);
			printf("  Target        : %s (%08x)\n", ipaddr_to_str(skb->ip->daddr), skb->ip->daddr);
			printf("  data len      : 0x%x (%d)\n", skb->ipdata_len, skb->ipdata_len);

			switch (skb->ip->protocol) {
				case IPPROTO_IP :
					printf("Raw IP packet\n");
					break;
				case IPPROTO_ICMP :
					printf("ICMP :\n");
					printf("  Type : %02x\n", skb->icmp->type);
					printf("  Code : %02x\n", skb->icmp->code);
					printf("  Checksum : %04x\n", skb->icmp->checksum);
					printf("ICMP header and data :\n");
					dump_memory(skb->ipdata, 0, skb->ipdata_len, 1);
					break;
				case IPPROTO_UDP :
					printf("UDP :\n");
					printf("  Source   : %04x (%d) (%s)\n", skb->udp->src, skb->udp->src, ipv4_ipport_str(skb->udp->src));
					printf("  Destin   : %04x (%d) (%s)\n", skb->udp->dest, skb->udp->dest, ipv4_ipport_str(skb->udp->dest));
					printf("  Length   : %04x\n", skb->udp->len);
					printf("  Checksum : %04x\n", skb->udp->checksum);
					printf("  data len : %x (%d)\n", skb->udpdata_len, skb->udpdata_len);

					switch (type) {
						case DUMP_PACKET_DEFAULT :
							printf("UDP data :\n");
							dump_memory(skb->udpdata, 0, skb->udpdata_len, 1);
							break;
						case DUMP_PACKET_BOOTP :
							bootp_dump_packet(skb, need_parsing);
							break;
						case DUMP_PACKET_TFTP :
							tftp_dump_packet(skb, need_parsing);
							break;
					}
					break;
			}
			break;

		case ETH_P_ARP :
			printf("ARP :\n");
			printf("  HWADDR type   : %04x\n", skb->arp->ar_hrd);
			printf("  Protocol type : %04x\n", skb->arp->ar_pro);
			printf("  HWADDR len    : %02x\n", skb->arp->ar_hln);
			printf("  Address len   : %02x\n", skb->arp->ar_pln);
			printf("  Opcode        : %04x\n", skb->arp->ar_op);
			printf("  Sender HWADDR : %02X:%02X:%02X:%02X:%02X:%02X\n",
					skb->arp->ar_sha[0], skb->arp->ar_sha[1],
					skb->arp->ar_sha[2], skb->arp->ar_sha[3],
					skb->arp->ar_sha[4], skb->arp->ar_sha[5]);
			printf("  Sender IPADDR : %s (%08x)\n", ipaddr_to_str(skb->arp->ar_sip), skb->arp->ar_sip);
			printf("  Target HWADDR : %02X:%02X:%02X:%02X:%02X:%02X\n",
					skb->arp->ar_tha[0], skb->arp->ar_tha[1],
					skb->arp->ar_tha[2], skb->arp->ar_tha[3],
					skb->arp->ar_tha[4], skb->arp->ar_tha[5]);
			printf("  Target IPADDR : %s (%08x)\n", ipaddr_to_str(skb->arp->ar_tip), skb->arp->ar_tip);
			break;

		case ETH_P_RARP :
			break;
	}
}
#endif
