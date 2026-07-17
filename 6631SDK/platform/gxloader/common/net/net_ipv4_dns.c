/*
 * net_ipv4_dns.c
 *
 * DNS client
 *
 * RFCs related to DNS
 *   RFC 1034 : Domain Names - Concepts and Facilities
 *   RFC 1035 : Domain Names - Implementatio and Specification
 *
 * by Ho Lee 03/29/2004
 */

#include "config.h"
#include "net_priv.h"
#include "net_ipv4.h"
#include "bootconfig.h"
#include "io.h"
#include "time.h"

#include <net.h>

/*
 * RR Format : RFC 1035 Section 3.2
 *
 * NAME : byte array, null-terminated, length can be odd
 * TYPE : 2 bytes
 * CLASS : 2 bytes
 * TTL : 4 bytes
 * RDLENGTH : 2 bytes
 * RDATA : variable
 */

// TYPE
#define RR_TYPE_A               1       // a host address
#define RR_TYPE_NS              2       // an authoriative name server
#define RR_TYPE_CNAME           5       // the canonical name for an alias
#define RR_TYPE_SOA             6       // amrks the start of a zone of authority
#define RR_TYPE_WKS             11      // a well known service description
#define RR_TYPE_PTR             12      // a domain name pointer
#define RR_TYPE_HINFO           13      // host information
#define RR_TYPE_MINFO           14      // mailbox or mail list information
#define RR_TYPE_MX              15      // mail exchange
#define RR_TYPE_TXT             16      // text string

// QTYPE
#define RR_QTYPE_AXFR           252     // a request for a transfer of an entire zone
#define RR_QYTPE_MAILB          253     // a request for mailbox-related records
#define RR_QYTPE_ALL            255     // a request for all records

// CLASS
#define RR_CLASS_IN             1       // the internet
#define RR_CLASS_CH             3       // the CHAOS class
#define RR_CLASS_HS             4       // Hesiod

// QCLASS
#define RR_QCLASS_ANY           255     // any class

typedef struct rrinfo {
	char name[256];
	int name_compressed;
	int type;
	int class;
	int ttl;
	int rdlength;
	unsigned char *rdata;
} rrinfo_t;

/*
 * Message format : RFC 1035 Section 4.1.1
 *
 * Header
 * Question
 * Answer
 * Authority
 */

// qr flag
#define DNSHDR_QR_QUERY         0
#define DNSHDR_QR_RESPONSE      1

// opcode
#define DNSHDR_OPCODE_QUERY     0
#define DNSHDR_OPCODE_IQUERY    1
#define DNSHDR_OPCODE_STATUS    2

// rcode
#define DNSHDR_RCODE_OK         0
#define DNSHDR_RCODE_FORMAT     1
#define DNSHDR_RCODE_SERVERFAIL 2
#define DNSHDR_RCODE_NAMEERR    3
#define DNSHDR_RCODE_NOTIMPL    4
#define DNSHDR_RCODE_REFUSED    5

typedef struct dnshdr {
	uint16_t id;
	union {
		uint16_t all;
		struct {
#ifdef CONFIG_BIGENDIAN
			uint16_t qr : 1;
			uint16_t opcode : 4;
			uint16_t aa : 1;
			uint16_t tc : 1;
			uint16_t rd : 1;
			uint16_t ra : 1;
			uint16_t z : 3;
			uint16_t rcode : 4;
#else
			uint16_t rcode : 4;
			uint16_t z : 3;
			uint16_t ra : 1;
			uint16_t rd : 1;
			uint16_t tc : 1;
			uint16_t aa : 1;
			uint16_t opcode : 4;
			uint16_t qr : 1;
		} __attribute__((packed)) s;
	} __attribute__((packed)) flags;
#endif
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} __attribute__((packed)) dnshdr_t;

#define MAX_DNS_PACKET          1024
#define MAX_DNS_ANSWERS         16

typedef struct dnspacket {
	struct {
		struct iphdr ip;
		struct udphdr udp;
		unsigned char dns[MAX_DNS_PACKET];
	} __attribute__((packed)) data;
	int len;
	dnshdr_t *hdr;
	unsigned char *question;
	unsigned char *answers[MAX_DNS_ANSWERS];
} dnspacket_t;

//
// function prototypes
//

// RR
static int dns_makestring(unsigned char *buf, char *name);
static int dns_getstring(unsigned char *buf, char *name);
static int dns_parserr(unsigned char *buf, rrinfo_t *prr, int query);

// DNS packet
static void dns_makequerypacket(dnspacket_t *packet, unsigned short xid, char *name);
static int dns_parsepacket(unsigned char *buf, dnspacket_t *packet, unsigned short xid, rrinfo_t *qd_rr, rrinfo_t an_rrs[]);
void dns_dumppacket(dnspacket_t *packet);

// API
in_addr_t ipv4_gethostbyname(char *name);

//
// RR
//

int dns_makestring(unsigned char *buf, char *name)
{
	int len, fieldlen;
	unsigned char *plen = buf;

	for (++buf, len = 1, fieldlen = 0; ; ++buf, ++name, ++len) {
		if (*name == '.' || *name == 0) {
			*plen = fieldlen;
			if (*name == 0)
				break;
			fieldlen = 0;
			plen = buf;
		} else {
			++fieldlen;
			*buf = *(unsigned char *) name;
		}
	}

	*buf++ = 0;

	return len + 1;
}

int dns_getstring(unsigned char *buf, char *name)
{
	int len, fieldlen;

	// check message compression (4.1.4)
	// when the 2 MSB bits are 1
	if (*buf & 0xc0) {
		*name++ = *buf & ~0xc0;
		*name = *buf;
		return 0;
	}

	// normal name
	for (len = 0; buf[len] != 0;) {
		fieldlen = buf[len++];
		while (fieldlen-- > 0)
			*name++ = buf[len++];
		if (*buf)
			*name++ = '.';
	}

	*name = 0;

	return ++len;
}

int dns_parserr(unsigned char *buf, rrinfo_t *prr, int query)
{
	int len;

	// Query RR : Refer to RFC 1035 4.1.2 for "question section format"
	// Answer RR : Refer to 4.1.3 for "resource record format"

	// name
	len = dns_getstring(buf, prr->name);
	if (len == 0) {
		prr->name_compressed = 1;
		len = 2;
	}
	// type
	prr->type = __raw_readw(buf + len);
	prr->type = ntohs(prr->type);
	len += 2;
	// class
	prr->class = __raw_readw(buf + len);
	prr->class = ntohs(prr->class);
	len += 2;

	if (!query) {
		// ttl
		prr->ttl = __raw_readl(buf + len);
		prr->ttl = ntohl(prr->ttl);
		len += 4;
		// data length
		prr->rdlength = __raw_readw(buf + len);
		prr->rdlength = ntohs(prr->rdlength);
		len += 2;
		// data
		prr->rdata = buf + len;
		len += prr->rdlength;
	}

	return len;
}

//
// DNS packet
//

void dns_makequerypacket(dnspacket_t *packet, unsigned short xid, char *name)
{
	dnshdr_t *phdr;
	unsigned char *cp;
	int len;

	// header
	// Refer to RFC 1035 4.1.1 for "header section format"
	phdr = packet->hdr = (dnshdr_t *) packet->data.dns;
	packet->len = sizeof(dnshdr_t);
	memset(phdr, 0, sizeof(dnshdr_t));

	phdr->id = htons(xid);
	phdr->flags.s.qr = DNSHDR_QR_QUERY;
	phdr->flags.s.opcode = DNSHDR_OPCODE_QUERY;
	phdr->flags.s.rd = 1;
	phdr->flags.all = htons(phdr->flags.all);
	phdr->qdcount = htons(1);

	// query RR
	// Refer to RFC 1035 4.1.2 for "question section format"
	cp = packet->question = packet->data.dns + packet->len;
	// name
	len = dns_makestring(cp, name);
	cp += len;
	// type = RR_TYPE_A
	__raw_writew((int)cp, htons(RR_TYPE_A));
	// class = inet
	__raw_writew((int)(cp + 2), htons(RR_CLASS_IN));
	packet->len += len + 4;
}

int dns_parsepacket(unsigned char *buf, dnspacket_t *packet, unsigned short xid, rrinfo_t *qd_rr, rrinfo_t an_rrs[])
{
	int i;
	dnshdr_t *phdr;

	memset(packet, 0, sizeof(dnspacket_t));

	// header
	phdr = packet->hdr = (dnshdr_t *) buf;
	buf += sizeof(dnshdr_t);
	if ((phdr->id = ntohs(phdr->id)) != xid) {
		printf("DNS packet ID mismatch\n");
		return 1;
	}
	phdr->flags.all = ntohs(phdr->flags.all);
	phdr->qdcount = ntohs(phdr->qdcount);
	phdr->ancount = ntohs(phdr->ancount);
	phdr->nscount = ntohs(phdr->nscount);
	phdr->arcount = ntohs(phdr->arcount);

	// question
	if (phdr->qdcount) {
		packet->question = buf;
		buf += dns_parserr(buf, qd_rr, 1);
	}

	// answer
	for (i = 0; i < phdr->ancount && i < MAX_DNS_ANSWERS; ++i) {
		packet->answers[i] = buf;
		buf += dns_parserr(buf, &an_rrs[i], 0);
	}

	return 0;
}

#if 0
void dns_dumppacket(dnspacket_t *packet)
{
	int i;
	dnshdr_t *phdr = packet->hdr;

	printf("DNS : \n");
	printf("  ID : %04x\n", phdr->id);
	printf("  Flags : %04x\n", phdr->flags.all);
	printf("  Questions : %d\n", phdr->qdcount);
	printf("  Answers : %d\n", phdr->ancount);
	printf("  Authority : %d\n", phdr->nscount);
	printf("  Additional : %d\n", phdr->arcount);

	printf("  Question ptr : %p\n", packet->question);

	printf("  Answers :\n");
	for (i = 0; i < phdr->ancount; ++i)
		printf("   %d : %p\n", i, packet->answers[i]);
}
#endif

//
// API
//

in_addr_t ipv4_gethostbyname(char *name)
{
	struct sk_buff *skb;
	dnspacket_t packet;
	int i, iport;
	unsigned short xid;
	rrinfo_t qd_rr, an_rrs[MAX_DNS_ANSWERS];
	in_addr_t retaddr = INADDR_INVALID;

	if (!ipv4_ipaddr_valid(g_arptable[ARP_DNS].ipaddr))
		return INADDR_INVALID;

	iport = ipv4_alloc_port();
	xid = gx_time_get_us() & 0xffff;
	dns_makequerypacket(&packet, xid, name);

	if (udp_transmit(g_bootconfig.dns, iport, IPPORT_DOMAINSERVER, packet.len, &packet.data, 0))
		return INADDR_INVALID;

	if ((skb = udp_receive(g_bootconfig.dns, 0, iport, TIMEOUT)) != NULL) {
		dns_parsepacket(skb->udpdata, &packet, xid, &qd_rr, an_rrs);
		for (i = 0; i < packet.hdr->ancount; ++i) {
			if (an_rrs[i].type == RR_TYPE_A && an_rrs[i].class == RR_CLASS_IN) {
				retaddr = __raw_readl(an_rrs[i].rdata);
				retaddr = ntohl(retaddr);
				break;
			}
		}
		skb_free(skb);
	}

	return retaddr;
}
