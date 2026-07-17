#ifdef ECOS_OS
#include "gxcore_bsp.h"
#include "config.h"
/* ------------------------- Main Chip ----------------------------------- */
#ifdef CONFIG_GX3201
CHIP_GX3201
#define CHIP gx3201
#endif

#ifdef CONFIG_GX3211
CHIP_GX3211
#define CHIP gx3211
#endif

#ifdef CONFIG_GEMINI
CHIP_GEMINI
#define CHIP gemini
#endif
#ifdef CONFIG_GX6605S
CHIP_GX6605S
#define CHIP gx6605s
#endif

#ifdef CONFIG_TAURUS
CHIP_TAURUS
#define CHIP taurus
#endif

#ifdef CONFIG_SIRIUS
CHIP_SIRIUS
#define CHIP sirius
#endif

#if defined(DLNADMP_SUPPORT) || defined(DLNASAT2IP_SUPPORT)
#define NETSIZE 557056
#else
#define NETSIZE 294912
#endif

#ifdef CONFIG_GEMINI
#define NET_ALLOC_F 1
#define NET_TX_SIZE 0
#define NET_RX_SIZE 0
#else
#define NET_ALLOC_F 0
#define NET_TX_SIZE 0
#define NET_RX_SIZE 0
#endif

/* ------------------------- Board support device ------------------------ */
MOD_WDT
#ifdef CONFIG_ENABLE_IRR
MOD_IRR
#endif
MOD_AV(NULL)
MOD_UART
MOD_SPINORFLASH(CHIP)
MOD_FLASHIO
MOD_SECURE_MODULE(CHIP, misc, otp)
MOD_SECURE_MODULE(CHIP, misc, firewall)
#if defined(CONFIG_ENABLE_USB_UPGRADE)
MOD_USB
#endif
#if defined(CONFIG_NET)
MOD_USB
MOD_NET(NETSIZE)
#if defined(CONFIG_WIFI_SUPPORT)
#if defined(CONFIG_WIFI_RTL8188FU)
MOD_WIFI_RTL8188FU
#endif
#if defined(CONFIG_WIFI_RT5370)
MOD_WIFI_RT5370
#endif
#if defined(CONFIG_WIFI_MT7601)
MOD_WIFI_MT7601
#endif
MOD_WIFI_COMMON
#endif
#if defined(CONFIG_ETH_SUPPORT)
MOD_ETH(NET_ALLOC_F,NET_TX_SIZE,NET_RX_SIZE)
#endif
#if defined(CONFIG_USB_ETH_SUPPORT)
MOD_USBNET
#endif
#if defined(MOD_3G_SUPPORT)
MOD_USB3G
#endif
#if defined(RNDIS_SUPPORT)
MOD_RNDIS
#endif
#endif
//MOD_HDMI
//MOD_PANEL
MOD_I2C
//MOD_GPIO
#if defined(IPTV_SUPPORT)
MOD_OTP
#endif
#if defined(SQUASHFS_SUPPER)
MOD_SQUASHFS
#endif
/* ------------------------------ Video & Audio Codec -------------------- */
#if 0
#if (defined(GX6613H2NNE))
CODEC_AUDIO(CHIP)
CODEC_VIDEO(CHIP)
#else
#if (defined TAURUS || defined GX6605S || defined GEMINI)
CODEC_MPEGA_MINI(CHIP)
#else
CODEC_MPEGA(CHIP)
#endif
//CODEC_DRA(CHIP)
#ifndef GX6605S
CODEC_AVSV(CHIP)
CODEC_H265(CHIP)
#endif
CODEC_H264(CHIP)
CODEC_MPEG2V(CHIP)
CODEC_MPEG4V(CHIP)
#if FLAC_AND_OGG_SUPPORT
CODEC_FLAC(CHIP)
CODEC_VORBIS(CHIP)
CODEC_OPUS(CHIP)
#endif
//CODEC_DOLBY(CHIP)
//CODEC_OGG
//CODEC_RA_AAC
//CODEC_RA_RA8LBR
//CODEC_RV
#endif
#endif
/* ------------------------- Support filesystem -------------------------- */
#ifdef CONFIG_ENABLE_USB_UPGRADE
MOD_FAT
#endif
//MOD_NTFS
//MOD_JFFS2
//MOD_CAMFS
//MOD_MINIFS
//MOD_ROMFS
MOD_RAMFS



/* -------------------------- Language Package --------------------------- */
//LANG_US
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
