#include <stdio.h>
#include <stdint.h>
#include "app_config.h"
#include "module/app_log.h"
#include "devapi/chip_info.h"
#include "gxsecure/gxfuse_api.h"

#define CHIP_NAME_LEN  12
#define CHIP_ID_LEN  8

int app_chip_sn_get(char *SnData, int BufferLen)
{
	if(NULL == SnData || BufferLen < CHIP_ID_LEN)
		return -1;

    if(GxFuse_GetCSSN((unsigned char *)SnData, BufferLen) != CHIP_ID_LEN)
        return -1;

    app_log_debug("chip sn: %02x %02x %02x %02x %02x %02x %02x %02x\n", SnData[0],
            SnData[1], SnData[2], SnData[3], SnData[4], SnData[5], SnData[6], SnData[7]);

	return 0;
}

int app_chip_name_get(char *SnData, int BufferLen)
{
	if(NULL == SnData || BufferLen < CHIP_NAME_LEN + 1)
		return -1;

    if(GxFuse_GetChipName((unsigned char *)SnData, BufferLen) != CHIP_NAME_LEN)
        return -1;

    app_log_debug("chip name: %s\n", SnData);

	return 0;
}

uint32_t app_chip_id_get(void)
{
    return GxChip_GetChipId();
}
