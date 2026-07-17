#ifndef __GXCAS_FLASH_H__
#define __GXCAS_FLASH_H__

#include "gxtype.h"
#define GXCA_MAX_NVRAM (128)
#define GXCAS_BLOCK_SIZE   (64 * 1024)
#define GXCAS_CRC_SIZE   (4)

typedef int32_t (*minifs_rebuild_func)(void);

typedef struct{
	char name[GXCA_MAX_NVRAM];
	uint32_t offset;
	uint32_t size;
	minifs_rebuild_func func;//when minifs cas partition damage, rebuild minifs cas partition.
}GxCas_FlashConfig;

int32_t GxCas_Flash_Init(GxCas_FlashConfig flash,GxCas_FlashConfig backup);
int32_t GxCas_Flash_Read(uint32_t offset, uint8_t *buf, size_t len);
int32_t GxCas_Flash_Write(uint32_t offset, const uint8_t *buf, size_t len);
int32_t GxCas_Flash_Async_Write(uint32_t offset, const uint8_t *buf, size_t len);
int32_t GxCas_Flash_GetInfo(uint32_t* start_addr, size_t *size);

#endif

