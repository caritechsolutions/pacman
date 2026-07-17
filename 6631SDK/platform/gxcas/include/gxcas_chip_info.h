#ifndef __GXCAS_CHIP_INFO_H__
#define __GXCAS_CHIP_INFO_H__

#include "gxtype.h"
#include "devapi/chip_info.h"

#ifndef GXCHIP_IS_CANOPUS
#define GXCHIP_IS_CANOPUS 0
#endif
#ifndef GXCHIP_IS_SCORPIO
#define GXCHIP_IS_SCORPIO 0
#endif

#define CONSTEL_CHIP ((GXCHIP_IS_TAURUS)||(GXCHIP_IS_SIRIUS)||(GXCHIP_IS_GEMINI)||(GXCHIP_IS_CANOPUS)||(GXCHIP_IS_CYGNUS)||(GXCHIP_IS_SCORPIO))

#endif
