/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_iso639.c
* Author    : 	shenbin
* Project   :	GoXceed 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.10.12	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_pmt.h"

extern int32_t des_table_memory_check_pmt_exit(PmtInfo *pmt_table,uint32_t max_size);

int32_t des_pmt_iso639(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	PmtInfo *pmt_table = (PmtInfo *)(p_parsed_data);
	uint8_t   des_len = p_section_data[1];
	uint8_t   iso639_count;
	uint8_t  i;

	GxSiDes_Trace();

    // 2nd protect line 46
    if((NULL==p_section_data) || (pmt_table->stream_count==0))
	{
		return GXCORE_ERROR;
	}

	// 1. audio_type     3. ISO_639_language_code len
	iso639_count = des_len/4;

	// No. is count-1
	pmt_table->stream_info[pmt_table->stream_count-1].iso639_count = iso639_count;

	if(des_table_memory_check_pmt_exit(pmt_table,sizeof(uint32_t)*iso639_count))
	{
		return GXCORE_ERROR;
	}

	pmt_table->tail = (uint8_t *)((uint32_t)(pmt_table->tail)-sizeof(Iso639Descriptor)*iso639_count);
	pmt_table->stream_info[pmt_table->stream_count-1].iso639_info = (Iso639Descriptor*)pmt_table->tail;

	p_section_data += 2;

	for(i=0; i<iso639_count; i++)
	{
		memcpy(pmt_table->stream_info[pmt_table->stream_count-1].iso639_info[i].iso639, p_section_data, 3);
		pmt_table->stream_info[pmt_table->stream_count-1].iso639_info[i].audio_type = p_section_data[3];
		p_section_data += 4;
	}

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

