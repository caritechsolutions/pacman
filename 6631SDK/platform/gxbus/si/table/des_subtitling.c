/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_subtitling.c
* Author    : 	shenbin
* Project   :	GoXceed 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.10.10	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_pmt.h"

#define MAX_SUBTITLE_DESCRIPTOR   12
extern int32_t des_table_memory_check_pmt_exit(PmtInfo *pmt_table,uint32_t max_size);
/**
 * @brief
 * @param
 * @Return
 */
static uint8_t parser_subt_des(uint8_t *p_section_data, SubtDescriptor *des_info_data, uint8_t *tail, uint16_t elem_pid)
{
	uint16_t len = p_section_data[1];
	uint8_t  i;
	
	// get elementary pid
	des_info_data->elem_pid = elem_pid;

	p_section_data += 2;

	des_info_data->subt_num = len/8; // 8bytes is the loop len
	
	tail = (uint8_t *)(((uint32_t)tail/4)*4-des_info_data->subt_num*sizeof(Subtiling));
	des_info_data->subt = (Subtiling*)tail;
	
	for (i=0; i<des_info_data->subt_num; i++)
	{
		memcpy(des_info_data->subt[i].iso639, p_section_data, 3);
		p_section_data += 3;

		des_info_data->subt[i].type = *p_section_data;
		p_section_data += 1;

		des_info_data->subt[i].composite_page_id = TODATA16(p_section_data[0], p_section_data[1]);
		p_section_data += 2;

		des_info_data->subt[i].ancillary_page_id = TODATA16(p_section_data[0], p_section_data[1]);
		p_section_data += 2;
	}

	return  (des_info_data->subt_num*sizeof(Subtiling));
}

int32_t des_pmt_subt(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	PmtInfo *pmt_table = (PmtInfo *)(p_parsed_data);
	uint16_t elem_pid = 0;
	uint8_t   des_len;

	GxSiDes_Trace();
	if(NULL== p_section_data)
	{
		return GXCORE_ERROR;
	}
		

	if(pmt_table->stream_count > 0)
	{
		elem_pid = pmt_table->stream_info[pmt_table->stream_count-1].elem_pid;
		if(pmt_table->stream_info[pmt_table->stream_count-1].stream_type != 0x06)
		{
			gxlogd("subt des should be stream_type 0x06 not in this stream type\n");
			return GXCORE_ERROR;
		}
	}
	
	if(NULL == pmt_table->subt_info){
		if(des_table_memory_check_pmt_exit(pmt_table,sizeof(uint32_t)*MAX_SUBTITLE_DESCRIPTOR) == GXCORE_ERROR)
		{
			return GXCORE_ERROR;
		}
		pmt_table->tail = (uint8_t *)((uint32_t)(pmt_table->tail)-sizeof(uint32_t)*MAX_SUBTITLE_DESCRIPTOR);	//default max  subt_des
		pmt_table->subt_info = (uint32_t *)pmt_table->tail;
		pmt_table->subt_count = 0;
	}
	if(pmt_table->subt_count < MAX_SUBTITLE_DESCRIPTOR)
	{
		if(des_table_memory_check_pmt_exit(pmt_table,sizeof(SubtDescriptor)) == GXCORE_ERROR)
		{
			return GXCORE_ERROR;
		}
		pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(SubtDescriptor));
		pmt_table->subt_info[pmt_table->subt_count] = (uint32_t)(pmt_table->tail);

		des_len = parser_subt_des(p_section_data, 
				(SubtDescriptor*)(pmt_table->subt_info[pmt_table->subt_count]), pmt_table->tail,
				elem_pid);
		pmt_table->tail=(uint8_t *)((uint32_t)pmt_table->tail-(des_len+3)/4*4);

		pmt_table->subt_count++;
	}
	else
	{
		gxlogd("\n[descriptor more than MAX_SUBTITLE_DESCRIPTOR(%d)]:%s,%s,%d\n",MAX_SUBTITLE_DESCRIPTOR,__FILE__,__FUNCTION__,__LINE__);
	}

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

