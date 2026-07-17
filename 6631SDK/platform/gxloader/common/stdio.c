#include "serial.h"
#include "config.h"
#include "gxdev.h"
#include "io.h"

#define O_RDONLY    (0)
#define O_WRONLY    (1)
#define O_RDWR      (2)

static int stdio_write(struct gx_devtab_entry *entry, const void *buf, unsigned int len)
{
	puts(buf);
}

static int stdio_read(struct gx_devtab_entry *entry, void *buf, unsigned int len)
{
	unsigned char *p_buf = buf;
	while (len --)
		*p_buf++ = uart_getc(CONFIG_UART_PORT);
}

static int stdio_open(struct gx_devtab_entry *entry, int success)
{
	return 0;
}

static int stdio_ioctl(struct gx_devtab_entry *entry, unsigned int key, void *buf)
{
	return 0;
}

static int stdio_close(struct gx_devtab_entry *entry)
{
	return 0;
}

static struct gx_devio_ops stdio_ops = {
	.open = stdio_open,
	.read = stdio_read,
	.write = stdio_write,
	.ioctl = stdio_ioctl,
	.close = stdio_close
};

static void register_stdio(void)
{
	dev_register(&stdio_ops, "/dev/stdin", NULL);
	dev_register(&stdio_ops, "/dev/stdout", NULL);
	dev_register(&stdio_ops, "/dev/stderr", NULL);
}

void stdio_init(void)
{
	register_stdio();
	open("/dev/stdin", O_RDONLY);
	open("/dev/stdout", O_RDWR);
	open("/dev/stderr", O_RDWR);
}
