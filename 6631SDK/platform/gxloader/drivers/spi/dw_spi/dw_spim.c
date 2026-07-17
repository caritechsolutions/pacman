#include <stdio.h>
#include <spi.h>
#include <device.h>
#include <io.h>
#include <linux/errno.h>
#include <gxhwlib_registers.h>
#include "board-common.h"
#include "dw_spim.h"
#include "gx_api.h"
#include "time.h"


#if 0
#define DBG   printf
#define PRINT_TRANS_DATA
#else
#define DBG(x...)
#endif


#undef MODULE_DEBUG
#ifdef MODULE_DEBUG
#define md_debug(fmt, arg...) \
	printf(fmt, ##arg)
#else
#define md_debug(fmt, arg...) \
	do {}while(0)
#endif


#define START_STATE     ((void *)0)
#define RUNNING_STATE   ((void *)1)
#define DONE_STATE      ((void *)2)
#define ERROR_STATE     ((void *)-1)

#define QUEUE_RUNNING   0
#define QUEUE_STOPPED   1

#define MRST_SPI_DISABLE   0
#define MRST_SPI_ENABLE    1

#ifdef CONFIG_ARCH_ARM_SIRIUS
#define BUS_CLK_FRE             (170*1000*1000)
#else
#define BUS_CLK_FRE             (1024*1024*118)
#endif


/* Slave spi_dev related */
struct chip_data {
	u16 cr0;
	u8 cs;          /* chip select pin */
	u8 n_bytes;     /* current is a 1/2/4 byte op */
	u8 tmode;       /* TR/TO/RO/EEPROM */
	u8 type;        /* SPI/SSP/MicroWire */
	u8 poll_mode;       /* 1 means use poll mode */
	u32 rx_threshold;
	u32 tx_threshold;
	u8 bits_per_word;
	u16 clk_div;        /* baud rate divider */
	u32 speed_hz;       /* baud rate */
	int (*write)(struct dw_spi *dws);
	int (*read)(struct dw_spi *dws);
	void (*cs_control)(struct dw_spi *dws, u32 cs, u8 flag);
};

static void spi_master_set_dws(struct spi_master *master, void *data)
{
	master->driver_data = data;
}
static struct dw_spi* spi_master_get_dws(struct spi_master *master)
{
	return master->driver_data;
}

/* ctldata is for the bus_master driver's runtime state */
static inline void *spi_get_ctldata(struct spi_device *spi)
{
	return spi->controller_state;
}

static inline void spi_set_ctldata(struct spi_device *spi, void *state)
{
	spi->controller_state = state;
}

static void wait_till_tf_empty(struct dw_spi *dws)
{
	int i = 10000;
	while (i--) {
		if (dw_readw(dws, SPIM_SR) & SR_TF_EMPT)
			return;
	}
}

static void wait_till_not_busy(struct dw_spi *dws)
{
	int i = 10000;
	while (i--)
	{
		if (!(dw_readw(dws, SPIM_SR) & SR_BUSY))
			return;
	}
	md_debug("DW SPI: Status keeps busy for 1000us after a read/write!\n");
}

static void flush(struct dw_spi *dws)
{
	while ((dw_readw(dws, SPIM_SR) & SR_RF_NOT_EMPT))
		dw_readw(dws, SPIM_RXDR);

	wait_till_not_busy(dws);
}

extern unsigned int gx_chip_id_probe(void);
//cs is gpio 64
static void spi_cs_init(void)
{
#if defined(CONFIG_ARCH_CKMMU_GX3211)
	/* multi pin config for dw spi */
	*(volatile unsigned int *)0xa030a14c |= 0xff;
	*(volatile unsigned int *)0xa0307000 |= (1<<0); // set gpio port64 output
	*(volatile unsigned int *)0xa0307008 |= (1<<0); // set gpio port64
#elif defined(CONFIG_ARCH_ARM_SIRIUS)
	// spi pin mux
	*(volatile unsigned int*)REG_PINMUX_PORTB |= 0x000001C0;
	*(volatile unsigned int*)REG_PINMUX_PORTC &= ~(1 << 23);
	*(volatile unsigned int*)REG_PINMUX_PORTD |= 0x0000001E;
	// spi cs init
	*(volatile unsigned int *)(REG_BASE_GPIO3 + 0x04) = (1 << 31);
#endif
}

static void spi_cs_control(struct dw_spi *dws, u32 cs, u8 flag)
{
	switch(flag) {

	case MRST_SPI_DISABLE:
#if defined(CONFIG_ARCH_CKMMU_GX3211)
		*(volatile unsigned int *)0xa0307008 |= (1<<0); //set gpio port64
#elif defined(CONFIG_ARCH_ARM_SIRIUS)
		*(volatile unsigned int *)(0x82406000 + 0x14) = (1<<31);
#endif
		break;
	case MRST_SPI_ENABLE:
#if defined(CONFIG_ARCH_CKMMU_GX3211)
		*(volatile unsigned int *)0xa030700c |= (1<<0); //clear gpio port64
#elif defined(CONFIG_ARCH_ARM_SIRIUS)
		*(volatile unsigned int *)(0x82406000 + 0x18) = (1<<31);
#endif
		break;
	default:
		return;
	}
}

static int null_writer(struct dw_spi *dws)
{
	u8 n_bytes = dws->n_bytes;

	if (!(dw_readw(dws, SPIM_SR) & SR_TF_NOT_FULL)
			|| (dws->tx == dws->tx_end))
		return 0;
	dw_writew(dws, SPIM_TXDR, 0);
	dws->tx += n_bytes;
	wait_till_not_busy(dws);

	return 1;
}

static int null_reader(struct dw_spi *dws)
{
	u8 n_bytes = dws->n_bytes;
	while (((dw_readw(dws, SPIM_SR) & SR_RF_NOT_EMPT))
			&& (dws->rx < dws->rx_end)) {
		dw_readw(dws, SPIM_RXDR);
		dws->rx += n_bytes;
	}
	wait_till_not_busy(dws);
	return dws->rx == dws->rx_end;
}

static int u8_writer(struct dw_spi *dws)
{
	if (!(dw_readw(dws, SPIM_SR) & SR_TF_NOT_FULL)
			|| (dws->tx == dws->tx_end))
		return 0;
	dw_writew(dws, SPIM_TXDR, *(u8*)(dws->tx));
	++(dws->tx);
	//wait_till_not_busy(dws);
	return 1;
}

static int u8_reader(struct dw_spi *dws)
{
	while ((dw_readw(dws, SPIM_SR) & SR_RF_NOT_EMPT)
			&& (dws->rx < dws->rx_end)) {
		*(u8 *)(dws->rx) = dw_readw(dws, SPIM_RXDR) & 0xFFU;
#if defined(PRINT_TRANS_DATA)
		DBG("rx: 0x%02x\n", *(u8 *)(dws->rx));
#endif
		++dws->rx;
	}

	wait_till_not_busy(dws);
	return dws->rx == dws->rx_end;
}


static void *next_transfer(struct dw_spi *dws)
{
	struct spi_message *msg = dws->cur_msg;
	struct spi_transfer *trans = dws->cur_transfer;
	/* Move to next transfer */
	if (trans->transfer_list.next != &msg->transfers) {
		dws->cur_transfer = list_entry(trans->transfer_list.next,
				struct spi_transfer, transfer_list);
		return RUNNING_STATE;
	} else
		return DONE_STATE;
}


static void spi_chip_sel(struct dw_spi *dws, u16 cs)
{
	if(cs >= dws->master->num_chipselect)
		return;

	if (dws->cs_control)
	{
		dws->cs_control(dws, cs, MRST_SPI_ENABLE);
	}
	dw_writel(dws, SPIM_SER, 1 << cs);
}

static void do_read(struct dw_spi *dws)
{
	int count = 0;
	u32 rxint_level = dws->rx_fifo_len-1;
	u32 left;
	u8 *tmp_rx=dws->rx;
	u32 time_out=dws->len+1;
	while(dws->rx_end > dws->rx)
	{
		left = ((dws->rx_end - dws->rx) / dws->n_bytes) - 1;
		left = (left > rxint_level) ? rxint_level : left;
		spi_enable_chip(dws, 0);
		dw_writew(dws, SPIM_CTRLR1, left);
		spi_enable_chip(dws, 1);
		dw_writew(dws, SPIM_TXDR, 0);
		tmp_rx+=left*dws->n_bytes+1;
		while (tmp_rx > dws->rx)
		{
			dws->read(dws);
			if (count++ == time_out)
			{
				return;
			}
		}
	}
}
static void do_write(struct dw_spi *dws)
{
	while (dws->tx<dws->tx_end) {
		dws->write(dws);
	}
}

/* Caller already set message->status; dma and pio irqs are blocked */
static void msg_giveback(struct dw_spi *dws)
{
	struct spi_transfer *last_transfer;
	struct spi_message *msg;

	msg = dws->cur_msg;
	dws->cur_msg = NULL;
	dws->cur_transfer = NULL;
	dws->prev_chip = dws->cur_chip;
	dws->cur_chip = NULL;
	dws->busy = 0;
	last_transfer = list_entry(msg->transfers.prev,
			struct spi_transfer, transfer_list);

	if (!last_transfer->cs_change && dws->cs_control)
		dws->cs_control(dws,msg->spi->chip_select,MRST_SPI_DISABLE);

	msg->state = NULL;
}


/* Must be called inside pump_transfers() */
static int do_half_transfer(struct dw_spi *dws)
{
	if (dws->rx)
	{
		if (dws->tx)
		{
			do_write(dws);
		}
		wait_till_tf_empty(dws);
		wait_till_not_busy(dws);
		do_read(dws);
	}
	else
	{
		do_write(dws);
		wait_till_tf_empty(dws);
		wait_till_not_busy(dws);
	}
	dws->cur_msg->actual_length += dws->len;
	/* Move to next transfer */
	dws->cur_msg->state = next_transfer(dws);
	if (dws->cur_msg->state == DONE_STATE)
	{
		dws->cur_msg->status = 0;
		//msg_giveback(dws);
		return 0;
	}
	else
	{
		return -1;
	}
}


/* Must be called inside pump_transfers() */
static int do_full_transfer(struct dw_spi *dws)
{
	if ((dws->read(dws)))
	{
		goto comple;
	}
	while (dws->tx<dws->tx_end)
	{
		dws->write(dws);
		dws->read(dws);
	}
	if (dws->rx < dws->rx_end)
	{
		dws->read(dws);
	}

comple:
	dws->cur_msg->actual_length += dws->len;
	/* Move to next transfer */
	dws->cur_msg->state = next_transfer(dws);
	if (dws->cur_msg->state == DONE_STATE)
	{
		dws->cur_msg->status = 0;
		//msg_giveback(dws);
		return 0;
	}
	else
	{
		return -1;
	}
}


static int dw_pump_transfers(struct dw_spi *dws, int mode)
{
	struct spi_message *message = NULL;
	struct spi_transfer *transfer = NULL;
	struct spi_transfer *previous = NULL;
	struct spi_device *spi = NULL;
	struct chip_data *chip = NULL;
	u8 bits = 0;
	u8 spi_dfs = 0;
	u8 cs_change = 0;
	u16 clk_div = 0;
	u32 speed = 0;
	u32 cr0 = 0;

	/* Get current state information */
	message = dws->cur_msg;
	transfer = dws->cur_transfer;
	chip = dws->cur_chip;
	spi = message->spi;


	if (unlikely(!chip->clk_div)) {
		chip->clk_div = BUS_CLK_FRE / chip->speed_hz;
		chip->clk_div = (chip->clk_div + 1) & 0xfffe;
	}
	if (message->state == ERROR_STATE)
	{
		message->status = -EIO;
		goto early_exit;
	}

	/* Handle end of message */
	if (message->state == DONE_STATE)
	{
		message->status = 0;
		goto early_exit;
	}

	/* Delay if requested at end of transfer*/
	if (message->state == RUNNING_STATE)
	{
		previous = list_entry(transfer->transfer_list.prev,
				struct spi_transfer, transfer_list);
		if (previous->delay_usecs)
			udelay(previous->delay_usecs);
	}
	dws->n_bytes = chip->n_bytes;
	dws->cs_control = chip->cs_control;

	dws->tx = (void *)transfer->tx_buf;
	dws->tx_end = dws->tx + transfer->len;
	dws->rx = transfer->rx_buf;
	dws->rx_end = dws->rx + transfer->len;
	dws->write = dws->tx ? chip->write : null_writer;
	dws->read = dws->rx ? chip->read : null_reader;
	if (dws->rx && dws->tx)
	{
		int temp_len = transfer->len;
		int len;
		unsigned char *tx_buf;
		for (len=0; *tx_buf++ != 0; len++);
		dws->tx_end = dws->tx + len;
		dws->rx_end = dws->rx + temp_len - len;
	}
	dws->cs_change = transfer->cs_change;
	dws->len = dws->cur_transfer->len;
	if (chip != dws->prev_chip)
		cs_change = 1;

	cr0 = chip->cr0;

	/* Handle per transfer options for bpw and speed */
	if (transfer->speed_hz)
	{
		speed = chip->speed_hz;
		if (transfer->speed_hz != speed)
		{
			speed = transfer->speed_hz;
			if (speed > BUS_CLK_FRE)
			{
				message->status = -EIO;
				goto early_exit;
			}
			/* clk_div doesn't support odd number */
			clk_div = BUS_CLK_FRE / speed;
			clk_div = (clk_div + 1) & 0xfffe;
			chip->speed_hz = speed;
			chip->clk_div = clk_div;
		}
	}
	if (transfer->bits_per_word)
	{
		bits = transfer->bits_per_word;
		switch (bits)
		{
			case 8:
				dws->n_bytes = 1;
				dws->read = (dws->read != null_reader) ?  u8_reader : null_reader;
				dws->write = (dws->write != null_writer) ?  u8_writer : null_writer;
				spi_dfs = SPI_DFS_8BIT;
				break;
			default:
				message->status = -EIO;
				goto early_exit;
		}

		cr0 = (spi_dfs << SPI_DFS_OFFSET)
			| (chip->type << SPI_FRF_OFFSET)
			| (spi->mode << SPI_MODE_OFFSET)
			| (chip->tmode << SPI_TMOD_OFFSET);
	}
	message->state = RUNNING_STATE;

	/*
	* Adjust transfer mode if necessary. Requires platform dependent
	* chipselect mechanism.
	*/
	if (dws->cs_control)
	{
		if (dws->rx && dws->tx)
			chip->tmode = SPI_TMOD_TR;
		else if (dws->rx)
			chip->tmode = SPI_TMOD_RO;
		else
			chip->tmode = SPI_TMOD_TO;

		cr0 &= ~(0x3 << SPI_TMOD_OFFSET);
		cr0 |= (chip->tmode << SPI_TMOD_OFFSET);
	}

	/*
	* Reprogram registers only if
	*  1. chip select changes
	*  2. clk_div is changed
	*  3. control value changes
	*/
	spi_enable_chip(dws, 0);
	if (dw_readl(dws, SPIM_CTRLR0) != cr0)
		dw_writel(dws, SPIM_CTRLR0, cr0);

	spi_set_clk(dws, clk_div ? clk_div : chip->clk_div);
	spi_chip_sel(dws, spi->chip_select);
	dw_writew(dws, SPIM_CTRLR1, 0);

	spi_enable_chip(dws, 1);
	if (cs_change)
		dws->prev_chip = chip;

	if (mode)
		return do_full_transfer(dws);
	else
		return do_half_transfer(dws);

early_exit:
	//msg_giveback(dws);
	return 0;
}

static void dw_pump_messages(struct dw_spi *dws, int mode)
{
	if (list_empty(&dws->queue))
	{
		dws->busy = 0;
		return;
	}
	/* Make sure we are not already running a message */
	if (dws->cur_msg)
	{
		return;
	}
	/* Extract head of queue */
	dws->cur_msg = list_entry(dws->queue.next, struct spi_message, queue);
	list_del_init(&dws->cur_msg->queue);

	/* Initial message state*/
	dws->cur_msg->state = START_STATE;
	dws->cur_transfer = list_entry(dws->cur_msg->transfers.next,
			struct spi_transfer, transfer_list);
	dws->cur_chip = spi_get_ctldata(dws->cur_msg->spi);
	dws->prev_chip = NULL;
	/* Mark as busy and launch transfers */
	dws->busy = 1;
	while (dw_pump_transfers(dws, mode)) ;
}

/* spi_device use this to queue in their spi_msg */
static int dw_spi_quick_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct dw_spi *dws = spi_master_get_dws(spi->master);

	msg->actual_length = 0;
	msg->status = -EINPROGRESS;
	msg->state = START_STATE;

	list_add_tail(&msg->queue, &dws->queue);
	dw_pump_messages(dws,0);
	msg_giveback(dws);

	return 0;

}

/* This may be called twice for each spi dev */
static int dw_spi_setup(struct spi_device *spi)
{
	struct chip_data *chip;
	u8 spi_dfs = 0;
	spi->bits_per_word=8;
	if (spi->bits_per_word != 8 && spi->bits_per_word != 16)
		return -EINVAL;

	/* Only alloc on first setup */
	chip = spi_get_ctldata(spi);
	if (!chip)
	{
		chip = malloc(sizeof(struct chip_data));
		if (!chip)
			return -ENOMEM;
		memset(chip, 0, sizeof(struct chip_data));
		chip->cs_control = spi_cs_control;
	}

	if (spi->bits_per_word == 8)
	{
		chip->n_bytes = 1;
		chip->read = u8_reader;
		chip->write = u8_writer;
		spi_dfs = SPI_DFS_8BIT;
	} else {
		/* Never take >16b case for MRST SPIC */
		return -EINVAL;
	}
	chip->bits_per_word = spi->bits_per_word;

	spi->max_speed_hz = 30000000;
	chip->speed_hz = spi->max_speed_hz;

	chip->tmode = 0; /* Tx & Rx */
	/* Default SPI mode is SCPOL = 0, SCPH = 0 */
	spi->mode =0x00;
	chip->cr0 = (spi_dfs << SPI_DFS_OFFSET)
		| (chip->type << SPI_FRF_OFFSET)
		| (spi->mode  << SPI_MODE_OFFSET)
		| (chip->tmode << SPI_TMOD_OFFSET);
	spi_set_ctldata(spi, chip);
	return 0;
}

static void dw_spi_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);
	free(chip);
}


/* Restart the controller, disable all interrupts, clean rx fifo */
static void spi_hw_init(struct dw_spi *dws)
{
	spi_enable_chip(dws, 0);
	spi_mask_intr(dws, 0xff);

	/*
	* Try to detect the FIFO depth if not set by interface driver,
	* the depth could be from 2 to 32 from HW spec
	*/
	if (!dws->tx_fifo_len)
	{
		u32 fifo;
		for (fifo = 2; fifo <= 257; fifo++)
		{
			dw_writew(dws, SPIM_TXFTLR, fifo);
			if (fifo != dw_readw(dws, SPIM_TXFTLR))
				break;
		}

		dws->tx_fifo_len = (fifo == 257) ? 0 : fifo;
		dw_writew(dws, SPIM_TXFTLR, 0);
	}
	if (!dws->rx_fifo_len)
	{
		u32 fifo;
		for (fifo = 2; fifo <= 257; fifo++)
		{
			dw_writew(dws, SPIM_RXFTLR, fifo);
			if (fifo != dw_readw(dws, SPIM_RXFTLR))
				break;
		}

		dws->rx_fifo_len = (fifo == 257) ? 0 : fifo;
		dw_writew(dws, SPIM_TXFTLR, 0);
	}
	spi_cs_init();
	spi_cs_control(dws,0,MRST_SPI_DISABLE);
	spi_enable_chip(dws, 1);
	//flush(dws);
}

struct dw_spi dwspi = {0};
struct spi_master dw_spi_master = {0};

void gx_spi_probe(void)
{
	struct spi_master * master = NULL;
	struct dw_spi * dws = NULL;
	int ret;
	ret = -ENOMEM;

	master = &dw_spi_master;

	spi_master_set_dws(master, &dwspi);
	dws = spi_master_get_dws(master);
	dws->regs = REG_BASE_SPI;//regs addr

	dws->master = master;
	dws->type = SSI_MOTO_SPI;
	dws->prev_chip = NULL;

	master->bus_num = 0x0;
	master->num_chipselect = 4;
	master->cleanup = dw_spi_cleanup;
	master->setup = dw_spi_setup;
	//quick_transfer
	master->transfer = dw_spi_quick_transfer;
	/* Basic HW init */
	spi_hw_init(dws);
	flush(dws);

	INIT_LIST_HEAD(&dws->queue);
	//修改状态, 队列启动
	dws->run = QUEUE_RUNNING;
	dws->cur_msg = NULL;
	dws->cur_transfer = NULL;
	dws->cur_chip = NULL;
	dws->prev_chip = NULL;

	spi_master_set_dws(master, dws);
	ret = spi_register_master(master);
	if (ret)
	{
		md_debug("problem registering spi master\n");
		return ;
	}
	return ;
}

