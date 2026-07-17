#ifndef __BOARD_PARAM_H__
#define __BOARD_PARAM_H__

#if defined(CONFIG_ARCH_CKMMU_TAURUS)
#include <cpu/taurus/taurus_param.h>
#elif defined(CONFIG_ARCH_CKMMU_GEMINI)
#include <cpu/gemini/gemini_param.h>
#elif defined(CONFIG_ARCH_ARM_VEGA)
#include <cpu/vega/vega_param.h>
#elif defined(CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#include <cpu/scorpio_taurus/scorpio_taurus_param.h>
#endif

#endif
