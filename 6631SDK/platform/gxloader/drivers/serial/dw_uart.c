#include "stdio.h"
#include "string.h"
#include "config.h"
#include "io.h"
#include "util.h"
#include "serial.h"
#include "uart_core.h"

#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
#include "interrupt.h"
#endif

#define  DSW_INTR_NUM 15
#define  DSW_SERIAL_BAUD_NUMS 22
#define  DSW_RBR  0x0
#define  DSW_THR  0x0
#define  DSW_DLL  0x0
#define  DSW_DLH  0x4
#define  DSW_IER  0x4
#define  DSW_FCR  0x8
#define  DSW_IIR  0x8
#define  DSW_LCR  0xc
#define  DSW_MCR  0x10
#define  DSW_LSR  0x14
#define  DSW_USR  0x7c
#define  DSW_FCR  0x8
#define  DSW_DLF  0xc0
#define  DSW_TFL  0x80
#define  DSW_RFL  0x84
#define  DSW_HTX  0xa4

#define  DSW_IER_PTIME 0x7
#define  DSW_IER_ELSI  0x2
#define  DSW_IER_ETBEI 0x1
#define  DSW_IER_ERBFI 0x0

#define  DSW_IIR_RXVAIL 0x4
#define  DSW_IIR_RXTIMEOUT 0xC
#define  DSW_IIR_THRE  0x2


#define  DSW_LCR_DLAB  0x7
#define  DSW_LCR_BC  0x6
#define  DSW_LCR_PEN  0x3
#define  DSW_LCR_STOP  0x2
#define  DSW_LCR_DLS  0x0

#define  DSW_LSR_DR  0x0
#define  DSW_LSR_TEMT 0x6
#define  DSW_LSR_THRE 0x5

#define  DSW_MCR_AFCE  0x20
#define  DSW_MCR_RTS   0x02
#define  DSW_MCR_DTR   0x0

#define  DSW_FCR_FIFOE  0x0
#define  DSW_FCR_RFIFOR  0x01
#define  DSW_FCR_XFIFOR  0x2
#define  DSW_FCR_TET  0x4
#define  DSW_FCR_RCVR  0x6

#define  DSW_USR_TFNF  0x1
#define  DSW_USR_RFNE  0x3
#define  DSW_USR_BUSY  0x0

#define DW_WAIT_IDLE(base, offset, state, bit) do {\
	state = __raw_readl(((base) + (offset)));\
} while( ((state) & (1 << (bit))) )

#if defined(CONFIG_ARCH_ARM_SIRIUS)
#define UART2_ISR_VEC_NUM 34
#define UART3_ISR_VEC_NUM 33
#elif defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
#define UART0_ISR_VEC_NUM 38
#define UART1_ISR_VEC_NUM 37
#define UART2_ISR_VEC_NUM 36
#else
#define UART2_ISR_VEC_NUM 15
#define UART3_ISR_VEC_NUM 26
#endif

#if defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
struct uart_device uart_dev[] = {
	{0, REG_BASE_UART0, UART0_ISR_VEC_NUM, NULL},
	{1, REG_BASE_UART1, UART1_ISR_VEC_NUM, NULL},
	{2, REG_BASE_UART2, UART2_ISR_VEC_NUM, NULL}
};
#else
struct uart_device uart_dev[] = {
	{0, REG_BASE_UART2, UART2_ISR_VEC_NUM, NULL},
	{1, REG_BASE_UART3, UART3_ISR_VEC_NUM, NULL}
};
#endif

#if defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_CYGNUS)
//#define DW_UART_FRACTIONAL_BAUD_DIV
//#define UART_DLF_SIZE 4
#endif


struct uart_device *get_uart_dev(int port)
{
	unsigned char i = 0;

#ifdef CONFIG_ARCH_ARM_SIRIUS
	if (port > 2)
		port -= 2;
#endif

	for (i = 0; i < sizeof(uart_dev)/sizeof(struct uart_device); i++) {
		if (uart_dev[i].port == port)
			return &uart_dev[i];
	}

	return &uart_dev[0];
}

#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
static enum interrupt_return uart_interrupt_isr(uint32_t vector, void *pdata)
{
	struct uart_device *p_uart_dev = (struct uart_device *)pdata;
	unsigned char c;
	unsigned int  state;

	state = __raw_readl(p_uart_dev->reg_base + DSW_LSR);
	if (state & (1 << DSW_LSR_DR)) {
		c = (0xff & __raw_readl(p_uart_dev->reg_base + DSW_RBR));
		if (p_uart_dev->rx_fifo != NULL)
			kfifo_put(p_uart_dev->rx_fifo, &c, 1);
	}

	return HANDLED;
}
#endif

static void dw_serial_set_pin_multip(void)
{
	unsigned int pin_sel0 = 0;
	unsigned int pin_sel1 = 0;

#if defined(CONFIG_ARCH_CKMMU_GX3201) || defined(CONFIG_ARCH_CKMMU_GX3113C)
	pin_sel0 = __raw_readl(0xa030a140);
	pin_sel1 = __raw_readl(0xa030a148);

	pin_sel0 |= (1 << 17) | (1 << 18); // Multiple pin
	pin_sel1 |= (1 << 6);

	__raw_writel(pin_sel0, 0xa030a140);
	__raw_writel(pin_sel1, 0xa030a148);
#elif defined(CONFIG_ARCH_CKMMU_GX3211)
	pin_sel1 = 0;
	pin_sel0 = __raw_readl(0xa030a14c);
	pin_sel0 |= (0x3 << 22); // Multiple pin
	__raw_writel(pin_sel0, 0xa030a14c);
#elif defined(CONFIG_ARCH_CKMMU_GX6605S)
	pin_sel1 = 0;
	pin_sel0 = __raw_readl(0xa030a14c);
	pin_sel0 |= (0x1 << 22); // Multiple pin
	__raw_writel(pin_sel0, 0xa030a14c);
#elif defined(CONFIG_ARCH_CKMMU_TAURUS) || defined(CONFIG_ARCH_CKMMU_GEMINI)
	pin_sel0 = __raw_readl(REG_PINMUX_PORTF);
	pin_sel0 |= (0x3 << 22); // Multiple pin
	__raw_writel(pin_sel0, REG_PINMUX_PORTF);
#elif defined(CONFIG_ARCH_CKMMU_CYGNUS)
	pin_sel0 = __raw_readl(REG_INTERNAL_FUNSEL);
	pin_sel0 |= (0x3 << 0); // Multiple pin
	__raw_writel(pin_sel0, REG_INTERNAL_FUNSEL);
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO)
	pin_sel0 = __raw_readl(REG_INTERNAL_FUNSEL);
	pin_sel0 |= (0x3 << 22); // Multiple pin
	__raw_writel(pin_sel0, REG_INTERNAL_FUNSEL);
#endif
}

static void uart_init_port(int baudrate, int port)
{
	uint32_t _usr = 0;
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	uart_set_baudrate(baudrate, port);
	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;
	if (p_uart_dev->is_init > 0)
		return;
	p_uart_dev->is_init = 1;

	dw_serial_set_pin_multip();

	__raw_writel(0x0, (dw_uartbase + DSW_IER));
	__raw_writel(0x3, (dw_uartbase + DSW_MCR));

	DW_WAIT_IDLE(dw_uartbase, DSW_USR, _usr, DSW_USR_BUSY);
	__raw_writel(0x03, (dw_uartbase + DSW_LCR));
	__raw_writel(0x01, (dw_uartbase + DSW_FCR));
#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
	p_uart_dev = get_uart_dev(port);
	if (p_uart_dev->rx_fifo == NULL) {
		p_uart_dev->rx_fifo = kfifo_alloc(UART_RX_FIFO_LEN);
		if (p_uart_dev->rx_fifo == NULL)
			return;
		__raw_writel(0x01, (dw_uartbase + DSW_IER));//enable receive interrupt
		gx_request_interrupt(p_uart_dev->isr_vec, IRQ, uart_interrupt_isr, p_uart_dev);
	}
#endif
}

#define DIV_ROUND_CLOSEST(x, divisor)( 			\
{                                               \
         typeof(divisor)__divisor = divisor;        \
         (((x)+ ((__divisor) / 2)) / (__divisor));  \
}                                                   \
)

static int fls(unsigned int v)
{
	int n = 32;

	if (!v) return -1;
	if (!(v & 0xFFFF0000)) { v <<= 16; n -= 16; }
	if (!(v & 0xFF000000)) { v <<=  8; n -= 8;  }
	if (!(v & 0xF0000000)) { v <<=  4; n -= 4;  }
	if (!(v & 0xC0000000)) { v <<=  2; n -= 2;  }
	if (!(v & 0x80000000)) { v <<=  1; n -= 1;  }

	return n;
}

int uart_set_baudrate(int baudrate, int port)
{
	int clkdiv;
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;
	unsigned int _lcr = 0;
	unsigned int _usr = 0;
	unsigned int _dlf = 0;
	unsigned int dlf_size;
	unsigned int reg, base_baud;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
	gx_disable_irq();
#endif
	_lcr = __raw_readl(dw_uartbase + DSW_LCR);
	_lcr |= (1 << DSW_LCR_DLAB);
	DW_WAIT_IDLE(dw_uartbase, DSW_USR, _usr, DSW_USR_BUSY);
	__raw_writel(_lcr, (dw_uartbase + DSW_LCR));
#ifdef DW_UART_FRACTIONAL_BAUD_DIV
	__raw_writel((~0U), dw_uartbase + DSW_DLF);
	reg = __raw_readl(dw_uartbase + DSW_DLF);
	__raw_writel(0, dw_uartbase + DSW_DLF);

	if (reg) {
		dlf_size = fls(reg);
	}

	base_baud = 16 * baudrate;
	clkdiv = (GX_UART_CLOCK) / base_baud;
	_dlf = DIV_ROUND_CLOSEST(((GX_UART_CLOCK % base_baud) << dlf_size), base_baud);
#else
	_dlf = 0;
	clkdiv = (GX_UART_CLOCK + (8 * baudrate)) / (16 * baudrate);
#endif
	__raw_writel(clkdiv & 0xff, (dw_uartbase + DSW_DLL));
	__raw_writel((clkdiv >> 8) & 0xff, (dw_uartbase + DSW_DLH));
	__raw_writel(_dlf, (dw_uartbase + DSW_DLF));

	_lcr &= ~(1 << DSW_LCR_DLAB);
	DW_WAIT_IDLE(dw_uartbase, DSW_USR, _usr, DSW_USR_BUSY);
	__raw_writel(_lcr, (dw_uartbase + DSW_LCR));
#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
	gx_enable_irq();
#endif

	return 0;
}

#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
int uart_flush_in(int port)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

	if (p_uart_dev->rx_fifo != NULL) {
		gx_disable_irq();
		kfifo_reset(p_uart_dev->rx_fifo);
		gx_enable_irq();
	}

	return 0;
}
#endif

static inline void uart_raw_putc(int ch, int port)
{
	unsigned int  state;
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

	do {
		state = __raw_readl(dw_uartbase + DSW_LSR);
	} while ( !(state & (1 << DSW_LSR_TEMT)) );

	__raw_writel((unsigned int)ch, (dw_uartbase + DSW_THR));
}

void uart_compatible_putc(int ch, int port)
{
#ifndef CONFIG_NO_PRINT
	/* If \n, also do \r, In order to compatible with the printf of Windows*/
	if (ch == '\n')
		uart_raw_putc('\r', port);
	uart_raw_putc(ch, port);
#endif
}

void uart_putc(int ch, int port)
{
	uart_raw_putc(ch, port);
}

void uart_putstring(int port, unsigned char *buffer, int len)
{
	while (len--)
		uart_raw_putc(*buffer++, port);
}

int uart_getc(int port)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;
#ifndef CONFIG_NO_GETC
#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
	unsigned char c = 0;

	if (p_uart_dev->rx_fifo != NULL) {
		while (!kfifo_len(p_uart_dev->rx_fifo));
		kfifo_get(p_uart_dev->rx_fifo, &c, 1);
		return c;
	} else
		return 0;
#else
	unsigned int state;
	unsigned int ch;

	do {
		state = __raw_readl(dw_uartbase + DSW_LSR);
	}while (!(state & (1 << DSW_LSR_DR)));

	ch = __raw_readl(dw_uartbase + DSW_RBR);

	return (ch & 0xff);
#endif
#else
	return 0;
#endif
}

int uart_try_getc(char *c, int port)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;
#ifndef CONFIG_NO_GETC
#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
	if (p_uart_dev->rx_fifo != NULL) {
		if (kfifo_len(p_uart_dev->rx_fifo) > 0) {
			kfifo_get(p_uart_dev->rx_fifo, c, 1);
			return 0;
		} else
			return -1;
	} else
		return -1;
#else
	unsigned int state;

	state = __raw_readl(dw_uartbase + DSW_LSR);
	if (state & (1 << DSW_LSR_DR)) {
		*c = (0xff & __raw_readl(dw_uartbase + DSW_RBR));
		return 0;
	}
	else
		return -1;
#endif
#else
	return -1;
#endif
}

#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
int uart_try_getstring(int port, unsigned char *buffer, int len)
{
	struct uart_device *p_uart_dev = get_uart_dev(port);

	if (p_uart_dev->rx_fifo != NULL)
		return kfifo_get(p_uart_dev->rx_fifo, buffer, len);
	else
		return -1;
}
#endif

void uart_init(int baudrate, int port)
{
	uart_init_port(baudrate, port);
}

void uart_reset(int baudrate, int port)
{
	uart_init_port(baudrate, port);
}

#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
int uart_close_irq(void)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;
	unsigned int port = 0;
	unsigned char i = 0;

	for (i = 0; i < sizeof(uart_dev)/sizeof(struct uart_device); i++) {
		p_uart_dev = &uart_dev[i];
		port = p_uart_dev->port;
		dw_uartbase = p_uart_dev->reg_base;

		__raw_writel(0x00, (dw_uartbase + DSW_IER));
		gx_free_interrupt(p_uart_dev->isr_vec);
	}
}
#endif

int uart_tstc(int port)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;
#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
	if (p_uart_dev->rx_fifo != NULL) {
		if (kfifo_len(p_uart_dev->rx_fifo) > 0)
			return 1;
		else
			return 0;
	} else
		return 0;
#else
	return (__raw_readl(dw_uartbase + DSW_LSR) & (1 << DSW_LSR_DR)) != 0;
#endif
}

void uart_halt_tx(int port, int enable)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

	if (enable)
		__raw_writel(0x01, dw_uartbase + DSW_HTX);
	else
		__raw_writel(0x00, dw_uartbase + DSW_HTX);
}

int uart_get_tx_fifo_size(int port)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

	return __raw_readl(dw_uartbase + DSW_TFL);
}

int uart_get_rx_fifo_size(int port)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

	return __raw_readl(dw_uartbase + DSW_RFL);
}

int uart_enable_auto_flow(int port, int enable)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;
	unsigned int mcr;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

	mcr = __raw_readl(dw_uartbase + DSW_MCR);
	if (enable)
		mcr |= DSW_MCR_AFCE | DSW_MCR_RTS;
	else
		mcr &= ~(DSW_MCR_AFCE | DSW_MCR_RTS);
	__raw_writel(mcr, dw_uartbase + DSW_MCR);
	__raw_writel(0xC7, dw_uartbase + DSW_FCR);
}

int uart_start_break(int port)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;
	unsigned int _lcr = 0;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

	_lcr = __raw_readl(dw_uartbase + DSW_LCR);
	_lcr |= 1 << (DSW_LCR_BC);
	__raw_writel(_lcr, dw_uartbase + DSW_LCR);

	return 0;
}

int uart_end_break(int port)
{
	unsigned int dw_uartbase = 0;
	struct uart_device *p_uart_dev;
	unsigned int _lcr = 0;

	p_uart_dev = get_uart_dev(port);
	dw_uartbase = p_uart_dev->reg_base;

	_lcr = __raw_readl(dw_uartbase + DSW_LCR);
	_lcr &= ~(1 << (DSW_LCR_BC));
	__raw_writel(_lcr, dw_uartbase + DSW_LCR);

	return 0;
}
