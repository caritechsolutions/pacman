#ifndef __GXLOWPOWER_H__
#define __GXLOWPOWER_H__

// pmu mode register
#define  DPRAM_BASE		(0x000)
#define  DPRAM_SIZE		(0x200)
#define  CPU_GPIO_FUNSEL01	(0x200)
#define  CPU_GPIO_FUNSEL10	(0x204)
#define  DRIVER			(0x208)
#define  CPU_CONFIG_REGS	(0x300)
#define  CPU_WR_SRAM		(0x304)
#define  CPU_RD_FSM		(0x308)
#define  MCU_INTR_REG		(0x30C)
#define  CPU_STATE_REG0		(0x310)
#define  CPU_STATE_REG1		(0x314)
#define  CPU_STATE_REG2		(0x318)
#define  CPU_STATE_REG3		(0x31C)

#define  DW8051_SRAM_SIZE	(0x2000)
#define  DW8051_SRAM_BASE	(0x0000)

#define GXLOWPOWER_MAGIC_NUMBER 0x54410003
struct gx_tag_mem {
	unsigned int    tag_header;
	unsigned char   wakeflag;
	unsigned char   reserved[3];
	unsigned int    utctime;
	unsigned int    curtime;
};

struct gx_tag_pmu { /*note: The address of each member of the structure must be a multiple of 4 */
	unsigned int    tag_header;
	unsigned int    waketime;
	unsigned int    gpiomask;
	unsigned int    gpiodata;
	unsigned int    keybuf[20];
	unsigned int    keynum;
	unsigned int    powerio[2];
	unsigned int    utctime;      /* utc time day of second */
	unsigned char   wakeflag;
	unsigned char   reserved[3];
	unsigned int    clock_speed;
	unsigned int    panelio[2]; //lowpower panel control io, panelio[0]: clk, panelio[1]: dat
	int             timeshowflag;
	int             timezone;
	int             timesummer;
	unsigned int    curtime;
	/* add new param after this */
};

enum {
	NOT_WAKE_FLAG = 0,
	MANUAL_WAKE_FLAG = 1,
	AUTO_WAKE_FLAG = 2,
	REBOOT_WAKE_FLAG = 3
};

#endif
