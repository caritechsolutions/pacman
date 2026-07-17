#include <stdio.h>
#include <spi.h>
#include <device.h>
#include <io.h>
#include <linux/errno.h>
#include <gxhwlib_registers.h>
#include "board-common.h"
#include "gpio.h"
#include "gx_spi.h"


/*#define MODULE_DEBUG 0*/
//#define MODULE_DEBUG
#undef MODULE_DEBUG
#ifdef MODULE_DEBUG
#define md_debug(fmt, arg...) \
	printf(fmt, ##arg)
#else
#define md_debug(fmt, arg...) \
	do {}while(0)
#endif

#define SPI_TRANSFER_SIZE	(sizeof(unsigned int))

static inline unsigned int readl(const volatile void __iomem *addr)
{
	unsigned int ret = __raw_readl(addr);
	return ret;
}

static inline void writel(unsigned int b, volatile void __iomem *addr)
{
	__raw_writel(b, addr);
}

static int gx_spi_setup(struct spi_device *spi)
{
	struct spi_regs *regs=(struct spi_regs *)spi->master->data;
	md_debug("spi: in %s.\n", __FUNCTION__);
	writel(SFLASH_CONTROL_VALUE, &regs->SPI_CR);
	spi->mode = SPI_RX_DUAL|SPI_TX_DUAL;

	return 0;
}

static inline void gx_spi_write(struct spi_device *spi, unsigned int data, unsigned int regval)
{
	struct spi_regs *regs=(struct spi_regs *)spi->master->data;
	writel(data, &regs->SPI_TX_DATA);
	regval |= CR_SPGO;
	writel(regval, &regs->SPI_CR);

	while(!(readl(&regs->SPI_SR) & SR_OPE_RDY));

	regval &= ~CR_SPGO;
	writel(regval, &regs->SPI_CR);
}

static inline unsigned int gx_spi_read(struct spi_device *spi, unsigned int regval)
{
	struct spi_regs *regs=(struct spi_regs *)spi->master->data;
	regval |= CR_SPGO;
	writel(regval, &regs->SPI_CR);

	while(!(readl(&regs->SPI_SR) & SR_OPE_RDY));

	regval &= ~CR_SPGO;
	writel(regval, &regs->SPI_CR);

	return readl(&regs->SPI_RX_DATA);
}

static inline unsigned int gx_spi_set_transfer_len(int size, unsigned int regval)
{
	switch(size){
		case 1:
			if((regval & CR_ICNT_4) != 0){
				regval &= ~CR_ICNT_4;
			}
			break;
		case 2:
			if((regval & CR_ICNT_4) != CR_ICNT_2){
				regval &= ~CR_ICNT_4;
				regval |= CR_ICNT_2;
			}
			break;
		case 3:
			if((regval & CR_ICNT_4) != CR_ICNT_3){
				regval &= ~CR_ICNT_4;
				regval |= CR_ICNT_3;
			}
			break;
		case 4:
			if((regval & CR_ICNT_4) != CR_ICNT_4){
				regval |= CR_ICNT_4;
			}
			break;
		default:
			break;
	}
	return regval;
}

static void spi_csoutput_enable(struct spi_device *spi)
{
	int regval;
	struct spi_regs *regs=(struct spi_regs *)spi->master->data;
	switch(spi->chip_select)
	{
		case 0:
			regval = readl(&regs->SPI_CR);
			regval &= ~CR_CS_SEL;
			regval &= ~CR_CSPOL_H;
			regval |= CR_CSVALID;
			writel(regval, &regs->SPI_CR);
			break;
		case 1:
			regval = readl(&regs->SPI_CR);
			regval |= CR_CS_SEL;
			regval |= CR_CSPOL_H;
			regval |= CR_CSVALID;
			writel(regval, &regs->SPI_CR);
			gx_gpio_setlevel(SPI_CS1_VIRTUAL_GPIO, 0);
			break;
		case 2:
			regval = readl(&regs->SPI_CR);
			regval |= CR_CS_SEL;
			regval |= CR_CSPOL_H;
			regval |= CR_CSVALID;
			writel(regval, &regs->SPI_CR);
			gx_gpio_setlevel(SPI_CS2_VIRTUAL_GPIO, 0);
			break;
		default:
			break;
	}
}

static void spi_csoutput_disable(struct spi_device *spi)
{
	int regval;
	struct spi_regs *regs=(struct spi_regs *)spi->master->data;
	switch(spi->chip_select)
	{
		case 0:
			regval = readl(&regs->SPI_CR);
			regval &= ~CR_CS_SEL;
			regval &= ~CR_CSPOL_H;
			regval &= ~CR_CSVALID;
			writel(regval, &regs->SPI_CR);
			break;
		case 1:
			regval = readl(&regs->SPI_CR);
			regval |= CR_CS_SEL;
			regval &= ~CR_CSPOL_H;
			regval &= ~CR_CSVALID;
			writel(regval, &regs->SPI_CR);
			gx_gpio_setlevel(SPI_CS1_VIRTUAL_GPIO, 1);
			break;
		case 2:
			regval = readl(&regs->SPI_CR);
			regval |= CR_CS_SEL;
			regval &= ~CR_CSPOL_H;
			regval &= ~CR_CSVALID;
			writel(regval, &regs->SPI_CR);
			gx_gpio_setlevel(SPI_CS2_VIRTUAL_GPIO, 1);
			break;
		default:
			break;
	}
}

static int gx_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	unsigned char *txbuf, *rxbuf, *p;
	unsigned int len = 0, data;
	struct list_head *cursor;
	struct list_head *head;
	struct spi_transfer *cur_transfer;
	int retval;
	unsigned int spi_cr ;
	struct spi_regs *regs=(struct spi_regs *)spi->master->data;

	md_debug("\n---spi: in %s.---\n", __FUNCTION__);
	head =&mesg->transfers;
	if(list_empty(head)) {
		mesg->status = -EFAULT;
		return -EFAULT;
	}

	spi_csoutput_enable(spi);
	mesg->actual_length = 0;

	spi_cr = readl(&((struct spi_regs *)(spi->master->data))->SPI_CR);

	list_for_each(cursor, head)
	{
		cur_transfer = list_entry(cursor, struct spi_transfer, transfer_list);
		txbuf = (unsigned char *)cur_transfer->tx_buf;
		rxbuf = (unsigned char *)cur_transfer->rx_buf;
		len = cur_transfer->len;
		if((len == 0) || (txbuf && rxbuf) || (txbuf == NULL && rxbuf == NULL))
		{
			retval = -EFAULT;
			goto out;
		}

		md_debug("len = %d.\n", len);

		if(txbuf){
			p = txbuf;
			if (cur_transfer->tx_nbits == SPI_NBITS_DUAL)
				spi_cr |= CR_DUAL_TX;

			if(len >= SPI_TRANSFER_SIZE)
				spi_cr = gx_spi_set_transfer_len(4, spi_cr);

			writel(spi_cr, &regs->SPI_CR);

			while(len >= SPI_TRANSFER_SIZE){
				data = (*p++) << 24;
				data |= (*p++) << 16;
				data |= (*p++) << 8;
				data |= (*p++);
				gx_spi_write(spi, data, spi_cr);
				len -= SPI_TRANSFER_SIZE;
			}

			if(len > 0){
				spi_cr = gx_spi_set_transfer_len(1, spi_cr);
				writel(spi_cr, &regs->SPI_CR);
				while(len--){
					data = *p++;
					gx_spi_write(spi, data, spi_cr);
				}
			}
			if (cur_transfer->tx_nbits == SPI_NBITS_DUAL) {
				spi_cr &= ~(CR_DUAL_TX);
				writel(spi_cr, &regs->SPI_CR);
			}
		}else if(rxbuf){
			p = rxbuf;

			if (cur_transfer->rx_nbits == SPI_NBITS_DUAL)
				spi_cr |= CR_DUAL_RX;

			if(len >= SPI_TRANSFER_SIZE)
			{
				spi_cr = gx_spi_set_transfer_len(4, spi_cr);
				writel(spi_cr, &regs->SPI_CR);
			}

			while(len >= SPI_TRANSFER_SIZE){
				data = gx_spi_read(spi, spi_cr);
				*p++ = (data >> 24) & 0xff;
				*p++ = (data >> 16) & 0xff;
				*p++ = (data >>  8) & 0xff;
				*p++ = data & 0xff;
				len -= SPI_TRANSFER_SIZE;
			}

			if(len > 0){
				spi_cr = gx_spi_set_transfer_len(1, spi_cr);
				writel(spi_cr, &regs->SPI_CR);
				while(len--) {
					data = gx_spi_read(spi, spi_cr);
					*p++ = data & 0xff;
				}
			}
			if (cur_transfer->rx_nbits == SPI_NBITS_DUAL) {
				spi_cr &= ~(CR_DUAL_RX);
				writel(spi_cr, &regs->SPI_CR);
			}
		}
		mesg->actual_length += cur_transfer->len;
	}

	retval = 0;
out:
	spi_csoutput_disable(spi);

	mesg->status = retval;

	return retval;
}

static void gx_spi_cleanup(struct spi_device *spi)
{
	spi = spi;
}

static struct spi_master gx_spi_master={
	.bus_num        = 0,
	.num_chipselect = 3,
	.setup          = gx_spi_setup,
	.transfer       = gx_spi_transfer,
	.cleanup        = gx_spi_cleanup,
	.data           = (void *)REG_BASE_SPI,
};

void gx_spi_probe(void)
{
#ifdef CONFIG_ARCH_ARM_SIRIUS
	// spi pin mux
	*(volatile unsigned int*)REG_PINMUX_PORTB |= 0x000001C0;
	*(volatile unsigned int*)REG_PINMUX_PORTC &= ~(1 << 23);
	*(volatile unsigned int*)REG_PINMUX_PORTD &= ~(0x0000001E);
#endif

#if defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_CKMMU_SCORPIO) || defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	__raw_writel(0 << 0, GX_REG_BASE_SPI1_SELECT);
#endif
	spi_register_master(&gx_spi_master);
}
