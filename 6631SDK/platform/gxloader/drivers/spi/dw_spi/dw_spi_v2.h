#ifndef __DW_SPI_V2_H__
#define __DW_SPI_V2_H__

/* SPI register offsets */
#define SPIM_CTRLR0              0x00
#define SPIM_CTRLR1              0x04
#define SPIM_ENR                 0x08
#define SPIM_MWCR                0x0c
#define SPIM_SER                 0x10
#define SPIM_BAUDR               0x14
#define SPIM_TXFTLR              0x18
#define SPIM_RXFTLR              0x1c
#define SPIM_TXFLR               0x20
#define SPIM_RXFLR               0x24
#define SPIM_SR                  0x28
#define SPIM_IMR                 0x2c
#define SPIM_ISR                 0x30
#define SPIM_RISR                0x34
#define DW_SPI_TXOICR            0x38
#define DW_SPI_RXOICR            0x3c
#define DW_SPI_RXUICR            0x40
#define SPIM_ICR                 0x48
#define SPIM_DMACR               0x4c
#define SPIM_DMATDLR             0x50
#define SPIM_AXIAWLEN            0x50
#define SPIM_DMARDLR             0x54
#define SPIM_AXIARLEN            0x54
#define SPIM_IDR                 0x58
#define SPIM_VERSION_ID          0x5c
#define SPIM_TXDR                0x60
#define SPIM_RXDR                0x60
#define SPIM_SAMPLE_DLY          0xF0
#define SPIM_SPI_CTRLR0          0xf4
#define SPIM_AXIAR0              0x128
#define SPIM_DONECR              0x134

#define SPIM_SPEED_LEVEL         40000000

/* --------Bit fields in CTRLR0--------begin */

#define SPI_DFS_OFFSET           0
#define SPI_FRF_OFFSET           6    /* Frame Format */
#define SPI_MODE_OFFSET          8     /* SCPH & SCOL */
#define SPI_TMOD_OFFSET          10
#define SPI_SSTE                 (1 << 14)
#define SPI_MST                  (1 << 31)
#define SPI_SLVOE_OFFSET         12

#define SPI_DFS_8BIT             0x07
#define SPI_DFS_16BIT            0x0F
#define SPI_DFS_32BIT            0x1F

#define SPI_FRF_SPI              0x0  /* motorola spi */
#define SPI_FRF_SSP              0x1  /* Texas Instruments SSP*/
#define SPI_FRF_MICROWIRE        0x2  /*  National Semiconductors Microwire */
#define SPI_FRF_RESV             0x3

#define SPI_MODE_MASK            (0x3 << SPI_MODE_OFFSET)     /* SCPH & SCOL */
#define SPI_SCPH_OFFSET          8     /* Serial Clock Phase */
#define SPI_SCPH_TOGMID          0     /* Serial clock toggles in middle of first data bit */
#define SPI_SCPH_TOGSTA          1     /* Serial clock toggles at start of first data bit */
#define SPI_SCOL_OFFSET          9     /* Serial Clock Polarity */

#define SPI_TMOD_MASK            (0x3 << SPI_TMOD_OFFSET)
#define SPI_TMOD_TR              0x0   /* xmit & recv */
#define SPI_TMOD_TO              0x1   /* xmit only */
#define SPI_TMOD_RO              0x2   /* recv only */
#define SPI_TMOD_EPROMREAD       0x3   /* eeprom read mode */

#define SPI_FRM_OFFSET           22
#define SPI_FRM_STD              0x0
#define SPI_FRM_DUAL             0x1
#define SPI_FRM_QUAD             0x2
#define SPI_FRM_OCTAL            0x3

#define SPI_SLV_OE               (1 << SPI_SLVOE_OFFSET)

#define SPI_SCKDV_OFFSET         0

/* --------Bit fields in CTRLR0--------end */
/* Bit fields in SR, 7 bits */
#define SR_MASK                 0x7f            /* cover 7 bits */
#define SR_BUSY                 (1 << 0)
#define SR_TF_NOT_FULL          (1 << 1)
#define SR_TF_EMPT              (1 << 2)
#define SR_RF_NOT_EMPT          (1 << 3)
#define SR_RF_FULL              (1 << 4)
#define SR_TX_ERR               (1 << 5)
#define SR_DCOL                 (1 << 6)
/* Bit fields in ISR, IMR, RISR, 7 bits */
#define SPI_INT_TXEI            (1 << 0)
#define SPI_INT_TXOI            (1 << 1)
#define SPI_INT_RXUI            (1 << 2)
#define SPI_INT_RXOI            (1 << 3)
#define SPI_INT_RXFI            (1 << 4)
#define SPI_INT_MSTI            (1 << 5)
#define SPI_INT_XRXOI           (1 << 6)
#define SPI_INT_TXUIM           (1 << 7)
#define SPI_INT_AXIES           (1 << 8)
#define SPI_INT_AXIEM           (1 << 8)
#define SPI_INT_SPITES          (1 << 10)
#define SPI_INT_DONES           (1 << 11)
#define SPI_INT_DONEM           (1 << 11)

#define SPI_INT_OVERFLOW        (SPI_INT_TXOI | SPI_INT_RXUI | SPI_INT_RXOI | SPI_INT_XRXOI | \
					SPI_INT_TXUIM | SPI_INT_AXIEM | SPI_INT_SPITES)

/* Bit fields in DMACR */
#define SPI_DMACR_TX_ENABLE     (1 << 1)
#define SPI_DMACR_RX_ENABLE     (1 << 0)

/* Bit fields in ICR */
#define SPI_CLEAR_INT_ALL       (1<< 0)

struct dw_spi {
	struct spi_master       *master;
	uint32_t                 regs;
	uint32_t                 cs_regs;
	uint32_t                 tx_fifo_len;       /* depth of the FIFO buffer */
	uint32_t                 rx_fifo_len;       /* depth of the FIFO buffer */
	struct spi_message       *cur_msg;
	struct spi_transfer      *cur_transfer;
	void                     *buffer;
	void                     *cur_buffer;
	uint32_t                 len;
	uint32_t                 complete;          /* current transfer byte number */
	uint32_t                 complete_minor;     /* current transfer byte number */
	uint8_t                  n_bytes;           /* current is a 1/2/4 bytes op */
	uint32_t                 hw_version;
};

#define dw_readl(dw, off)          __raw_readl(dw->regs + off)
#define dw_writel(dw, off, val)    __raw_writel(val, dw->regs + off)

static inline void spi_enable_chip(struct dw_spi *dws, int enable)
{
	dw_writel(dws, SPIM_ENR, (enable ? 1 : 0));
}

static inline void spi_set_clk(struct dw_spi *dws, u16 div)
{
	dw_writel(dws, SPIM_BAUDR, div < 2 ? 2 : div);   // div 不能小于 2, 否则spi不工作
}

/* Disable IRQ bits */
static inline void spi_mask_intr(struct dw_spi *dws, uint32_t mask)
{
	uint32_t new_mask;

	new_mask = dw_readl(dws, SPIM_IMR) & ~mask;
	dw_writel(dws, SPIM_IMR, new_mask);
}

/* Enable IRQ bits */
static inline void spi_umask_intr(struct dw_spi *dws, uint32_t mask)
{
	uint32_t new_mask;

	new_mask = dw_readl(dws, SPIM_IMR) | mask;
	dw_writel(dws, SPIM_IMR, new_mask);
}

#endif /* __DW_SPI_V2_H__ */
