#ifndef __SERIAL_H__
#define __SERIAL_H__

void uart_init(int baudrate, int port);
void uart_putc(int ch, int port);
void uart_putstring(int port, unsigned char *buffer, int len);
void uart_reset(int baudrate, int port);
int uart_getc(int port);
int uart_try_getc(char *c, int port);
void uart_compatible_putc(int ch, int port);
int uart_set_baudrate(int baudrate, int port);
int uart_tstc(int port); /* Returns positive if character is available, zero otherwise. */
#if defined (CONFIG_ENABLE_IRQ) && defined (CONFIG_ENABLE_UART_IRQ)
int uart_try_getstring(int port, unsigned char *buffer, int len);
/* must be not used in interrupt handle func */
int uart_flush_in(int port);
int uart_close_irq(void);
#endif
void uart_halt_tx(int port, int enable);
int uart_get_tx_fifo_size(int port);
int uart_get_rx_fifo_size(int port);
int uart_enable_auto_flow(int port, int enable);
int uart_start_break(int port);
int uart_end_break(int port);
#endif
