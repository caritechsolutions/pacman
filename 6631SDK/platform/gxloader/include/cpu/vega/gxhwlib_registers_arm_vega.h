#ifndef __EMHWLIB_REGISTERS_ARM_VEGA_H__
#define __EMHWLIB_REGISTERS_ARM_VEGA_H__

/*************************************** Peripheral base address **********************************************/

//---pin multiplex configure
#define REG_PINMUX_PORTA            (0x8940013C + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTB            (0x89400140 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTC            (0x89400144 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTD            (0x89400148 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTE            (0x8940014C + GX_REG_VIRTUAL_BASE1)

//---gpio
#define REG_BASE_GPIO1           (0x82404000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_GPIO2           (0x82405000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_GPIO3           (0x82406000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_GPIO4           (0x82407000 + GX_REG_VIRTUAL_BASE1)

//---rcc
#define RCC_BASE_ADDR            (0x89800000 + GX_REG_VIRTUAL_BASE1)

//---counter
#define REG_BASE_COUNTER         (0x8200C000 + GX_REG_VIRTUAL_BASE1)

//---chip configure
#define REG_BASE_CHIPCONFIG      (0x89400000 + GX_REG_VIRTUAL_BASE1)

//---DRAM Controllers
#define REG_BASE_DRAM            (0x80000000 + GX_REG_VIRTUAL_BASE1)

//---UART
//------DW UART
#define REG_BASE_UART0           (0x82800000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_UART1           (0x82801000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_UART2           (0x82802000 + GX_REG_VIRTUAL_BASE1)

//---WDT
#define REG_BASE_WDT             (0x8200E000 + GX_REG_VIRTUAL_BASE1)

//---IRR
#define REG_BASE_IRR             (0x82008000 + GX_REG_VIRTUAL_BASE1)

//---SCPU
#define REG_BASE_SCPU            (0x00000000) // modify later

//---I2C
//------GX I2C
#define REG_BASE_I2C1            (0x82000000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C2            (0x82001000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C3            (0x82002000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C4            (0x82003000 + GX_REG_VIRTUAL_BASE1)
//------DW I2C
#define REG_BASE_I2C5            (0x82004000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C6            (0x82005000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C7            (0x82006000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C8            (0x82007000 + GX_REG_VIRTUAL_BASE1)

//---RTC
//------GX RTC
#define REG_BASE_RTC             (0x8200A000 + GX_REG_VIRTUAL_BASE1)

//---Interupt controller
#define REG_INTC_BASE_NO_MMU     (0xa2000000)
#define REG_INTC_BASE            (0xa2000000 + GX_REG_VIRTUAL_BASE1)

//---NAND FLASH register address's define
#define REG_BASE_NANDFLASH       (0x88000000 + GX_REG_VIRTUAL_BASE1)

//---SPI
//------DW SPI
#define REG_BASE_SPI             (0x81000000 + GX_REG_VIRTUAL_BASE1)

//---ETH MAC
#define REG_BASE_ETH_MAC         (0x88100000 + GX_REG_VIRTUAL_BASE1)

//---USB
//------ OHCI0
#define REG_BASE_USB_OHCI0       (0x88200000 + GX_REG_VIRTUAL_BASE1)
//------ OHCI1
#define REG_BASE_USB_OHCI1       (0x88201000 + GX_REG_VIRTUAL_BASE1)
//------ EHCI
#define REG_BASE_USB_EHCI        (0x88204000 + GX_REG_VIRTUAL_BASE1)
//------ USB PHY
#define USB_PHY_BASE             (0x88209000 + GX_REG_VIRTUAL_BASE1)
//------ usb phy port regs offset
#define PORT1_OFFSET             0x0
#define PORT2_OFFSET             0x400
#define USB_PHY_PORT1            (USB_PHY_BASE + PORT1_OFFSET)
#define USB_PHY_PORT2            (USB_PHY_BASE + PORT2_OFFSET)
//------ USB CONFIG
#define USB_CONFIG_BASE          (0x8820a000 + GX_REG_VIRTUAL_BASE1)

//---FIREWALL
#define REG_BASE_FIREWALL        (0x88E00000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_FIREWALL_SRAM   (0x83C00000 + GX_REG_VIRTUAL_BASE1)

//--M2M
#define REG_BASE_M2M             (0x89200000 + GX_REG_VIRTUAL_BASE1)

//--OTP
#define REG_BASE_OTP             (0x89900000 + GX_REG_VIRTUAL_BASE1)

//--GP
#define REG_BASE_GP              (0x8A000000 + GX_REG_VIRTUAL_BASE1)

//--DEMUX
#define REG_BASE_DEMUX           (0x8A200000 + GX_REG_VIRTUAL_BASE1)

//--Pidfilter
#define REG_BASE_PIDFILTER       (0x8AA00000 + GX_REG_VIRTUAL_BASE1)

//--AUDIO_OR
#define REG_BASE_AUDIO_OR        (0x8A500000 + GX_REG_VIRTUAL_BASE1)

//---JPEG
#define JPEG_BASE_ADDR           (0x8A600000 + GX_REG_VIRTUAL_BASE1)

//---GA
#define REG_BASE_GA              (0x8A700000 + GX_REG_VIRTUAL_BASE1)

//---VPU
#define VPU_BASE_ADDR            (0x8A800000 + GX_REG_VIRTUAL_BASE1)
//---VPU_OUT
#define VPU_VOUT_BASE_ADDR       (0x8A808000 + GX_REG_VIRTUAL_BASE1)
//---SVPU
#define SVPU_BASE_ADDR           (0x8A806000 + GX_REG_VIRTUAL_BASE1)
//---SVPU_OUT
#define SVPU_VOUT_BASE_ADDR      (0x8A809000 + GX_REG_VIRTUAL_BASE1)
//---VDAC
#define VDAC_BASE_ADDR           (0x8A80F000 + GX_REG_VIRTUAL_BASE1)

//---HASH
#define REG_BASE_HASH            (0x89600000 + GX_REG_VIRTUAL_BASE1)

//---CRYPTO
#define REG_BASE_CRYPTO          (0x89000000 + GX_REG_VIRTUAL_BASE1)
//---PAGE TABLE
#define HARDWARE_PAGE_TABLE_BASE (0x80800000 + GX_REG_VIRTUAL_BASE1)

//---JLINK CTL
#define REG_ARM_JTAG_CTL         (0x89400380 + GX_REG_VIRTUAL_BASE1)

//---SDRAM
#define REG_BASE_DENALI          (0x88500000 + GX_REG_VIRTUAL_BASE1)

//---SDRAM
#define SDRAM_BASE_ADDR          (0x00000000)

//---PMU
#define REG_BASE_PMU             (0x89700000 + GX_REG_VIRTUAL_BASE1)

//-- PKA
#define REG_BASE_PKA             (0x88600000 + GX_REG_VIRTUAL_BASE1)

//-- PIN CONFIG
#define REG_BASE_PIN_CONFIG      (0x89D00000 + GX_REG_VIRTUAL_BASE1)
/*************************************** other address **********************************************/

//---system related
#define TIMING_CONFIG            (0x0000)
#define DRAM_CONFIG              (0x0004)
#define SDC_STATUS               (0x0010)
#define TASK_LOCK_EN             (0x0014)
#define DDR_PP_EN                (0x002c)
#define PAD_CONFIG               (0x0030)

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

#define DEMUX_TSW0_SEC_BUF_ADDR  (REG_BASE_DEMUX + 0x4828)
#define DEMUX_TSW0_SEC_BUF_SIZE  (REG_BASE_DEMUX + 0x482c)
#define DEMUX_TSW1_SEC_BUF_ADDR  (REG_BASE_DEMUX + 0x4830)
#define DEMUX_TSW1_SEC_BUF_SIZE  (REG_BASE_DEMUX + 0x4834)
#define DEMUX_TSR0_SEC_BUF_ADDR  (REG_BASE_DEMUX + 0x4838)
#define DEMUX_TSR0_SEC_BUF_SIZE  (REG_BASE_DEMUX + 0x483c)
#define DEMUX_TSR1_SEC_BUF_ADDR  (REG_BASE_DEMUX + 0x4840)
#define DEMUX_TSR1_SEC_BUF_SIZE  (REG_BASE_DEMUX + 0x4844)

#define DEMUX_SVPU_SEC_BUF_ADDR  (VPU_BASE_ADDR + 0x278)
#define DEMUX_SVPU_SEC_BUF_SIZE  (VPU_BASE_ADDR + 0x27c)

//---secure related
#define REG_ENABLE_NDS_BOOT(offset) ( ( \
			(*(volatile unsigned int*)(REG_BASE_OTP + 0x8 + (offset / 32 * 4))) \
			>> (offset % 32) ) \
			& 1 )

/* OTP CFG BIT list*/
#define FLASH_DECRYPT_ENABLE_OTP_BIT                 18
#define TEE_FLASH_DECRYPT_ENABLE_OTP_BIT             19
#define DDR_CONTENT_SCRAMBLER_ENABLE_OTP_BIT         32
#define AUX_CODE_DECRYPT_OTP_BIT                     33
#define SECURE_BOOT_ENABLE_OTP_BIT                   34
#define TEE_ENABLE_OTP_BIT                           39
#define TEE_SECURE_BOOT_ENABLE_OTP_BIT               44
#define ACPU_FLASH_ENCRYPTION_EXT_KEY_ENABLE_OTP_BIT 53
#define SECURE_UART_BOOT_ENABLE_OTP_BIT              61
#define SMI_MODE_OTP_BIT                             65
#define NAGRA_MODE_OTP_BIT                           119
#define UART_DEBUG_DISABLE_OTP_BIT                   136

/*  OTP ADDR list */
#define REE_MARKET_ID_OTP_ADDR 0x224
#define REE_VERSION_OTP_ADDR   0x207
#define TEE_MARKET_ID_OTP_ADDR 0x228
#define TEE_VERSION_OTP_ADDR   0x20D

#define DTE_BOOT_CODE_VMA_OTP_ADDR 0x22e
#define TEE_LOADER_VMA_OTP_ADDR 0x232
#define SCS_TOTAL_AREA_SIZE_OTP_ADDR 0x236
#define TEE_LOADER_SIZE_OTP_ADDR 0x236

#define BOOT_MODE_SELECT_OTP_ADDR 0x22c

#define DDR_INFO_OTP_ADDR 0x23c

#endif
