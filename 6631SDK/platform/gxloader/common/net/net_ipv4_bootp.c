/* This file is part of the boot loader */

/*
 * net_ipv4_bootp.c
 *
 * BOOTP/DHCP client
 *
 * RFCs related to BOOTP/DHCP
 *   RFC 951  "Bootstrap Protocol" (Obsoleted by 1395)
 *   RFC 1395 "BOOTP Vendor Information Extensions" (Obsoleted by 1497)
 *   RFC 1497 "BOOTP Vendor Information Extensions" (Obsoleted by 1533)
 *   RFC 1533 "DHCP Options and BOOTP Vendor Extensions" (Obsoleted by 2132)
 *   RFC 1534 "Interoperation Between DHCP and BOOTP"
 *   RFC 2132 "DHCP Options and BOOTP Vendor Extensions"
 *
 * by Ho Lee 11/19/2002
 */

#include "config.h"
#include "net_priv.h"
#include "net_ipv4.h"
#include "bootconfig.h"
#include "time.h"

#include <net.h>

/*
 *  DHCP mechanism :
 *
 *  client              server
 *  --------------------------------
 *  DHCPDISCOVER =>                     : client broadcasts looking for DHCP server
 *                      <= DHCPOFFER    : server broadcasts to client with offer (IP address)
 *  DHCPREQUEST =>                      : client broadcasts to server requesting IP which server offered
 *                      <= DHCPACK      : server response with ACK
*/

// data structure for saving infomation from DHCP option
// used by dhcp_parse_option()
typedef struct dhcp_info {
	int message_type;
	uint32_t server_id;
	uint32_t lease_time;
	in_addr_t subnet;
	in_addr_t router;
	in_addr_t dns_server;
	in_addr_t broadcast;
	char domain[64];
	char tftpserver[64];
	char bootfile[128];
} dhcp_info_t;

//
// function prototypes
//

int dhcp_send_discover(unsigned int xid);
int dhcp_send_request(unsigned int xid, in_addr_t ipaddr, in_addr_t serverid);
void dhcp_parse_option(unsigned char *str, dhcp_info_t *info);

// parse bootp packet
int bootp_parsepacket(struct sk_buff *skb)
{
	skb->bootp = (struct bootphdr *) ((char *) skb->udp + sizeof(struct udphdr));
	skb->bootp->bp_secs = ntohs(skb->bootp->bp_secs);
	skb->bootp->bp_flags = ntohs(skb->bootp->bp_flags);
	skb->bootp->bp_ciaddr = ntohl(skb->bootp->bp_ciaddr);
	skb->bootp->bp_yiaddr = ntohl(skb->bootp->bp_yiaddr);
	skb->bootp->bp_siaddr = ntohl(skb->bootp->bp_siaddr);
	skb->bootp->bp_giaddr = ntohl(skb->bootp->bp_giaddr);

	return 0;
}

// bootp == 0 : DHCP protocol
// bootp != 0 : BOOP protocol
int ipv4_bootp(int bootp)
{
	int retry;
	unsigned int xid;
	struct sk_buff *skb;
	dhcp_info_t info;
	in_addr_t yiaddr;
	unsigned int startticks;

	// bootp header
	memcpy(&xid, &g_arptable[ARP_CLIENT].node[2], sizeof(xid));
	//    xid += em86xx_random();

	if (bootp)
		printf("Looking for BOOTP server...\n");
	else
		printf("Discovering DHCP server...\n");

	// BOOTP :
	// DHCP : send DHCPDISCOVER message
	// wating for reply
	for (retry = 1; retry <= MAX_BOOTP_RETRIES; ++retry) {
		// flush RX buffer
		eth_receive(WAIT_NOTHING, 0);

		// send discover
		if (bootp)
			;
		else
			dhcp_send_discover(xid);

		startticks = gx_time_get_ms();

		do {
			if ((skb = udp_receive(0, 0, IPPORT_BOOTP_CLIENT, TIMEOUT)) != NULL) {
				if (bootp_parsepacket(skb) == 0 &&
						skb->bootp->bp_op == BOOTP_REPLY && skb->bootp->bp_xid == xid)
					break;

				skb_free(skb);
				skb = NULL;
			}
		} while (!timer_timeout(startticks, TIMEOUT));

		if (skb)
			break;

		if (retry == MAX_BOOTP_RETRIES)
			printf("Timeout. Connection failed.\n");
		else
			printf("Timeout. Retrying...\n");
	}

	// BOOTP :
	// DHCP : got DHCPOFFER
	if (skb) {
		if (bootp) {
			// Not implemented
			skb_free(skb);
			return 1;
		} else {
			memset(&info, 0, sizeof info);
			dhcp_parse_option(skb->bootp->bp_vend, &info);
			if (info.message_type != DHCPOFFER)
				return 1;

			yiaddr = skb->bootp->bp_yiaddr;
			skb_free(skb);

			printf("Sending DHCP request...\n");

			// Send request and wait for reply
			startticks = gx_time_get_ms();

			for (retry = 1; retry <= MAX_BOOTP_RETRIES; ++retry) {
				do {
					dhcp_send_request(xid, yiaddr, info.server_id);
					if ((skb = udp_receive(0, 0, IPPORT_BOOTP_CLIENT, TIMEOUT)) != NULL) {
						if (bootp_parsepacket(skb) == 0 &&
								skb->bootp->bp_op == BOOTP_REPLY && skb->bootp->bp_xid == xid) {
							memset(&info, 0, sizeof info);
							dhcp_parse_option(skb->bootp->bp_vend, &info);
							if (info.message_type == DHCPACK)
								break;
							else if (info.message_type == DHCPOFFER) {
								yiaddr = skb->bootp->bp_yiaddr;
								skb_free(skb);
								continue;
							}
						}

						skb_free(skb);
						skb = NULL;
					}
				} while (!timer_timeout(startticks, TIMEOUT));

				if (skb)
					break;

				if (retry == MAX_BOOTP_RETRIES)
					printf("Timeout. Connection failed.\n");
				else
					printf("Timeout. Retrying...\n");
			}

			if (skb) {
				g_bootconfig.ipaddr = skb->bootp->bp_yiaddr;
				g_bootconfig.netmask = info.subnet;
				g_bootconfig.gateway = info.router;
				g_bootconfig.dns = info.dns_server;
				if (info.domain[0] != 0)
					strcpy(g_bootconfig.domain, info.domain);
				net_arp_setup();

				printf("Got IP Address %s\n", ipaddr_to_str(skb->bootp->bp_yiaddr));

#ifdef CONFIG_ENABLE_NETWORK_DNS
				// parse server address if available
				if (info.tftpserver[0] != 0) {
					printf("Resolving tftp server %s...\n", info.tftpserver);
					g_bootconfig.server = ipv4_gethostbyname(info.tftpserver);
					if (!ipv4_ipaddr_valid(g_bootconfig.server)) {
						char hostname[256];
						sprintf(hostname, "%s.%s", info.tftpserver, g_bootconfig.domain);
						printf("Resolving tftp server %s...\n", hostname);
						g_bootconfig.server = ipv4_gethostbyname(hostname);
					}
					if (ipv4_ipaddr_valid(g_bootconfig.server)) {
						printf("Set server address to tftp server address %s\n",
								ipaddr_to_str(g_bootconfig.server));
					} else
						printf("Resolving failed\n");
				}
#endif
				if (!ipv4_ipaddr_valid(g_bootconfig.server)) {
					g_bootconfig.server = skb->bootp->bp_siaddr;
					printf("Set server address to DHCP server address %s\n",
							ipaddr_to_str(skb->bootp->bp_siaddr));
				}

				// boot filename
				if (info.bootfile[0] != 0) {
					strcpy(g_bootconfig.kernel_filename, info.bootfile);
					printf("Set kernel filename to %s\n", info.bootfile);
				}
				else if (skb->bootp->bp_file[0] != 0) {
					strcpy(g_bootconfig.kernel_filename, (char*)skb->bootp->bp_file);
					printf("Set kernel filename to %s\n", skb->bootp->bp_file);
				}

				skb_free(skb);

				return 0;
			}
		}
	}

	return 1;
}

/*
 * DHCP protocol definitions
 *
 * from RFC 2132
 */

#define DHCP_MAGIC              0x63825363

#define DHCP_PADDING            0x00    //
#define DHCP_SUBNET             0x01    // [4] subnet mask
#define DHCP_TIME_OFFSET        0x02    // [4] client's subnet in section from UTC
#define DHCP_ROUTER             0x03    // [x4] router addresses
#define DHCP_TIME_SERVER        0x04    // [x4] time server addresses
#define DHCP_NAME_SERVER        0x05    // [x4] name server addresses
#define DHCP_DNS_SERVER         0x06    // [x4] DNS server addresses
#define DHCP_LOG_SERVER         0x07    // [x4] log server addresses
#define DHCP_COOKIE_SERVER      0x08    // [x4] cookie server addresses
#define DHCP_LPR_SERVER         0x09    // [x4] LPR (line printer server) addresses
#define DHCP_IMPRESS_SERVER     0x0a    // [x4] impress server addresses
#define DHCP_RESOURCE_SERVER    0x0b    // [x4] resource location server addresses
#define DHCP_HOST_NAME          0x0c    // [v] name of the client
#define DHCP_BOOT_SIZE          0x0d    // [2] boot file size
#define DHCP_MERIT_DUMP_FILE    0x0e    // [v] name of the file to which the client's core image should be dumped
#define DHCP_DOMAIN_NAME        0x0f    // [v] domain name
#define DHCP_SWAP_SERVER        0x10    // [4] IP address of the swap server
#define DHCP_ROOT_PATH          0x11    // [v] root disk's path name
#define DHCP_EXTENSION_PATH     0x12    // [v] file name retrievable via TFTP
#define DHCP_IP_FORWARDING      0x13    // [1] IP forwarding enable / disable
#define DHCP_SOURCE_ROUTING     0x14    // [1] Non-local source routing enable / disable
#define DHCP_POLICY_FILTER      0x15    // [x8] policy filter for non-local source routing
#define DHCP_MAX_DGRAM_REASM    0x16    // [2] maximum datagram reassembly size
#define DHCP_IP_TTL             0x17    // [1] IP Time-to-live
#define DHCP_PATH_MTU_TIMEOUT   0x18    // [4] Path MTU aging timeout
#define DHCP_PATH_MTU_PLATEAU   0x19    // [x2] Path MTU plateau table
#define DHCP_MTU                0x1a    // [2] interface MTU
#define DHCP_SUBNET_LOCAL       0x1b    // [1] All subnets are local option
#define DHCP_BROADCAST          0x1c    // [4] broadcast address
#define DHCP_MASK_DISCOVERY     0x1d    // [1] perform mask discovery
#define DHCP_MASK_SUPPLIER      0x1e    // [1] mask supplier
#define DHCP_ROUTER_DISCOVERY   0x1f    // [1] perform router discovery
#define DHCP_ROUTER_SOLICITATE  0x20    // [4] router solicitation address
#define DHCP_STATIC_ROUTE       0x21    // [x8] static route
#define DHCP_TRAILER_ENCAP      0x22    // [1] trailer encapsulation
#define DHCP_ARP_CACHE_TIMEOUT  0x23    // [4] ARP cache timeout ini seconds
#define DHCP_ETHERNET_ENCAP     0x24    // [1] ethernet encapsulation
#define DHCP_TCP_TTL            0x25    // [1] TCP default TTL
#define DHCP_TCP_KEEPALIVE_INT  0x26    // [4] TCP keepalive interval
#define DHCP_TCP_KEEPALIVE_GAR  0x27    // [1] TCP keepalive garbage
#define DHCP_NIS_DOMAIN         0x28    // [v] NIS (Network information service) domain name
#define DHCP_NIS_SERVER         0x29    // [x4] NIS server addresses
#define DHCP_NTP_SERVER         0x2a    // [x4] NTP (Network time protocol) server addresses
#define DHCP_VENDOR_SPECIFIC    0x2b    // [v] vendor specific information
#define DHCP_NBNS_SERVER        0x2c    // [x4] NetBIOS over TCP/IP name server
#define DHCP_NBDD_SERVER        0x2d    // [x4] NetBIOS over TCP/IP datagram distribution server
#define DHCP_NB_NODETYPE        0x2e    // [1] NetBIOS over TCP/IP node type
#define DHCP_NB_SCOPE           0x2f    // [v] NetBIOS over TCP/IP scove
#define DHCP_XFONT_SERVER       0x30    // [x4] X Window font server
#define DHCP_XDISPLAY_MANAGER   0x31    // [x4] X Window system display manager
#define DHCP_REQUESTED_IP       0x32    // [4] requested IP address
#define DHCP_LEASE_TIME         0x33    // [4] IP address lease time
#define DHCPOPTION_OVERLOAD     0x34    // [1] option overload
#define DHCP_MESSAGE_TYPE       0x35    // [1] DHCP message type
#define DHCP_SERVER_ID          0x36    // [4] server identifier (DHCPOFFER, DHCPREQUEST)
#define DHCP_PARAM_REQUEST      0x37    // [v] parameter request list
#define DHCP_MESSAGE            0x38    // [v] error message
#define DHCP_MAX_SIZE           0x39    // [2] maximum DHCP message size
#define DHCP_T1                 0x3a    // [4] renewal (T1) time value
#define DHCP_T2                 0x3b    // [4] rebinding (T2) time value
#define DHCP_VENDOR_CLASS_ID    0x3c    // [v] vendor class identifier
#define DHCP_CLIENT_ID          0x3d    // [v] client identifier
#define DHCP_NISPLUS_DOMAIN     0x40    // [v] NIS+ domain name
#define DHCP_NISPLUS_SERVER     0x41    // [x4] NIS server
#define DHCP_TFTP_SERVERNAME    0x42    // [v] TFTP server
#define DHCP_BOOTFILE           0x43    // [v] bootfile name
#define DHCP_MOBILE_IP_AGENT    0x44    // [x4] Mobile IP home agent
#define DHCP_SMTP_SERVER        0x45    // [x4] SMTP (Simple Mail Transfer Protocol) server
#define DHCP_POP3_SERVER        0x46    // [x4] POP3 (Post Office Protocol) server
#define DHCP_NNTP_SERVER        0x47    // [x4] NNTP (Network News Transport Protocol) server
#define DHCP_WWW_SERVER         0x48    // [x4] default WWW (World Wide Web) server
#define DHCP_FINGER_SERVER      0x49    // [x4] default finger server
#define DHCP_IRC_SERVER         0x4a    // [x4] default IRC (internet Relay Chat) server
#define DHCP_ST_SERVER          0x4b    // [x4] StreetTalk server
#define DHCP_ST_DA_SERVER       0x4c    // [x4] StreetTalk Directory Assistance server
#define DHCP_END                0xff    //

/*
 * DHCP options definitions
 */

// option type
enum {
	DHCPOPT_INT8,                       // 1 byte integer
	DHCPOPT_INT16,                      // 2 bytes integer
	DHCPOPT_INT32,                      // 4 bytes integer
	DHCPOPT_IPADDR,                     // 4 bytes IP addresses
	DHCPOPT_IPADDR2,                    // pair of IP addresses
	DHCPOPT_STR,                        // zero-terminated string (variant size)
	DHCPOPT_VARIABLE,                   // non zero-terminated array of bytes

	DHCPOPT_LIST = 0x100,               // list
	DHCPOPT_TYPEMASK = 0xff,
};

static int s_option_len[] = {
	1, 2, 4, 4, 4, -1, -2 };

struct {
	int code;
	int flags;
} s_option_list[] = {
	{ DHCP_SUBNET,              DHCPOPT_IPADDR },
	{ DHCP_TIME_OFFSET,         DHCPOPT_INT32 },
	{ DHCP_ROUTER,              DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_TIME_SERVER,         DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_NAME_SERVER,         DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_DNS_SERVER,          DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_LOG_SERVER,          DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_COOKIE_SERVER,       DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_LPR_SERVER,          DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_IMPRESS_SERVER,      DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_RESOURCE_SERVER,     DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_HOST_NAME,           DHCPOPT_STR },
	{ DHCP_BOOT_SIZE,           DHCPOPT_INT16 },
	{ DHCP_MERIT_DUMP_FILE,     DHCPOPT_STR },
	{ DHCP_DOMAIN_NAME,         DHCPOPT_STR },
	{ DHCP_SWAP_SERVER,         DHCPOPT_IPADDR },
	{ DHCP_ROOT_PATH,           DHCPOPT_STR },
	{ DHCP_EXTENSION_PATH,      DHCPOPT_STR },
	{ DHCP_IP_FORWARDING,       DHCPOPT_INT8 },
	{ DHCP_SOURCE_ROUTING,      DHCPOPT_INT8 },
	{ DHCP_POLICY_FILTER,       DHCPOPT_IPADDR2 | DHCPOPT_LIST },
	{ DHCP_MAX_DGRAM_REASM,     DHCPOPT_INT16 },
	{ DHCP_IP_TTL,              DHCPOPT_INT8 },
	{ DHCP_PATH_MTU_TIMEOUT,    DHCPOPT_INT32 },
	{ DHCP_PATH_MTU_PLATEAU,    DHCPOPT_INT16 | DHCPOPT_LIST },
	{ DHCP_MTU,                 DHCPOPT_INT16 },
	{ DHCP_SUBNET_LOCAL,        DHCPOPT_INT8 },
	{ DHCP_BROADCAST,           DHCPOPT_IPADDR },
	{ DHCP_MASK_DISCOVERY,      DHCPOPT_INT8 },
	{ DHCP_MASK_SUPPLIER,       DHCPOPT_INT8 },
	{ DHCP_ROUTER_DISCOVERY,    DHCPOPT_INT8 },
	{ DHCP_ROUTER_SOLICITATE,   DHCPOPT_IPADDR },
	{ DHCP_STATIC_ROUTE,        DHCPOPT_IPADDR2 | DHCPOPT_LIST },
	{ DHCP_TRAILER_ENCAP,       DHCPOPT_INT8 },
	{ DHCP_ARP_CACHE_TIMEOUT,   DHCPOPT_INT32 },
	{ DHCP_ETHERNET_ENCAP,      DHCPOPT_INT8 },
	{ DHCP_TCP_TTL,             DHCPOPT_INT8 },
	{ DHCP_TCP_KEEPALIVE_INT,   DHCPOPT_INT32 },
	{ DHCP_TCP_KEEPALIVE_GAR,   DHCPOPT_INT32 },
	{ DHCP_NIS_DOMAIN,          DHCPOPT_STR },
	{ DHCP_NIS_SERVER,          DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_NTP_SERVER,          DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_VENDOR_SPECIFIC,     DHCPOPT_STR },
	{ DHCP_NBNS_SERVER,         DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_NBDD_SERVER,         DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_NB_NODETYPE,         DHCPOPT_INT8 },
	{ DHCP_XFONT_SERVER,        DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_XDISPLAY_MANAGER,    DHCPOPT_IPADDR },
	{ DHCP_REQUESTED_IP,        DHCPOPT_IPADDR },
	{ DHCP_LEASE_TIME,          DHCPOPT_INT32 },
	{ DHCPOPTION_OVERLOAD,      DHCPOPT_INT8 },
	{ DHCP_MESSAGE_TYPE,        DHCPOPT_INT8 },
	{ DHCP_SERVER_ID,           DHCPOPT_INT32 },
	{ DHCP_PARAM_REQUEST,       DHCPOPT_INT8 | DHCPOPT_LIST },
	{ DHCP_MESSAGE,             DHCPOPT_STR },
	{ DHCP_MAX_SIZE,            DHCPOPT_INT16 },
	{ DHCP_T1,                  DHCPOPT_INT32 },
	{ DHCP_T2,                  DHCPOPT_INT32 },
	{ DHCP_VENDOR_CLASS_ID,     DHCPOPT_STR },
	{ DHCP_CLIENT_ID,           DHCPOPT_VARIABLE },
	{ DHCP_NISPLUS_DOMAIN,      DHCPOPT_STR },
	{ DHCP_NISPLUS_SERVER,      DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_TFTP_SERVERNAME,     DHCPOPT_STR },
	{ DHCP_BOOTFILE,            DHCPOPT_STR },
	{ DHCP_MOBILE_IP_AGENT,     DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_SMTP_SERVER,         DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_POP3_SERVER,         DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_NNTP_SERVER,         DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_WWW_SERVER,          DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_FINGER_SERVER,       DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_IRC_SERVER,          DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_ST_SERVER,           DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ DHCP_ST_DA_SERVER,        DHCPOPT_IPADDR | DHCPOPT_LIST },
	{ 0,                        0 },
};

/* OPTION : [code] [len] [data...] */
#define DHCPOPT_CODE            0
#define DHCPOPT_LEN             1
#define DHCPOPT_DATA            2

/*
 * function prototypes
 */

int dhcp_send_discover(unsigned int xid);
int dhcp_send_request(unsigned int xid, in_addr_t ipaddr, in_addr_t serverid);

void dhcp_init_packet(bootphdr_t *pbp, int type);
void dhcp_add_request_option(bootphdr_t *pbp);

void dhcp_option_appendstr(unsigned char *str, unsigned char *opt);
void dhcp_option_add(unsigned char *str, int code, unsigned int data, int list);

/*
 * DHCP protocol
 */

/* Broadcast a DHCP discover packet to the network, with an optionally requested IP */
int dhcp_send_discover(unsigned int xid)
{
	bootpip_t packet;

	dhcp_init_packet(&packet.bp, DHCPDISCOVER);
	packet.bp.bp_xid = xid;
	dhcp_add_request_option(&packet.bp);

	return udp_transmit(INADDR_BROADCAST, IPPORT_BOOTP_CLIENT, IPPORT_BOOTP_SERVER, sizeof packet.bp, &packet, 0);
}

int dhcp_send_request(unsigned int xid, in_addr_t ipaddr, in_addr_t serverid)
{
	bootpip_t packet;

	dhcp_init_packet(&packet.bp, DHCPREQUEST);
	packet.bp.bp_xid = xid;

	dhcp_option_add(packet.bp.bp_vend, DHCP_REQUESTED_IP, ipaddr, 0);
	dhcp_option_add(packet.bp.bp_vend, DHCP_SERVER_ID, serverid, 0);
	dhcp_add_request_option(&packet.bp);

	return udp_transmit(INADDR_BROADCAST, IPPORT_BOOTP_CLIENT, IPPORT_BOOTP_SERVER, sizeof packet.bp, &packet, 0);
}

/*
 * DHCP packet
 */

/* initialize a packet with the proper defaults */
void dhcp_init_packet(bootphdr_t *pbp, int type)
{
	static char s_vendorid[] = "Sigma Designs Bootloader";
	unsigned char clientid[8];

	memset(pbp, 0, sizeof(bootphdr_t));

	switch (type) {
		case DHCPDISCOVER :
		case DHCPREQUEST :
		case DHCPRELEASE :
		case DHCPINFORM :
			pbp->bp_op = BOOTP_REQUEST;
			break;
		case DHCPOFFER :
		case DHCPACK :
		case DHCPNAK :
			pbp->bp_op = BOOTP_REPLY;
			break;
	}

	pbp->bp_htype = 1;          // 10MB ethernet
	pbp->bp_hlen = ETH_ALEN;    // 10MB ethernet
	pbp->bp_yiaddr = INADDR_NONE;
	pbp->bp_siaddr = INADDR_NONE;
	memcpy(pbp->bp_hwaddr, g_arptable[ARP_CLIENT].node, ETH_ALEN); //

	// DHCP magic code
	unsigned int *p = (unsigned int*)pbp->bp_vend;
	*p = htonl(DHCP_MAGIC);
	pbp->bp_vend[4] = DHCP_END;

	// DHCP options
	dhcp_option_add(pbp->bp_vend, DHCP_MESSAGE_TYPE, type, 0);
	dhcp_option_add(pbp->bp_vend, DHCP_VENDOR_CLASS_ID, (unsigned int) s_vendorid, 0);
	clientid[0] = 1;
	memcpy(clientid + 1, g_arptable[ARP_CLIENT].node, ETH_ALEN);
	dhcp_option_add(pbp->bp_vend, DHCP_CLIENT_ID, (unsigned int) clientid, 7);
}

void dhcp_add_request_option(bootphdr_t *pbp)
{
	static unsigned char s_request[] = {
		DHCP_SUBNET, DHCP_ROUTER, DHCP_DNS_SERVER, DHCP_HOST_NAME, DHCP_DOMAIN_NAME, DHCP_BROADCAST, DHCP_TFTP_SERVERNAME, DHCP_BOOTFILE };

	dhcp_option_add(pbp->bp_vend, DHCP_PARAM_REQUEST, (unsigned int) s_request, sizeof s_request);
}

/*
 * DHCP options :
 */

void dhcp_option_appendstr(unsigned char *str, unsigned char *opt)
{
	int i, len;

	// look for the end of option
	for (i = 4; str[i] != DHCP_END;) {
		if (str[i] == DHCP_PADDING)
			++i;
		else
			i += str[i + DHCPOPT_LEN] + 2;
	}

	len = opt[DHCPOPT_LEN] + 2;
	memcpy(str + i, opt, len);
	str[i + len] = DHCP_END;
}

// data :
//   1, 2, 4 byte integer
//   type casted pointer to string
//   type casted pointer to array
// list :
//   0 : data is normal integer or pointer to string
//   1 : data is array with one item
//   > 1 : data is array with more than one item
//   with the option type 'DHCPOPT_VARIABLE', list means size of array
void dhcp_option_add(unsigned char *str, int code, unsigned int data, int list)
{
	int i, len, copylen;
	unsigned char optstr[128];
	uint8_t *ptr8, *ptrarr8;
	uint16_t *ptrarr16, data16;
	uint32_t *ptrarr32, data32;

	for (i = 0; s_option_list[i].code != 0; ++i)
		if (s_option_list[i].code == code)
			break;

	// invalid option code
	if (s_option_list[i].code == 0)
		return;

	// code
	optstr[DHCPOPT_CODE] = (unsigned char) code;

	// copy data
	len = s_option_len[s_option_list[i].flags & DHCPOPT_TYPEMASK];
	copylen = 0;

	// don't use pointer to uint16_t or uint32_t
	// due to the alignment, pointer returns wrong value
	ptr8 = (uint8_t *) (optstr + DHCPOPT_DATA);

	switch (len) {
		case 1 :
			if (list) {
				ptrarr8 = (uint8_t *) data;
				copylen = list * 1;
				while (list-- > 0)
					*ptr8++ = *ptrarr8++;
			} else {
				copylen = 1;
				*ptr8 = (uint8_t) data;
			}
			break;
		case 2 :
			if (list) {
				ptrarr16 = (uint16_t *) data;
				copylen = list * 2;
				while (list-- > 0) {
					data16 = htons(*ptrarr16++);
					memcpy(ptr8, &data16, 2);
					ptr8 += 2;
				}
			} else {
				copylen = 2;
				data16 = htons((uint16_t) data);
				memcpy(ptr8, &data16, 2);
				ptr8 += 2;
			}
			break;
		case 4 :
			if (list) {
				ptrarr32 = (uint32_t *) data;
				copylen = list * 4;
				while (list-- > 0) {
					data32 = htonl(*ptrarr32++);
					memcpy(ptr8, &data32, 4);
					ptr8 += 4;
				}
			} else {
				copylen = 4;
				data32 = htonl((uint32_t) data);
				memcpy(ptr8, &data32, 4);
				ptr8 += 4;
			}
			break;
		case -1 :
			copylen = strlen((char *) data);
			memcpy(optstr + DHCPOPT_DATA, (void *) data, copylen);
			break;
		case -2 :
			copylen = list;
			memcpy(optstr + DHCPOPT_DATA, (void *) data, copylen);
			break;
		default :
			// invalid length.
			break;
	}

	// option length
	optstr[DHCPOPT_LEN] = copylen;

	dhcp_option_appendstr(str, optstr);
}

void dhcp_parse_option(unsigned char *str, dhcp_info_t *info)
{
	unsigned char *cp = str;
	uint8_t *ptr8;
	uint16_t data16;
	uint32_t data32;

	memset(info, 0, sizeof(dhcp_info_t));

	// look for the end of option
	for (cp += 4; *cp != DHCP_END;) {
		if (*cp == DHCP_PADDING)
			++cp;
		else {
			// don't use pointer to uint16_t or uint32_t
			// due to the alignment, pointer returns wrong value
			ptr8 = cp + DHCPOPT_DATA;
			data16 = (ptr8[0] << 8) | (ptr8[1]);
			data32 = (data16 << 16) | (ptr8[2] << 8) | (ptr8[3]);

			switch (cp[DHCPOPT_CODE]) {
				case DHCP_MESSAGE_TYPE :
					info->message_type = *ptr8;
					break;
				case DHCP_SERVER_ID :
					info->server_id = data32;
					break;
				case DHCP_LEASE_TIME :
					info->lease_time = data32;
					break;
				case DHCP_SUBNET :
					info->subnet = data32;
					break;
				case DHCP_ROUTER :
					info->router = data32;
					break;
				case DHCP_DNS_SERVER :
					info->dns_server = data32;
					break;
				case DHCP_BROADCAST :
					info->broadcast = data32;
					break;
				case DHCP_DOMAIN_NAME :
					strlcpy(info->domain, (char*)ptr8, cp[DHCPOPT_LEN] + 1);
					break;
				case DHCP_TFTP_SERVERNAME :
					strlcpy(info->tftpserver, (char*)ptr8, cp[DHCPOPT_LEN] + 1);
					break;
				case DHCP_BOOTFILE :
					strlcpy(info->bootfile, (char*)ptr8, cp[DHCPOPT_LEN] + 1);
					break;
			}

			cp += cp[DHCPOPT_LEN] + 2;
		}
	}
}

// show BOOTP packet contents
void bootp_dump_packet(struct sk_buff *skb, int need_parsing)
{
	int i;
	unsigned char *cp;

	if (need_parsing)
		bootp_parsepacket(skb);

	printf("BOOTP : \n");
	printf("  opcode : %02x (%s)\n",
			skb->bootp->bp_op,
			skb->bootp->bp_op == BOOTP_REQUEST ? "Request" : "Reply");
	printf("  htype : %02x\n", skb->bootp->bp_htype);
	printf("  hlen : %02x\n", skb->bootp->bp_hlen);
	printf("  xid : %08x\n", skb->bootp->bp_xid);
	printf("  secs : %04x\n", skb->bootp->bp_secs);
	printf("  Client Address : %s (%08x)\n",
			ipaddr_to_str(skb->bootp->bp_ciaddr), skb->bootp->bp_ciaddr);
	printf("  Your Address : %s (%08x)\n",
			ipaddr_to_str(skb->bootp->bp_yiaddr), skb->bootp->bp_yiaddr);
	printf("  Server Address : %s (%08x)\n",
			ipaddr_to_str(skb->bootp->bp_siaddr), skb->bootp->bp_siaddr);
	printf("  Gateway Address : %s (%08x)\n",
			ipaddr_to_str(skb->bootp->bp_giaddr), skb->bootp->bp_giaddr);
	printf("  Hardware Address : %02X:%02X:%02X:%02X:%02X:%02X\n",
			skb->bootp->bp_hwaddr[0], skb->bootp->bp_hwaddr[1],
			skb->bootp->bp_hwaddr[2], skb->bootp->bp_hwaddr[3],
			skb->bootp->bp_hwaddr[4], skb->bootp->bp_hwaddr[5]);
	printf("  Options :\n");

	for (cp = skb->bootp->bp_vend + 4; *cp != DHCP_END;) {
		if (*cp == DHCP_PADDING)
			++cp;
		else {
			printf("    %02x, %02x : ", cp[0], cp[1]);
			for (i = 2; i < cp[1] + 2; ++i)
				printf("%02x ", cp[i]);
			printf("\n");
			cp += cp[DHCPOPT_LEN] + 2;
		}
	}
}

