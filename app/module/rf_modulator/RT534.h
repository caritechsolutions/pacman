#include "app.h"
#include "app_module.h"
#if ((RF_MODULATOR_SUPPORT > 0)&&(RF_RT534 >0))
#ifndef  __RT534_H_
#define __RT534_H_

#define V_GPIO_RF_ON_OFF		30   // just for zxt
#define V_GPIO_RF_CH3_CH4		31   //just for zxt
void RT534_RF_ON_OFF(int  tmp);
void RT534_RF_CH3_CH4(int tmp);

#endif
#endif
