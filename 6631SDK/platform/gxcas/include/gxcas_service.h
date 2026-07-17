#ifndef __GXCAS_SERVICE_H__
#define __GXCAS_SERVICE_H__

#include "gxtype.h"
#include "gxcas.h"

typedef enum{
	GXCAS_SERVICE_INIT = 0,
	GXCAS_SERVICE_AV = 1 << 0,
	GXCAS_SERVICE_OTHER = 1 << 1,
	GXCAS_SERVICE_PVR = 1 << 2,
}GxCas_Service_Type;

typedef struct{
	uint16_t pid;
	uint16_t ecm_pid;
	uint8_t record;
	int32_t descid;
	GxCas_Service_Type type;
}GxCas_Service_Info;

void GxCas_Service_Init(void);
int32_t GxCas_Service_Add(GxCas_Service_Info service);
int32_t GxCas_Service_Delete(GxCas_Service_Info service);

void GxCas_Service_Config(GxCas_Service_Info service);
int32_t GxCas_Service_Bind(GxCas_Service_Info service);
int32_t GxCas_Service_SetCW(GxCas_Service_Info service, uint8_t *oddkey, uint8_t *evenkey, uint32_t keylen);
uint32_t GxCas_Service_GetTotalCount(void);
void GxCas_Service_GetInfo(GxCas_Service_Info *arry);

#endif

