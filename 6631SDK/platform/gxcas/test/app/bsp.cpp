#include "gxcore_bsp.h"

/* ------------------------- Main Chip ----------------------------------- */
CHIP_TAURUS
//CHIP_GX6605S
#define CHIP taurus
/* ------------------------- Board support device ------------------------ */
//MOD_WDT
MOD_IRR
MOD_SECURE
MOD_AV("|3")
MOD_MTC
MOD_UART
MOD_SPINORFLASH(CHIP)
MOD_I2C
MOD_OTP
MOD_SCI
MOD_SCPU
MOD_MINIFS
MOD_ROMFS
MOD_CAMFS
MOD_FLASHIO
MOD_RAMFS
MOD_USB
MOD_USBSERIAL
/* ------------------------------ Video & Audio Codec -------------------- */
CODEC_DRA(CHIP)
CODEC_VIDEO(CHIP)
CODEC_MPEGA(taurus)
//CODEC_MPEG212A(gx3211)

/* -------------------------- Frontend ----------------------------------- */

//MOD_113x(1,"|4:2:0x80:41:0:0xC0:&0:1:7:0")
//MOD_1001(1,"|0:2:0x18:78:0:0xc0:&0:1")

/* -------------------------- Language Package --------------------------- */
LANG_US
