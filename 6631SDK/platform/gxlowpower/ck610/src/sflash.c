#include "gxcomm.h"

#define SFLASH_CONTROL_VALUE 0x8429c043 //((1<<31)|(1<<26)|(1<<21)|(1<<19)|(1<<16)|(0x3<<14)|(0x1<<6)|(0x3))

//spi nor flash status regsiter
#define GX_STAT_WIP           0x01  // write in progress bit

// spi nor flash command
#define GX_CMD_READ           0x03  // read data bytes
#define GX_CMD_PP             0x02  // page program
#define GX_CMD_BE             0xd8  // block erase
#define GX_CMD_WREN           0x06  // write enable
#define GX_CMD_RDSR           0x05  // read status register

// page size of pageprogram command
#define GX_PAGESIZE            256  // GX25L : 256 bytes

//about spi_cr
#define CR_CSEN             (0x1 << 17)
#define CR_SPGO             (0x1 << 12)
//about spi_sr
#define SR_OPE_RDY          (0x1 << 0)

#define CR_ICNT_BIT  (14)

#define SPI_TRANSFER_SIZE (4)

static void gx_spi_write(unsigned int data)
{
	rSPI_TX_DATA=data;
	rSPI_CR |= CR_SPGO;
	while(!((rSPI_SR) & SR_OPE_RDY));
	rSPI_CR &= ~CR_SPGO;
}

static unsigned int gx_spi_read(void)
{
	rSPI_CR |= CR_SPGO;
	while(!((rSPI_SR) & SR_OPE_RDY));
	rSPI_CR &= ~CR_SPGO;
	return rSPI_RX_DATA;
}

static void gx_spi_set_transfer_len(int size)
{
	unsigned int reg_val=rSPI_CR;
	if(size<1)
		return;
	reg_val &=~(0x3<<CR_ICNT_BIT);
	reg_val |=((size-1) <<CR_ICNT_BIT);
	rSPI_CR=reg_val;
}

static inline void spi_csoutput_enable(void)
{
	rSPI_CR |= CR_CSEN;
}

static inline void spi_csoutput_disable(void)
{
	rSPI_CR &= ~CR_CSEN;
}
static int gx_spi_tx(unsigned char *txbuf,int len)
{
	unsigned char *p;
	unsigned int data;

	if((len == 0) || (txbuf == NULL))
	{
		return -1;
	}
	p = txbuf;

	if(len >= SPI_TRANSFER_SIZE)
		gx_spi_set_transfer_len(4);

	while(len >= SPI_TRANSFER_SIZE)
	{
		data = (*p++) << 24;
		data |= (*p++) << 16;
		data |= (*p++) << 8;
		data |= (*p++);
		gx_spi_write(data);
		len -= SPI_TRANSFER_SIZE;
	}

	if(len > 0)
	{
		gx_spi_set_transfer_len(1);
		while(len--)
		{
			data = *p++;
			gx_spi_write(data);
		}
	}
	return 0;
}
static int gx_spi_rx(unsigned char *rxbuf,int len)
{
	unsigned char *p;
	unsigned int data;

	if((len == 0) || (rxbuf == NULL))
	{
		return -1;
	}
	p = rxbuf;
	if(len >= SPI_TRANSFER_SIZE)
		gx_spi_set_transfer_len(4);

	while(len >= SPI_TRANSFER_SIZE)
	{
		data = gx_spi_read();
		*p++ = (data >> 24) & 0xff;
		*p++ = (data >> 16) & 0xff;
		*p++ = (data >>  8) & 0xff;
		*p++ = data & 0xff;
		len -= SPI_TRANSFER_SIZE;
	}

	if(len > 0)
	{
		gx_spi_set_transfer_len(1);
		while(len--){
			data = gx_spi_read();
			*p++ = data & 0xff;
		}
	}
	return 0;
}

void spi_setup(void)
{
	rSPI_CR = SFLASH_CONTROL_VALUE;
}

static int read_sr1(void)
{
	unsigned char code = GX_CMD_RDSR;    
	unsigned char val;              
	spi_csoutput_enable();
	gx_spi_tx(&code,1);
	gx_spi_rx(&val,1);
	spi_csoutput_disable();
	return val;          
}   

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int wait_till_ready(void)
{
	int sr;
	do{
		sr = read_sr1();
	}while (sr & GX_STAT_WIP);
	return 1;
}
static void sflash_addr2cmd(unsigned int addr, unsigned char *cmd)
{
	/* opcode is in cmd[0] */
	cmd[1] = (addr >> 16)&0xff;
	cmd[2] = (addr >> 8)&0xff;
	cmd[3] = (addr >> 0)&0xff;
}

static void write_enable(void)
{
	unsigned char code = GX_CMD_WREN;
	spi_csoutput_enable();
	gx_spi_tx(&code,1);
	spi_csoutput_disable();
}

int flash_read(unsigned int addr, unsigned char *buf,unsigned int len)
{
	unsigned char cmd[4];
	/* sanity checks */
	if (!len)
		return -1;

	spi_setup();

	/* Wait till previous write/erase is done. */
	wait_till_ready();

	/* Set up the write data buffer. */
	cmd[0] = GX_CMD_READ;
    sflash_addr2cmd(addr,cmd);
	spi_csoutput_enable();
	gx_spi_tx(cmd,4);
	gx_spi_rx(buf,len);
	spi_csoutput_disable();

	return 0;
}
int flash_write(unsigned int addr,unsigned char *buf,unsigned int len)
{
	unsigned char cmd[4];
	unsigned int page_offset, size;
	if (!len)
		return -1;
	spi_setup();
	page_offset = addr % GX_PAGESIZE;
	while(len)
	{
		if(len+page_offset>GX_PAGESIZE)
		{
			size=GX_PAGESIZE-page_offset;
		}
		else
		{
			size=len;
		}
		cmd[0]=GX_CMD_PP;
		sflash_addr2cmd(addr,cmd);
		wait_till_ready();
		write_enable();
		spi_csoutput_enable();
		gx_spi_tx(cmd,4);
		gx_spi_tx(buf,size);
		spi_csoutput_disable();
		page_offset=0;
		addr += size;
		buf += size;
		len -= size;
	}
	return 0;
}

void flash_erase(unsigned int addr)
{
	unsigned char cmd[4];
	spi_setup();
	wait_till_ready();
	write_enable(); 

	cmd[0]=GX_CMD_BE;
	sflash_addr2cmd(addr,cmd);
	spi_csoutput_enable();
	gx_spi_tx(cmd,4);
	spi_csoutput_disable();
}

