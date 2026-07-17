/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2020, All right reserved
******************************************************************************

******************************************************************************
* File Name :   des_compontent.c
* Author    :   luyong
* Project   :   GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION   Date              AUTHOR         Description
   0.0      2020.04.28        luyong            creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_sdt.h"

int32_t des_component_descriptor(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	SdtInfo *sdt_table = (SdtInfo *)(p_parsed_data);
	service_info_t *si = &sdt_table->service_info[sdt_table->service_count-1];
	component_info_t *component_info = &si->component_info;
	uint8_t des_len = p_section_data[1];

	if(des_len > 6)
	{
		uint32_t text_len = des_len - 6;
		sdt_table->tail = (uint8_t *)((uint32_t)sdt_table->tail - (text_len + 3) / 4 * 4);
		component_info->text = sdt_table->tail;
		memcpy(component_info->text, p_section_data + 8, text_len);
	}

	component_info->stream_content_ext = (p_section_data[2] & 0xF0) >> 4;
	component_info->stream_content     = p_section_data[2] & 0x0F;
	component_info->component_type     = p_section_data[3];
	component_info->component_tag      = p_section_data[4];
	memcpy(component_info->iso639, p_section_data + 5, 3);

	return GXCORE_SUCCESS;
}
