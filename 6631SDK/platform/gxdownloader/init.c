#include "stdio.h"
#include "string.h"
#include "io.h"
#include "types.h"

#include "gxloader.h"
#include "config.h"
#include "gpio.h"
#include "common/gxlog.h"
#include "common/get_mem_info.h"

extern int GxCore_Startup(int argc, char **argv);

int main(int argc, char **argv)
{
	extern int __bss_start__, __bss_end__,__text_start__;
	/* clears BSS section */
	memset(&__bss_start__, 0, (uint32_t)&__bss_end__ - (uint32_t)&__bss_start__);

	unsigned long heap_start = 0;
	unsigned long heap_end = 0;

	/* initialize heap */
	heap_start = HEAP_START_ADDR;
	heap_end   = HEAP_END_ADDR;
	gxlogd("heap info: start=0x%x,end=0x%x,heap size is 0x%x\n", heap_start, heap_end, heap_end - heap_start);
	heap_init((void *)heap_start, heap_end - heap_start);

	parse_tag_cmdline();
	gx_interrupt_init();
	gx_time_init();
	/* initializes UART */
	uart_init(CONFIG_UART_BAUDRATE, CONFIG_UART_PORT);

	io_framework_init();
	stdio_init(); //init stdin, stdout, stderr, so can use fprintf(stdout, "nationalchip")
	irr_init();//这个不能注释，必须留这里不然会先ota不能引导app问题
	gx_gpio_init();

	gxflash_init();

	GxCore_Startup(0,NULL);

	while(1);
}
