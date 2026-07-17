#ifndef __EMHWLIB_REGISTERS_CSKY_GX3201_H__
#define __EMHWLIB_REGISTERS_CSKY_GX3201_H__

/*************************************** Peripheral base address **********************************************/

//---pin multiplex configure
#define REG_PINMUX_PORTA         (0x0030a140 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTB         (0x0030a144 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTC         (0x0030a148 + GX_REG_VIRTUAL_BASE1)
#define REG_PINMUX_PORTD         (0x0030a14c + GX_REG_VIRTUAL_BASE1)

//---gpio
#define REG_BASE_GPIO1           (0x00305000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_GPIO2           (0x00306000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_GPIO3           (0x00307000 + GX_REG_VIRTUAL_BASE1)

//---counter
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

//---WDT
#define REG_BASE_WDT             (0x0020B000 + GX_REG_VIRTUAL_BASE1)

//---IRR
#define REG_BASE_IRR             (0x00204000 + GX_REG_VIRTUAL_BASE1)

//---SCPU
#define REG_BASE_SCPU            (0x0030c000 + GX_REG_VIRTUAL_BASE1)

//---I2C
#define REG_BASE_I2C1            (0x00205000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C2            (0x00206000 + GX_REG_VIRTUAL_BASE1)
#define REG_BASE_I2C3            (0x00203000 + GX_REG_VIRTUAL_BASE1)

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
#define REG_BASE_SPI             (0x00303000 + GX_REG_VIRTUAL_BASE1)
#endif

//---ETH MAC
#define REG_BASE_ETH_MAC         (0x00700000 + GX_REG_VIRTUAL_BASE1)

//---USB
//------ EHCI
#define REG_BASE_USB_EHCI        (0x00900000 + GX_REG_VIRTUAL_BASE1)
//------ OHCI0
#define REG_BASE_USB_OHCI0       (0x00A00000 + GX_REG_VIRTUAL_BASE1)
//------ OHCI1
#define REG_BASE_USB_OHCI1       (0x00B00000 + GX_REG_VIRTUAL_BASE1)

//---DEMUX
#define REG_BASE_DEMUX           (0x04000000 + GX_REG_VIRTUAL_BASE1)

//---JPEG
#define JPEG_BASE_ADDR           (0x04400000 + GX_REG_VIRTUAL_BASE1)
//---VPU
#define VPU_BASE_ADDR            (0x04800000 + GX_REG_VIRTUAL_BASE1)
//---VPU_OUT
#define VPU_VOUT_BASE_ADDR       (0x04804000 + GX_REG_VIRTUAL_BASE1)
//---SVPU
#define SVPU_BASE_ADDR           (0x04900000 + GX_REG_VIRTUAL_BASE1)
//---SVPU_OUT
#define SVPU_VOUT_BASE_ADDR      (0x04904000 + GX_REG_VIRTUAL_BASE1)

//--SDRAM
#define REG_BASE_DENALI          (0x00c00000 + GX_REG_VIRTUAL_BASE1)

//--SDRAM
#define SDRAM_BASE_ADDR          (0x10000000)

/*************************************** other address **********************************************/

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
#define BIT_ADVANCE_BOOT         (16)
#define REG_ENABLE_NDS_BOOT         (((*(volatile unsigned int*)(ADVANCE_CONFIG))>>BIT_ADVANCE_BOOT)&MASK_ADVANCE_BOOT)

#endif
