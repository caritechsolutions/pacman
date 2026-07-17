#ifndef __UART_CORE_H__
#define __UART_CORE_H__

#include "kfifo.h"

#define UART_RX_FIFO_LEN 1024*1024

struct uart_device {
	unsigned int port;
	unsigned int reg_base;
	unsigned int isr_vec;
	struct kfifo * volatile rx_fifo;
	unsigned int is_init;
};

#endif
