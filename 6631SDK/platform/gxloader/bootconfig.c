#include "config.h"
#include <net_ipv4.h>

#include "bootconfig.h"

bootconfig_t g_bootconfig = {
	.macaddr   = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 },
#if 1
	.protocol  = BOOTNET_STATIC,
	.ipaddr    = INADDR_FROM_4INTS(192,168,120,3),
	.netmask   = INADDR_FROM_4INTS(255,255,255,0),
#else
	.protocol  = BOOTNET_DHCP,
	.ipaddr    = INADDR_FROM_4INTS(0,0,0,0),
	.netmask   = INADDR_FROM_4INTS(255,255,0,0),
#endif
	.gateway   = INADDR_FROM_4INTS(0,0,0,0),
	.server    = INADDR_FROM_4INTS(0,0,0,0),
	.dns       = 0x00000000,
	.domain    = "localdomain.com",
	.autocmd   = CONFIG_AUTO_CMD,
	.tftp_port = IPPORT_TFTP,
	.delay     = CONFIG_BOOTDELAY,
};

//
// function prototypes
//
int bootconfig_load(int index);
int bootconfig_save(void);
int bootconfig_lastsaved(int *idx);

//
// Load & Save
//

// index :
//   0 : last saved
//   > 0 : specified index-th saved configuration
// return 0 on success, otherwise -1
int bootconfig_load(int index)
{
	return -1;
}

// return 0 on success, otherwise -1
int bootconfig_save(void)
{
	return -1;
}

// return the index of configuration last saved in idx parameter
// return 0 on success, otherwise -1
int bootconfig_lastsaved(int *idx)
{
	return -1;
}

