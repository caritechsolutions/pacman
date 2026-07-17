/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou NationalChip Science and Technology Co., Ltd.
*                      (C)2006-2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :   gxcomm.h
* Author    :   liuyx
* Project   :   lowpower
* Type      :   csky
******************************************************************************
* Purpose   :   define the common types and functions
*****************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __GX_COMM_H__
#define __GX_COMM_H__

/* Includes --------------------------------------------------------------- */
#include "gxdefs.h"
#include "config.h"


/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Private types/constants ------------------------------------------------ */
/* The struct of functions in low-power mode */
typedef struct gxlowpower_func_s
{
	void (*ClosePll)(void);
	void (*ConfigPll)(void);
	void (*CloseModule)(void);
	void (*SetGpio)(U64 GpioMask,U64 GpioData);
	void (*ConfigPin)(void);
}gxlowpower_func_t;

/*
 * Base Address : interrupt
 */
#define REG_INTC_BASE_NO_MMU    (0x00500000)

#define INTC_irq_STATUS          0x00
#define INTC_irq_PENDSTATUS      0x10
#define INTC_irq_ENSET           0x20
#define INTC_irq_ENABLECLR       0x30
#define INTC_irq_ENABLEINT       0x40
#define INTC_irq_MASK            0x50	/* INTMASK register */

#define INTC_irq_STATUS_HI       0x04
#define INTC_irq_PENDSTATUS_HI   0x14
#define INTC_irq_ENSET_HI        0x24
#define INTC_irq_ENABLECLR_HI    0x34
#define INTC_irq_ENABLEINT_HI    0x44
#define INTC_irq_MASK_HI         0x54

#define INTC_IRQ_NSOURCE03_00    0x60
#define INTC_IRQ_NSOURCE07_04    0x64
#define INTC_IRQ_NSOURCE11_08    0x68
#define INTC_IRQ_NSOURCE15_12    0x6c
#define INTC_IRQ_NSOURCE19_16    0x70
#define INTC_IRQ_NSOURCE23_20    0x74
#define INTC_IRQ_NSOURCE27_24    0x78
#define INTC_IRQ_NSOURCE31_28    0x7c
#define INTC_IRQ_NSOURCE35_32    0x80
#define INTC_IRQ_NSOURCE39_36    0x84
#define INTC_IRQ_NSOURCE43_40    0x88
#define INTC_IRQ_NSOURCE47_44    0x8c
#define INTC_IRQ_NSOURCE51_48    0x90
#define INTC_IRQ_NSOURCE55_52    0x94
#define INTC_IRQ_NSOURCE59_56    0x98
#define INTC_IRQ_NSOURCE63_60    0x9c
#define INTC_IRQ_NSOURCE_END    INTC_IRQ_NSOURCE63_60

#define INTC_fiq_STATUS          0x100
#define INTC_fiq_PENDSTATUS      0x110
#define INTC_fiq_ENSET           0x120
#define INTC_fiq_ENABLECLR       0x130
#define INTC_fiq_ENABLEINT       0x140
#define INTC_fiq_MASK            0x150	/* INTMASK register */

#define INTC_fiq_STATUS_HI       0x104
#define INTC_fiq_PENDSTATUS_HI   0x114
#define INTC_fiq_ENSET_HI        0x124
#define INTC_fiq_ENABLECLR_HI    0x134
#define INTC_fiq_ENABLEINT_HI    0x144
#define INTC_fiq_MASK_HI         0x154

#define INTX_FIQ_NSOURCE03_00    0x160
#define INTX_FIQ_NSOURCE07_04    0x164
#define INTX_FIQ_NSOURCE11_08    0x168
#define INTX_FIQ_NSOURCE15_12    0x16c
#define INTX_FIQ_NSOURCE19_16    0x170
#define INTX_FIQ_NSOURCE23_20    0x174
#define INTX_FIQ_NSOURCE27_24    0x178
#define INTX_FIQ_NSOURCE31_28    0x17c
#define INTX_FIQ_NSOURCE35_32    0x180
#define INTX_FIQ_NSOURCE39_36    0x184
#define INTX_FIQ_NSOURCE43_40    0x188
#define INTX_FIQ_NSOURCE47_44    0x18c
#define INTX_FIQ_NSOURCE51_48    0x190
#define INTX_FIQ_NSOURCE55_52    0x194
#define INTX_FIQ_NSOURCE59_56    0x198
#define INTC_FIQ_NSOURCE63_60    0x19c

/* Private Macros --------------------------------------------------------- */
#define IRR_PROTCOL_TIME                    (562)
#define IRR_STD_UINT_SIG_ZERO               (IRR_PROTCOL_TIME << 2)

#define IRR_STD_UINT_SIG_ONE                (IRR_PROTCOL_TIME << 1)
#define IRR_AVERAGE_PULSE_WIDTH \
	    ((IRR_STD_UINT_SIG_ZERO + IRR_STD_UINT_SIG_ONE) >> 1)

#define IRR_STD_UINT_SIG_ZERO_STB40         (3*IRR_PROTCOL_TIME )
#define IRR_AVERAGE_PULSE_WIDTH_STB40 \
	    ((IRR_STD_UINT_SIG_ZERO_STB40 + IRR_STD_UINT_SIG_ONE) >> 1)

/* the follwing macro constants are decide by HARDWARE */
#define IRR_MAX_PULSE_NUM                    ( 64)
#define IRR_MAX_SIMCODE_PULSE_NUM            (  3)
#define IRR_MAX_FULLCODE_DVB40BIT_PULSE_NUM  ( 42)
#define IRR_MAX_FULLCODE_PANASONIC_PULSE_NUM ( 49)
#define IRR_MAX_FULLCODE_PULSE_NUM           ( 33)
#define IRR_MAX_PULSE_NUM_PHILIPS            ( 13)
#define IRR_MIN_PULSE_NUM_PHILIPS            (  6)
#define IRR_MAX_PULSE_NUM_BESCON             ( 16)
#define IRR_MIN_PULSE_NUM_BESCON             (  9)
#define IRR_EMBEDDED_NOISE_ONCE              ( 34)
#define IRR_EMBEDDED_NOISE_MORE              ( 48)

#define IRR_RC5_MAX_PULSE_NUM                ( 20)
#define IRR_RC5_MIN_PULSE_NUM                (  6)
#define IRR_RC5_PROTCOL_TIME                 (889)

// 1M频率下philips1位高电平或低电平的计数值
#define IRR_PROTCOL_TIME_PHILIPS             (888)
// 1M频率下bescon 1位高电平或低电平的计数值,偏大点
#define IRR_PROTCOL_TIME_BESCON              (440)
// 1M频率下panasonic 1位高电平或低电平的计数值
#define IRR_PROTCOL_TIME_PANASONIC           (380)
//Ioctl definitions
#define IRR_SET_MODE                (0x1000)

enum {
	IRR_PROTOCOL_NEC  = 1,
	IRR_PROTOCOL_PHILIPS,
	IRR_PROTOCOL_BESCON,
	IRR_PROTOCOL_PANASONIC,
	IRR_PROTOCOL_40BIT
};

struct gx_irr_info
{
	unsigned int pulse_val[IRR_MAX_PULSE_NUM];
	unsigned int pulse_num;
	unsigned int key_code;
	unsigned char protocol;
};

#define TAG_MAGIC    0x54410003
struct gx_fw_status
{
	unsigned int  tag_header;
	unsigned char wakeflag;
	unsigned char reserved[3];
	unsigned int  utctime;
	unsigned int  curtime;
};

struct gx_parse_cmdline
{
	unsigned int curtime;
	unsigned int waketime;
	unsigned int gpionum;
	unsigned int gpiolevel;
	unsigned int keybuf[20];
	unsigned int keynum;
	unsigned int powerio[2];
	unsigned int panelio[2];
	unsigned int timeshowflag;
	int          timezone;   /* unit : second */
	unsigned int timesummer; /* unit : hour */
	unsigned int lowpower_clock_speed;
	unsigned int panel_dot_ctrl;
	unsigned char panel_brightness;
	unsigned char reboot_flag;
};

/* Private Variables ------------------------------------------------------ */

/* Export Variables ------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Init the UART with baudrate being equry to 115200 */
#if (DEBUG_EN > 0)&&(GXCHIP_TYPE == GXCHIP_TYPE_GX3201 || GXCHIP_TYPE == GXCHIP_TYPE_GX3113C)
#define UART_SEL()                        \
do{                                       \
	rMULPIN0_CONFIG &= (~(0x3<<17));  \
	rMULPIN2_CONFIG &= (~(0x01<<6));  \
}while(0)
#elif (DEBUG_EN > 0)&&(GXCHIP_TYPE == GXCHIP_TYPE_GX3211 || GXCHIP_TYPE == GXCHIP_TYPE_GX6605S)
#define UART_SEL()                         \
do{                                        \
	rMULPIN4_CONFIG &= (~(0x3<<22));   \
}while(0)
#else
#define UART_SEL()  \
do{                 \
	;           \
}while(0)
#endif

#if DEBUG_EN > 0
#define UART_INIT(SysClk,BaudRate)                                              \
do{                                                                             \
	unsigned int tmp;                                                       \
	rUART_SCICR = 0x0000;                                                   \
	tmp = (SysClk)/16/BaudRate - 1;                                         \
	if ((SysClk/16/(tmp+1) - BaudRate) > (BaudRate - (SysClk/16/(tmp+1+1))))\
		tmp += 1;                                                       \
	rUART_SCIBR = tmp;                                                      \
	rUART_SCICR = 0x0600;                                                   \
	UART_SEL();                                                             \
}while(0)
#else
#define UART_INIT(SysClk,BaudRate) \
do{                                \
	;                          \
}while(0)
#endif


/* Init and enable IRR */
#define IRR_INIT(SysClk)                                              \
    do{                                                               \
          rIRR_DIV &= 0xFFFFFF00;                                     \
          rIRR_DIV |= SysClk/1000000 - 1;                             \
          rIRR_CNTL = (20000 << 12)|(33 << 6)|(1 << 5)|(1 << 1);      \
          rIRR_CNTL |= (1 << 3);                                      \
    }while(0)

/* Init and enable WDT */
#define WDT_INIT(SysClk)                                              \
    do{                                                               \
        rWDT_MATCH = ((SysClk/10000 - 1) << 16)|(0x10000 - 5000);  \
        rWDT_CTRL = 3;                                                \
	    GXLOWPOWER_TinyPutStr("WDT will reset the system now.\n");    \
    }while(0)

/* Exported Functions ----------------------------------------------------- */
int gxlowpower_irrGetKey(struct gx_irr_info* info);

void GXLOWPOWER_TinyPutStr(char *pStr);
void GXLOWPOWER_TinyPutNum(S32 Num);

extern void time_write_back(void);
extern int parse_cmdline(struct gx_parse_cmdline *cmdline);
extern int flash_read(unsigned int addr, unsigned char *buf,unsigned int len);
extern int flash_write(unsigned int addr,unsigned char *buf,unsigned int len);
extern void flash_erase(unsigned int addr);

#define __raw_writel(val, addr) \
	(void)((*(volatile unsigned int *)(unsigned int)(addr)) = (unsigned int)(val))
#define __raw_readl(addr) \
	({ unsigned int __v   = (*(volatile unsigned int *) (addr)); __v; })

#define REG_SET_CLR_BIT(reg, bit, bit_val) do {     \
    unsigned int tmp = __raw_readl(reg);        \
    tmp &= ~(1UL << (bit));         \
    tmp |= ((bit_val) << (bit));            \
    __raw_writel(tmp, reg);             \
}while(0)

void fw_status_init(void);
void gx_irq_init(void);
typedef int (*irq_handler_t) (int irq, void *pdata);
void gx_request_irq(int irq, irq_handler_t handler, void *pdata);
void gx_free_irq(int irq);
void gx_mask_irq(unsigned int irq);
void gx_unmask_irq(unsigned int irq);
void gx_disable_all_interrupt(void);

/* init ctr: every 1ms generate a interrupt */
int  gx_ctr_init(void);
/* disable ctr & disable interrupt*/
int  gx_ctr_disable(void);
/* register callback fun
 *
 * @fun              : the functions to be execution
 * @interval_time_ms : the time interval to run fun
 * @repeat_times     : the number of executions of this fun
 *                         repeat_times = 0, do not execution the fun
 *                         repeat_times > 0, the number of executions of this fun
 *                         repeat_times < 0, The number of executions is not limited
 * @private_data     : irq private data
 * */
int gx_ctr_callback_register(int (*fun)(void*), int interval_time_ms,
		int repeat_times, void *private_data);

#ifdef __cplusplus
}
#endif

#endif
