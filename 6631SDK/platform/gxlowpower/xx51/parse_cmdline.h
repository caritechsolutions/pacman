#ifndef __PARSE_CMDLINE_H__
#define __PARSE_CMDLINE_H__

struct gx_parse_cmdline /*note: 这个结构体必须与ecos和linux内核gx_lowpower.h定义的结构体一致 */
{
	unsigned long tag_header;
	unsigned long waketime;
	unsigned long gpiomask;
	unsigned long gpiolevel;
	unsigned long keybuf[20];
	unsigned long keynum;
	unsigned long powerio[2];
	unsigned long utctime;		/* utc time day of second */
	unsigned char wakeflag;
	unsigned char reserved[3];
	unsigned long clock_speed;
	unsigned long panelio[2];
	long          timeshowflag;
	long          timezone;        /* unit : seconds */
	long          timesummer;      /* unit : hour */
	unsigned long curtime;        /*  unit : seconds  */
	unsigned long cecmode;
	unsigned long panel_dot_ctrl;
	unsigned long panel_brightness;
	unsigned long wakeupio[2];
	unsigned long reboot_flag;
};

enum lowpower_clock_speed {
	LOWPOWER_27MHZ_CLOCK = 0,
	LOWPOWER_24MHZ_CLOCK = 1
};

enum CEC_MODE {
	CEC_OFF = 0,
	CEC_ON = 1,
	CEC_NONE_WAKE_TV = 2,
};

extern __xdata struct gx_parse_cmdline *g_cmdline;	/* use this pointer can get cmdline*/
extern __xdata void *g_extra_param;

extern void parse_cmdline_init(void);
extern char get_localtime_hour(void);
extern char get_localtime_min(void);
extern void local_time_add(void);
extern void time_write_back(void);
extern char get_clock_speed(void);
extern unsigned long get_clock_frequency(void);
extern unsigned long get_localtime_in_sec(void);
#endif
