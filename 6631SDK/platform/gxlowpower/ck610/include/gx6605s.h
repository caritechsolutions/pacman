/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou NationalChip Science and Technology Co., Ltd.
*                      (C)2006-2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :   gx6605s.h
* Author    :   liuyx
* Project   :   Low-power mode
* Type      :   csky
******************************************************************************
* Purpose   :   the head file of GX6605S in low-power mode
*****************************************************************************/
#ifndef __GX6605S_H__
#define __GX6605S_H__

#ifdef __cplusplus
extern "C" {
#endif
/* Includes --------------------------------------------------------------- */
#include "gxchips.h"

/* Private types/constants ------------------------------------------------ */

/* Private Macros --------------------------------------------------------- */

/* Private Variables ------------------------------------------------------ */

/* Export Variables ------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

#define GXCHIP_TYPE                       GXCHIP_TYPE_GX6605S
#define XTAL_CLOCK_FREQ                   27000000
#define DEBUG_EN                          1
#define IRR_SYSTEM_CODE                   0x11
#define IRR_DATA_CODE                     0x22

#ifndef __INCLUDE_BY_ASM
#include "gxdefs.h"

/* Define some base address of the registers in GX6605S */
#define UART_BASE_ADDR              (0x00400000)	//have been confirmed
#define CTR_BASE_ADDR               (0x0020A000)	//have been confirmed
#define INTC_BASE_ADDR              (0x00800000)
#define IRR_BASE_ADDR               (0x00204000)	//have been confirmed
#define SPI_BASE_ADDR               (0x00302000)	//have been confirmed
#define EPORT_BASE_ADDR             (0x00305000)	//have been confirmed
#define EPORT2_BASE_ADDR            (0x00306000)	//have been confirmed
#define EPORT3_BASE_ADDR            (0x00307000)	//have been confirmed
#define WDT_BASE_ADDR               (0x0020B000)	//have been confirmed
#define CONFIG_BASE_ADDR            (0x0030A000)	//have been confirmed
#define SDC_BASE_ADDR               (0x01000000)
#define VIDEO_OUT_ADDR              (0x01104000)


#define rUART_SCICR                 (*(volatile U32 *)(UART_BASE_ADDR))
#define rUART_SCISR                 (*(volatile U32 *)(UART_BASE_ADDR + 0x04))
#define rUART_SCIDR                 (*(volatile U32 *)(UART_BASE_ADDR + 0x08))
#define rUART_SCIBR                 (*(volatile U32 *)(UART_BASE_ADDR + 0x0C))

#define rCOUNTER1_STATUS            (CTR_BASE_ADDR)
#define rCOUNTER1_CTRL              (CTR_BASE_ADDR + 0x10)
#define rCOUNTER1_CFIG              (CTR_BASE_ADDR + 0x20)
#define rCOUNTER1_DIV               (CTR_BASE_ADDR + 0x24)
#define rCOUNTER1_INI               (CTR_BASE_ADDR + 0x28)

#define rINTC_NENCLR                (*(volatile U32 *)(INTC_BASE_ADDR + 0x30))
#define rINTC_NENCLR2               (*(volatile U32 *)(INTC_BASE_ADDR + 0x34))
#define rINTC_FENCLR                (*(volatile U32 *)(INTC_BASE_ADDR + 0x130))
#define rINTC_FENCLR2               (*(volatile U32 *)(INTC_BASE_ADDR + 0x134))

#define rIRR_CNTL                   (*(volatile U32 *)(IRR_BASE_ADDR))
#define rIRR_INT                    (*(volatile U32 *)(IRR_BASE_ADDR + 0x04))
#define rIRR_FIFO                   (*(volatile U32 *)(IRR_BASE_ADDR + 0x08))
#define rIRR_DIV                    (*(volatile U32 *)(IRR_BASE_ADDR + 0x0C))

#define rSPI_CR                     (*(volatile U32 *)(SPI_BASE_ADDR))
#define rSPI_SR                     (*(volatile U32 *)(SPI_BASE_ADDR + 0x04))
#define rSPI_TX_DATA                (*(volatile U32 *)(SPI_BASE_ADDR + 0x08))
#define rSPI_RX_DATA                (*(volatile U32 *)(SPI_BASE_ADDR + 0x0C))

#define rEPDDR                      (*(volatile U32 *)(EPORT_BASE_ADDR))
#define rEPBSET                     (*(volatile U32 *)(EPORT_BASE_ADDR + 0x08))
#define rEPBCLR                     (*(volatile U32 *)(EPORT_BASE_ADDR + 0x0C))

#define rWDT_CTRL                   (*(volatile U32 *)(WDT_BASE_ADDR))
#define rWDT_MATCH                  (*(volatile U32 *)(WDT_BASE_ADDR + 0x04))
#define rWDT_COUMT                  (*(volatile U32 *)(WDT_BASE_ADDR + 0x08))
#define rWDT_WSR                    (*(volatile U32 *)(WDT_BASE_ADDR + 0x0C))

#define rDLL_CONFIG                 (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x150))
#define rCLK_CONFIG                 (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x18))
#define rDIV_CONFIG                 (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x24))
#define rDTO5_CONFIG                (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x38))
#define rDTO6_CONFIG                (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x3C))
#define rPLL1_CONFIG                (*(volatile U32 *)(CONFIG_BASE_ADDR + 0xC0))
#define rPLL2_CONFIG                (*(volatile U32 *)(CONFIG_BASE_ADDR + 0xC4))
#define rPLL3_CONFIG                (*(volatile U32 *)(CONFIG_BASE_ADDR + 0xC8))
#define rPLL4_CONFIG                (*(volatile U32 *)(CONFIG_BASE_ADDR + 0xCC))
#define rPLL5_CONFIG                (*(volatile U32 *)(CONFIG_BASE_ADDR + 0xD0))
#define rMULPIN0_CONFIG             (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x13c))
#define rMULPIN1_CONFIG             (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x140))
#define rMULPIN2_CONFIG             (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x144))
#define rMULPIN3_CONFIG             (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x148))
#define rMULPIN4_CONFIG             (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x14c))
#define SOURCE_SEL                  (*(volatile U32 *)(CONFIG_BASE_ADDR + 0x170))

#define rSDC_IO_MUX                 (*(volatile U32 *)(SDC_BASE_ADDR + 0x24))
#define rSDC_PAD_CON                (*(volatile U32 *)(SDC_BASE_ADDR + 0x30))

#define rVIDEOOUT_CTL               (*(volatile U32 *)(VIDEO_OUT_ADDR))

#endif

#ifdef __cplusplus
}
#endif

#endif

