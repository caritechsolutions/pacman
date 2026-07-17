#ifndef __GXCAS_MISC_H__
#define __GXCAS_MISC_H__

#include "gxtype.h"
#include "gxcas.h"

typedef struct{
	int32_t (* set)(unsigned long type, void *value);
	int32_t (* get)(unsigned long type, void *value);
}gxcas_control;

void  GxCas_Init(gxcas_control *ca, const GxCasInitParam param);
void GxCas_VersionInfo();
uint8_t GxCas_Os_Sleep(uint32_t ms);

#ifdef GXCAS_SCPU_SOFT
void StrToHex(uint8_t* out, char* in, int len);
int set_dsk(char *buf, int len);
#endif

void* GxCas_AlignMalloc(size_t size, size_t alignment);
void GxCas_AlignFree(void *pointer);
uint32_t GxCas_Crc32(uint8_t* pBuffer, uint32_t nSize);
int32_t GxCas_ChipId();
#endif

