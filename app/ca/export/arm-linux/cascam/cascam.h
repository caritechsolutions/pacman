#ifndef __IRDETO_CCA_API_H__
#define __IRDETO_CCA_API_H__
#include "UniversalClient_SPI.h"
#include "gxcas.h"

typedef struct{
	uint8_t byMessageType;
	uint8_t res;
}GxCas_IrdetoShowPromptMessage;

typedef struct{
	uc_device_platform_identifiers ManuInfo;
}GxCas_IrdetoManuInfo;

#define GXCAS_IRDETO_SWITCH_CHANNEL					    GXCAS_IOR(GXCAS_SET, 0, GxCasSet_SwitchChannel)
#define GXCAS_IRDETO_STOP_DESCRAMBLE					GXCAS_IO(GXCAS_SET, 1)
#define GXCAS_IRDETO_STOP_CA							GXCAS_IO(GXCAS_SET, 2)
#define GXCAS_IRDETO_GET_MANUINFO						GXCAS_IOR(GXCAS_GET, 3, GxCas_IrdetoManuInfo)



int32_t GxCas_IrdetoInit(const GxCasInitParam init);
#endif
