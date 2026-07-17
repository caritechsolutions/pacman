#ifndef __OTP_H__
#define __OTP_H__

#include "io.h"
#include "gx_otp.h"

#if defined (CONFIG_ARCH_CKMMU_TAURUS) || defined (CONFIG_ARCH_CKMMU_GEMINI) || defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_CKMMU_SCORPIO) || defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#define IS_TAURUS_SERIES
#endif

#if defined (CONFIG_ARCH_ARM_SIRIUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)
#define IS_SIRIUS_SERIES
#endif

#ifdef CONFIG_ARCH_CKMMU_GX6605S
#define GX_OTP_CON_REG         ((unsigned int)(REG_BASE_OTP + 0x00))  // OTP control register
#define GX_OTP_CFG_REG         ((unsigned int)(REG_BASE_OTP + 0x04))  // OTP config register
#define GX_OTP_STA_REG         ((unsigned int)(REG_BASE_OTP + 0x08))  // OTP state register
#endif

#ifdef CONFIG_ARCH_CKMMU_GX3113C
#define GX_OTP_CON_REG         (*(volatile unsigned int*)(REG_BASE_OTP + 0x80))  // OTP control register
#define GX_OTP_CFG_REG         (*(volatile unsigned int*)(REG_BASE_OTP + 0x84))  // OTP config register
#define GX_OTP_STA_REG         (*(volatile unsigned int*)(REG_BASE_OTP + 0x88))  // OTP state register
#endif

#if defined (IS_TAURUS_SERIES) || defined (CONFIG_ARCH_CKMMU_GX3211)
#define BIT_OTP_FLAG_DDR_SCRAMBLE      (59)
#define BIT_OTP_FLAG_DDR_CONTENT_LIMIT (60)
#define BIT_OTP_FLAG_TS_BUF_PROTECT    (62)
#define BIT_OTP_FLAG_ES_BUF_PROTECT    (63)
#define BIT_OTP_FLAG_FW_BUF_PROTECT    (99)
#define BIT_OTP_FLAG_SCPU_BUF_PROTECT  (100)

#define GX_OTP_CON_REG         ((unsigned int)(REG_BASE_OTP + 0x80))  // OTP control register
#define GX_OTP_CFG_REG         ((unsigned int)(REG_BASE_OTP + 0x84))  // OTP config register
#define GX_OTP_STA_REG         ((unsigned int)(REG_BASE_OTP + 0x88))  // OTP state register
#define GX_OTP_FLAG_REG(id)    ((unsigned int)(REG_BASE_OTP + 0x8c + (id)*4))
#endif

#ifdef IS_SIRIUS_SERIES
  #ifdef CONFIG_ARCH_ARM_VEGA
	#define BIT_OTP_FLAG_DDR_SCRAMBLE      (32)
	#define BIT_OTP_FLAG_DDR_CONTENT_LIMIT (47)
	#define BIT_OTP_FLAG_TS_BUF_PROTECT    (48)
	#define BIT_OTP_FLAG_ES_BUF_PROTECT    (49)
	#define BIT_OTP_FLAG_FW_BUF_PROTECT    (52)
	#define BIT_OTP_FLAG_VOUT_BUF_PROTECT  (50)
	#define BIT_OTP_FLAG_AOUT_BUF_PROTECT  (51)
  #else
	#define BIT_OTP_FLAG_DDR_SCRAMBLE      (42)
	#define BIT_OTP_FLAG_DDR_CONTENT_LIMIT (43)
	#define BIT_OTP_FLAG_TS_BUF_PROTECT    (28)
	#define BIT_OTP_FLAG_ES_BUF_PROTECT    (30)
	#define BIT_OTP_FLAG_GP_BUF_PROTECT    (31)
	#define BIT_OTP_FLAG_FW_BUF_PROTECT    (32)
	#define BIT_OTP_FLAG_VOUT_BUF_PROTECT  (33)
	#define BIT_OTP_FLAG_AOUT_BUF_PROTECT  (34)
  #endif
#define GX_OTP_FLAG_REG(id)	   ((unsigned int)(REG_BASE_OTP + 0x08 + (id)*4))
#endif

#define OTP_FLAG_IS_TRUE(bit) (__raw_readl(GX_OTP_FLAG_REG((bit/32))) & (0x1<<(bit%32)))

#endif
