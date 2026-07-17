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
MOD_SPINORFLASH(gx3211)
MOD_FLASHIO
//MOD_NANDFLASH
//MOD_NET
MOD_USB
#ifdef NETWIFI
MOD_WIFI
MOD_NET
#endif
#ifdef NETETH
MOD_ETH
MOD_NET
#endif
#ifdef NETBOTH
MOD_WIFI
MOD_ETH
MOD_NET
#endif
//MOD_HDMI
//MOD_PANEL
MOD_I2C
//MOD_GPIO

/* ------------------------------ Video & Audio Codec -------------------- */
CHIP_GX3211 //CHIP_GEMINI
CODEC_DRA(gx3211)
CODEC_MPEGA(gx3211)
CODEC_H264(gx3211)
CODEC_AVSV(gx3211)
CODEC_H265(gx3211)
CODEC_MPEG2V(gx3211)
CODEC_MPEG4V(gx3211)

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
