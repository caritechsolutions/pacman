/**
 *
 * @file        gxota.h
 * @brief
 * @version:    1.1.0
 * @date        12/07/2010 08:48:26 AM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#ifndef __GXOTA_H__
#define __GXOTA_H__

#include "partition.h"

void gxota_entry(void);
int doload_ota(struct partition_info* ota);
#ifdef OTA_FORCE_MULTI_SECTION_FLASH
int gxota_load(uint8_t *ota_bin_buf, uint32_t ota_bin_len);
int gxota_get_bin_size(void);
#endif
#endif /*__GXOTA_H__*/
