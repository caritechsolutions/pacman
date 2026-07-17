#ifndef __BOOTLOADER_BOOTCONFIG_H
#define __BOOTLOADER_BOOTCONFIG_H

#define BOOTCONFIG_MAJOR        0x01
#define BOOTCONFIG_MINOR        0x00

#define BOOTCONFIG_KERNELPARAMLEN   256
#define BOOTCONFIG_PATHLEN      24

enum { BOOTNET_NONE, BOOTNET_STATIC, BOOTNET_BOOTP, BOOTNET_DHCP };

typedef struct {
	unsigned short major_version;   // major version of configuration
	unsigned short minor_version;   // minor version of configuration
	unsigned int checksum;          // checksum : BOOTCONFIG_BODYSIGN

	// network configuration
	unsigned char macaddr[6];       // hardware address
	unsigned short protocol;        // BOOTNET_xxx
	unsigned int ipaddr;            // IP address
	unsigned int netmask;           // network mask
	unsigned int gateway;           // gateway
	unsigned int server;            // server for download
	unsigned int dns;               // DNS server
	char domain[24];                // domain name

	char kernel_filename[BOOTCONFIG_PATHLEN];
	char autocmd[64];
	int tftp_port;
	unsigned int delay;
} bootconfig_1_0_t;

typedef bootconfig_1_0_t bootconfig_t;

// global variables

extern bootconfig_t g_bootconfig;

// function prototypes

int bootconfig_load(int index);
int bootconfig_save(void);
int bootconfig_lastsaved(int *idx);

#endif
