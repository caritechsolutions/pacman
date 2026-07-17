/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou NationalChip Science and Technology Co., Ltd.
*                      (C)2006-2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :   main.c
* Author    :   liuyx
* Project   :   lowpower
* Type      :   csky
******************************************************************************
* Purpose   :   The main function of the lowpower
*****************************************************************************/
/* Includes --------------------------------------------------------------- */
#include "gxcomm.h"

/* Private types/constants ------------------------------------------------ */
#define SEC_PER_DAY (60 * 60 * 24)
#define SEC_PER_HOUR (60 * 60)

/* Private Macros --------------------------------------------------------- */

/* Private Variables ------------------------------------------------------ */
U32 s_cur_time_sec = 0;
U32 s_tm_hour = 0;
U32 s_tm_min = 0;

/* Export Variables ------------------------------------------------------- */
extern struct gx_fw_status *g_fw_status;
extern gxlowpower_func_t gxlowpower_func;
extern void _fd650_mute_config(void)	;
extern void _fd650_mulpin_config(void);
extern unsigned char _fd650_read_key(void);
extern void _fd650_show_time(unsigned int time, unsigned int count);
extern void _fd650_show_on(void);
extern void _fd650_show_off(void);
extern void _fd650_init(struct gx_parse_cmdline *pcmdline);

/* Exported Constants ----------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
void poweron(struct gx_parse_cmdline *pcmdline);

void ad_delay(unsigned int time)
{
	U32 i,j ;

	for (i=0;i<time;i++)
	{
		for(j=0;j<3000;j++);
	}

	return;
}

static int waketime_check(void *data)
{
	static U32 TimeCount = 0;
	struct gx_parse_cmdline *pcmdline = data;

	TimeCount++;
	GXLOWPOWER_TinyPutNum((S32)TimeCount);
	GXLOWPOWER_TinyPutStr("s\n");
	if(TimeCount >= pcmdline->waketime)
	{
		GXLOWPOWER_TinyPutStr("\nIt is time to reset without receiveing any keys.\n");
		if (pcmdline->reboot_flag)
			g_fw_status->wakeflag = 3;
		else
			g_fw_status->wakeflag = 2;
		time_write_back();
		poweron(pcmdline);
		WDT_INIT(pcmdline->lowpower_clock_speed);
	}

	return 0;
}

static int showtime_check(void *data)
{
	U32 temp_sec = 0;
	struct gx_parse_cmdline *pcmdline = data;

	s_cur_time_sec++;
	temp_sec = s_cur_time_sec % SEC_PER_DAY;
	s_tm_hour = temp_sec / SEC_PER_HOUR;

	temp_sec %= SEC_PER_HOUR;
	s_tm_min = temp_sec / 60;

	if(pcmdline->timeshowflag == 1) {
		_fd650_show_time((s_tm_hour * 100) + (s_tm_min & 0xff), s_cur_time_sec);
	}

	return 0;
}
void QueryTask(struct gx_parse_cmdline *pcmdline)
{
	U32 i;
	U32 IrrKey;
	U32 eport_base = 0;
	U32 gpio_offs = 0;
	struct gx_irr_info info;
	unsigned char keycode = 0;
	GXLOWPOWER_TinyPutStr("query [key/waketime/gpio] to reset...\n");

	if (pcmdline->gpionum < 32) {
		eport_base = EPORT_BASE_ADDR;
		gpio_offs = pcmdline->gpionum;
	} else if (pcmdline->gpionum < 64) {
		eport_base = EPORT_BASE_ADDR + 0x1000;
		gpio_offs = pcmdline->gpionum - 32;
	} else if (pcmdline->gpionum < 96) {
		eport_base = EPORT_BASE_ADDR + 0x2000;
		gpio_offs = pcmdline->gpionum - 64;
	} else {
		eport_base = 0xFFFFFFFF;
		gpio_offs = 0xFFFFFFFF;
	}
	if (0 != pcmdline->waketime)
		gx_ctr_callback_register(waketime_check, 1000, -1, pcmdline);

	gx_ctr_callback_register(showtime_check, 1000, -1, pcmdline);

	while(1)
	{
		if(0 == gxlowpower_irrGetKey(&info))
		{
			GXLOWPOWER_TinyPutNum(info.key_code);
			GXLOWPOWER_TinyPutStr("\n");

			for(i=0;i<pcmdline->keynum;i++)
			{
				if((info.protocol == IRR_PROTOCOL_NEC) && (pcmdline->keybuf[i] <= 0xFFFF))
				{
					IrrKey=((info.key_code & 0xFF000000)>>16)|((info.key_code & 0xFF00)>>8);
				}
				else
				{
					IrrKey=info.key_code;
				}
				if(pcmdline->keybuf[i] == IrrKey )
				{
					GXLOWPOWER_TinyPutStr("\nThe valid key of IRR is received to reset.\n");
					GXLOWPOWER_TinyPutNum((S32)IrrKey);
					GXLOWPOWER_TinyPutStr("\n");
					return;
				}
			}
		}

		if (eport_base == 0xFFFFFFFF) {
			;
		} else if (pcmdline->gpiolevel == (((*(volatile unsigned int*)(eport_base + 4)) & (1 << gpio_offs)) >> gpio_offs)) {
			GXLOWPOWER_TinyPutStr("\npower gpio reset.\n");
			break;
		}
		keycode =_fd650_read_key();
		if (pcmdline->keynum > 0) {
			if(keycode == pcmdline->keybuf[pcmdline->keynum - 1])
			{
				GXLOWPOWER_TinyPutStr("\nThe powerkey of panel is received to reset.\n");
				return;
			}
		}
	}
}

void clr_bss(void)
{
	unsigned int i;
	extern int _sbss, _ebss;
	for(i = (unsigned int)&_sbss; i < (unsigned int)&_ebss; i++)
		*(unsigned char*)i = 0;
}
void powercut(struct gx_parse_cmdline *pcmdline)
{
	U32 eport_base = 0;
	U32 gpio_offs = 0;
	if (pcmdline->powerio[0] < 32) {
		eport_base = EPORT_BASE_ADDR;
		gpio_offs = pcmdline->powerio[0];
	} else if (pcmdline->powerio[0] < 64) {
		eport_base = EPORT_BASE_ADDR + 0x1000;
		gpio_offs = pcmdline->powerio[0] - 32;
	} else if (pcmdline->powerio[0] < 96) {
		eport_base = EPORT_BASE_ADDR + 0x2000;
		gpio_offs = pcmdline->powerio[0] - 64;
	} else {
		//GXLOWPOWER_TinyPutStr("\npowercut error: no phy gpio!.\n");
		return;
	}
	if(pcmdline->powerio[1] == 1)
	{
		*(volatile unsigned int*)(eport_base+0x08)|= (1 << gpio_offs);
	}
	else
	{
		*(volatile unsigned int*)(eport_base+0x0c)|= (1 << gpio_offs);
	}
	GXLOWPOWER_TinyPutStr("\npowercut set phy gpio ");
	GXLOWPOWER_TinyPutNum((S32)pcmdline->powerio[0]);
	GXLOWPOWER_TinyPutStr(" to ");
	GXLOWPOWER_TinyPutNum((S32)pcmdline->powerio[1]);
	GXLOWPOWER_TinyPutStr(" !\n");
}

void poweron(struct gx_parse_cmdline *pcmdline)
{
	U32 eport_base = 0;
	U32 gpio_offs = 0;
	if (pcmdline->powerio[0] < 32) {
		eport_base = EPORT_BASE_ADDR;
		gpio_offs = pcmdline->powerio[0];
	} else if (pcmdline->powerio[0] < 64) {
		eport_base = EPORT_BASE_ADDR + 0x1000;
		gpio_offs = pcmdline->powerio[0] - 32;
	} else if (pcmdline->powerio[0] < 96) {
		eport_base = EPORT_BASE_ADDR + 0x2000;
		gpio_offs = pcmdline->powerio[0] - 64;
	} else {
		//GXLOWPOWER_TinyPutStr("\npowercut error: no phy gpio!.\n");
		return;
	}
	if(pcmdline->powerio[1] == 1)
	{
		*(volatile unsigned int*)(eport_base+0x0c)|= (1 << gpio_offs);
	}
	else
	{
		*(volatile unsigned int*)(eport_base+0x08)|= (1 << gpio_offs);
	}
	GXLOWPOWER_TinyPutStr("\npoweron set phy gpio ");
	GXLOWPOWER_TinyPutNum((S32)pcmdline->powerio[0]);
	GXLOWPOWER_TinyPutStr(" to ");
	GXLOWPOWER_TinyPutNum(!(S32)pcmdline->powerio[1]);
	GXLOWPOWER_TinyPutStr(" !\n");
}

void *memset(void *m, int c, int n)
{
	char *s = (char *) m;
	unsigned int d = c & 0xff;	/* To avoid sign extension, copy C to an
					   unsigned variable.  */
	while (n--)
	{
		*s++ = (char)d;
	}

	return m;
}

static void cmdline_init(struct gx_parse_cmdline *pcmdline)
{
	memset(pcmdline, 0x00, sizeof(struct gx_parse_cmdline));
	pcmdline->powerio[0]=0xFFFFFFFF;
	pcmdline->curtime = 0xFFFFFFFF;
}

int Standby(void)
{
	struct gx_parse_cmdline cmdline;
	cmdline_init(&cmdline);

	//Make sure all the gxlowpower_func' functions have been initialed
	//except ConfigPll
	if(NULL == (U32)gxlowpower_func.CloseModule ||
			NULL == (U32)gxlowpower_func.SetGpio     ||
			NULL == (U32)gxlowpower_func.ClosePll)
	{
		return 0;
	}

	clr_bss();
	fw_status_init();
	parse_cmdline(&cmdline);
	if (cmdline.curtime == 0xFFFFFFFF)
		cmdline.curtime = g_fw_status->utctime + cmdline.timezone + cmdline.timesummer * 3600;
	else {
		cmdline.timezone = 0;
		cmdline.timesummer = 0;
	}

	if (cmdline.lowpower_clock_speed == 0x0)
		cmdline.lowpower_clock_speed = XTAL_CLOCK_FREQ;
	//Close the modules such as WDT, INTC and TV
	gxlowpower_func.CloseModule();

	//Set the gpio
	gxlowpower_func.SetGpio(cmdline.gpionum,cmdline.gpiolevel);

	if(NULL != (U32)gxlowpower_func.ConfigPll)
	{
		gxlowpower_func.ConfigPll();
	}

	//config the pin and gpio
	gxlowpower_func.ConfigPin();

	//Close all the clock except the APB and cpu clock
	gxlowpower_func.ClosePll();
	_fd650_mute_config();

	//delay to avoid that reset from irr which is used to enter the lowpower
	ad_delay(40);	//g3201 no cache about 2s

	//init module
	IRR_INIT(cmdline.lowpower_clock_speed);
	UART_INIT(cmdline.lowpower_clock_speed,115200);
	gx_irq_init();
	_fd650_init(&cmdline);
	g_fw_status->tag_header = TAG_MAGIC;
	GXLOWPOWER_TinyPutStr("\nThe frequency of the AHB is the same as the crystal's now.\n");
	GXLOWPOWER_TinyPutStr("Close the PLL.\n\n");

	gx_ctr_init();
	GXLOWPOWER_TinyPutStr("Counter is running ...\n");
	U32 temp_sec = 0;
	s_cur_time_sec = cmdline.curtime;
	temp_sec = s_cur_time_sec % SEC_PER_DAY;
	s_tm_hour = temp_sec / SEC_PER_HOUR;

	temp_sec %= SEC_PER_HOUR;
	s_tm_min = temp_sec / 60;

	powercut(&cmdline);
	_fd650_mulpin_config();

	if(cmdline.timeshowflag == 1)
		_fd650_show_time((s_tm_hour*100)+(s_tm_min&0xff), 0);
	else if (cmdline.timeshowflag == 0)
		_fd650_show_off();

	//query [key/waketime/gpio]
	QueryTask(&cmdline);
	if(cmdline.timeshowflag != 2)
		_fd650_show_on();
	g_fw_status->wakeflag = 1;
	time_write_back();
	poweron(&cmdline);
	//Initial the WDT to reset the system
	WDT_INIT(cmdline.lowpower_clock_speed);
	while(1);
	return 0;
}

