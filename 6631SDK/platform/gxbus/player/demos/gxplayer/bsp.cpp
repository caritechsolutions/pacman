#ifdef ECOS_OS
#include "gxcore_bsp.h"

/* ------------------------- Board support device ------------------------ */
MOD_WDT
MOD_IRR
//MOD_SCI
//MOD_MMC
MOD_AV(NULL)
MOD_UART
//MOD_NORFLASH
MOD_SPINORFLASH(vega)
MOD_FLASHIO
//MOD_NANDFLASH
//MOD_NET
MOD_USB
MOD_USBNET
MOD_NET(2097152)
//MOD_WIFI
MOD_ETH(1, 0, 0)
//MOD_HDMI
//MOD_PANEL
MOD_I2C
//MOD_GPIO
HDMI_HDCP(vega)
CHIP_VEGA

/* ------------------------------ Video & Audio Codec -------------------- */
/* ======================================================================= */
//CODEC_DRA(gemini)
//CODEC_MPEGA(gemini)
//CODEC_H264(gemini)
//CODEC_AVSV(gemini)
//CODEC_H265(gemini)
//CODEC_MPEG2V(gemini)
//CODEC_MPEG4V(gemini)
/* ====================================================================== */
CODEC_DRA(vega)
CODEC_MPEGA(vega)
CODEC_H264(vega)
CODEC_AVSV(vega)
CODEC_H265(vega)
CODEC_MPEG2V(vega)
CODEC_MPEG4V(vega)

/* ------------------------- Support filesystem -------------------------- */
MOD_FAT
MOD_NTFS
//MOD_JFFS2
MOD_CAMFS
MOD_MINIFS
MOD_ROMFS
MOD_RAMFS

/* -------------------------- Frontend ----------------------------------- */
#ifdef LIB_1_0_2
#if (DEMOD_TYPE == 1)
MOD_GX1101
#else
MOD_GX1131
#endif
//MOD_GX1201
//MOD_GX1501
#endif

/* -------------------------- Language Package --------------------------- */
LANG_US
//LANG_SIMP_CHINESE
//LANG_TRAD_CHINESE
//LANG_KOREAN
//LANG_JAPANESE
//LANG_ARABIC_O
//LANG_ARABIC_W
//LANG_GREEK_O
//LANG_GREEK_W
//LANG_CENT_EUR
//LANG_BALTIC_O
//LANG_BALTIC_W
//LANG_MULTI_LANTIN1
//LANG_LATIN2_O
//LANG_LATIN1_W
//LANG_CYRILLIC_O
//LANG_CYRILLIC_W
//LANG_RUSSIAN_O
//LANG_TURKISH_O
//LANG_TURKISH_W
//LANG_MULTI_LATIN1_EUR
//LANG_HEBREW_O
//LANG_HEBREW_W
//LANG_THAI
//LANG_VIETNAM
#endif
