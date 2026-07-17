#ifndef _DRVKEYPAD_H
#define _DRVKEYPAD_H

#include "y_gendef.h"

void MDrv_FP_Init(void);
UINT8 MDrv_FP_GetKey(void);
void MDrv_FP_SetDigital(char ch, UINT8 index);
UINT8 MDrv_FD650_Powerkey_Statu(UINT8 powerkey);
int MDrv_FP_ShowTime(void);
int MDrv_FP_ShowOFF(void);
#endif

