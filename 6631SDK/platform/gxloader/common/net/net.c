
#include "net_priv.h"
#include <net.h>

#include "bootconfig.h"
#include "sys/types.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

#if 0
#define DPRINTF(x...)       printf(x)
#else
#define DPRINTF(x...)
#endif

/* huangjb tmp debug */
#define CONFIG_ENABLE_NETWORK_SYNOPSIS

// from sysnopsis.c
extern int synopsis_probe(struct net_device *dev);

#define ETHER_BUF_NUM           512

static unsigned char *g_ether_buf;
static volatile int *g_ether_bufstat;
static volatile int g_ether_first_free;

void ether_buf_init(void);
unsigned char *ether_buf_alloc(int len);
void ether_buf_free(unsigned char *buf, int len);

void ether_buf_init(void)
{
	g_ether_buf = (unsigned char *) malloc(ETHER_BUF_NUM * ETH_BUF_LEN);
	g_ether_bufstat = (int *) malloc(ETHER_BUF_NUM * sizeof(int));
	memset((void *)g_ether_bufstat, 0, ETHER_BUF_NUM * sizeof(int));
	g_ether_first_free = 0;
}

unsigned char *ether_buf_alloc(int len)
{
	int i;

	i = g_ether_first_free;
	do {
		if (g_ether_bufstat[i] == 0) {
			g_ether_bufstat[i] = 1;
			g_ether_first_free = (i + 1) % ETHER_BUF_NUM;
			return g_ether_buf + i * ETH_BUF_LEN;
		}
		if (++i == ETHER_BUF_NUM)
			i = 0;
	} while (i != g_ether_first_free);

	return NULL;
}

void ether_buf_free(unsigned char *buf, int len)
{
	int idx;
	unsigned int offset = buf - g_ether_buf;

	if ((offset % ETH_BUF_LEN) != 0) {
		DPRINTF("Try to free invalid ethernet buffer pointer %p\n", buf);
		return;
	}

	idx = offset / ETH_BUF_LEN;

	if (g_ether_bufstat[idx] == 0) {
		DPRINTF("Try to free unallocated ethernet buffer index %d\n", idx);
		return;
	}

	g_ether_bufstat[idx] = 0;
}

//
// socket buffer manipulation
//

#define SOCKET_BUF_MAX      512

struct sk_buff *g_skb;
static volatile int *g_skbstat;                             // socket buffer is allocated
static volatile int g_skb_first_free;                       // first unused socket buffer
static volatile struct sk_buff *g_skb_head, *g_skb_tail;    // list of socket buffer with data
static volatile int g_skb_time_begin, g_skb_time_end;       // timestamp

void skb_init(void);
struct sk_buff *skb_alloc(int len);
void skb_free(struct sk_buff *skb);
struct sk_buff *skb_dup(struct sk_buff *skb);
void skb_put(struct sk_buff *skb);
struct sk_buff *skb_get(void);

void skb_init(void)
{
	g_skb = (struct sk_buff *) malloc(sizeof(struct sk_buff) * SOCKET_BUF_MAX);
	memset(g_skb, 0, sizeof(struct sk_buff) * SOCKET_BUF_MAX);
	g_skbstat = (int *) malloc(sizeof(int) * SOCKET_BUF_MAX);
	memset((void *)g_skbstat, 0, sizeof(int) * SOCKET_BUF_MAX);
	g_skb_first_free = 0;
	g_skb_head = g_skb_tail = NULL;
	g_skb_time_begin = g_skb_time_end = 0;
}

// allocate socket buffer from socket buffer pool, net\core\skbuff.c
struct sk_buff *skb_alloc(int len)
{
	int i;
	struct sk_buff *skb;

try_alloc:
	i = g_skb_first_free;
	do {
		if (g_skbstat[i] == 0) {
			skb = g_skb + i;

			if ((skb->data = ether_buf_alloc(len)) == NULL)
				goto retry_alloc;

			skb->data += 2;
			g_skbstat[i] = 1;
			g_skb_first_free = (i + 1) % SOCKET_BUF_MAX;
			skb->len = len;
			skb->buflen = len;
			skb->valid = 0;
			skb->owned = 0;
			skb->next = NULL;
			DPRINTF("skb alloc %d\n", i);
			return skb;
		}
		if (++i == SOCKET_BUF_MAX)
			i = 0;
	} while (i != g_skb_first_free);

retry_alloc:
	// if the buffer is full, remove lastest sk_buff from buffer, and retry
	if ((skb = skb_get()) == NULL) {
		printf("skb_alloc: No buffer available\n");
#if 0
		{
			int i;
			struct sk_buff *skb;

			printf("### Socket buffer status ###\n");

			for (i = 0; i < SOCKET_BUF_MAX; ++i) {
				skb = g_skb + i;
				printf("  %3d : %d %d %d",
						i, skb->valid, skb->owned, g_skbstat[i]);
				if (skb->valid)
					printf(" %p, %4d %4d %05d %3d", skb->data, skb->len, skb->buflen, skb->timestamp, skb->next == NULL ? -1 : skb->next - g_skb);
				printf("\n");
			}
		}
#endif
		return NULL;
	} else
		skb_free(skb);
	goto try_alloc;
}

// free socket buffer
void skb_free(struct sk_buff *skb)
{
	int idx = skb - g_skb;

	if (skb == NULL)
		return;

	if (g_skbstat[idx] == 0) {
		DPRINTF("Try to free unallocated socket buffer index %d\n", idx);
		return;
	}

	skb->data -= 2;
	ether_buf_free(skb->data, skb->buflen);

	skb->valid = 0;
	skb->owned = 0;
	g_skbstat[idx] = 0;
	DPRINTF("skb free %d\n", idx);

}

struct sk_buff *skb_dup(struct sk_buff *skb)
{
	struct sk_buff *new_skb = skb_alloc(skb->len);

	if (new_skb)
		memcpy(new_skb->data, skb->data, skb->len);

	return new_skb;
}

// packet is ready. mark the packet is valid
void skb_put(struct sk_buff *skb)
{
	if (skb == NULL)
		return;

	if (ipv4_check_for_me(skb)) {
		if (net_defaultaction(skb) == 0) {
			DPRINTF("skb put %d\n", skb - g_skb);
			skb->valid = 1;
			skb->next = NULL;
			skb->timestamp = g_skb_time_end++;
			if (g_skb_head == NULL) {
				g_skb_head = g_skb_tail = skb;
			} else {
				g_skb_tail->next = skb;
				g_skb_tail = skb;
			}
			return;
		}
	}

	skb_free(skb);
}

// get next available packet
struct sk_buff *skb_get(void)
{
	struct sk_buff *skb;

	if (g_skb_head == NULL)
		return NULL;

	skb = (struct sk_buff *)g_skb_head;
	skb->owned = 1;
	g_skb_head = g_skb_head->next;
	DPRINTF("skb get %d\n", skb - g_skb);
	if (skb->timestamp != g_skb_time_begin) {
		DPRINTF("skb_get: unexpected timestamp %d (should be %d)\n", skb->timestamp, g_skb_time_begin);
	}
	++g_skb_time_begin;

	return skb;
}

struct sk_buff *skb_alloc_tx(int len)
{
	return skb_alloc(len);
}

//
// network device manipulation
//

static volatile int g_net_waitpacket = 0;
static int net_inited = 0;
struct net_device g_netdev;

int net_init(void)
{
	if (net_inited != 0)
		goto network_inited;

	ether_buf_init();
	skb_init();

	// initialize protocol stack
	ipv4_init();


#ifdef CONFIG_ENABLE_NETWORK_NE2KPCI
	if (ne2kpci_probe(&g_netdev))
		goto network_inited;
#endif

#ifdef CONFIG_ENABLE_NETWORK_RTL81XX
	if (rtl81xx_probe(&g_netdev))
		goto network_inited;
#endif

#ifdef CONFIG_ENABLE_NETWORK_TANGO15
	if (em86xx_eth_probe(&g_netdev))
		goto network_inited;
#endif

#ifdef CONFIG_ENABLE_USB_NETWORK_ASIX88179
	if (ax88179_probe(&g_netdev))
		goto network_inited;
#endif

#ifdef CONFIG_ENABLE_NETWORK_SYNOPSIS
	if (synopsis_probe(&g_netdev))
		goto network_inited;
#endif

	return 1;

network_inited:
	net_inited = 1;
	return 0;
}

int net_found(void)
{
	return (g_netdev.state);
}

int net_arp_setup(void)
{
	ipv4_setup_arptable(g_bootconfig.ipaddr, g_bootconfig.netmask, g_netdev.dev_addr,
			g_bootconfig.gateway, g_bootconfig.dns);

	return 0;
}

int net_dev_up(void)
{
	if ((g_netdev.state) && (g_netdev.state == NETDEV_UP))
		printf("Network is up already.\n");
	else if ((g_netdev.state) && (g_netdev.state != NETDEV_UP)) {
		// setup network before enable device
		memcpy(g_netdev.dev_addr, g_bootconfig.macaddr, MACADDR_LEN);

		// initialize device
		g_netdev.open(&g_netdev);

		// bring up the protocol stack
		switch (g_bootconfig.protocol) {
			case BOOTNET_STATIC :
				if (!ipv4_ipaddr_valid(g_bootconfig.ipaddr)) {
					printf("Invalid static IP address.\n");
				} else {
					printf("Bring up network with static IP address.\n");
					net_arp_setup();
				}
				break;
#if 1
			case BOOTNET_BOOTP :
			case BOOTNET_DHCP :
				printf("Bring up network with %s.\n", g_bootconfig.protocol == BOOTNET_BOOTP ? "BOOTP" : "DHCP");
				arp_setaddr_index(ARP_CLIENT, INADDR_ANY, INADDR_ANY, g_netdev.dev_addr);
				mdelay(500);

				return ipv4_bootp(g_bootconfig.protocol == BOOTNET_BOOTP ? 1 : 0);
#endif
		}

		// setup after enable device

		return 0;
	}
	return 1;
}

int net_dev_down(void)
{
	if ((g_netdev.state) && (g_netdev.state == NETDEV_DOWN))
		printf("Network is down already.\n");
	else if ((g_netdev.state) && (g_netdev.state == NETDEV_UP)) {
		g_netdev.close(&g_netdev);
		return 0;
	}
	return 1;
}

//
// Network packet manipulation
//

// timeout :
//   0 : no wait
//   > 0 : wait until timeout
//   < 0 : wait until get any packet
struct sk_buff *net_getpacket(int timeout)
{
	struct sk_buff *skbuff = NULL;
	unsigned int startticks = gx_time_get_ms();//timer_getticks();

	if (!g_netdev.state)
		return NULL;

	// if there is no packet in the buffer, check the device if any packet is ready
	g_net_waitpacket = 1;
	do {
		//if (!em86xx_irqenabled())
		if(1)   	//modify by huangjb. query the flags to replace the mode of isr.
			g_netdev.receive_packet(&g_netdev);
		if ((skbuff = skb_get()) != NULL)
			break;
		/*if(g_netdev.receive_packet(&g_netdev) == 0){	//return ok
		  if ((skbuff = skb_get()) != NULL)
		  break;
		  }*/
	} while (!timer_timeout(startticks, timeout));
	g_net_waitpacket = 0;

	return skbuff;
}

int net_sendpacket(struct sk_buff *skb, int async)
{
	if (!g_netdev.state || !skb->valid)
		return 1;

	g_netdev.send_packet(skb, &g_netdev, async);

	return 0;
}

// return
//   -1 : ignore packet
//   0 : packet is not processed (keep)
//   1 : packet is processed (remove)
int net_defaultaction(struct sk_buff *skb)
{
	if (ipv4_parsepacket(skb) != 0)
		return -1;
	if (ipv4_processpacket(skb))
		return 1;
	return 0;
}

void net_device_status(void)
{
	if (g_netdev.print_status)
		g_netdev.print_status(&g_netdev);
}
