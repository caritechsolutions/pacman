#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_pmt.h"

#define MAX_ARIBCC_DESCRIPTOR   12
extern int32_t des_table_memory_check_pmt_exit(PmtInfo *pmt_table,uint32_t max_size);


/**
 * @brief
 * @param
 * @Return
 */
static uint8_t parser_aribcc_des(uint8_t *p_section_data, AribccDescriptor *des_info_data, uint8_t *tail, uint16_t elem_pid)
{
	uint16_t len = p_section_data[1];
	uint8_t  i;
	uint8_t  des_tag;

	// get elementary pid
	des_info_data->elem_pid = elem_pid;

	des_tag = p_section_data[0];
	if (des_tag != 0xFD)
	{
		gxlogd("cc des should be stream_type 0x06  and tag should be 0xFD not in this stream type\n");
		return GXCORE_ERROR;
	}

	p_section_data += 2;
	if ((((p_section_data[0] << 8) & 0xFF00) |(p_section_data[1] & 0xFF)) != 0x0008)
	{
		gxlogd("[%s]-line=%d\n",__FUNCTION__,__LINE__);
		return GXCORE_ERROR;
	}
	des_info_data->aribcc_num = len/8; // 8bytes is the loop len

	tail = (uint8_t *)(((uint32_t)tail/4)*4-des_info_data->aribcc_num*sizeof(Subtiling));
	des_info_data->aribcc= (Subtiling*)tail;

	for (i=0; i<des_info_data->aribcc_num; i++)
	{
		memcpy(des_info_data->aribcc[i].iso639, p_section_data, 3);
		p_section_data += 3;

		des_info_data->aribcc[i].type = *p_section_data;
		p_section_data += 1;

		des_info_data->aribcc[i].composite_page_id = TODATA16(p_section_data[0], p_section_data[1]);
		p_section_data += 2;

		des_info_data->aribcc[i].ancillary_page_id = TODATA16(p_section_data[0], p_section_data[1]);
		p_section_data += 2;
	}

	return  len;//(des_info_data->aribcc_num*sizeof(Subtiling));
}


int32_t des_pmt_aribcc(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
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
			gxlogd("-cc des should be stream_type 0x06 not in this stream type\n");
			return GXCORE_ERROR;
		}
	}

	if (NULL == pmt_table->aribcc_info)
	{
		if(des_table_memory_check_pmt_exit(pmt_table,sizeof(uint32_t)*MAX_ARIBCC_DESCRIPTOR) == GXCORE_ERROR)
		{
			return GXCORE_ERROR;
		}
		pmt_table->tail = (uint8_t *)((uint32_t)(pmt_table->tail)-sizeof(uint32_t)*MAX_ARIBCC_DESCRIPTOR);	//default max  subt_des
		pmt_table->aribcc_info = (uint32_t *)pmt_table->tail;
		pmt_table->aribcc_count = 0;
	}
	if(pmt_table->aribcc_count < MAX_ARIBCC_DESCRIPTOR )
	{
		if(des_table_memory_check_pmt_exit(pmt_table,sizeof(AribccDescriptor)) == GXCORE_ERROR)
		{
			return GXCORE_ERROR;
		}
		pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(AribccDescriptor));
		pmt_table->aribcc_info[pmt_table->aribcc_count] = (uint32_t)(pmt_table->tail);
		//pmt_table->aribcc_info[pmt_table->aribcc_count] = pmt_table->stream_info[pmt_table->stream_count-1].elem_pid;
		des_len = parser_aribcc_des(p_section_data,
				(AribccDescriptor*)(pmt_table->aribcc_info[pmt_table->aribcc_count]), pmt_table->tail,
				elem_pid);
		pmt_table->tail=(uint8_t *)((uint32_t)pmt_table->tail-(des_len+3)/4*4);

		pmt_table->aribcc_count++;
	}
	else
	{
		gxlogd("\n[descriptor more than \n");
	}

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

