#include "gxhwlib_registers.h"
#include "watchdog.h"
#include "interrupt.h"

#define rWDT_CTRL  (*(volatile unsigned int *)(REG_BASE_WDT))
#define rWDT_MATCH  (*(volatile unsigned int *)(REG_BASE_WDT + 0x04))
#define rWDT_COUNT  (*(volatile unsigned int *)(REG_BASE_WDT + 0x08))
#define rWDT_WSR  (*(volatile unsigned int *)(REG_BASE_WDT + 0x0C))

#define WDT_SYS_CLK  27000000

#define ENABLE_WDT   (1 << 0)
#define ENABLE_ASSERT_RESET (1 << 1)
#define ENABLE_TIMER_INTERRUPT (1 << 3)

#define DISABLE_WDT   (0)
#define DISABLE_ASSERT_RESET (0 << 1)

#define WDT_ENABLE_MASK  0x3

#if defined(CONFIG_ARCH_ARM_SIRIUS)
#define WDT_ISR_VEC 48
#elif defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA)
#define WDT_ISR_VEC 55
#else
#define WDT_ISR_VEC 14
#endif

void gx_wdt_init(int clk)
{
	rWDT_MATCH = 0;
	rWDT_MATCH |= ((WDT_SYS_CLK / clk - 1) << 16);
}

void gx_wdt_start(void)
{
	rWDT_CTRL |= (ENABLE_ASSERT_RESET | ENABLE_WDT);
}

void gx_wdt_stop(void)
{
	rWDT_WSR = 0x0;
	rWDT_CTRL &= ~WDT_ENABLE_MASK;
}

void gx_wdt_settimeout(int ms)
{
	gx_wdt_stop();
	rWDT_MATCH &= 0xFFFF0000;
	rWDT_MATCH |= (0x10000 - ms);
}

void gx_wdt_ping(void)
{
	unsigned int val;

	val = (rWDT_WSR & 0xFFFF0000);
	rWDT_WSR = (0x5555 | val);
	rWDT_WSR = (0xAAAA | val);
}

void gx_wdt_reset_soon(int ms)
{
	gx_wdt_stop();
	gx_wdt_init(1000);
	gx_wdt_settimeout(ms);
	gx_wdt_start();
	while (1);
}

#ifdef CONFIG_ENABLE_IRQ
static enum interrupt_return wdt_interrupt_isr(uint32_t vector, void* pdata)
{
	gx_mask_interrupt(vector); //mask wdt irq

	printf("wdt_count : %08x\n", rWDT_COUNT);
	printf("wdt_ctrl : %08x\n", rWDT_CTRL);
	gx_wdt_ping();
	rWDT_CTRL |= (1 << 4);

	gx_unmask_interrupt(vector);

	return HANDLED;
}

void gx_wdt_timer(int32_t ms)
{
	gx_wdt_stop();
	gx_wdt_init(1000);
	rWDT_MATCH &= 0xFFFF0000;
	rWDT_WSR |= (ms) << 16;
	rWDT_CTRL |= (ENABLE_TIMER_INTERRUPT | ENABLE_WDT);

	gx_request_interrupt(WDT_ISR_VEC, IRQ, wdt_interrupt_isr, NULL);
}
#endif

