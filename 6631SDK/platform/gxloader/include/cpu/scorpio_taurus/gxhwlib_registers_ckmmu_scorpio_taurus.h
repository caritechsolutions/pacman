#ifndef __EMHWLIB_REGISTERS_CSKY_SCORPIO_TAURUS_H__
#define __EMHWLIB_REGISTERS_CSKY_SCORPIO_TAURUS_H__

/*************************************** Peripheral base address **********************************************/

//---pin multiplex configure
#define REG_PINMUX_PORTA         (0x0030a13c + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTB         (0x0030a140 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTC         (0x0030a144 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTD         (0x0030a148 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTE         (0x0030a14c + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTF         (0x0030a150 + GX_REG_VIRTUAL_BASE1)

#define REG_INTERNAL_FUNSEL      (0x0030a420 + GX_REG_VIRTUAL_BASE1)

//---gpio
#define REG_BASE_GPIO1           (0x00305000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_GPIO2           (0x00306000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_GPIO3           (0x00307000 + GX_REG_VIRTUAL_BASE1)

//---rcc
#define RCC_BASE_ADDR            (0x00f80000 + GX_REG_VIRTUAL_BASE1)

//--counter
#define REG_BASE_COUNTER         (0x0020a000 + GX_REG_VIRTUAL_BASE1)

//---chip configure
#define REG_BASE_CHIPCONFIG      (0x0030a000 + GX_REG_VIRTUAL_BASE1)

//---DRAM Controllers
#define REG_BASE_DRAM            (0x01000000 + GX_REG_VIRTUAL_BASE1)

//---UART
//------GX UART
#define REG_BASE_UART0           (0x00400000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_UART1           (0x00401000 + GX_REG_VIRTUAL_BASE1)
//------DW UART
#define REG_BASE_UART2           (0x00402000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_UART3           (0x00403000 + GX_REG_VIRTUAL_BASE1)

//---smartcard
#define REG_BASE_SMARTCARD       (0x00600000 + GX_REG_VIRTUAL_BASE1)

//---WDT
#define REG_BASE_WDT             (0x0020B000 + GX_REG_VIRTUAL_BASE1)

//---IRR
#define REG_BASE_IRR             (0x00204000 + GX_REG_VIRTUAL_BASE1)

//---I2C
//------GX I2C
#define REG_BASE_I2C1            (0x00205000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C2            (0x00203000 + GX_REG_VIRTUAL_BASE1)

//------DW I2C
#define REG_BASE_I2C3            (0x00202000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C4            (0x00213000 + GX_REG_VIRTUAL_BASE1)

//---RTC
#define REG_BASE_RTC             (0x00209000 + GX_REG_VIRTUAL_BASE1)

//---Interupt controller
#define REG_INTC_BASE_NO_MMU     (0x00500000)
#define REG_INTC_BASE            (0x00500000 + GX_REG_VIRTUAL_BASE1)

//---NAND FLASH register address's define
#define REG_BASE_NANDFLASH       (0x01038000 + GX_REG_VIRTUAL_BASE1)


//---SPI
#ifdef CONFIG_ENABLE_GXSPI
//------GX SPI
#define REG_BASE_SPI             (0x00302000 + GX_REG_VIRTUAL_BASE1)
#else
//------DW SPI
#define REG_BASE_SPI             (0x00e00000 + GX_REG_VIRTUAL_BASE1)
#endif

//---ETH MAC
#define REG_BASE_ETH_MAC         (0x00a00000 + GX_REG_VIRTUAL_BASE1)

//---USB
//------ OHCI0
#define REG_BASE_USB_OHCI0       (0x00900000 + GX_REG_VIRTUAL_BASE1)
//------ OHCI1
#define REG_BASE_USB_OHCI1       (0x00901000 + GX_REG_VIRTUAL_BASE1)
//------ EHCI
#define REG_BASE_USB_EHCI        (0x00904000 + GX_REG_VIRTUAL_BASE1)
//------ USB PHY
#define USB_PHY_BASE             (0x00908000 + GX_REG_VIRTUAL_BASE1)
//------ usb phy port regs offset
#define PORT1_OFFSET             0x0
#define PORT2_OFFSET             0x400
#define USB_PHY_PORT1            (USB_PHY_BASE + PORT1_OFFSET)
#define USB_PHY_PORT2            (USB_PHY_BASE + PORT2_OFFSET)
#define USB_CONFIG_BASE          (0x0090a000 + GX_REG_VIRTUAL_BASE1)

//---SCPU
#define REG_BASE_SCPU            (0x00b00000 + GX_REG_VIRTUAL_BASE1)

//--OTP
#define REG_BASE_OTP             (0x00F80000 + GX_REG_VIRTUAL_BASE1)

//---MTC
#define MTC_BASE_ADDR            (0x00FC0000 + GX_REG_VIRTUAL_BASE1)

//--DEMUX
#define REG_BASE_DEMUX           (0x04000000 + GX_REG_VIRTUAL_BASE1)

//--Pidfilter
#define REG_BASE_PIDFILTER       (0x04100000 + GX_REG_VIRTUAL_BASE1)

//--VEDIO DECODER C&M
#define REG_BASE_VDEC            (0x04200000 + GX_REG_VIRTUAL_BASE1)

//---JPEG
#define JPEG_BASE_ADDR           (0x04400000 + GX_REG_VIRTUAL_BASE1)

//--AUDIO DECODER ADDR
#define REG_BASE_ADEC            (0x04600000 + GX_REG_VIRTUAL_BASE1)

//---GA
#define REG_BASE_GA              (0x04700000 + GX_REG_VIRTUAL_BASE1)

//---VPU
#define VPU_BASE_ADDR            (0x04800000 + GX_REG_VIRTUAL_BASE1)
//---VPU_OUT
#define VPU_VOUT_BASE_ADDR       (0x04808000 + GX_REG_VIRTUAL_BASE1)
#define TAURUSVPU_VOUT_BASE_ADDR (0x04804000 + GX_REG_VIRTUAL_BASE1)
//---SVPU
#define SVPU_BASE_ADDR           (0x04806000 + GX_REG_VIRTUAL_BASE1)
//---SVPU_OUT
#define SVPU_VOUT_BASE_ADDR      (0x04809000 + GX_REG_VIRTUAL_BASE1)
#define TAURUSSVPU_VOUT_BASE_ADDR (0x04904000 + GX_REG_VIRTUAL_BASE1)

//---HDMI
#define REG_BASE_HDMI            (0x04F00000 + GX_REG_VIRTUAL_BASE1)

//---VDAC
#define VDAC_BASE_ADDR           (0x00701000 + GX_REG_VIRTUAL_BASE1)

//--PMU
#define REG_BASE_PMU             (0x04D00000 + GX_REG_VIRTUAL_BASE1)

//--SDRAM
#define REG_BASE_DENALI          (0x00c00000 + GX_REG_VIRTUAL_BASE1)

//--SDRAM
#define SDRAM_BASE_ADDR          (0x10000000)

/*************************************** other address **********************************************/

//--VEDIO DECODER C&M offset
#define VDEC_CODE_RUN            (REG_BASE_VDEC + 0x000)
#define VDEC_CODE_DOWN           (REG_BASE_VDEC + 0x004)
#define VDEC_CODE_BUF_SIZE       (REG_BASE_VDEC + 0x058)
#define VDEC_CODE_BUF_ADDR       (REG_BASE_VDEC + 0x100)
#define VDEC_BUSY_FLAG           (REG_BASE_VDEC + 0x160)
#define VDEC_INT_ENABLE          (REG_BASE_VDEC + 0x170)

//--AUDIO DECODER offset
#define ADEC_CODE_BUF_ADDR       (REG_BASE_ADEC + 0x11C)
#define ADEC_CODE_BUF_SIZE       (REG_BASE_ADEC + 0x1E8)

//--SCPU offset
#define SCPU_CODE_DDR_CTR        (REG_BASE_SCPU + 0x50)
#define SCPU_CODE_DDR_SIZE       (REG_BASE_SCPU + 0x54)

//--DEMUX offset
#define DEMUX_ES0_BUF_ADDR       (REG_BASE_DEMUX + 0x3400)
#define DEMUX_ES0_BUF_SIZE       (REG_BASE_DEMUX + 0x3404)
#define DEMUX_ES1_BUF_ADDR       (REG_BASE_DEMUX + 0x3420)
#define DEMUX_ES1_BUF_SIZE       (REG_BASE_DEMUX + 0x3424)
#define DEMUX_ES2_BUF_ADDR       (REG_BASE_DEMUX + 0x3440)
#define DEMUX_ES2_BUF_SIZE       (REG_BASE_DEMUX + 0x3444)
#define DEMUX_ES3_BUF_ADDR       (REG_BASE_DEMUX + 0x3460)
#define DEMUX_ES3_BUF_SIZE       (REG_BASE_DEMUX + 0x3464)
#define DEMUX_TS0_BUF_ADDR       (REG_BASE_DEMUX + 0x37f0)
#define DEMUX_TS0_BUF_SIZE       (REG_BASE_DEMUX + 0x37f4)
#define DEMUX_TS1_BUF_ADDR       (REG_BASE_DEMUX + 0x3810)
#define DEMUX_TS1_BUF_SIZE       (REG_BASE_DEMUX + 0x3814)

//--SPI offset
#define GX_REG_BASE_SPI1_SELECT  (REG_BASE_CHIPCONFIG + 0x70c)
#define GX_REG_BASE_SPI1_CS      (REG_BASE_CHIPCONFIG + 0x708)

//---system related
#define TIMING_CONFIG            (0x0000)
#define DRAM_CONFIG              (0x0004)
#define SDC_STATUS               (0x0010)
#define TASK_LOCK_EN             (0x0014)
#define DDR_PP_EN                (0x002c)
#define PAD_CONFIG               (0x0030)

//---secure related
#define REG_BASE_ADVANCE         (0x00f80000 + GX_REG_VIRTUAL_BASE1)
#define ADVANCE_CONFIG           (REG_BASE_ADVANCE + 0x8c)
#define MASK_ADVANCE_BOOT        (0x1)
#define BIT_ADVANCE_BOOT         (49)
#define BIT_ADVANCE_BOOT_FEATURE (26)
#define FLASH_ENCRYPT_ENABLE     (109)
#define OTP_GET_CFG_DATA(offset, mask)  (( \
        (*(volatile unsigned int*)(ADVANCE_CONFIG + offset / 32 * 4)) \
        >> (offset % 32) ) & (mask))
#define REG_ENABLE_NDS_BOOT      \
        (OTP_GET_CFG_DATA(BIT_ADVANCE_BOOT, MASK_ADVANCE_BOOT) \
        & OTP_GET_CFG_DATA(BIT_ADVANCE_BOOT_FEATURE, MASK_ADVANCE_BOOT))

#define REG_BASE_ADVANCE_NOMMU         (0x00f80000)
#define ADVANCE_CONFIG_NOMMU           (REG_BASE_ADVANCE_NOMMU + 0x8c)
#define OTP_GET_CFG_DATA_NOMMU(offset, mask)  (( \
        (*(volatile unsigned int*)(ADVANCE_CONFIG_NOMMU + offset / 32 * 4)) \
        >> (offset % 32) ) & (mask))

#endif
