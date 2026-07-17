#ifndef __GXCAS_PSI_H__
#define __GXCAS_PSI_H__
#include "gxcore.h"

#define PSI_INVALID_PID (0x1FFF)
#define CAT_PID (0x01)
#define CAT_TID (0x01)
#define PMT_TID (0x02)
#define NIT_PID (0x10)
#define NIT_TID (0x40)
#define CA_DESCRIPTOR_TAG   (0x09)
#define SCRAMBLING_DESCRIPTOR_TAG   (0x65)

#define MAX_ECM_COUNT (8)
#define MAX_EMM_COUNT (8)

typedef bool (*CheckCAId)(uint16_t caid, uint16_t pid);

typedef struct{
	uint32_t ecm_count;
	uint16_t ecm_pid[MAX_ECM_COUNT];
	uint16_t pid[MAX_ECM_COUNT];
}GxCas_EcmInfo;

void GxCas_Pmt_Init(CheckCAId docheck);
handle_t GxCas_Pmt_Open(uint16_t pid, uint32_t service_id, uint8_t version, uint8_t versionEQ);
int32_t GxCas_Pmt_Close(handle_t handle);
int32_t GxCas_Pmt_Parse(uint8_t *section, uint32_t size,GxCas_EcmInfo *ecminfo);

typedef struct {
	uint32_t emm_count;
	uint16_t emmpid[MAX_EMM_COUNT];
}GxCas_EmmInfo;

void GxCas_Cat_Init(CheckCAId docheck);
handle_t GxCas_Cat_Open(uint8_t version, uint8_t versionEQ);
int32_t GxCas_Cat_Close(handle_t handle);
int32_t GxCas_Cat_Parse(uint8_t *section, uint32_t size,GxCas_EmmInfo *emminfo);

void GxCas_Nit_Init(void);
handle_t GxCas_Nit_Open(uint8_t version, uint8_t versionEQ);
int32_t GxCas_Nit_Close(handle_t handle);

#endif

