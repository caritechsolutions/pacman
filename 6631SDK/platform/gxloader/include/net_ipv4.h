/* This file is part of the boot loader */

/*
 * net_ipv4.h
 *
 * UDP/IP protocol implementation (net_ipv4.c)
 * BOOTP/DHCP service (net_ipv4_bootp.c)
 * TFTP service (net_ipv4_tftp.c)
 */

#ifndef _NET_IPV4_H_
#define _NET_IPV4_H_

#include <sys/types.h>

/*
 * Basic TCP / IP related definitions
 * Some of these definitions are from /usr/include/netinet/in.h
 */

/* Standard well-defined IP protocols. : RFC 790 */
enum {
    IPPROTO_IP = 0,                     /* Dummy protocol for TCP.  */
    IPPROTO_ICMP = 1,                   /* Internet Control Message Protocol.  */
    IPPROTO_IGMP = 2,                   /* Internet Group Management Protocol. */
    IPPROTO_TCP = 6,                    /* Transmission Control Protocol.  */
    IPPROTO_UDP = 17,                   /* User Datagram Protocol.  */
    IPPROTO_RAW = 255,                  /* Raw IP packets.  */
    IPPROTO_MAX
};

/* Standard well-known ports.  */
enum {
    IPPORT_ECHO = 7,                    /* Echo service.  */
    IPPORT_DISCARD = 9,                 /* Discard transmissions service.  */
    IPPORT_SYSTAT = 11,                 /* System status service.  */
    IPPORT_DAYTIME = 13,                /* Time of day service.  */
    IPPORT_NETSTAT = 15,                /* Network status service.  */
    IPPORT_FTP = 21,                    /* File Transfer Protocol.  */
    IPPORT_TELNET = 23,                 /* Telnet protocol.  */
    IPPORT_SMTP = 25,                   /* Simple Mail Transfer Protocol.  */
    IPPORT_TIMESERVER = 37,             /* Timeserver service.  */
    IPPORT_WHOIS = 43,                  /* Internet Whois service.  */
    IPPORT_DOMAINSERVER = 53,           /* Domain name server */

    IPPORT_BOOTP_SERVER = 67,           /* Bootstrap Protocol service */
    IPPORT_BOOTP_CLIENT = 68,           /* Bootstrap Protocol service */
    IPPORT_TFTP = 69,                   /* Trivial File Transfer Protocol.  */
    IPPORT_FINGER = 79,                 /* Finger service.  */
    IPPORT_SUNRPC = 111,                /* SUN Remote Procedure Call protocol */

    /* Ports less than this value are reserved for privileged processes.  */
    IPPORT_RESERVED = 1024,

    /* Ports greater this value are reserved for (non-privileged) servers.  */
    IPPORT_USERRESERVED = 5000
};

/* 
 * Internet address 
 * Some of these definitions are from /usr/include/netinet/in.h
 */

typedef uint32_t in_addr_t;

#define INADDR_TO_STR(ipaddr)   ((unsigned char *) &(ipaddr))
#define INADDR_FROM_STR(ipaddr) (*(in_addr_t *) ipaddr)
#define INADDR_FROM_4INTS(a, b, c, d) ((d) | ((c) << 8) | ((b) << 16) | ((a) << 24))

// Definitions of the bits in an Internet address integer.
// On subnets, host and network parts are found according
// to the subnet mask, not these masks.

#define IN_CLASSA(a)            ((((long int) (a)) & 0x80000000) == 0)
#define IN_CLASSA_NET           0xff000000
#define IN_CLASSA_NSHIFT        24
#define IN_CLASSA_HOST          (0xffffffff & ~IN_CLASSA_NET)
#define IN_CLASSA_MAX           128

#define IN_CLASSB(a)            ((((long int) (a)) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET           0xffff0000
#define IN_CLASSB_NSHIFT        16
#define IN_CLASSB_HOST          (0xffffffff & ~IN_CLASSB_NET)
#define IN_CLASSB_MAX           65536

#define IN_CLASSC(a)            ((((long int) (a)) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET           0xffffff00
#define IN_CLASSC_NSHIFT        8
#define IN_CLASSC_HOST          (0xffffffff & ~IN_CLASSC_NET)

#define IN_CLASSD(a)            ((((long int) (a)) & 0xf0000000) == 0xe0000000)
#define IN_MULTICAST(a)         IN_CLASSD(a)
#define IN_MULTICAST_NET        0xF0000000

/* Address to accept any incoming messages. */
#define INADDR_ANY              ((unsigned long int) 0x00000000)
/* Address to send to all hosts. */
#define INADDR_BROADCAST        ((unsigned long int) 0xffffffff)
/* Address indicating an error return. */
#define INADDR_NONE             ((unsigned long int) 0xffffffff)
/* Address internally used as invalid address */
#define INADDR_INVALID          ((unsigned long int) 0x00000000)

/*
 * Ethernet related definitions
 * Some of the definitions are from /usr/include/linux/if_ether.h
 */

// IEEE 802.3 Ethernet magic constants.  The frame sizes omit the preamble
// and FCS/CRC (frame check sequence). 
#define ETH_ALEN                6       /* Octets in one ethernet addr   */
#define ETH_HLEN                14      /* Total octets in header.   */
#define ETH_ZLEN                60      /* Min. octets in frame sans FCS */
#define ETH_DATA_LEN            1500    /* Max. octets in payload    */
#define ETH_FRAME_LEN           1514    /* Max. octets in frame sans FCS */
#define ETH_BUF_LEN             1536    /* buffer */

// ethernet protocol ID
#define ETH_P_LOOP              0x0060  /* Ethernet Loopback packet */
#define ETH_P_IP                0x0800  /* Internet Protocol packet */
#define ETH_P_ARP               0x0806  /* Address Resolution packet    */
#define ETH_P_RARP              0x8035  /* Reverse Addr Res packet  */

// ethernet packet header
typedef struct ethhdr {
    uint8_t h_dest[ETH_ALEN];           /* destination eth addr */
    uint8_t h_source[ETH_ALEN];         /* source ether addr    */
    uint16_t h_proto;                   /* packet type ID field */
} __attribute__((packed)) ethhdr_t;

/*
 * ARP related definitions
 * Some of the definitions are from /usr/include/linux/if_arp.h
 */

// ARP retry maximum count
#define MAX_ARP_RETRIES         2

// index in the arptable
#define ARP_CLIENT              0       /* local address */ 
#define ARP_GATEWAY             1       /* gateway */
#define ARP_DNS                 2       /* DNS server */
#define ARP_USER                3       /* from this, general purpose ARP slot */
#define MAX_ARPTABLE            16      /* size of internal ARP table buffer */

// internal data structure containing ARP list
typedef struct arptable {
    in_addr_t ipaddr;                   // IP address in network byte order
    in_addr_t netmask;                  // network mask in network byte order
    uint8_t node[6];                    // hardware address
} __attribute__((packed)) arptable_t;

extern arptable_t g_arptable[];

/* ARP protocol HARDWARE identifiers. */
#define ARPHRD_ETHER            1       /* Ethernet 10Mbps      */

/* ARP protocol opcodes. */
#define ARPOP_REQUEST           1       /* ARP request          */
#define ARPOP_REPLY             2       /* ARP reply            */
#define ARPOP_RREQUEST          3       /* RARP request         */
#define ARPOP_RREPLY            4       /* RARP reply           */

typedef struct arprequest {
    uint16_t ar_hrd;                    /* format of hardware address */
    uint16_t ar_pro;                    /* format of protocol address */
    uint8_t ar_hln;                     /* length of hardware address */
    uint8_t ar_pln;                     /* length of protocol address */
    uint16_t ar_op;                     /* ARP opcode (command) */

    // Ethernet ARP packet data 
    uint8_t ar_sha[6];                  /* sender hardware address */
    in_addr_t ar_sip;                   /* sender IP address */
    uint8_t ar_tha[6];                  /* target hardware address */
    in_addr_t ar_tip;                   /* target IP address */
} __attribute__((packed)) arprequest_t;

/*
 * IP related definitions
 * Some of the definitions are from /usr/include/linux/ip.h
 */

// IP packet header
typedef struct iphdr {
#ifdef CONFIG_BIGENDIAN
    uint8_t version : 4;
    uint8_t ihl : 4;
#else
    uint8_t ihl : 4;
    uint8_t version : 4;
#endif
    uint8_t tos;                        /* */
    uint16_t tot_len;                   /* size of header + body */
    uint16_t id;                        /* */
    uint16_t frag_off;                  /* */
    uint8_t ttl;                        /* */
    uint8_t protocol;                   /* */
    uint16_t check;                     /* checksum of IP header */
    in_addr_t saddr;                    /* */
    in_addr_t daddr;                    /* */
} __attribute__((packed)) iphdr_t;

/*
 * ICMP related definitions
 * Some of these definitions are from /usr/include/linux/icmp.h
 * RFC 792
 */

// ICMP type
#define ICMP_ECHOREPLY      0           /* Echo Reply               */
#define ICMP_DEST_UNREACH   3           /* Destination Unreachable  */
#define ICMP_SOURCE_QUENCH  4           /* Source Quench            */
#define ICMP_REDIRECT       5           /* Redirect (change route)  */
#define ICMP_ECHO           8           /* Echo Request             */
#define ICMP_TIME_EXCEEDED  11          /* Time Exceeded            */
#define ICMP_PARAMETERPROB  12          /* Parameter Problem        */
#define ICMP_TIMESTAMP      13          /* Timestamp Request        */
#define ICMP_TIMESTAMPREPLY 14          /* Timestamp Reply          */
#define ICMP_INFO_REQUEST   15          /* Information Request      */
#define ICMP_INFO_REPLY     16          /* Information Reply        */
#define ICMP_ADDRESS        17          /* Address Mask Request     */
#define ICMP_ADDRESSREPLY   18          /* Address Mask Reply       */
#define NR_ICMP_TYPES       18

// ICMP data length 
#define MAX_ICMP_DATA       128

// ICMP packet header and body
typedef struct icmphdr {
    uint8_t type;                       // ICMP_
    uint8_t code;
    uint16_t checksum;                  // checksum of ICMP header + data
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;                         // ICMP_ECHO, ICMP_ECHOREPLY
        uint32_t gateway;
        struct {
            uint16_t __unused;
            uint16_t mtu;
        } frag;
    } un;
    unsigned char data[MAX_ICMP_DATA];
} __attribute__((packed)) icmphdr_t;

// ICMP IP packet
typedef struct icmpip {
    struct iphdr ip;
    struct icmphdr icmp;
} __attribute__((packed)) icmpip_t;

/*
 * UDP related definitions
 * Some of these definitions are from /usr/include/linux/udp.h
 */

// UDP packet header
typedef struct udphdr {
    uint16_t src;                       // source : source port
    uint16_t dest;                      // dest : destination port
    uint16_t len;                       // len : length including header and data
    uint16_t checksum;                  // check : checksum of UDP header, pseudo header and data
} __attribute__((packed)) udphdr_t;

// pseudo TCP/UDP header
// used for calculating checksum of TCP/UDP packet
typedef struct tcpudp_pseudohdr {
    in_addr_t src;                      // source address
    in_addr_t dest;                     // destination address
    uint8_t zero;                       // 0
    uint8_t protocol;                   // protocol
    uint16_t length;                    // TCP/UDP length
} __attribute__((packed)) tcpudp_pseudohdr_t;

/*
 * BOOTP / DHCP related definitions
 */

// BOOTP retry maximum count
#define MAX_BOOTP_RETRIES       3

// DHCP option length
#define DHCP_OPT_LEN            312

// opcode : bp_op
#define BOOTP_REQUEST           1
#define BOOTP_REPLY             2

// DHCP_MESSAGE_TYPE code
#define DHCPDISCOVER            1
#define DHCPOFFER               2
#define DHCPREQUEST             3
#define DHCPDECLINE             4
#define DHCPACK                 5
#define DHCPNAK                 6
#define DHCPRELEASE             7
#define DHCPINFORM              8

// BOOTP/DHCP packet header and body (option)
typedef struct bootphdr {
    uint8_t bp_op;                      // opcode : BOOTP_REQUEST / BOOTP_REPLY
    uint8_t bp_htype;                   // hardware address type
    uint8_t bp_hlen;                    // hardware address length
    uint8_t bp_hops;                    // 0
    uint32_t bp_xid;                    // random transaction ID
    uint16_t bp_secs;                   // seconds elapsed
    uint16_t bp_flags;                  // unused
    in_addr_t bp_ciaddr;                // client IP address
    in_addr_t bp_yiaddr;                // your (client) IP address
    in_addr_t bp_siaddr;                // server IP address
    in_addr_t bp_giaddr;                // gateway IP address
    uint8_t bp_hwaddr[16];              // client hardware address
    uint8_t bp_sname[64];               // optional server hostname (null-terminated)
    uint8_t bp_file[128];               // boot file name(null-terminated)
    uint8_t bp_vend[DHCP_OPT_LEN];      // optional vendor-specific area
} __attribute__((packed)) bootphdr_t;

// BOOTP/DHCP IP packet
typedef struct bootpip {
    struct iphdr ip;
    struct udphdr udp;
    struct bootphdr bp;
} __attribute__((packed)) bootpip_t;

/*
 * TFTP related definitions
 */

// TFTP retry maximum count
#define MAX_TFTP_RETRIES        3

// TFTP packet size
#define TFTP_DEF_PACKET         512
#define TFTP_MAX_PACKET         1024

// TFTP packet type
#define TFTPOP_RRQ              1       // Read Request
#define TFTPOP_WRQ              2       // Write Request
#define TFTPOP_DATA             3       // Data
#define TFTPOP_ACK              4       // Acknowledgment
#define TFTPOP_ERROR            5       // Error
#define TFTPOP_OACK             6       // Option Acknowledgment

// TFTP error code
#define TFTPERR_FILENOTFOUND    1       // File not found
#define TFTPERR_ACCESSDENIED    2       // Access violation
#define TFTPERR_DISKFULL        3       // Disk full or allocation exceeded
#define TFTPERR_ILLEGALOP       4       // Illeegal TFTP operation
#define TFTPERR_UNKNOWNID       5       // Unknown transfer ID
#define TFTPERR_FILEEXIST       6       // File already exists
#define TFTPERR_NOUSER          7       // No such user

// return code of ipv4_tftp()
#define TFTPRET_NETWORKERR      1   
#define TFTPRET_FILENOTFOUND    2
#define TFTPRET_ACCESSDENIED    3
#define TFTPRET_OTHER           4

// TFTP header and body
typedef struct tftphdr {
    uint16_t opcode;                    // TFTPOP_
    union {
        uint8_t rrq[TFTP_DEF_PACKET];   // TFTPOP_RRQ, TFTPOP_WRQ
        struct {
            uint16_t block;
            uint8_t data[TFTP_MAX_PACKET];
        } data;                         // TFTPOP_DATA
        struct {
            uint16_t block;
        } ack;                          // TFTPOP_ACK
        struct {
            uint16_t errcode;
            uint8_t errmsg[TFTP_DEF_PACKET];
        } err;                          // TFTPOP_ERROR
        struct {
            uint8_t data[TFTP_DEF_PACKET];
        } oack;                         // TFTPOP_OACK
    } u;
} __attribute__((packed)) tftphdr_t;

// TFTP IP packet
typedef struct tftpip {
    struct iphdr ip;
    struct udphdr udp;
    struct tftphdr tftp;
} __attribute__((packed)) tftpip_t;

/*
 * function prototypes
 */

struct sk_buff;

#define TIMEOUT         (10 * 1000)
#define ARP_REQUEST_TIMEOUT (5 * 1000)

// init
int ipv4_init(void);
void ipv4_setup_arptable(in_addr_t ipaddr, in_addr_t netmask, unsigned char *node, in_addr_t gateway, in_addr_t dns);

// TCP/IP basic
int ipv4_get_ipaddr_class(in_addr_t ipaddr);
in_addr_t ipv4_get_default_netmask(in_addr_t ipaddr);
int ipv4_ipaddr_valid(in_addr_t ipaddr);
int ipv4_alloc_port(void);

// packet processing
int ipv4_check_for_me(struct sk_buff *skb);
int ipv4_parsepacket(struct sk_buff *skb);
int ipv4_processpacket(struct sk_buff *skb);

// Ethernet
enum {
    WAIT_NOTHING,
    WAIT_ARP_REPLY,
    WAIT_ICMP_REPLY,
    WAIT_UDP,
};

int eth_transmit(unsigned char *dest, uint16_t type, unsigned int len, const void *buf, int async);
struct sk_buff *eth_receive(int type, int timeout);

// arp
arptable_t *arp_lookup(unsigned long ipaddr);
arptable_t *arp_alloc(void);
int arp_valid(arptable_t *arp);
void arp_setaddr(arptable_t *arp, in_addr_t ipaddr, in_addr_t netmask, unsigned char *node);
void arp_setaddr_index(int index, in_addr_t ipaddr, in_addr_t netmask, unsigned char *node);
void arp_listtable(arptable_t *arp);
int arp_transmit(int opcode, in_addr_t ipaddr, unsigned char *node, int async);
arptable_t *arp_dorequest(unsigned long ipaddr, int async);

// IP
int ip_transmit(in_addr_t destip, int protocol, int len, const void *buf, int async);
unsigned int ipv4_calc_sum(const void *ip, int len);
unsigned short ipv4_calc_checksum(const void *ip, int len);

// ICMP / IP
int icmp_ping(in_addr_t destip, int count);

// UDP / IP
int udp_transmit(in_addr_t destip, unsigned int srcsock, unsigned int destsock, int len, const void *buf, int async);
unsigned short ipv4_calc_tcpudp_checksum(const void *data, int len, in_addr_t src, in_addr_t dest);
struct sk_buff *udp_receive(in_addr_t fromip, unsigned int srcsock, unsigned int destsock, int timeout);

// BOOTP / DHCP (net_ipv4_bootp.c)
int ipv4_bootp(int bootp);

// TFTP (net_ipv4_tftp.c)
int ipv4_tftp(in_addr_t ipaddr, int port, int opcode, char *filename, unsigned int addr, unsigned int *datalen);

// DNS (net_ipv4_dns.c)
in_addr_t ipv4_gethostbyname(char *name);

// Miscellaneous
void ipv4_dump_packet(struct sk_buff *skb, int type, int need_parsing);
void bootp_dump_packet(struct sk_buff *skb, int need_parsing);
void tftp_dump_packet(struct sk_buff *skb, int need_parsing);

#endif
